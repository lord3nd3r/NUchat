// ── Command dispatch table ──
// Called once lazily from handleSlashCommand.
// Each entry maps a command name (uppercase) to a lambda handler.
#include "IRCConnectionManager.h"
#include "DccManager.h"
#include "IrcConnection.h"
#include "MessageModel.h"
#include "Settings.h"
#include <QPointer>
#include <QProcess>
#include <QTimer>

void IRCConnectionManager::initCommandTable() {
  auto &T = m_commandTable;

  // Helper: service alias (e.g. /NS -> NickServ)
  auto svcAlias = [](const QString &svc) {
    return [svc](IrcConnection *conn, const QString &, const QString &args) -> bool {
      conn->sendMessage(svc, args); return true;
    };
  };
  // Helper: channel mode shortcut (e.g. /OP -> MODE +o)
  auto modeAlias = [](const QString &flag) {
    return [flag](IrcConnection *conn, const QString &target, const QString &args) -> bool {
      QString who = args.isEmpty() ? conn->nickname() : args.trimmed();
      conn->sendRaw("MODE " + target + " " + flag + " " + who); return true;
    };
  };
  // Helper: ban mode (auto-appends !*@* if needed)
  auto banAlias = [](const QString &flag) {
    return [flag](IrcConnection *conn, const QString &target, const QString &args) -> bool {
      QString mask = args.trimmed();
      if (!mask.contains('!') && !mask.contains('@') && !mask.contains('*'))
        mask += "!*@*";
      conn->sendRaw("MODE " + target + " " + flag + " " + mask); return true;
    };
  };
  // Helper: simple raw passthrough (e.g. /OPER -> OPER args)
  auto rawCmd = [](const QString &prefix) {
    return [prefix](IrcConnection *conn, const QString &, const QString &args) -> bool {
      conn->sendRaw(prefix + (args.isEmpty() ? "" : " " + args)); return true;
    };
  };

  // ═══════════════════════════════════════════════════
  //  Chat commands
  // ═══════════════════════════════════════════════════

  T["SAY"] = [this](IrcConnection *conn, const QString &target, const QString &args) -> bool {
    QString ch = target.isEmpty() ? m_activeChannel : target;
    if (!ch.isEmpty() && !args.isEmpty()) {
      conn->sendMessage(ch, args);
      QString nick = conn->nickname(), displayNick = nick;
      ChannelKey key{m_activeServer, ch};
      if (m_users.contains(key)) {
        for (const QString &u : m_users[key]) {
          auto [pfx, bare] = IrcConnection::stripNickPrefix(u);
          if (bare.compare(nick, Qt::CaseInsensitive) == 0) {
            if (!pfx.isEmpty()) displayNick = pfx[0] + nick;
            break;
          }
        }
      }
      QString text = "<" + displayNick + "> " + args;
      if (m_msgModel && m_activeChannel == ch) m_msgModel->addMessage("chat", text);
      appendToChannel(m_activeServer, ch, "chat", text);
    }
    return true;
  };

  auto msgHandler = [this](IrcConnection *conn, const QString &, const QString &args) -> bool {
    QString tgt = args.section(' ', 0, 0), msg = args.section(' ', 1);
    if (!tgt.startsWith('#') && !tgt.startsWith('&')) openQuery(m_activeServer, tgt);
    if (!msg.isEmpty()) {
      conn->sendMessage(tgt, msg);
      QString text = "<" + conn->nickname() + "> " + msg;
      appendToChannel(m_activeServer, tgt, "chat", text);
      if (m_msgModel && m_activeChannel == tgt) m_msgModel->addMessage("chat", text);
    }
    return true;
  };
  T["MSG"] = msgHandler; T["PRIVMSG"] = msgHandler; T["M"] = msgHandler;
  T["QUERY"] = msgHandler; T["Q"] = msgHandler;

  T["ME"] = [this](IrcConnection *conn, const QString &target, const QString &args) -> bool {
    conn->sendMessage(target, "\x01" "ACTION " + args + "\x01");
    QString text = "* " + conn->nickname() + " " + args;
    if (m_msgModel && m_activeChannel == target) m_msgModel->addMessage("action", text);
    appendToChannel(m_activeServer, target, "action", text);
    return true;
  };

  T["NOTICE"] = [](IrcConnection *conn, const QString &, const QString &args) -> bool {
    conn->sendNotice(args.section(' ', 0, 0), args.section(' ', 1)); return true;
  };

  T["SLAP"] = [this](IrcConnection *conn, const QString &target, const QString &args) -> bool {
    QString who = args.trimmed();
    QString msg = "slaps " + who + " around a bit with a large trout";
    conn->sendMessage(target, "\x01" "ACTION " + msg + "\x01");
    QString text = "* " + conn->nickname() + " " + msg;
    if (m_msgModel && m_activeChannel == target) m_msgModel->addMessage("action", text);
    appendToChannel(m_activeServer, target, "action", text);
    return true;
  };

  T["CTCP"] = [](IrcConnection *conn, const QString &, const QString &args) -> bool {
    QString tgt = args.section(' ', 0, 0);
    QString type = args.section(' ', 1, 1).toUpper();
    QString extra = args.section(' ', 2);
    if (type.isEmpty()) type = "VERSION";
    conn->sendMessage(tgt, "\x01" + type + (extra.isEmpty() ? "" : " " + extra) + "\x01");
    return true;
  };

  // ═══════════════════════════════════════════════════
  //  Channel commands
  // ═══════════════════════════════════════════════════

  auto joinHandler = [](IrcConnection *conn, const QString &, const QString &args) -> bool {
    conn->joinChannel(args.section(' ', 0, 0), args.section(' ', 1, 1)); return true;
  };
  T["JOIN"] = joinHandler; T["J"] = joinHandler;

  auto partHandler = [](IrcConnection *conn, const QString &target, const QString &args) -> bool {
    conn->partChannel(args.isEmpty() ? target : args); return true;
  };
  T["PART"] = partHandler; T["LEAVE"] = partHandler; T["P"] = partHandler;

  auto topicHandler = [](IrcConnection *conn, const QString &target, const QString &args) -> bool {
    conn->sendRaw(args.isEmpty() ? "TOPIC " + target : "TOPIC " + target + " :" + args);
    return true;
  };
  T["TOPIC"] = topicHandler; T["T"] = topicHandler;

  T["MODE"] = [](IrcConnection *conn, const QString &target, const QString &args) -> bool {
    conn->sendRaw("MODE " + (args.isEmpty() ? target : args)); return true;
  };

  T["KICK"] = [](IrcConnection *conn, const QString &target, const QString &args) -> bool {
    QString who = args.section(' ', 0, 0), reason = args.section(' ', 1);
    conn->sendRaw("KICK " + target + " " + who + (reason.isEmpty() ? "" : " :" + reason));
    return true;
  };

  auto inviteHandler = [](IrcConnection *conn, const QString &target, const QString &args) -> bool {
    conn->sendRaw("INVITE " + args.trimmed() + " " + target); return true;
  };
  T["INVITE"] = inviteHandler; T["INV"] = inviteHandler;

  auto cycleHandler = [](IrcConnection *conn, const QString &target, const QString &) -> bool {
    conn->partChannel(target, "Cycling"); conn->joinChannel(target); return true;
  };
  T["CYCLE"] = cycleHandler; T["REJOIN"] = cycleHandler;

  T["CLEAR"] = [this](IrcConnection *, const QString &, const QString &) -> bool {
    if (m_msgModel) m_msgModel->clear();
    return true;
  };
  T["CLOSE"] = [](IrcConnection *conn, const QString &target, const QString &args) -> bool {
    conn->partChannel(args.isEmpty() ? target : args.trimmed()); return true;
  };

  // ═══════════════════════════════════════════════════
  //  User-mode shortcuts
  // ═══════════════════════════════════════════════════

  T["OP"] = modeAlias("+o");       T["DEOP"] = modeAlias("-o");
  T["HALFOP"] = modeAlias("+h");   T["HOP"] = modeAlias("+h");
  T["DEHALFOP"] = modeAlias("-h"); T["DEHOP"] = modeAlias("-h");
  T["VOICE"] = modeAlias("+v");    T["V"] = modeAlias("+v");
  T["DEVOICE"] = modeAlias("-v");

  // ═══════════════════════════════════════════════════
  //  Ban shortcuts
  // ═══════════════════════════════════════════════════

  T["BAN"] = banAlias("+b"); T["B"] = banAlias("+b");
  T["UNBAN"] = banAlias("-b");

  T["KB"] = [this](IrcConnection *conn, const QString &target, const QString &args) -> bool {
    QString who = args.section(' ', 0, 0), reason = args.section(' ', 1);
    if (reason.isEmpty()) reason = "Banned";
    QString mask = who;
    if (!mask.contains('!') && !mask.contains('@') && !mask.contains('*')) mask = who + "!*@*";
    conn->sendRaw("MODE " + target + " +b " + mask);
    conn->sendRaw("KICK " + target + " " + who + " :" + reason);
    return true;
  };
  T["KICKBAN"] = T["KB"];

  // ═══════════════════════════════════════════════════
  //  Service aliases
  // ═══════════════════════════════════════════════════

  T["NS"] = svcAlias("NickServ"); T["NICKSERV"] = svcAlias("NickServ");
  T["CS"] = svcAlias("ChanServ"); T["CHANSERV"] = svcAlias("ChanServ");
  T["OS"] = svcAlias("OperServ"); T["OPERSERV"] = svcAlias("OperServ");
  T["HS"] = svcAlias("HostServ"); T["HOSTSERV"] = svcAlias("HostServ");
  T["MS"] = svcAlias("MemoServ"); T["MEMOSERV"] = svcAlias("MemoServ");
  T["BS"] = svcAlias("BotServ");  T["BOTSERV"]  = svcAlias("BotServ");
  T["HELPSERV"] = svcAlias("HelpServ");

  // ═══════════════════════════════════════════════════
  //  Connection commands
  // ═══════════════════════════════════════════════════

  T["NICK"] = [](IrcConnection *conn, const QString &, const QString &args) -> bool {
    conn->changeNick(args.trimmed()); return true;
  };

  auto quitHandler = [this](IrcConnection *conn, const QString &, const QString &args) -> bool {
    m_userDisconnected.insert(serverNameFor(conn));
    conn->disconnectFromServer(args.isEmpty() ? "NUchat" : args);
    return true;
  };
  T["QUIT"] = quitHandler; T["DISCONNECT"] = quitHandler; T["BYE"] = quitHandler;

  T["AWAY"] = [this](IrcConnection *conn, const QString &, const QString &args) -> bool {
    if (args.isEmpty()) {
      conn->setBack(); m_isAway = false; emit awayStateChanged(false);
    } else {
      conn->setAway(args); m_isAway = true; m_awayLog.clear(); emit awayStateChanged(true);
    }
    return true;
  };
  T["BACK"] = [this](IrcConnection *conn, const QString &, const QString &) -> bool {
    conn->setBack(); m_isAway = false; emit awayStateChanged(false); return true;
  };

  auto rawHandler = [](IrcConnection *conn, const QString &, const QString &args) -> bool {
    conn->sendRaw(args); return true;
  };
  T["RAW"] = rawHandler; T["QUOTE"] = rawHandler;

  // ═══════════════════════════════════════════════════
  //  Info / lookup (WHOIS, WHO, etc.)
  // ═══════════════════════════════════════════════════

  auto whoisHandler = [](IrcConnection *conn, const QString &, const QString &args) -> bool {
    conn->whois(args.trimmed()); return true;
  };
  T["WHOIS"] = whoisHandler; T["W"] = whoisHandler; T["WI"] = whoisHandler;

  auto whowasHandler = [](IrcConnection *conn, const QString &, const QString &args) -> bool {
    conn->sendRaw("WHOWAS " + args.trimmed()); return true;
  };
  T["WHOWAS"] = whowasHandler; T["WW"] = whowasHandler;

  T["WHO"] = [](IrcConnection *conn, const QString &target, const QString &args) -> bool {
    conn->sendRaw("WHO " + (args.isEmpty() ? target : args)); return true;
  };
  T["NAMES"] = [](IrcConnection *conn, const QString &target, const QString &args) -> bool {
    conn->sendRaw("NAMES " + (args.isEmpty() ? target : args)); return true;
  };

  // Simple raw passthrough commands
  for (const char *c : {"LINKS", "LUSERS", "MOTD", "ADMIN", "STATS", "TRACE",
                        "USERHOST", "VERSION", "TIME", "MAP", "LIST",
                        "OPER", "SQUIT"}) {
    const QString cmd = QString::fromLatin1(c);
    T[cmd] = rawCmd(cmd);
  }

  // Special raw commands
  T["WALLOPS"] = [](IrcConnection *conn, const QString &, const QString &args) -> bool {
    conn->sendRaw("WALLOPS :" + args); return true;
  };
  T["REHASH"] = [](IrcConnection *conn, const QString &, const QString &) -> bool {
    conn->sendRaw("REHASH"); return true;
  };
  T["KILL"] = [](IrcConnection *conn, const QString &, const QString &args) -> bool {
    QString who = args.section(' ', 0, 0), reason = args.section(' ', 1);
    conn->sendRaw("KILL " + who + (reason.isEmpty() ? "" : " :" + reason)); return true;
  };
  for (const char *c : {"GLINE", "KLINE", "ZLINE", "DLINE"}) {
    const QString cmd = QString::fromLatin1(c);
    T[cmd] = rawCmd(cmd);
  }

  // ═══════════════════════════════════════════════════
  //  Ignore
  // ═══════════════════════════════════════════════════

  T["IGNORE"] = [this](IrcConnection *, const QString &, const QString &args) -> bool {
    if (args.trimmed().isEmpty()) {
      if (m_ignoreList.isEmpty()) {
        if (m_msgModel) m_msgModel->addMessage("system", "Ignore list is empty");
      } else {
        if (m_msgModel) m_msgModel->addMessage("system",
            "Ignore list (" + QString::number(m_ignoreList.size()) + " entries):");
        for (const auto &mask : m_ignoreList)
          if (m_msgModel) m_msgModel->addMessage("system", "  " + mask);
      }
    } else {
      addIgnore(args.trimmed());
      if (m_msgModel) m_msgModel->addMessage("system", "Added to ignore list: " + args.trimmed());
    }
    return true;
  };
  T["UNIGNORE"] = [this](IrcConnection *, const QString &, const QString &args) -> bool {
    if (args.trimmed().isEmpty()) {
      if (m_msgModel) m_msgModel->addMessage("system", "Usage: /UNIGNORE <nick!user@host>");
    } else {
      removeIgnore(args.trimmed());
      if (m_msgModel) m_msgModel->addMessage("system", "Removed from ignore list: " + args.trimmed());
    }
    return true;
  };

  // ═══════════════════════════════════════════════════
  //  System info
  // ═══════════════════════════════════════════════════

  T["SYSINFO"] = [this](IrcConnection *conn, const QString &target, const QString &) -> bool {
    QString info = gatherSysInfo();
    conn->sendRaw("PRIVMSG " + target + " :\x01" "ACTION " + info + "\x01");
    QString text = "* " + conn->nickname() + " " + info;
    if (m_msgModel) m_msgModel->addMessage("action", text);
    appendToChannel(m_activeServer, target, "action", text);
    return true;
  };

  // ═══════════════════════════════════════════════════
  //  Connection to new servers
  // ═══════════════════════════════════════════════════

  // /SERVER <host> [port] [password] — port prefixed with + means SSL
  auto serverHandler = [this](IrcConnection *, const QString &, const QString &args) -> bool {
    QString host = args.section(' ', 0, 0).trimmed();
    if (host.isEmpty()) {
      if (m_msgModel) m_msgModel->addMessage("system", "Usage: /SERVER <host> [port] [password]");
      return true;
    }
    QString portStr = args.section(' ', 1, 1);
    QString pass = args.section(' ', 2);
    bool ssl = false;
    int port = 6667;
    if (portStr.startsWith('+')) {
      ssl = true;
      port = portStr.mid(1).toInt();
      if (port == 0) port = 6697;
    } else if (!portStr.isEmpty()) {
      port = portStr.toInt();
      if (port == 0) port = 6667;
    }
    QString nick = m_settings ? m_settings->getString("user/nickname", "NUchat_user") : "NUchat_user";
    QString user = m_settings ? m_settings->getString("user/username", "nuchat") : "nuchat";
    QString real = m_settings ? m_settings->getString("user/realname", "NUchat User") : "NUchat User";
    connectToServer(host, port, ssl, nick, user, real, pass);
    return true;
  };
  T["SERVER"] = serverHandler; T["NEWSERVER"] = serverHandler;

  // ═══════════════════════════════════════════════════
  //  Utility commands (/TIMER, /LASTLOG, /EXEC, /ALIAS, /NOTIFY)
  // ═══════════════════════════════════════════════════

  // /TIMER <seconds> <command or text> — runs once after the delay
  T["TIMER"] = [this](IrcConnection *, const QString &target, const QString &args) -> bool {
    double secs = args.section(' ', 0, 0).toDouble();
    QString cmd = args.section(' ', 1).trimmed();
    if (secs <= 0 || cmd.isEmpty()) {
      if (m_msgModel) m_msgModel->addMessage("system", "Usage: /TIMER <seconds> <command>");
      return true;
    }
    QString srv = m_activeServer, tgt = target;
    QTimer::singleShot(static_cast<int>(secs * 1000), this, [this, srv, tgt, cmd]() {
      QString prevServer = m_activeServer, prevChannel = m_activeChannel;
      m_activeServer = srv;
      m_activeChannel = tgt;
      sendMessage(tgt, cmd);
      m_activeServer = prevServer;
      m_activeChannel = prevChannel;
    });
    if (m_msgModel)
      m_msgModel->addMessage("system", "Timer set: \"" + cmd + "\" in " + QString::number(secs) + "s");
    return true;
  };

  // /LASTLOG <text> — show matching lines from the current buffer
  T["LASTLOG"] = [this](IrcConnection *, const QString &, const QString &args) -> bool {
    QString pattern = args.trimmed();
    if (pattern.isEmpty()) {
      if (m_msgModel) m_msgModel->addMessage("system", "Usage: /LASTLOG <text>");
      return true;
    }
    if (!m_msgModel) return true;
    ChannelKey key{m_activeServer, m_activeChannel};
    const auto &hist = m_history.value(key);
    QVector<StoredMessage> matches;
    for (const auto &m : hist) {
      if (m.text.contains(pattern, Qt::CaseInsensitive))
        matches.append(m);
    }
    static constexpr int kMaxLastlog = 100;
    int start = qMax(0, matches.size() - kMaxLastlog);
    m_msgModel->addMessage("system",
        "── LastLog: \"" + pattern + "\" (" + QString::number(matches.size()) + " matches) ──");
    for (int i = start; i < matches.size(); ++i)
      m_msgModel->addMessage(matches[i].type, matches[i].text, matches[i].timestamp);
    m_msgModel->addMessage("system", "── End of LastLog ──");
    return true;
  };

  // /EXEC [-o] <command> — run a shell command; -o sends output to the channel
  T["EXEC"] = [this](IrcConnection *conn, const QString &target, const QString &args) -> bool {
    bool sendOutput = false;
    QString cmdLine = args;
    if (cmdLine.startsWith("-o ")) {
      sendOutput = true;
      cmdLine = cmdLine.mid(3);
    }
    cmdLine = cmdLine.trimmed();
    if (cmdLine.isEmpty()) {
      if (m_msgModel) m_msgModel->addMessage("system", "Usage: /EXEC [-o] <command>");
      return true;
    }
    auto *proc = new QProcess(this);
    QPointer<IrcConnection> connGuard(conn);
    QString tgt = target;
    connect(proc, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this, proc, connGuard, tgt, sendOutput](int, QProcess::ExitStatus) {
      const QString out = QString::fromUtf8(proc->readAllStandardOutput()) +
                          QString::fromUtf8(proc->readAllStandardError());
      static constexpr int kMaxExecLines = 20;
      QStringList lines = out.split('\n', Qt::SkipEmptyParts);
      if (lines.size() > kMaxExecLines) {
        int dropped = lines.size() - kMaxExecLines;
        lines = lines.mid(0, kMaxExecLines);
        lines << QStringLiteral("... (%1 more lines suppressed)").arg(dropped);
      }
      for (const QString &line : lines) {
        if (sendOutput && connGuard && connGuard->isConnected()) {
          connGuard->sendMessage(tgt, line);
          QString echo = "<" + connGuard->nickname() + "> " + line;
          appendToChannel(m_activeServer, tgt, "chat", echo);
          if (m_msgModel && m_activeChannel == tgt)
            m_msgModel->addMessage("chat", echo);
        } else if (m_msgModel) {
          m_msgModel->addMessage("system", line);
        }
      }
      proc->deleteLater();
    });
    // Kill runaway commands after 15s
    QTimer::singleShot(15000, proc, [proc]() {
      if (proc->state() != QProcess::NotRunning) proc->kill();
    });
#ifdef Q_OS_WIN
    proc->start("cmd", {"/c", cmdLine});
#else
    proc->start("/bin/sh", {"-c", cmdLine});
#endif
    return true;
  };

  // /ALIAS [<name> <expansion>] — define or list user aliases ($* = args)
  T["ALIAS"] = [this](IrcConnection *, const QString &, const QString &args) -> bool {
    if (!m_msgModel) return true;
    QString name = args.section(' ', 0, 0).trimmed().toUpper();
    QString expansion = args.section(' ', 1).trimmed();
    if (name.startsWith('/')) name = name.mid(1);
    if (name.isEmpty()) {
      // List all aliases
      if (!m_settings) return true;
      const QStringList aliasKeys = m_settings->allKeysIn("aliases");
      if (aliasKeys.isEmpty()) {
        m_msgModel->addMessage("system", "No aliases defined — use /ALIAS <name> <expansion>");
      } else {
        m_msgModel->addMessage("system", "Aliases:");
        for (const QString &k : aliasKeys)
          m_msgModel->addMessage("system",
              "  /" + k + " → " + m_settings->getString("aliases/" + k));
      }
      return true;
    }
    if (expansion.isEmpty()) {
      m_msgModel->addMessage("system", "Usage: /ALIAS <name> <expansion>   ($* = arguments)");
      return true;
    }
    if (m_settings) {
      m_settings->setString("aliases/" + name, expansion);
      m_settings->sync();
    }
    m_msgModel->addMessage("system", "Alias added: /" + name + " → " + expansion);
    return true;
  };
  T["UNALIAS"] = [this](IrcConnection *, const QString &, const QString &args) -> bool {
    QString name = args.trimmed().toUpper();
    if (name.startsWith('/')) name = name.mid(1);
    if (name.isEmpty()) {
      if (m_msgModel) m_msgModel->addMessage("system", "Usage: /UNALIAS <name>");
      return true;
    }
    if (m_settings) {
      m_settings->remove("aliases/" + name);
      m_settings->sync();
    }
    if (m_msgModel) m_msgModel->addMessage("system", "Alias removed: /" + name);
    return true;
  };

  // /NOTIFY [nick | -nick] — manage the notify (friends) list via MONITOR
  T["NOTIFY"] = [this](IrcConnection *conn, const QString &, const QString &args) -> bool {
    if (!m_settings || !m_msgModel) return true;
    QStringList list = m_settings->value("notify/list").toStringList();
    QString arg = args.trimmed();
    if (arg.isEmpty()) {
      if (list.isEmpty())
        m_msgModel->addMessage("system", "Notify list is empty. Use /NOTIFY <nick> to add.");
      else
        m_msgModel->addMessage("system", "Notify list: " + list.join(", "));
      return true;
    }
    bool remove = arg.startsWith('-');
    QString nick = remove ? arg.mid(1) : arg;
    if (remove) {
      list.removeAll(nick);
      conn->sendRaw("MONITOR - " + nick);
      m_msgModel->addMessage("system", "Removed from notify list: " + nick);
    } else if (!list.contains(nick, Qt::CaseInsensitive)) {
      list.append(nick);
      conn->sendRaw("MONITOR + " + nick);
      m_msgModel->addMessage("system", "Added to notify list: " + nick);
    } else {
      m_msgModel->addMessage("system", nick + " is already on the notify list");
    }
    m_settings->setValue("notify/list", list);
    m_settings->sync();
    return true;
  };

  // ═══════════════════════════════════════════════════
  //  DCC
  // ═══════════════════════════════════════════════════

  T["DCC"] = [this](IrcConnection *conn, const QString &, const QString &args) -> bool {
    QString sub = args.section(' ', 0, 0).toUpper();
    if (sub == "SEND") {
      QString nick = args.section(' ', 1, 1);
      QString path = args.section(' ', 2).trimmed();
      if (nick.isEmpty() || path.isEmpty()) {
        if (m_msgModel) m_msgModel->addMessage("system", "Usage: /DCC SEND <nick> <filepath>");
      } else if (m_dccManager) {
        // Wire the active connection so the CTCP offer is actually sent
        m_dccManager->setConnection(conn);
        m_dccManager->sendFile(nick, path);
        if (m_msgModel) m_msgModel->addMessage("system", "DCC: Sending " + path + " to " + nick);
      }
    } else {
      if (m_msgModel) m_msgModel->addMessage("system", "Usage: /DCC SEND <nick> <filepath>");
    }
    return true;
  };
}
