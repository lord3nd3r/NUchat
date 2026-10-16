#pragma once

#include <QDateTime>
#include <QPair>
#include <QString>
#include <QStringList>

// ASCII case-fold kept for callers that are not IRC nick/channel identity
// (log paths, host labels). Nick and channel identity uses ircLower() with
// the server's CASEMAPPING.
inline bool ircSame(const QString &a, const QString &b) {
  return a.compare(b, Qt::CaseInsensitive) == 0;
}

inline QString ircFold(const QString &s) { return s.toLower(); }

// IRCv3 CASEMAPPING. Until RPL_ISUPPORT arrives the historical default is
// rfc1459 (RFC 2812-style, as defined by the ISUPPORT token — not the
// stricter original RFC 1459 mapping).
enum class IrcCaseMapping { Ascii, Rfc1459, StrictRfc1459 };

// Fold per the ISUPPORT token:
//   ascii:            A-Z → a-z
//   strict-rfc1459:   also [\] → {|}
//   rfc1459:          also ^ → ~
QString ircLower(const QString &s, IrcCaseMapping mapping);
bool ircEquals(const QString &a, const QString &b, IrcCaseMapping mapping);
int ircCompare(const QString &a, const QString &b, IrcCaseMapping mapping);

IrcCaseMapping ircCaseMappingFromToken(const QString &token);

// IRCv3 server-time value, e.g. 2011-10-19T16:40:51.620Z. Invalid if empty
// or not a timestamp.
QDateTime parseServerTime(const QString &value);

// SASL AUTHENTICATE lines for a base64 payload. Chunks are at most 400
// bytes. An empty payload, or one whose length is a multiple of 400, ends
// with "AUTHENTICATE +".
QStringList saslAuthenticateCommands(const QByteArray &base64Payload);

// Initial reconnect delay is baseSeconds; each later attempt doubles it.
// The result is capped (15 minutes) and always fits in QTimer's int msec.
int reconnectDelayMs(int baseSeconds, int attempt);

// /TIMER delay in milliseconds, or -1 when the value is not a finite
// positive delay that fits in a QTimer int.
int timerIntervalMs(double seconds);

// How sendMessage should treat one input line.
enum class OutboundKind { Command, Literal };

struct OutboundLine {
  OutboundKind kind = OutboundKind::Literal;
  QString command; // without the slash, uppercased
  QString args;
  QString literal; // text to send when kind == Literal
};

// "//hello" is the literal message "/hello". A first token that itself
// contains a slash ("/etc/passwd") is literal text, not a command.
OutboundLine classifyOutbound(const QString &message);

// Parsed RPL_ISUPPORT (005) state. Defaults match the historical hard-coded
// maps so a server that never sends 005 keeps the previous behavior.
class IrcSupport {
public:
  IrcSupport();

  void apply005(const QStringList &params);
  void applyToken(const QString &token);

  IrcCaseMapping caseMapping() const { return m_case; }
  QString fold(const QString &s) const { return ircLower(s, m_case); }
  bool same(const QString &a, const QString &b) const {
    return ircEquals(a, b, m_case);
  }

  bool isChannel(const QString &name) const;
  QString channelTypes() const { return m_chanTypes; }

  QString prefixModes() const { return m_prefixModes; }
  QString prefixSymbols() const { return m_prefixSymbols; }
  QPair<QString, QString> splitNick(const QString &nick) const;
  QChar prefixForMode(QChar mode) const;
  int symbolRank(QChar symbol) const;

  // CHANMODES groups A (list) and B (always) take a parameter either way.
  // Group C takes one only when the mode is being set. Prefix modes are
  // handled separately and always take a nick.
  bool modeTakesParam(QChar mode, bool adding) const;

  QString listModes() const { return m_listModes; }
  QString alwaysParamModes() const { return m_alwaysParam; }
  QString paramWhenSetModes() const { return m_setParam; }

private:
  IrcCaseMapping m_case;
  QString m_chanTypes;
  QString m_prefixModes;
  QString m_prefixSymbols;
  QString m_listModes;
  QString m_alwaysParam;
  QString m_setParam;
};

struct IrcNetworkFacts {
  IrcCaseMapping mapping = IrcCaseMapping::Rfc1459;
  QString prefixSymbols = QStringLiteral("~&@%+");
  QString chanTypes = QStringLiteral("#&");
};

// Per-server facts for map keys and UI that do not hold the connection.
// Unknown servers use the rfc1459 / "#&" / "~&@%+" defaults.
class IrcNetworkRegistry {
public:
  static IrcNetworkFacts factsFor(const QString &server);
  static bool has(const QString &server);
  static void set(const QString &server, const IrcNetworkFacts &facts);
  static void clear();
};
