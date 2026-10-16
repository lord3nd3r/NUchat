#include <QtTest>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "DccManager.h"
#include "ImageDownloader.h"
#include "NetSafety.h"

class TestNetSecurity : public QObject {
  Q_OBJECT

private slots:
  void initTestCase() {
    QCoreApplication::setOrganizationName(QStringLiteral("NUchatTest"));
    QCoreApplication::setApplicationName(QStringLiteral("NUchatNetSecurity"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    m_settingsDir.reset(new QTemporaryDir);
    QVERIFY(m_settingsDir->isValid());
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir->path());
  }

  void sanitizeStripsCrLfAndNul() {
    QCOMPARE(NetSafety::sanitizeIrcLine(QStringLiteral("PRIVMSG #a :hi\r\nQUIT :bye")),
             QStringLiteral("PRIVMSG #a :hiQUIT :bye"));
    QCOMPARE(NetSafety::sanitizeIrcLine(QStringLiteral("PRIVMSG #a :hello")),
             QStringLiteral("PRIVMSG #a :hello"));
    QString withNul = QStringLiteral("A\nB\rC");
    withNul.append(QChar('\0'));
    withNul.append(QLatin1Char('D'));
    QCOMPARE(NetSafety::sanitizeIrcLine(withNul), QStringLiteral("ABCD"));
    QVERIFY(NetSafety::sanitizeIrcLine(QStringLiteral("\r\n")).isEmpty());
  }

  void redactCredentials() {
    const QString pass = NetSafety::redactIrcLine(QStringLiteral("PASS :s3cret"));
    QVERIFY(!pass.contains(QLatin1String("s3cret")));
    QCOMPARE(pass, QStringLiteral("PASS <redacted>"));

    QCOMPARE(NetSafety::redactIrcLine(QStringLiteral("OPER admin hunter2")),
             QStringLiteral("OPER <redacted>"));

    QCOMPARE(NetSafety::redactIrcLine(QStringLiteral("AUTHENTICATE PLAIN")),
             QStringLiteral("AUTHENTICATE PLAIN"));
    QCOMPARE(NetSafety::redactIrcLine(QStringLiteral("AUTHENTICATE +")),
             QStringLiteral("AUTHENTICATE +"));

    const char raw[] = "user\0user\0hunter2";
    const QByteArray blob(raw, int(sizeof(raw) - 1));
    const QString b64 = QString::fromLatin1(blob.toBase64());
    const QString sasl = NetSafety::redactIrcLine(QStringLiteral("AUTHENTICATE ") + b64);
    QVERIFY(!sasl.contains(b64));
    QVERIFY(!sasl.contains(QLatin1String("hunter2")));
    QCOMPARE(sasl, QStringLiteral("AUTHENTICATE <redacted>"));

    const QString ident =
        NetSafety::redactIrcLine(QStringLiteral("PRIVMSG NickServ :IDENTIFY hunter2"));
    QVERIFY(!ident.contains(QLatin1String("hunter2")));
    QCOMPARE(ident, QStringLiteral("PRIVMSG NickServ :<redacted>"));

    const QString ns =
        NetSafety::redactIrcLine(QStringLiteral("PRIVMSG NS :IDENTIFY mynick hunter2"));
    QVERIFY(!ns.contains(QLatin1String("hunter2")));

    QCOMPARE(NetSafety::redactIrcLine(QStringLiteral("NICKSERV IDENTIFY hunter2")),
             QStringLiteral("NICKSERV <redacted>"));

    const QString chat =
        NetSafety::redactIrcLine(QStringLiteral("PRIVMSG #chan :please IDENTIFY with services"));
    QCOMPARE(chat, QStringLiteral("PRIVMSG #chan :please IDENTIFY with services"));

    QCOMPARE(NetSafety::redactIrcLine(QStringLiteral("PRIVMSG #chan :hello")),
             QStringLiteral("PRIVMSG #chan :hello"));
  }

  void tlsIgnoreListMatchesReadme() {
    QVERIFY(NetSafety::isIgnorableTlsError(QSslError::SelfSignedCertificate));
    QVERIFY(NetSafety::isIgnorableTlsError(QSslError::SelfSignedCertificateInChain));
    QVERIFY(NetSafety::isIgnorableTlsError(QSslError::HostNameMismatch));
    QVERIFY(NetSafety::isIgnorableTlsError(QSslError::CertificateUntrusted));
    QVERIFY(NetSafety::isIgnorableTlsError(QSslError::UnableToGetIssuerCertificate));
    QVERIFY(NetSafety::isIgnorableTlsError(QSslError::UnableToGetLocalIssuerCertificate));
    QVERIFY(NetSafety::isIgnorableTlsError(QSslError::UnableToVerifyFirstCertificate));

    QVERIFY(!NetSafety::isIgnorableTlsError(QSslError::CertificateExpired));
    QVERIFY(!NetSafety::isIgnorableTlsError(QSslError::CertificateNotYetValid));
    QVERIFY(!NetSafety::isIgnorableTlsError(QSslError::CertificateRevoked));
    QVERIFY(!NetSafety::isIgnorableTlsError(QSslError::CertificateSignatureFailed));
    QVERIFY(!NetSafety::isIgnorableTlsError(QSslError::NoPeerCertificate));
  }

  void proxyDoesNotFallBack() {
    const auto direct = NetSafety::resolveProxy(0, QString(), 0, false, {}, {});
    QCOMPARE(direct.result, NetSafety::ProxyResolution::Result::Direct);
    QCOMPARE(direct.proxy.type(), QNetworkProxy::NoProxy);

    const auto emptyHost = NetSafety::resolveProxy(2, QStringLiteral("  "), 1080, false, {}, {});
    QCOMPARE(emptyHost.result, NetSafety::ProxyResolution::Result::Rejected);
    QVERIFY(!emptyHost.error.isEmpty());

    const auto socks4 =
        NetSafety::resolveProxy(1, QStringLiteral("proxy.example"), 1080, false, {}, {});
    QCOMPARE(socks4.result, NetSafety::ProxyResolution::Result::Rejected);
    QVERIFY(socks4.error.contains(QLatin1String("SOCKS4")));

    const auto badPort =
        NetSafety::resolveProxy(2, QStringLiteral("proxy.example"), 0, false, {}, {});
    QCOMPARE(badPort.result, NetSafety::ProxyResolution::Result::Rejected);

    const auto hugePort =
        NetSafety::resolveProxy(3, QStringLiteral("proxy.example"), 70000, false, {}, {});
    QCOMPARE(hugePort.result, NetSafety::ProxyResolution::Result::Rejected);

    const auto socks5 = NetSafety::resolveProxy(2, QStringLiteral("proxy.example"), 1080, true,
                                                QStringLiteral("u"), QStringLiteral("p"));
    QCOMPARE(socks5.result, NetSafety::ProxyResolution::Result::UseProxy);
    QCOMPARE(socks5.proxy.type(), QNetworkProxy::Socks5Proxy);
    QCOMPARE(socks5.proxy.hostName(), QStringLiteral("proxy.example"));
    QCOMPARE(socks5.proxy.port(), static_cast<quint16>(1080));
    QCOMPARE(socks5.proxy.user(), QStringLiteral("u"));
    QCOMPARE(socks5.proxy.password(), QStringLiteral("p"));

    const auto http =
        NetSafety::resolveProxy(3, QStringLiteral("proxy.example"), 8080, false, {}, {});
    QCOMPARE(http.result, NetSafety::ProxyResolution::Result::UseProxy);
    QCOMPARE(http.proxy.type(), QNetworkProxy::HttpProxy);
  }

  void blockedAddresses() {
    const auto blocked = [](const char *text) {
      return NetSafety::isBlockedAddress(QHostAddress(QString::fromLatin1(text)));
    };
    QVERIFY(blocked("127.0.0.1"));
    QVERIFY(blocked("127.0.0.2"));
    QVERIFY(blocked("0.0.0.0"));
    QVERIFY(blocked("0.1.2.3"));
    QVERIFY(blocked("10.1.2.3"));
    QVERIFY(blocked("172.16.0.1"));
    QVERIFY(blocked("172.31.255.255"));
    QVERIFY(blocked("192.168.1.1"));
    QVERIFY(blocked("100.64.0.1"));
    QVERIFY(blocked("100.127.255.255"));
    QVERIFY(blocked("169.254.169.254"));
    QVERIFY(blocked("224.0.0.1"));
    QVERIFY(blocked("240.0.0.1"));
    QVERIFY(blocked("255.255.255.255"));
    QVERIFY(blocked("192.0.2.1"));
    QVERIFY(blocked("::1"));
    QVERIFY(blocked("::"));
    QVERIFY(blocked("::ffff:10.1.2.3"));
    QVERIFY(blocked("::ffff:127.0.0.1"));
    QVERIFY(blocked("fc00::1"));
    QVERIFY(blocked("fe80::1"));

    QVERIFY(!blocked("8.8.8.8"));
    QVERIFY(!blocked("1.1.1.1"));
    QVERIFY(!blocked("11.0.0.1"));
    QVERIFY(!blocked("172.15.255.255"));
    QVERIFY(!blocked("172.32.0.1"));
    QVERIFY(!blocked("100.63.255.255"));
    QVERIFY(!blocked("100.128.0.1"));
    QVERIFY(!blocked("::ffff:8.8.8.8"));
    QVERIFY(!blocked("2001:4860:4860::8888"));
  }

  void imageUrlSafety() {
    QVERIFY(NetSafety::isSafeImageUrl(QUrl(QStringLiteral("https://example.com/a.png"))));
    QVERIFY(NetSafety::isSafeImageUrl(QUrl(QStringLiteral("http://8.8.8.8/a.png"))));
    QVERIFY(!NetSafety::isSafeImageUrl(QUrl(QStringLiteral("file:///etc/passwd"))));
    QVERIFY(!NetSafety::isSafeImageUrl(QUrl(QStringLiteral("http://localhost/a.png"))));
    QVERIFY(!NetSafety::isSafeImageUrl(QUrl(QStringLiteral("http://127.0.0.1/a.png"))));
    QVERIFY(!NetSafety::isSafeImageUrl(QUrl(QStringLiteral("http://10.1.2.3/a.png"))));
    QVERIFY(!NetSafety::isSafeImageUrl(QUrl(QStringLiteral("http://2130706433/"))));
    QVERIFY(!NetSafety::isSafeImageUrl(QUrl(QStringLiteral("http://0x7f000001/"))));
    QVERIFY(!NetSafety::isSafeImageUrl(QUrl(QStringLiteral("http://[::1]/a.png"))));
    QVERIFY(!NetSafety::isSafeImageUrl(QUrl(QStringLiteral("http://[::ffff:10.1.2.3]/a.png"))));
    QVERIFY(!NetSafety::isSafeImageUrl(QUrl(QStringLiteral("http://metadata.google.internal/"))));
    QVERIFY(!NetSafety::isSafeImageUrl(QUrl(QStringLiteral("https://photos.local/a.png"))));
    QVERIFY(!NetSafety::isSafeImageUrl(QUrl()));
  }

  void imageBytesAndCache() {
    QVERIFY(!ImageDownloader::isDecodableImage(QByteArray("not an image")));
    QVERIFY(!ImageDownloader::isDecodableImage(QByteArray()));

    const QByteArray png = QByteArray::fromHex(
        "89504e470d0a1a0a0000000d4948445200000001000000010802000000907753de"
        "0000000c49444154789c63f8cfc0000003010100c9fe92ef0000000049454e44ae426082");
    QVERIFY2(ImageDownloader::isDecodableImage(png), "Qt could not decode a 1x1 PNG");

    const QString name = ImageDownloader::cacheFileName(QStringLiteral("https://example.com/a.png"));
    QVERIFY(name.endsWith(QLatin1String(".png")));
    QVERIFY(name.size() >= 64);
    QVERIFY(ImageDownloader::cacheFileName(QStringLiteral("https://example.com/a.png")) !=
            ImageDownloader::cacheFileName(QStringLiteral("https://example.com/b.png")));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QDateTime t0 = QDateTime::currentDateTimeUtc().addSecs(-3600);
    const struct {
      const char *file;
      char fill;
      int offsetSec;
    } files[] = {{"old.bin", 'a', 0}, {"mid.bin", 'b', 10}, {"new.bin", 'c', 20}};
    for (const auto &spec : files) {
      QFile f(dir.filePath(QString::fromLatin1(spec.file)));
      QVERIFY(f.open(QIODevice::WriteOnly));
      QCOMPARE(f.write(QByteArray(100, spec.fill)), static_cast<qint64>(100));
      f.close();
      QFile stamp(dir.filePath(QString::fromLatin1(spec.file)));
      QVERIFY(stamp.open(QIODevice::ReadWrite));
      QVERIFY(stamp.setFileTime(t0.addSecs(spec.offsetSec), QFileDevice::FileModificationTime));
      stamp.close();
    }

    QCOMPARE(ImageDownloader::trimCacheDirectory(dir.path(), 150), 2);
    QVERIFY(!QFile::exists(dir.filePath(QStringLiteral("old.bin"))));
    QVERIFY(!QFile::exists(dir.filePath(QStringLiteral("mid.bin"))));
    QVERIFY(QFile::exists(dir.filePath(QStringLiteral("new.bin"))));
  }

  void dccFileNames() {
    QCOMPARE(NetSafety::sanitizeDccFileName(QStringLiteral("photo.jpg")),
             QStringLiteral("photo.jpg"));
    QCOMPARE(NetSafety::sanitizeDccFileName(QStringLiteral("../etc/passwd")),
             QStringLiteral("passwd"));
    QCOMPARE(NetSafety::sanitizeDccFileName(QStringLiteral("..\\..\\secret.txt")),
             QStringLiteral("____secret.txt"));
    QCOMPARE(NetSafety::sanitizeDccFileName(QStringLiteral(".bashrc")),
             QStringLiteral("bashrc"));
    QCOMPARE(NetSafety::sanitizeDccFileName(QStringLiteral("CON")),
             QStringLiteral("_CON"));
    QCOMPARE(NetSafety::sanitizeDccFileName(QStringLiteral("aux.txt")),
             QStringLiteral("_aux.txt"));
    QCOMPARE(NetSafety::sanitizeDccFileName(QString()),
             QStringLiteral("dcc-download"));
    QCOMPARE(NetSafety::sanitizeDccFileName(QStringLiteral("...")), QStringLiteral("_"));
    QVERIFY(!NetSafety::sanitizeDccFileName(QStringLiteral("a/b\\c")).contains(QLatin1Char('/')));
    QVERIFY(!NetSafety::sanitizeDccFileName(QStringLiteral("a/b\\c")).contains(QLatin1Char('\\')));
  }

  void dccPeerValidation() {
    const auto ip = [](const char *text) {
      return QHostAddress(QString::fromLatin1(text)).toIPv4Address();
    };
    QVERIFY(NetSafety::dccReceiveBlockReason(ip("8.8.8.8"), 5000, false).isEmpty());
    QVERIFY(NetSafety::dccReceiveBlockReason(ip("8.8.8.8"), 0, true).contains(QLatin1String("port")));
    QVERIFY(!NetSafety::dccReceiveBlockReason(ip("127.0.0.1"), 22, true).isEmpty());
    QVERIFY(!NetSafety::dccReceiveBlockReason(ip("0.0.0.0"), 80, true).isEmpty());
    QVERIFY(!NetSafety::dccReceiveBlockReason(ip("169.254.1.1"), 80, true).isEmpty());
    QVERIFY(NetSafety::dccReceiveBlockReason(ip("192.168.1.5"), 5000, false)
                .contains(QLatin1String("private")));
    QVERIFY(NetSafety::dccReceiveBlockReason(ip("10.0.0.5"), 5000, true).isEmpty());
    QVERIFY(NetSafety::dccReceiveBlockReason(ip("100.64.1.1"), 5000, false)
                .contains(QLatin1String("private")));

    QVERIFY(!NetSafety::isUsableDccBindAddress(QHostAddress::Any));
    QVERIFY(!NetSafety::isUsableDccBindAddress(QHostAddress::AnyIPv4));
    QVERIFY(!NetSafety::isUsableDccBindAddress(QHostAddress(QStringLiteral("0.0.0.0"))));
    QVERIFY(!NetSafety::isUsableDccBindAddress(QHostAddress(QStringLiteral("::1"))));
    QVERIFY(NetSafety::isUsableDccBindAddress(QHostAddress(QStringLiteral("192.168.1.20"))));
    QVERIFY(NetSafety::isUsableDccBindAddress(QHostAddress(QStringLiteral("127.0.0.1"))));
  }

  void dccOfferRejectedBeforeConnect() {
    DccManager mgr;
    QSignalSpy failed(&mgr, &DccManager::transferFailed);
    const quint32 loopback = QHostAddress(QStringLiteral("127.0.0.1")).toIPv4Address();
    mgr.handleDccSend(QStringLiteral("evil"), QStringLiteral("a.txt"), loopback, 22, 100);
    QCOMPARE(mgr.count(), 0);
    QCOMPARE(failed.count(), 1);

    mgr.handleDccSend(QStringLiteral("evil"), QStringLiteral("a.txt"), 0, 80, 100);
    QCOMPARE(mgr.count(), 0);

    const quint32 lan = QHostAddress(QStringLiteral("192.168.1.5")).toIPv4Address();
    mgr.setAutoAccept(true);
    mgr.handleDccSend(QStringLiteral("bob"), QStringLiteral("pic.png"), lan, 5000, 100);
    QCOMPARE(mgr.count(), 1);
    QCOMPARE(mgr.data(mgr.index(0), DccManager::StatusRole).toString(),
             QStringLiteral("Pending"));

    DccTransfer direct;
    direct.peerAddress = loopback;
    direct.peerPort = 80;
    direct.fileName = QStringLiteral("x.txt");
    direct.direction = DccTransfer::Receive;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    direct.startReceive(dir.path());
    QCOMPARE(direct.state, DccTransfer::Failed);
    QCOMPARE(QDir(dir.path()).entryList(QDir::Files).size(), 0);
  }

  void dccSendRefusesWildcardBind() {
    DccTransfer send;
    send.startSend(QStringLiteral("/etc/hostname"), 0, QHostAddress::Any);
    QCOMPARE(send.state, DccTransfer::Failed);
    QCOMPARE(send.direction, DccTransfer::Send);
    QCOMPARE(send.peerPort, static_cast<quint16>(0));
  }

  void dccFailureDeletesPartialReceiveOnly() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    DccTransfer recv;
    recv.direction = DccTransfer::Receive;
    recv.state = DccTransfer::Transferring;
    recv.filePath = dir.filePath(QStringLiteral("partial.bin"));
    QFile partial(recv.filePath);
    QVERIFY(partial.open(QIODevice::WriteOnly));
    partial.write("partial");
    partial.close();
    recv.onIdleTimeout();
    QCOMPARE(recv.state, DccTransfer::Failed);
    QVERIFY(!QFile::exists(recv.filePath));

    DccTransfer send;
    send.direction = DccTransfer::Send;
    send.state = DccTransfer::Transferring;
    send.filePath = dir.filePath(QStringLiteral("original.bin"));
    QFile original(send.filePath);
    QVERIFY(original.open(QIODevice::WriteOnly));
    original.write("keep");
    original.close();
    send.failTransfer(QStringLiteral("stalled"));
    QCOMPARE(send.state, DccTransfer::Failed);
    QVERIFY(QFile::exists(send.filePath));
  }

private:
  QScopedPointer<QTemporaryDir> m_settingsDir;
};

QTEST_MAIN(TestNetSecurity)
#include "test_netsecurity.moc"
