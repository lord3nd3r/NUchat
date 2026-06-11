// ── Command dispatch table ──
// Called once lazily from handleSlashCommand.
// Each entry maps a command name (uppercase) to a lambda handler.
#include "IRCConnectionManager.h"
#include "DccManager.h"
#include "IrcConnection.h"
#include "MessageModel.h"

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
