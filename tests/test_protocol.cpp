#include <QtTest>
#include "DccManager.h"
#include "IRCConnectionManager.h"
#include "IrcConnection.h"
#include "IrcIdentity.h"
#include "MessageModel.h"
#include "ServerChannelModel.h"
#include "SpellDict.h"
#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTemporaryDir>

class IrcConnectionTestable : public IrcConnection {
public:
  using IrcConnection::IrcConnection;
  void feedLine(const QString &line) { processLine(line); }
  void forceConnectTimeout() { handleConnectTimeout(); }
  void armConnect() {
    m_connectPending = true;
    m_socketReady = false;
    m_host = QStringLiteral("irc.example");
  }
};

class TestProtocol : public QObject {
  Q_OBJECT

private slots:
  void init() { IrcNetworkRegistry::clear(); }

  void testCaseMappingRfc1459() {
    QCOMPARE(ircCaseMappingFromToken(QStringLiteral("rfc1459")),
             IrcCaseMapping::Rfc1459);
    QCOMPARE(ircCaseMappingFromToken(QStringLiteral("strict-rfc1459")),
             IrcCaseMapping::StrictRfc1459);
    QCOMPARE(ircCaseMappingFromToken(QStringLiteral("ascii")),
             IrcCaseMapping::Ascii);
    QCOMPARE(ircCaseMappingFromToken(QStringLiteral("unknown")),
             IrcCaseMapping::Rfc1459);

    QVERIFY(ircEquals(QStringLiteral("Nick^"), QStringLiteral("nick~"),
                      IrcCaseMapping::Rfc1459));
    QVERIFY(ircEquals(QStringLiteral("#Chan["), QStringLiteral("#chan{"),
                      IrcCaseMapping::Rfc1459));
    QVERIFY(!ircEquals(QStringLiteral("Nick^"), QStringLiteral("nick~"),
                       IrcCaseMapping::StrictRfc1459));
    QVERIFY(ircEquals(QStringLiteral("#Chan["), QStringLiteral("#chan{"),
                      IrcCaseMapping::StrictRfc1459));
    QVERIFY(!ircEquals(QStringLiteral("Nick^"), QStringLiteral("nick~"),
                       IrcCaseMapping::Ascii));
    QVERIFY(!ircEquals(QStringLiteral("#Chan["), QStringLiteral("#chan{"),
                       IrcCaseMapping::Ascii));
    QVERIFY(ircEquals(QStringLiteral("Alice"), QStringLiteral("alice"),
                      IrcCaseMapping::Ascii));
  }

  void testIsupportTokens() {
    IrcSupport support;
    support.apply005(QStringList()
                     << QStringLiteral("PREFIX=(ov)@+")
                     << QStringLiteral("CHANTYPES=#&+")
                     << QStringLiteral("CHANMODES=beI,k,l,imnpst")
                     << QStringLiteral("CASEMAPPING=ascii")
                     << QStringLiteral("are supported by this server"));
    QCOMPARE(support.prefixModes(), QStringLiteral("ov"));
    QCOMPARE(support.prefixSymbols(), QStringLiteral("@+"));
    QCOMPARE(support.prefixForMode(QLatin1Char('o')), QLatin1Char('@'));
    QCOMPARE(support.prefixForMode(QLatin1Char('v')), QLatin1Char('+'));
    QVERIFY(support.prefixForMode(QLatin1Char('q')).isNull());
    QVERIFY(support.isChannel(QStringLiteral("+chat")));
    QVERIFY(support.isChannel(QStringLiteral("#chat")));
    QVERIFY(!support.isChannel(QStringLiteral("nick")));
    QCOMPARE(support.caseMapping(), IrcCaseMapping::Ascii);
    QVERIFY(support.modeTakesParam(QLatin1Char('b'), false));
    QVERIFY(support.modeTakesParam(QLatin1Char('k'), false));
    QVERIFY(support.modeTakesParam(QLatin1Char('l'), true));
    QVERIFY(!support.modeTakesParam(QLatin1Char('l'), false));
    QVERIFY(!support.modeTakesParam(QLatin1Char('i'), true));
    const auto split = support.splitNick(QStringLiteral("@+alice"));
    QCOMPARE(split.first, QStringLiteral("@+"));
    QCOMPARE(split.second, QStringLiteral("alice"));
  }

  void testDefaultModesMatchPreviousHardCoding() {
    IrcSupport support;
    QVERIFY(support.isChannel(QStringLiteral("#a")));
    QVERIFY(support.isChannel(QStringLiteral("&a")));
    QVERIFY(!support.isChannel(QStringLiteral("+a")));
    QCOMPARE(support.prefixForMode(QLatin1Char('q')), QLatin1Char('~'));
    QCOMPARE(support.prefixForMode(QLatin1Char('o')), QLatin1Char('@'));
    QVERIFY(support.modeTakesParam(QLatin1Char('I'), false));
    QVERIFY(support.modeTakesParam(QLatin1Char('k'), false));
    QVERIFY(support.modeTakesParam(QLatin1Char('f'), true));
    QVERIFY(!support.modeTakesParam(QLatin1Char('f'), false));
    QVERIFY(support.modeTakesParam(QLatin1Char('j'), true));
    QVERIFY(!support.modeTakesParam(QLatin1Char('n'), true));
  }

  void testSaslChunking() {
    QCOMPARE(saslAuthenticateCommands(QByteArray()),
             QStringList() << QStringLiteral("AUTHENTICATE +"));
    const QStringList one =
        saslAuthenticateCommands(QByteArray("abc"));
    QCOMPARE(one, QStringList() << QStringLiteral("AUTHENTICATE abc"));

    const QByteArray exact(400, 'A');
    const QStringList split = saslAuthenticateCommands(exact);
    QCOMPARE(split.size(), 2);
    QCOMPARE(split.at(0).size(), QStringLiteral("AUTHENTICATE ").size() + 400);
    QCOMPARE(split.at(1), QStringLiteral("AUTHENTICATE +"));

    const QByteArray over = exact + 'B';
    const QStringList longer = saslAuthenticateCommands(over);
    QCOMPARE(longer.size(), 2);
    QVERIFY(!longer.at(1).endsWith(QLatin1Char('+')));
    QCOMPARE(longer.at(1), QStringLiteral("AUTHENTICATE B"));
  }

  void testTimerAndReconnectBounds() {
    QCOMPARE(timerIntervalMs(1.5), 1500);
    QCOMPARE(timerIntervalMs(0.0), -1);
    QCOMPARE(timerIntervalMs(-3), -1);
    QVERIFY(timerIntervalMs(1e20) < 0);
    QVERIFY(timerIntervalMs(3000000.0) < 0);

    QCOMPARE(reconnectDelayMs(10, 1), 10000);
    QCOMPARE(reconnectDelayMs(10, 2), 20000);
    QCOMPARE(reconnectDelayMs(10, 3), 40000);
    QCOMPARE(reconnectDelayMs(10, 30), 15 * 60 * 1000);
    QCOMPARE(reconnectDelayMs(2000000000, 8), 15 * 60 * 1000);
  }

  void testClassifyOutbound() {
    const OutboundLine slash = classifyOutbound(QStringLiteral("//hello"));
    QCOMPARE(int(slash.kind), int(OutboundKind::Literal));
    QCOMPARE(slash.literal, QStringLiteral("/hello"));

    const OutboundLine path = classifyOutbound(QStringLiteral("/etc/passwd"));
    QCOMPARE(int(path.kind), int(OutboundKind::Literal));
    QCOMPARE(path.literal, QStringLiteral("/etc/passwd"));

    const OutboundLine cmd = classifyOutbound(QStringLiteral("/msg Bob hi there"));
    QCOMPARE(int(cmd.kind), int(OutboundKind::Command));
    QCOMPARE(cmd.command, QStringLiteral("MSG"));
    QCOMPARE(cmd.args, QStringLiteral("Bob hi there"));

    const OutboundLine plain = classifyOutbound(QStringLiteral("hello"));
    QCOMPARE(int(plain.kind), int(OutboundKind::Literal));
    QCOMPARE(plain.literal, QStringLiteral("hello"));
  }

  void testServerTimeParse() {
    const QDateTime dt = parseServerTime(QStringLiteral("2011-10-19T16:40:51.620Z"));
    QVERIFY(dt.isValid());
    QCOMPARE(dt.toUTC().date(), QDate(2011, 10, 19));
    QCOMPARE(dt.toUTC().time().hour(), 16);
    QVERIFY(!parseServerTime(QString()).isValid());
    QVERIFY(!parseServerTime(QStringLiteral("yesterday")).isValid());
  }

  void testSpellDictionarySearch() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile dic(dir.filePath(QStringLiteral("en_US.dic")));
    QFile aff(dir.filePath(QStringLiteral("en_US.aff")));
    QVERIFY(dic.open(QIODevice::WriteOnly));
    QVERIFY(aff.open(QIODevice::WriteOnly));
    dic.write("1\nhello\n");
    aff.write("SET UTF-8\n");
    dic.close();
    aff.close();

    const auto found = locateHunspellDictionary(QStringLiteral("en-US"),
                                                QStringList() << dir.path());
    QCOMPARE(found.first, dic.fileName());
    QCOMPARE(found.second, aff.fileName());

    const auto missing = locateHunspellDictionary(QStringLiteral("zz_ZZ"),
                                                  QStringList() << dir.path());
    QVERIFY(missing.first.isEmpty());

    const QStringList roots = spellDictionaryRoots();
    QVERIFY(!roots.isEmpty());
#if defined(Q_OS_WIN)
    QVERIFY(roots.join(QLatin1Char('\n')).contains(QStringLiteral("hunspell")));
#elif defined(Q_OS_MAC)
    QVERIFY(roots.contains(QStringLiteral("/opt/homebrew/share/hunspell")) ||
            roots.contains(QStringLiteral("/usr/local/share/hunspell")));
#else
    QVERIFY(roots.contains(QStringLiteral("/usr/share/hunspell")));
    QVERIFY(roots.contains(QStringLiteral("/usr/share/myspell/dicts")));
#endif
  }

  void testDccAddressAndPorts() {
    const auto ephemeral = dccPortCandidates(0, 0);
    QCOMPARE(ephemeral, QList<quint16>() << 0);
    const auto range = dccPortCandidates(41000, 41002);
    QCOMPARE(range.size(), 3);
    QCOMPARE(range.at(0), quint16(41000));
    QCOMPARE(range.at(2), quint16(41002));
    QCOMPARE(dccPortCandidates(41000, 42000, 2).size(), 2);

    const auto manual = dccAdvertisedAddress(2, 1, QStringLiteral("198.51.100.10"));
    QVERIFY(manual.ok);
    QCOMPARE(manual.ipv4, QHostAddress(QStringLiteral("198.51.100.10")).toIPv4Address());
    QVERIFY(!dccAdvertisedAddress(2, 1, QStringLiteral("not-an-ip")).ok);
    QVERIFY(!dccAdvertisedAddress(2, 1, QString()).ok);
    const auto automatic = dccAdvertisedAddress(0, 0x7f000001, QString());
    QVERIFY(automatic.ok);
    QCOMPARE(automatic.ipv4, quint32(0x7f000001));
    const auto fromServer = dccAdvertisedAddress(1, 0x0a000001, QStringLiteral("1.2.3.4"));
    QVERIFY(fromServer.ok);
    QCOMPARE(fromServer.ipv4, quint32(0x0a000001));
  }

  void testDccListensInRange() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("offer.bin"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("hi");
    file.close();

    DccTransfer transfer;
    transfer.startSend(path, 0, QHostAddress(QHostAddress::LocalHost), 42000, 42010);
    QCOMPARE(transfer.state, DccTransfer::Pending);
    QVERIFY(transfer.peerPort >= 42000 && transfer.peerPort <= 42010);
    transfer.cancel();
  }

  void testClientCertLoad() {
    const ClientCertMaterial none = loadClientCertificate(QString(), QString());
    QVERIFY(none.error.isEmpty());
    QVERIFY(!none.requested);

    const ClientCertMaterial half =
        loadClientCertificate(QStringLiteral("/tmp/cert.pem"), QString());
    QVERIFY(!half.error.isEmpty());

    const ClientCertMaterial missing = loadClientCertificate(
        QStringLiteral("/no/such/cert.pem"), QStringLiteral("/no/such/key.pem"));
    QVERIFY(missing.error.contains(QStringLiteral("Cannot read")));
  }

  void testConnectTimeout() {
    IrcConnectionTestable conn;
    QSignalSpy spy(&conn, &IrcConnection::errorOccurred);
    conn.armConnect();
    conn.forceConnectTimeout();
    QCOMPARE(spy.count(), 1);
    QVERIFY(spy.first().at(0).toString().contains(QStringLiteral("timed out")));
    conn.forceConnectTimeout();
    QCOMPARE(spy.count(), 1);
  }

  void testRefusedConnectEmitsDisconnectOnce() {
    // A closed localhost port is refused immediately: Qt emits errorOccurred
    // but never disconnected(), so the connection must report it itself.
    quint16 port = 0;
    {
      QTcpServer probe;
      QVERIFY(probe.listen(QHostAddress::LocalHost, 0));
      port = probe.serverPort();
    }
    IrcConnection conn;
    QSignalSpy gone(&conn, &IrcConnection::disconnectedFromServer);
    conn.connectToServer(QStringLiteral("127.0.0.1"), port, false);
    QTRY_VERIFY_WITH_TIMEOUT(gone.count() >= 1, 5000);
    QTest::qWait(200);
    // What matters is exactly one disconnect, so exactly one reconnect is
    // scheduled. Whether an error message is also emitted depends on the
    // platform's socket backend (macOS reports the refusal differently).
    QCOMPARE(gone.count(), 1);
  }

  void testLiteralSlashAndIsupportInManager() {
    IRCConnectionManager mgr;
    MessageModel model;
    ServerChannelModel tree;
    mgr.setMessageModel(&model);
    mgr.setServerChannelModel(&tree);

    auto *conn = new IrcConnectionTestable;
    conn->setNickname(QStringLiteral("Alice"));
    mgr.m_connections.append(conn);
    mgr.m_connToName.insert(conn, QStringLiteral("alpha"));
    mgr.wireConnection(conn);
    mgr.m_activeServer = QStringLiteral("alpha");
    mgr.m_activeChannel = QStringLiteral("#room");

    mgr.sendMessage(QStringLiteral("#room"), QStringLiteral("//hello"));
    QVERIFY(historyContains(mgr, QStringLiteral("#room"), QStringLiteral("<Alice> /hello")));

    mgr.sendMessage(QStringLiteral("#room"), QStringLiteral("/etc/passwd"));
    QVERIFY(historyContains(mgr, QStringLiteral("#room"),
                            QStringLiteral("<Alice> /etc/passwd")));

    mgr.sendMessage(QStringLiteral("#room"), QStringLiteral("/ECHO kept-local"));
    QVERIFY(modelText(model).contains(QStringLiteral("kept-local")));

    mgr.sendMessage(QStringLiteral("#room"),
                    QStringLiteral("/TIMER 9000000 /ECHO too-far"));
    QVERIFY(modelText(model).contains(QStringLiteral("too large")));

    conn->feedLine(QStringLiteral(
        ":irc.example 005 Alice PREFIX=(y)~ CHANTYPES=#&+ "
        "CHANMODES=b,k,l,nt CASEMAPPING=ascii :are supported by this server"));
    QCOMPARE(conn->support().caseMapping(), IrcCaseMapping::Ascii);
    QVERIFY(conn->support().isChannel(QStringLiteral("+lounge")));
    QVERIFY(!conn->support().same(QStringLiteral("A^"), QStringLiteral("a~")));

    conn->feedLine(QStringLiteral(":irc.example 353 Alice = #room :alice"));
    conn->feedLine(QStringLiteral(":irc.example 366 Alice #room :End of /NAMES list"));
    conn->feedLine(QStringLiteral(":op!o@h MODE #room +y alice"));
    const auto users =
        mgr.m_users.value(IRCConnectionManager::ChannelKey{QStringLiteral("alpha"),
                                                           QStringLiteral("#room")});
    QCOMPARE(users, QStringList() << QStringLiteral("~alice"));

    conn->feedLine(QStringLiteral(
        "@time=2011-10-19T16:40:51.620Z :bob!b@h PRIVMSG #room :from-server"));
    const QDateTime expected =
        QDateTime::fromString(QStringLiteral("2011-10-19T16:40:51.620Z"),
                              Qt::ISODateWithMs)
            .toLocalTime();
    bool stamped = false;
    const auto hist =
        mgr.m_history.value(IRCConnectionManager::ChannelKey{QStringLiteral("alpha"),
                                                             QStringLiteral("#room")});
    for (const auto &msg : hist) {
      if (msg.text.contains(QStringLiteral("from-server"))) {
        QCOMPARE(msg.timestamp, expected.toString(Qt::ISODate));
        stamped = true;
      }
    }
    QVERIFY(stamped);
    QVERIFY(modelText(model).contains(QStringLiteral("from-server")));
  }

private:
  static bool historyContains(IRCConnectionManager &mgr, const QString &channel,
                              const QString &needle) {
    const IRCConnectionManager::ChannelKey key{QStringLiteral("alpha"), channel};
    const auto it = mgr.m_history.constFind(key);
    if (it == mgr.m_history.constEnd())
      return false;
    for (const auto &msg : *it) {
      if (msg.text.contains(needle))
        return true;
    }
    return false;
  }

  static QString modelText(const MessageModel &model) {
    QString out;
    for (int i = 0; i < model.rowCount(); ++i)
      out += model.data(model.index(i), MessageModel::TextRole).toString();
    return out;
  }
};

QTEST_MAIN(TestProtocol)
#include "test_protocol.moc"
