#include "NetSafety.h"

#include <QSettings>

namespace {

bool startsCommand(const QString &line, const QString &cmd) {
  if (!line.startsWith(cmd, Qt::CaseInsensitive))
    return false;
  return line.size() == cmd.size() || line.at(cmd.size()).isSpace();
}

QString commandRemainder(const QString &line, const QString &cmd) {
  return line.mid(cmd.size()).trimmed();
}

bool startsWord(const QString &text, const char *word) {
  const QString w = QString::fromLatin1(word);
  return text.startsWith(w, Qt::CaseInsensitive) &&
         (text.size() == w.size() || text.at(w.size()).isSpace());
}

// NickServ (and the NS alias) commands that carry a password.
bool isCredentialVerb(const QString &message) {
  const QString m = message.trimmed();
  if (startsWord(m, "IDENTIFY") || startsWord(m, "REGISTER") ||
      startsWord(m, "AUTH") || startsWord(m, "GHOST") ||
      startsWord(m, "RELEASE") || startsWord(m, "RECOVER") ||
      startsWord(m, "GROUP"))
    return true;
  return m.startsWith(QLatin1String("SET PASSWORD"), Qt::CaseInsensitive);
}

bool isNickServTarget(const QString &target) {
  return target.compare(QLatin1String("NickServ"), Qt::CaseInsensitive) == 0 ||
         target.compare(QLatin1String("NS"), Qt::CaseInsensitive) == 0;
}

bool isReservedWindowsName(const QString &stem) {
  static const char *kReserved[] = {
      "CON",  "PRN",  "AUX",  "NUL",  "COM1", "COM2", "COM3", "COM4", "COM5",
      "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5",
      "LPT6", "LPT7", "LPT8", "LPT9"};
  for (const char *name : kReserved) {
    if (stem.compare(QLatin1String(name), Qt::CaseInsensitive) == 0)
      return true;
  }
  return false;
}

NetSafety::DccPeerClass scopeOfIpv4(const QHostAddress &addr) {
  if (addr.isLoopback() || addr.isLinkLocal() || addr.isMulticast() ||
      addr.isBroadcast())
    return NetSafety::DccPeerClass::Forbidden;
  // 0.0.0.0/8 (includes "this host") and Class E / limited broadcast.
  if (addr.isInSubnet(QHostAddress(QStringLiteral("0.0.0.0")), 8) ||
      addr.isInSubnet(QHostAddress(QStringLiteral("240.0.0.0")), 4))
    return NetSafety::DccPeerClass::Forbidden;
  // Documentation, benchmarking, and IETF protocol assignments — not peers.
  if (addr.isInSubnet(QHostAddress(QStringLiteral("192.0.0.0")), 24) ||
      addr.isInSubnet(QHostAddress(QStringLiteral("192.0.2.0")), 24) ||
      addr.isInSubnet(QHostAddress(QStringLiteral("198.51.100.0")), 24) ||
      addr.isInSubnet(QHostAddress(QStringLiteral("203.0.113.0")), 24) ||
      addr.isInSubnet(QHostAddress(QStringLiteral("198.18.0.0")), 15) ||
      addr.isInSubnet(QHostAddress(QStringLiteral("192.88.99.0")), 24))
    return NetSafety::DccPeerClass::Forbidden;
  // RFC1918 and CGNAT. Legitimate on a LAN, unsafe as an automatic target.
  if (addr.isInSubnet(QHostAddress(QStringLiteral("10.0.0.0")), 8) ||
      addr.isInSubnet(QHostAddress(QStringLiteral("172.16.0.0")), 12) ||
      addr.isInSubnet(QHostAddress(QStringLiteral("192.168.0.0")), 16) ||
      addr.isInSubnet(QHostAddress(QStringLiteral("100.64.0.0")), 10))
    return NetSafety::DccPeerClass::PrivateLan;
  return NetSafety::DccPeerClass::Public;
}

} // namespace

QString NetSafety::sanitizeIrcLine(const QString &line) {
  QString out;
  out.reserve(line.size());
  for (const QChar c : line) {
    if (c == QLatin1Char('\r') || c == QLatin1Char('\n') || c.isNull())
      continue;
    out.append(c);
  }
  return out;
}

QString NetSafety::redactIrcLine(const QString &line) {
  const QString t = line.trimmed();
  if (t.isEmpty())
    return line;

  if (startsCommand(t, QStringLiteral("PASS")))
    return QStringLiteral("PASS <redacted>");
  if (startsCommand(t, QStringLiteral("OPER")))
    return QStringLiteral("OPER <redacted>");

  if (startsCommand(t, QStringLiteral("AUTHENTICATE"))) {
    const QString arg = commandRemainder(t, QStringLiteral("AUTHENTICATE"));
    if (arg.isEmpty() || arg == QLatin1String("+") || arg == QLatin1String("*") ||
        arg.compare(QLatin1String("PLAIN"), Qt::CaseInsensitive) == 0 ||
        arg.compare(QLatin1String("EXTERNAL"), Qt::CaseInsensitive) == 0 ||
        arg.compare(QLatin1String("SCRAM-SHA-256"), Qt::CaseInsensitive) == 0)
      return QStringLiteral("AUTHENTICATE ") + arg;
    return QStringLiteral("AUTHENTICATE <redacted>");
  }

  if (startsCommand(t, QStringLiteral("NICKSERV")) ||
      startsCommand(t, QStringLiteral("NS"))) {
    const QString cmd = startsCommand(t, QStringLiteral("NICKSERV"))
                            ? QStringLiteral("NICKSERV")
                            : QStringLiteral("NS");
    if (isCredentialVerb(commandRemainder(t, cmd)))
      return cmd + QStringLiteral(" <redacted>");
    return t;
  }

  if (startsCommand(t, QStringLiteral("PRIVMSG")) ||
      startsCommand(t, QStringLiteral("NOTICE"))) {
    const int sp = t.indexOf(QLatin1Char(' '));
    const QString rest = t.mid(sp + 1).trimmed();
    QString target;
    QString msg;
    if (rest.startsWith(QLatin1Char(':'))) {
      msg = rest.mid(1);
    } else {
      const int sp2 = rest.indexOf(QLatin1Char(' '));
      if (sp2 < 0)
        return t;
      target = rest.left(sp2);
      msg = rest.mid(sp2 + 1).trimmed();
      if (msg.startsWith(QLatin1Char(':')))
        msg = msg.mid(1);
    }
    if (isNickServTarget(target) && isCredentialVerb(msg))
      return t.left(sp) + QLatin1Char(' ') + target + QStringLiteral(" :<redacted>");
  }

  return t;
}

bool NetSafety::isIgnorableTlsError(QSslError::SslError error) {
  switch (error) {
  case QSslError::SelfSignedCertificate:
  case QSslError::SelfSignedCertificateInChain:
  case QSslError::HostNameMismatch:
  case QSslError::CertificateUntrusted:
  case QSslError::UnableToGetIssuerCertificate:
  case QSslError::UnableToGetLocalIssuerCertificate:
  case QSslError::UnableToVerifyFirstCertificate:
    return true;
  default:
    return false;
  }
}

NetSafety::ProxyResolution
NetSafety::resolveProxy(int typeIndex, const QString &host, int port, bool useAuth,
                        const QString &user, const QString &password) {
  ProxyResolution r;
  if (typeIndex <= 0) {
    r.result = ProxyResolution::Result::Direct;
    r.proxy = QNetworkProxy(QNetworkProxy::NoProxy);
    return r;
  }

  // Qt has no SOCKS4 client. Substituting SOCKS5 (or connecting directly)
  // would ignore the protocol the user selected.
  if (typeIndex == 1) {
    r.result = ProxyResolution::Result::Rejected;
    r.error = QStringLiteral(
        "SOCKS4 proxies are not supported. Select SOCKS5 or HTTP CONNECT. "
        "Refusing to connect directly or via a different proxy protocol.");
    return r;
  }

  QNetworkProxy::ProxyType type = QNetworkProxy::NoProxy;
  if (typeIndex == 2) {
    type = QNetworkProxy::Socks5Proxy;
  } else if (typeIndex == 3) {
    type = QNetworkProxy::HttpProxy;
  } else {
    r.result = ProxyResolution::Result::Rejected;
    r.error = QStringLiteral("Unknown proxy type. Refusing to connect directly.");
    return r;
  }

  const QString trimmed = host.trimmed();
  if (trimmed.isEmpty()) {
    r.result = ProxyResolution::Result::Rejected;
    r.error = QStringLiteral(
        "A proxy is enabled but no proxy host is configured. "
        "Refusing to connect directly.");
    return r;
  }
  if (port <= 0 || port > 65535) {
    r.result = ProxyResolution::Result::Rejected;
    r.error = QStringLiteral(
        "A proxy is enabled but the proxy port is invalid. "
        "Refusing to connect directly.");
    return r;
  }

  r.result = ProxyResolution::Result::UseProxy;
  r.proxy = QNetworkProxy(type, trimmed, static_cast<quint16>(port),
                          useAuth ? user : QString(),
                          useAuth ? password : QString());
  return r;
}

NetSafety::ProxyResolution NetSafety::proxyFromSettings() {
  QSettings settings;
  const int typeIndex = settings.value(QStringLiteral("conn/proxyTypeIndex"), 0).toInt();
  const QString host = settings.value(QStringLiteral("conn/proxyHost")).toString();
  const int port = settings.value(QStringLiteral("conn/proxyPort"), 1080).toInt();
  const bool useAuth = settings.value(QStringLiteral("conn/proxyAuth"), false).toBool();
  QString user;
  QString pass;
  if (useAuth) {
    user = settings.value(QStringLiteral("conn/proxyUser")).toString();
    pass = settings.value(QStringLiteral("conn/proxyPass")).toString();
  }
  return resolveProxy(typeIndex, host, port, useAuth, user, pass);
}

bool NetSafety::isBlockedAddress(const QHostAddress &input) {
  if (input.isNull())
    return true;

  if (input.protocol() == QAbstractSocket::IPv4Protocol)
    return classifyDccPeer(input.toIPv4Address()) != DccPeerClass::Public;

  if (input.protocol() == QAbstractSocket::IPv6Protocol) {
    bool ok = false;
    const quint32 v4 = input.toIPv4Address(&ok);
    // ::ffff:a.b.c.d must be judged as the embedded IPv4 address. A plain
    // IPv6 range check does not see 10/8 inside the mapped form.
    if (ok)
      return classifyDccPeer(v4) != DccPeerClass::Public;
    if (input.isLoopback() || input.isLinkLocal() || input.isMulticast() ||
        input == QHostAddress::AnyIPv6)
      return true;
    if (input.isInSubnet(QHostAddress(QStringLiteral("fc00::")), 7) ||
        input.isInSubnet(QHostAddress(QStringLiteral("fec0::")), 10) ||
        input.isInSubnet(QHostAddress(QStringLiteral("2001:db8::")), 32) ||
        input.isInSubnet(QHostAddress(QStringLiteral("100::")), 64) ||
        input.isInSubnet(QHostAddress(QStringLiteral("64:ff9b::")), 96) ||
        input.isInSubnet(QHostAddress(QStringLiteral("64:ff9b:1::")), 48))
      return true;
    return false;
  }
  return true;
}

bool NetSafety::parseHostLiteral(const QString &host, QHostAddress *out) {
  if (!out)
    return false;
  QString h = host.trimmed();
  if (h.startsWith(QLatin1Char('[')) && h.endsWith(QLatin1Char(']')))
    h = h.mid(1, h.size() - 2);
  if (h.isEmpty())
    return false;

  const QHostAddress direct(h);
  if (!direct.isNull()) {
    *out = direct;
    return true;
  }

  bool ok = false;
  if (h.startsWith(QLatin1String("0x"), Qt::CaseInsensitive)) {
    const qulonglong n = h.mid(2).toULongLong(&ok, 16);
    if (ok && n <= 0xFFFFFFFFULL) {
      *out = QHostAddress(static_cast<quint32>(n));
      return true;
    }
    return false;
  }

  // Decimal dotted-quad alternative used in SSRF payloads (2130706433 == 127.0.0.1).
  // Leading zeros are left alone so "0127" is not silently treated as decimal.
  if (h.at(0).isDigit() && !h.contains(QLatin1Char('.')) && !h.contains(QLatin1Char(':'))) {
    if (h.size() > 1 && h.at(0) == QLatin1Char('0'))
      return false;
    const qulonglong n = h.toULongLong(&ok, 10);
    if (ok && n <= 0xFFFFFFFFULL) {
      *out = QHostAddress(static_cast<quint32>(n));
      return true;
    }
  }
  return false;
}

bool NetSafety::isSafeImageUrl(const QUrl &url) {
  const QString scheme = url.scheme().toLower();
  if (scheme != QLatin1String("https") && scheme != QLatin1String("http"))
    return false;

  QString host = url.host().trimmed();
  if (host.startsWith(QLatin1Char('[')) && host.endsWith(QLatin1Char(']')))
    host = host.mid(1, host.size() - 2);
  if (host.isEmpty())
    return false;

  const QString lower = host.toLower();
  if (lower == QLatin1String("localhost") ||
      lower == QLatin1String("localhost.localdomain") ||
      lower.endsWith(QLatin1String(".localhost")) ||
      lower.endsWith(QLatin1String(".local")) ||
      lower.endsWith(QLatin1String(".internal")) ||
      lower == QLatin1String("metadata.google.internal"))
    return false;

  QHostAddress literal;
  if (parseHostLiteral(host, &literal))
    return !isBlockedAddress(literal);

  // A colon that QHostAddress could not parse is a broken IPv6 literal.
  if (host.contains(QLatin1Char(':')))
    return false;
  return true;
}

bool NetSafety::anyAddressBlocked(const QList<QHostAddress> &addrs) {
  if (addrs.isEmpty())
    return true;
  for (const QHostAddress &addr : addrs) {
    if (isBlockedAddress(addr))
      return true;
  }
  return false;
}

NetSafety::DccPeerClass NetSafety::classifyDccPeer(quint32 ipv4) {
  return scopeOfIpv4(QHostAddress(ipv4));
}

QString NetSafety::dccReceiveBlockReason(quint32 ipv4, quint16 port, bool allowPrivateLan) {
  if (port == 0)
    return QStringLiteral("DCC offer has an invalid port");
  switch (classifyDccPeer(ipv4)) {
  case DccPeerClass::Forbidden:
    return QStringLiteral(
        "DCC offer address is not allowed (loopback, unspecified, link-local, or non-unicast)");
  case DccPeerClass::PrivateLan:
    if (!allowPrivateLan)
      return QStringLiteral("DCC offer uses a private address");
    return {};
  case DccPeerClass::Public:
    return {};
  }
  return QStringLiteral("DCC offer has an invalid address");
}

bool NetSafety::isUsableDccBindAddress(const QHostAddress &addr) {
  if (addr.protocol() != QAbstractSocket::IPv4Protocol || addr.isNull())
    return false;
  if (addr.toIPv4Address() == 0)
    return false;
  if (addr.isMulticast() || addr.isBroadcast() || addr.isLinkLocal())
    return false;
  return true;
}

QString NetSafety::sanitizeDccFileName(const QString &name) {
  // Take the last '/' component only. QFileInfo::fileName() treats '\\' as a
  // separator on Windows and would drop the prefix that other platforms keep
  // (and that the tests require) before it is escaped to '_'.
  QString base = name;
  const int slash = base.lastIndexOf(QLatin1Char('/'));
  if (slash >= 0)
    base = base.mid(slash + 1);
  if (base.isEmpty() || base == QLatin1String(".") || base == QLatin1String(".."))
    base = name;

  QString cleaned;
  cleaned.reserve(base.size());
  for (const QChar c : base) {
    const ushort u = c.unicode();
    if (u < 32 || c == QLatin1Char('<') || c == QLatin1Char('>') ||
        c == QLatin1Char(':') || c == QLatin1Char('"') || c == QLatin1Char('|') ||
        c == QLatin1Char('?') || c == QLatin1Char('*') || c == QLatin1Char('/') ||
        c == QLatin1Char('\\')) {
      cleaned.append(QLatin1Char('_'));
    } else {
      cleaned.append(c);
    }
  }
  cleaned.replace(QStringLiteral(".."), QStringLiteral("_"));

  while (cleaned.startsWith(QLatin1Char('.')) || cleaned.startsWith(QLatin1Char(' ')))
    cleaned.remove(0, 1);
  while (cleaned.endsWith(QLatin1Char('.')) || cleaned.endsWith(QLatin1Char(' ')))
    cleaned.chop(1);

  const int dot = cleaned.indexOf(QLatin1Char('.'));
  const QString stem = dot >= 0 ? cleaned.left(dot) : cleaned;
  if (isReservedWindowsName(stem))
    cleaned.prepend(QLatin1Char('_'));

  if (cleaned.isEmpty())
    cleaned = QStringLiteral("dcc-download");
  if (cleaned.size() > 200)
    cleaned = cleaned.left(200);
  return cleaned;
}
