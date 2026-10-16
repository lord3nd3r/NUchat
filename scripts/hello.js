// Called for PRIVMSG and NOTICE. `message` is the message text
// (a channel line "!ping" arrives as "!ping", not the raw IRC line).
function onMessage(conn, sender, message) {
    if (message.indexOf("!ping") === 0) {
        conn.sendRaw("PRIVMSG #general :PONG from script");
    }
}
