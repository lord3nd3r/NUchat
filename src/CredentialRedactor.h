#pragma once

#include <QString>

// Redacts credentials in text that is shown to the user or written to disk.
// The bytes actually sent to the IRC server are never passed through here.
namespace CredentialRedactor {

// True for NickServ and the other service nicks we auto-identify with,
// including "nick@services..." targets. Comparison is case-insensitive.
bool isServiceNick(const QString &target);

// Replace password arguments of IDENTIFY / AUTH / REGISTER / GHOST / ...
// The command word is kept so the buffer still shows what was sent.
QString redactServiceMessage(const QString &message);

// redactServiceMessage() when `target` is a service nick, otherwise `message`.
QString redactOutgoingChat(const QString &target, const QString &message);

// Redact PASS, SASL AUTHENTICATE payloads, OPER passwords, and service
// PRIVMSG/NOTICE bodies. Tags and the source prefix are preserved.
// A line that contains nothing sensitive is returned unchanged.
QString redactIrcLine(const QString &line);

}
