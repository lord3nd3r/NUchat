// simple script example
function onMessage(conn, sender, message) {
    if (message.indexOf("!ping") === 0) {
        conn.sendRaw("PRIVMSG #general :PONG from script");
    }
}
