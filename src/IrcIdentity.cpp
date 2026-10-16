#include "IrcIdentity.h"

#include <QHash>
#include <QtGlobal>

#include <cmath>
#include <limits>

QString ircLower(const QString &s, IrcCaseMapping mapping) {
  QString out;
  out.reserve(s.size());
  for (const QChar &ch : s) {
    const char16_t u = ch.unicode();
    if (u >= u'A' && u <= u'Z') {
      out.append(QChar(static_cast<char16_t>(u + 32)));
      continue;
    }
    if (mapping == IrcCaseMapping::Ascii) {
      out.append(ch);
      continue;
    }
    if (u == u'[')
      out.append(QLatin1Char('{'));
    else if (u == u'\\')
      out.append(QLatin1Char('|'));
    else if (u == u']')
      out.append(QLatin1Char('}'));
    else if (mapping == IrcCaseMapping::Rfc1459 && u == u'^')
      out.append(QLatin1Char('~'));
    else
      out.append(ch);
  }
  return out;
}

bool ircEquals(const QString &a, const QString &b, IrcCaseMapping mapping) {
  return ircLower(a, mapping) == ircLower(b, mapping);
}

int ircCompare(const QString &a, const QString &b, IrcCaseMapping mapping) {
  return ircLower(a, mapping).compare(ircLower(b, mapping));
}

IrcCaseMapping ircCaseMappingFromToken(const QString &token) {
  const QString v = token.toLower();
  if (v == QLatin1String("ascii"))
    return IrcCaseMapping::Ascii;
  if (v == QLatin1String("strict-rfc1459"))
    return IrcCaseMapping::StrictRfc1459;
  return IrcCaseMapping::Rfc1459;
}

QDateTime parseServerTime(const QString &value) {
  if (value.isEmpty())
    return {};
  QDateTime dt = QDateTime::fromString(value, Qt::ISODateWithMs);
  if (!dt.isValid())
    dt = QDateTime::fromString(value, Qt::ISODate);
  return dt;
}

QStringList saslAuthenticateCommands(const QByteArray &base64Payload) {
  QStringList lines;
  if (base64Payload.isEmpty()) {
    lines << QStringLiteral("AUTHENTICATE +");
    return lines;
  }
  constexpr int kChunk = 400;
  for (int i = 0; i < base64Payload.size(); i += kChunk) {
    lines << QStringLiteral("AUTHENTICATE ") +
                 QString::fromLatin1(base64Payload.mid(i, kChunk));
  }
  if (base64Payload.size() % kChunk == 0)
    lines << QStringLiteral("AUTHENTICATE +");
  return lines;
}

int reconnectDelayMs(int baseSeconds, int attempt) {
  if (baseSeconds < 1)
    baseSeconds = 1;
  if (attempt < 1)
    attempt = 1;
  constexpr int kCapSeconds = 15 * 60;
  int shift = attempt - 1;
  if (shift > 20)
    shift = 20;
  qint64 seconds = static_cast<qint64>(baseSeconds) << shift;
  if (seconds > kCapSeconds)
    seconds = kCapSeconds;
  qint64 ms = seconds * 1000;
  if (ms > std::numeric_limits<int>::max())
    ms = std::numeric_limits<int>::max();
  return static_cast<int>(ms);
}

int timerIntervalMs(double seconds) {
  if (!std::isfinite(seconds) || seconds <= 0.0)
    return -1;
  // Stay a full second under INT_MAX so the * 1000 conversion cannot overflow.
  constexpr double kMaxSeconds = 2147483.0;
  if (seconds > kMaxSeconds)
    return -1;
  const double ms = seconds * 1000.0;
  if (!(ms >= 1.0) || ms > static_cast<double>(std::numeric_limits<int>::max()))
    return -1;
  return static_cast<int>(ms);
}

OutboundLine classifyOutbound(const QString &message) {
  OutboundLine out;
  if (message.startsWith(QLatin1String("//"))) {
    out.kind = OutboundKind::Literal;
    out.literal = message.mid(1);
    return out;
  }
  if (message.startsWith(QLatin1Char('/'))) {
    const QString token = message.section(QLatin1Char(' '), 0, 0);
    const QString body = token.mid(1);
    if (body.isEmpty() || body.contains(QLatin1Char('/'))) {
      out.kind = OutboundKind::Literal;
      out.literal = message;
      return out;
    }
    out.kind = OutboundKind::Command;
    out.command = body.toUpper();
    out.args = message.section(QLatin1Char(' '), 1);
    return out;
  }
  out.kind = OutboundKind::Literal;
  out.literal = message;
  return out;
}

IrcSupport::IrcSupport()
    : m_case(IrcCaseMapping::Rfc1459), m_chanTypes(QStringLiteral("#&")),
      m_prefixModes(QStringLiteral("qaohv")),
      m_prefixSymbols(QStringLiteral("~&@%+")),
      m_listModes(QStringLiteral("beI")), m_alwaysParam(QStringLiteral("k")),
      m_setParam(QStringLiteral("lfj")) {}

void IrcSupport::apply005(const QStringList &params) {
  for (const QString &p : params) {
    if (p.isEmpty() || p.contains(QLatin1Char(' ')))
      continue;
    applyToken(p);
  }
}

void IrcSupport::applyToken(const QString &token) {
  const int eq = token.indexOf(QLatin1Char('='));
  const QString key = (eq < 0 ? token : token.left(eq)).toUpper();
  const QString val = eq < 0 ? QString() : token.mid(eq + 1);
  if (key == QLatin1String("PREFIX")) {
    if (val.startsWith(QLatin1Char('('))) {
      const int end = val.indexOf(QLatin1Char(')'));
      if (end > 1 && end + 1 <= val.size()) {
        const QString modes = val.mid(1, end - 1);
        const QString symbols = val.mid(end + 1);
        if (!modes.isEmpty() && modes.size() == symbols.size()) {
          m_prefixModes = modes;
          m_prefixSymbols = symbols;
        }
      }
    }
  } else if (key == QLatin1String("CHANTYPES")) {
    m_chanTypes = val;
  } else if (key == QLatin1String("CHANMODES")) {
    const QStringList groups = val.split(QLatin1Char(','));
    m_listModes = groups.value(0);
    m_alwaysParam = groups.value(1);
    m_setParam = groups.value(2);
  } else if (key == QLatin1String("CASEMAPPING")) {
    m_case = ircCaseMappingFromToken(val);
  }
}

bool IrcSupport::isChannel(const QString &name) const {
  return !name.isEmpty() && !m_chanTypes.isEmpty() &&
         m_chanTypes.contains(name.at(0));
}

QPair<QString, QString> IrcSupport::splitNick(const QString &nick) const {
  QString pfx;
  QString bare = nick;
  while (!bare.isEmpty() && m_prefixSymbols.contains(bare.at(0))) {
    pfx += bare.at(0);
    bare = bare.mid(1);
  }
  return {pfx, bare};
}

QChar IrcSupport::prefixForMode(QChar mode) const {
  const int i = m_prefixModes.indexOf(mode);
  if (i < 0 || i >= m_prefixSymbols.size())
    return {};
  return m_prefixSymbols.at(i);
}

int IrcSupport::symbolRank(QChar symbol) const {
  const int i = m_prefixSymbols.indexOf(symbol);
  return i < 0 ? 99 : i;
}

bool IrcSupport::modeTakesParam(QChar mode, bool adding) const {
  if (m_listModes.contains(mode) || m_alwaysParam.contains(mode))
    return true;
  return adding && m_setParam.contains(mode);
}

namespace {
QHash<QString, IrcNetworkFacts> &registry() {
  static QHash<QString, IrcNetworkFacts> facts;
  return facts;
}

QString registryKey(const QString &server) { return server.toLower(); }
} // namespace

IrcNetworkFacts IrcNetworkRegistry::factsFor(const QString &server) {
  const auto it = registry().constFind(registryKey(server));
  if (it == registry().constEnd())
    return {};
  return it.value();
}

bool IrcNetworkRegistry::has(const QString &server) {
  return registry().contains(registryKey(server));
}

void IrcNetworkRegistry::set(const QString &server, const IrcNetworkFacts &facts) {
  registry().insert(registryKey(server), facts);
}

void IrcNetworkRegistry::clear() { registry().clear(); }
