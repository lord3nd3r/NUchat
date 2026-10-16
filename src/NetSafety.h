#pragma once

#include <QHostAddress>
#include <QList>
#include <QNetworkProxy>
#include <QSslError>
#include <QString>
#include <QUrl>

// Pure checks shared by the IRC socket, the image fetcher, and DCC.
// Kept free of sockets so unit tests can exercise them directly.
class NetSafety {
public:
  // Drop CR, LF, and NUL so one outbound line cannot become several commands.
  static QString sanitizeIrcLine(const QString &line);

  // Log form of an IRC line with PASS / SASL / NickServ secrets removed.
  static QString redactIrcLine(const QString &line);

  // True only for the TLS failures the README says "accept self-signed" covers:
  // self-signed certs, hostname mismatches, and untrusted issuer chains.
  static bool isIgnorableTlsError(QSslError::SslError error);

  struct ProxyResolution {
    enum class Result { Direct, UseProxy, Rejected };
    Result result = Result::Direct;
    QString error;
    QNetworkProxy proxy;
  };

  // typeIndex matches Preferences: 0 none, 1 SOCKS4, 2 SOCKS5, 3 HTTP CONNECT.
  // A requested proxy that Qt cannot apply is Rejected — never turned into a
  // direct connection or a different proxy protocol.
  static ProxyResolution resolveProxy(int typeIndex, const QString &host, int port,
                                      bool useAuth, const QString &user,
                                      const QString &password);
  static ProxyResolution proxyFromSettings();

  // True for loopback, private, link-local, CGNAT, reserved, and IPv4-mapped
  // forms of those ranges. Used to block image fetches.
  static bool isBlockedAddress(const QHostAddress &addr);
  static bool parseHostLiteral(const QString &host, QHostAddress *out);
  static bool isSafeImageUrl(const QUrl &url);
  // Empty list fails closed (treat as blocked).
  static bool anyAddressBlocked(const QList<QHostAddress> &addrs);

  enum class DccPeerClass { Public, PrivateLan, Forbidden };
  static DccPeerClass classifyDccPeer(quint32 ipv4);
  // Empty when the offer may be acted on. Private LAN addresses are rejected
  // unless allowPrivateLan is true; loopback and non-unicast always are.
  static QString dccReceiveBlockReason(quint32 ipv4, quint16 port, bool allowPrivateLan);
  static bool isUsableDccBindAddress(const QHostAddress &addr);
  static QString sanitizeDccFileName(const QString &name);
};
