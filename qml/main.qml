import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs

ApplicationWindow {
    id: root
    visible: true
    width: appSettings.value("window/width", 950)
    height: appSettings.value("window/height", 650)
    x: appSettings.value("window/x", (Screen.width - width) / 2)
    y: appSettings.value("window/y", (Screen.height - height) / 2)
    title: {
        if (currentChannel === "" || currentChannel === currentServer)
            return "NUchat"
        var t = "NUchat: " + currentChannel + " on " + currentServer
        if (channelTopic) {
            // Strip HTML tags then IRC control codes (\x02 bold, \x03 color, \x0F reset, \x1D italic, \x1F underline, \x16 reverse)
            var plain = channelTopic.replace(/<[^>]*>/g, '').replace(/[\x02\x03\x0F\x1D\x1F\x16](\d{1,2}(,\d{1,2})?)?/g, '')
            if (plain.trim()) t += " \u2014 " + plain.trim()
        }
        return t
    }
    color: theme.windowBg

    // ── Theme Manager ──
    ThemeManager {
        id: theme
        Component.onCompleted: {
            var saved = appSettings.value("ui/themeIndex", 0)
            if (saved >= 0 && saved < themes.length)
                setTheme(saved)
            updateNickDarkMode()
        }
        onCurrentThemeIndexChanged: {
            appSettings.setValue("ui/themeIndex", currentThemeIndex)
            appSettings.sync()
            updateNickDarkMode()
        }
        function updateNickDarkMode() {
            // Compute perceived brightness of chatBg (0-255 scale)
            var c = chatBg
            var lum = c.r * 0.299 + c.g * 0.587 + c.b * 0.114
            msgModel.setDarkMode(lum < 0.5)
        }
    }

    // ── Save window geometry on close ──
    onClosing: function(close) {
        appSettings.setValue("window/width", root.width)
        appSettings.setValue("window/height", root.height)
        appSettings.setValue("window/x", root.x)
        appSettings.setValue("window/y", root.y)
        appSettings.sync()
        close.accepted = true
    }

    property string currentChannel: ""
    property string currentServer: ""
    property string pendingAutoJoin: ""
    property string channelTopic: ircManager.channelTopic
    property var channelUsers: ircManager.channelUsers
    property var selectedNicks: []       // multi-select nick list (bare nicks, no prefix)
    property int lastClickedNickIndex: -1  // for shift-click range select
    property string selectedNick: selectedNicks.length > 0 ? selectedNicks[0] : ""  // compat: first selected

    // ── Preference-driven UI visibility ──
    property bool prefShowServerTree:  appSettings.value("ui/showServerTree", true) === true || appSettings.value("ui/showServerTree", true) === "true"
    property bool prefShowUserList:    appSettings.value("ui/showUserList", true) === true || appSettings.value("ui/showUserList", true) === "true"
    property bool prefShowTopicBar:    appSettings.value("ui/showTopicBar", true) === true || appSettings.value("ui/showTopicBar", true) === "true"
    property bool prefShowModeButtons: appSettings.value("ui/showModeButtons", true) === true || appSettings.value("ui/showModeButtons", true) === "true"
    property bool prefShowTimestamps:  appSettings.value("ui/showTimestamps", true) === true || appSettings.value("ui/showTimestamps", true) === "true"
    property bool prefShowNickColors:  appSettings.value("ui/showNickColors", true) === true || appSettings.value("ui/showNickColors", true) === "true"
    property bool prefStripColors:     appSettings.value("ui/stripMircColors", false) === true || appSettings.value("ui/stripMircColors", false) === "true"
    property string prefFontFamily: {
        var idx = parseInt(appSettings.value("ui/fontFamilyIndex", 0)) || 0
        var fonts = ["Monospace", "DejaVu Sans Mono", "Liberation Mono", "Courier New", "Consolas", "Fira Code"]
        return fonts[idx] || "Monospace"
    }
    property int prefFontSize: parseInt(appSettings.value("ui/fontSize", 12)) || 12

    // Helper: run a command for each selected nick
    function forEachSelectedNick(callback) {
        for (var i = 0; i < selectedNicks.length; i++) callback(selectedNicks[i])
    }
    // Helper: strip prefix from nick entry
    function bareNick(entry) { return entry.replace(/^[@%+~&]/, '') }

    // Helper: simulate sidebar click at index
    function sidebarClickAt(idx) {
        if (idx < 0 || idx >= channelListModel.count) return
        var entry = channelListModel.get(idx)
        if (entry.entryType === "channel") {
            var srv = ""
            for (var k = idx - 1; k >= 0; k--) {
                if (channelListModel.get(k).entryType === "server") { srv = channelListModel.get(k).name; break }
            }
            currentServer = srv; currentChannel = entry.name
            ircManager.switchToChannel(srv, entry.name)
            channelTopic = ircManager.channelTopic; channelUsers = ircManager.channelUsers
        } else {
            currentServer = entry.name; currentChannel = ""
            ircManager.switchToChannel(entry.name, entry.name)
        }
    }

    // Reconnect timer (disconnects then re-connects after 1s)
    Timer {
        id: reconnectTimer; interval: 1000; repeat: false
        onTriggered: quickConnectDialog.open()
    }

    // ── Flatten tree model into a list for the sidebar ──
    ListModel { id: channelListModel }

    function refreshChannelList() {
        channelListModel.clear()
        var rc = treeModel.rowCount()
        for (var i = 0; i < rc; i++) {
            var sIdx = treeModel.index(i, 0)
            var sName = treeModel.data(sIdx)
            channelListModel.append({name: sName, entryType: "server", hasUnread: false, hasHighlight: false})
            var cc = treeModel.rowCount(sIdx)
            for (var j = 0; j < cc; j++) {
                var cIdx = treeModel.index(j, 0, sIdx)
                var chName = treeModel.data(cIdx)
                channelListModel.append({
                    name: chName, entryType: "channel",
                    hasUnread: ircManager.hasUnread(sName, chName),
                    hasHighlight: ircManager.hasHighlight(sName, chName)
                })
            }
        }
    }

    function updateUnreadStates() {
        var currentSrv = ""
        for (var i = 0; i < channelListModel.count; i++) {
            var entry = channelListModel.get(i)
            if (entry.entryType === "server") {
                currentSrv = entry.name
            } else {
                channelListModel.setProperty(i, "hasUnread", ircManager.hasUnread(currentSrv, entry.name))
                channelListModel.setProperty(i, "hasHighlight", ircManager.hasHighlight(currentSrv, entry.name))
            }
        }
    }

    Component.onCompleted: {
        refreshChannelList()
        // auto-select first channel if available
        for (var i = 0; i < channelListModel.count; i++) {
            if (channelListModel.get(i).entryType === "channel") {
                serverTree.currentIndex = i
                currentChannel = channelListModel.get(i).name
                break
            }
        }
    }

    // ── Menu Bar ──
    menuBar: MenuBar {
        id: mainMenuBar
        background: Rectangle { color: theme.menuBarBg }

        delegate: MenuBarItem {
            id: menuBarDelegate
            contentItem: Text {
                text: menuBarDelegate.text
                color: menuBarDelegate.highlighted ? theme.highlightText : theme.textSecondary
                font.pixelSize: 12
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: menuBarDelegate.highlighted ? theme.highlight : "transparent"
            }
        }

        // ═══ NUchat menu (like HexChat app menu) ═══
        Menu {
            title: "NUchat"
            Action { text: "Network List...";         onTriggered: networkListDialog.open() }
            Action { text: "Quick Connect...";        onTriggered: quickConnectDialog.open() }
            MenuSeparator {}
            Action { text: "New Server Tab";          onTriggered: quickConnectDialog.open() }
            Action { text: "New Channel Tab";         onTriggered: joinChannelDialog.open() }
            Action { text: "New Private Tab";         onTriggered: {
                    privateNickDialog.open()
                }
            }
            MenuSeparator {}
            Action { text: "Disconnect";              onTriggered: { if (currentServer) ircManager.disconnectFromServer(currentServer) } }
            Action { text: "Reconnect";               onTriggered: {
                    if (currentServer) {
                        ircManager.disconnectFromServer(currentServer)
                        // Re-connect after a short delay
                        reconnectTimer.start()
                    }
                }
            }
            MenuSeparator {}
            Action { text: "Close Tab";               onTriggered: {
                    if (currentChannel !== "" && currentChannel.startsWith("#")) {
                        ircManager.partChannel(currentChannel, "")
                    }
                }
            }
            Action { text: "Quit";                    onTriggered: Qt.quit() }
        }

        // ═══ View ═══
        Menu {
            title: "View"
            Action { text: "Channel List...";         onTriggered: channelListDialog.open() }
            Action { text: "DCC Transfers...";        onTriggered: dccDialog.open() }
            Action { text: "URL Grabber...";          onTriggered: urlGrabberDialog.open() }
            Action { text: "Raw Log...";              onTriggered: rawLogDialog.open() }
            MenuSeparator {}
            Action { text: "Plugins and Scripts...";  onTriggered: scriptsDialog.open() }
            Action { text: "Away Log";               onTriggered: msgModel.addMessage("system", "Away log not yet implemented") }
            MenuSeparator {}
            Action { text: "Search Text...";          onTriggered: searchDialog.open() }
            Action { text: "Fullscreen";              onTriggered: { root.visibility = (root.visibility === Window.FullScreen) ? Window.Windowed : Window.FullScreen } }
        }

        // ═══ Server ═══
        Menu {
            title: "Server"
            Action { text: "Connect";                 onTriggered: quickConnectDialog.open() }
            Action { text: "Disconnect";              onTriggered: { if (currentServer) ircManager.disconnectFromServer(currentServer) } }
            Action { text: "Reconnect";               onTriggered: {
                    if (currentServer) {
                        ircManager.disconnectFromServer(currentServer)
                        reconnectTimer.start()
                    }
                }
            }
            MenuSeparator {}
            Action { text: "Join Channel...";         onTriggered: joinChannelDialog.open() }
            Action { text: "Part Channel";            onTriggered: { if (currentChannel !== "") ircManager.partChannel(currentChannel, "") } }
            MenuSeparator {}
            Action { text: "Change Nickname...";      onTriggered: nickChangeDialog.open() }
            Action { text: "Set Away";                onTriggered: ircManager.sendRawCommand("AWAY :Away") }
            Action { text: "Set Back";                onTriggered: ircManager.sendRawCommand("AWAY") }
            MenuSeparator {}
            Action { text: "SASL Authentication...";  onTriggered: networkListDialog.open() }
            Action { text: "Server Password...";      onTriggered: serverPassDialog.open() }
            Action { text: "Send Raw Command...";     onTriggered: rawLogDialog.open() }
        }

        // ═══ Channel ═══
        Menu {
            title: "Channel"
            Action { text: "Set Topic...";            onTriggered: topicDialog.open() }
            Action { text: "Channel Modes...";        onTriggered: channelModeDialog.open() }
            Action { text: "Ban List...";             onTriggered: banListDialog.open() }
            MenuSeparator {}
            Action { text: "Clear Buffer";            onTriggered: msgModel.clear() }
            Action { text: "Save Buffer...";          onTriggered: msgModel.addMessage("system", "Buffer save not yet implemented") }
            MenuSeparator {}
            Action { text: "Invite User...";          onTriggered: inviteDialog.open() }
            Action { text: "Op User";                 onTriggered: { if (currentChannel !== "") forEachSelectedNick(function(n) { ircManager.sendRawCommand("MODE " + currentChannel + " +o " + n) }) } }
            Action { text: "DeOp User";               onTriggered: { if (currentChannel !== "") forEachSelectedNick(function(n) { ircManager.sendRawCommand("MODE " + currentChannel + " -o " + n) }) } }
            Action { text: "Voice User";              onTriggered: { if (currentChannel !== "") forEachSelectedNick(function(n) { ircManager.sendRawCommand("MODE " + currentChannel + " +v " + n) }) } }
            Action { text: "DeVoice User";            onTriggered: { if (currentChannel !== "") forEachSelectedNick(function(n) { ircManager.sendRawCommand("MODE " + currentChannel + " -v " + n) }) } }
            Action { text: "Kick User...";            onTriggered: { if (currentChannel !== "") forEachSelectedNick(function(n) { ircManager.sendRawCommand("KICK " + currentChannel + " " + n + " :Kicked") }) } }
            Action { text: "Ban User...";             onTriggered: { if (currentChannel !== "") forEachSelectedNick(function(n) { ircManager.sendRawCommand("MODE " + currentChannel + " +b " + n + "!*@*") }) } }
            Action { text: "Kick+Ban...";             onTriggered: {
                    if (currentChannel !== "") forEachSelectedNick(function(n) {
                        ircManager.sendRawCommand("MODE " + currentChannel + " +b " + n + "!*@*")
                        ircManager.sendRawCommand("KICK " + currentChannel + " " + n + " :Banned")
                    })
                }
            }
            MenuSeparator {}
            Action { text: "CTCP Ping";               onTriggered: { if (selectedNick !== "") ircManager.sendRawCommand("PRIVMSG " + selectedNick + " :\x01PING " + Date.now() + "\x01") } }
            Action { text: "CTCP Version";            onTriggered: { if (selectedNick !== "") ircManager.sendRawCommand("PRIVMSG " + selectedNick + " :\x01VERSION\x01") } }
            Action { text: "CTCP Time";               onTriggered: { if (selectedNick !== "") ircManager.sendRawCommand("PRIVMSG " + selectedNick + " :\x01TIME\x01") } }
            Action { text: "CTCP Finger";             onTriggered: { if (selectedNick !== "") ircManager.sendRawCommand("PRIVMSG " + selectedNick + " :\x01FINGER\x01") } }
        }

        // ═══ User ═══
        Menu {
            title: "User"
            Action { text: "WHOIS";                   onTriggered: { if (selectedNick !== "") ircManager.sendRawCommand("WHOIS " + selectedNick) } }
            Action { text: "DNS Lookup";              onTriggered: { if (selectedNick !== "") ircManager.sendRawCommand("USERHOST " + selectedNick) } }
            Action { text: "Ping";                    onTriggered: { if (selectedNick !== "") ircManager.sendRawCommand("PRIVMSG " + selectedNick + " :\x01PING " + Date.now() + "\x01") } }
            MenuSeparator {}
            Action { text: "Send Private Message";    onTriggered: {
                    if (selectedNick !== "") {
                        ircManager.openQuery(currentServer, selectedNick)
                        messageInput.forceActiveFocus()
                    }
                }
            }
            Action { text: "Direct Chat (DCC)";      onTriggered: dccDialog.open() }
            Action { text: "Send File (DCC)...";      onTriggered: dccDialog.open() }
            MenuSeparator {}
            Action { text: "Ignore User";             onTriggered: { if (selectedNick !== "") msgModel.addMessage("system", "Ignore list not yet implemented") } }
            Action { text: "Unignore User";           onTriggered: { if (selectedNick !== "") msgModel.addMessage("system", "Ignore list not yet implemented") } }
            Action { text: "Ignore List...";          onTriggered: msgModel.addMessage("system", "Ignore list not yet implemented") }
            MenuSeparator {}
            Action { text: "Notify List...";          onTriggered: msgModel.addMessage("system", "Notify list not yet implemented") }
        }

        // ═══ Settings ═══
        Menu {
            title: "Settings"
            Action { text: "Preferences...";          onTriggered: preferencesDialog.open() }
            MenuSeparator {}

            Menu {
                id: themeMenu
                title: "Themes"
                Instantiator {
                    model: theme.themeNames
                    delegate: MenuItem {
                        text: (index === theme.currentThemeIndex ? "\u2713 " : "   ") + modelData
                        onTriggered: theme.setTheme(index)
                    }
                    onObjectAdded: function(index, object) { themeMenu.insertItem(index, object) }
                    onObjectRemoved: function(index, object) { themeMenu.removeItem(object) }
                }
            }

            MenuSeparator {}
            Action { text: "Next Theme";              onTriggered: theme.nextTheme() }
            Action { text: "Previous Theme";          onTriggered: theme.prevTheme() }
            MenuSeparator {}
            Action { text: "Auto-Replace...";         onTriggered: preferencesDialog.open() }
            Action { text: "URL Handlers...";         onTriggered: preferencesDialog.open() }
            Action { text: "Text Events...";          onTriggered: preferencesDialog.open() }
            Action { text: "Keyboard Shortcuts...";   onTriggered: preferencesDialog.open() }
            MenuSeparator {}
            Action { text: "Identd Server";            onTriggered: msgModel.addMessage("system", "Identd server not yet implemented") }
            Action { text: "Perform List...";         onTriggered: preferencesDialog.open() }
        }

        // ═══ Window ═══
        Menu {
            title: "Window"
            Action { text: "Previous Tab";            onTriggered: { if (serverTree.currentIndex > 0) { serverTree.currentIndex--; sidebarClickAt(serverTree.currentIndex) } } }
            Action { text: "Next Tab";                onTriggered: { if (serverTree.currentIndex < channelListModel.count - 1) { serverTree.currentIndex++; sidebarClickAt(serverTree.currentIndex) } } }
            MenuSeparator {}
            Action { text: "Detach Tab";              onTriggered: msgModel.addMessage("system", "Tab detach not yet implemented") }
            Action { text: "Attach Tab";              onTriggered: msgModel.addMessage("system", "Tab attach not yet implemented") }
            MenuSeparator {}
            Action { text: "Close Tab";               onTriggered: {
                    if (currentChannel !== "" && currentChannel.startsWith("#")) {
                        ircManager.partChannel(currentChannel, "")
                    }
                }
            }
            Action { text: "Close All Tabs";          onTriggered: ircManager.disconnectAll() }
        }


        // ═══ Scripts ═══
        Menu {
            title: "Scripts"

            Action { text: "Loaded Scripts..."; onTriggered: scriptsDialog.open() }
            MenuSeparator {}
            Action { text: "Load Script..."; onTriggered: scriptFileDialog.open() }
            Action { text: "Reload All Scripts"; onTriggered: { if (typeof pyEngine !== 'undefined') pyEngine.reloadAll() } }
            MenuSeparator {}
            Action { text: "Open Scripts Folder"; onTriggered: { if (typeof pyEngine !== 'undefined') pyEngine.openScriptsFolder() } }
        }

        // ═══ Services ═══
        Menu {
            title: "Services"

            // ── NickServ ──
            Menu {
                title: "NickServ"
                Action { text: "Identify...";      onTriggered: { messageInput.text = "/NS IDENTIFY "; messageInput.forceActiveFocus() } }
                Action { text: "Register...";      onTriggered: { messageInput.text = "/NS REGISTER "; messageInput.forceActiveFocus() } }
                Action { text: "Ghost...";         onTriggered: { messageInput.text = "/NS GHOST "; messageInput.forceActiveFocus() } }
                Action { text: "Release...";       onTriggered: { messageInput.text = "/NS RELEASE "; messageInput.forceActiveFocus() } }
                Action { text: "Set Password...";  onTriggered: { messageInput.text = "/NS SET PASSWORD "; messageInput.forceActiveFocus() } }
                Action { text: "Info...";          onTriggered: { messageInput.text = "/NS INFO "; messageInput.forceActiveFocus() } }
                Action { text: "Help";             onTriggered: ircManager.sendRawCommand("PRIVMSG NickServ :HELP") }
            }

            // ── ChanServ (channel-level only) ──
            Menu {
                title: "ChanServ"
                Action { text: "Info";        onTriggered: { if (currentChannel) ircManager.sendRawCommand("PRIVMSG ChanServ :INFO " + currentChannel) } }
                Action { text: "Topic...";    onTriggered: { messageInput.text = "/CS TOPIC " + currentChannel + " "; messageInput.forceActiveFocus() } }
                Action { text: "Mode...";     onTriggered: { messageInput.text = "/CS MODE " + currentChannel + " "; messageInput.forceActiveFocus() } }
                MenuSeparator {}
                Menu {
                    title: "Access List"
                    Action { text: "Add..."; onTriggered: { messageInput.text = "/CS ACCESS " + currentChannel + " ADD "; messageInput.forceActiveFocus() } }
                    Action { text: "Del..."; onTriggered: { messageInput.text = "/CS ACCESS " + currentChannel + " DEL "; messageInput.forceActiveFocus() } }
                    Action { text: "List";   onTriggered: { if (currentChannel) ircManager.sendRawCommand("PRIVMSG ChanServ :ACCESS " + currentChannel + " LIST") } }
                }
                Menu {
                    title: "Transfer / Suspend"
                    Action { text: "Transfer..."; onTriggered: { messageInput.text = "/CS TRANSFER " + currentChannel + " "; messageInput.forceActiveFocus() } }
                    Action { text: "Suspend";     onTriggered: { if (currentChannel) ircManager.sendRawCommand("PRIVMSG ChanServ :SUSPEND " + currentChannel) } }
                }
                Action { text: "Help"; onTriggered: ircManager.sendRawCommand("PRIVMSG ChanServ :HELP") }
            }

            // ── OperServ (channel/server admin only) ──
            Menu {
                title: "OperServ"
                Menu {
                    title: "Channel Status"
                    Action { text: "List Bans";  onTriggered: { if (currentChannel) ircManager.sendRawCommand("PRIVMSG OperServ :LISTBANS " + currentChannel) } }
                    Action { text: "List Modes"; onTriggered: { if (currentChannel) ircManager.sendRawCommand("PRIVMSG OperServ :LISTMODES " + currentChannel) } }
                    Action { text: "List Users"; onTriggered: { if (currentChannel) ircManager.sendRawCommand("PRIVMSG OperServ :LIST " + currentChannel) } }
                }
                Menu {
                    title: "Extended"
                    Action { text: "AKILL...";  onTriggered: { messageInput.text = "/OS AKILL "; messageInput.forceActiveFocus() } }
                    Action { text: "RAKILL..."; onTriggered: { messageInput.text = "/OS RAKILL "; messageInput.forceActiveFocus() } }
                    Action { text: "SET...";    onTriggered: { messageInput.text = "/OS SET "; messageInput.forceActiveFocus() } }
                    Action { text: "GET...";    onTriggered: { messageInput.text = "/OS GET "; messageInput.forceActiveFocus() } }
                }
                Action { text: "Help"; onTriggered: ircManager.sendRawCommand("PRIVMSG OperServ :HELP") }
            }

            // ── HostServ ──
            Menu {
                title: "HostServ"
                Action { text: "Request vHost..."; onTriggered: { messageInput.text = "/HS REQUEST "; messageInput.forceActiveFocus() } }
                Action { text: "On";               onTriggered: ircManager.sendRawCommand("PRIVMSG HostServ :ON") }
                Action { text: "Off";              onTriggered: ircManager.sendRawCommand("PRIVMSG HostServ :OFF") }
                Action { text: "Info...";          onTriggered: { messageInput.text = "/HS INFO "; messageInput.forceActiveFocus() } }
                Action { text: "Help";             onTriggered: ircManager.sendRawCommand("PRIVMSG HostServ :HELP") }
            }

            // ── MemoServ ──
            Menu {
                title: "MemoServ"
                Action { text: "Send..."; onTriggered: { messageInput.text = "/MS SEND "; messageInput.forceActiveFocus() } }
                Action { text: "Read..."; onTriggered: { messageInput.text = "/MS READ "; messageInput.forceActiveFocus() } }
                Action { text: "List";    onTriggered: ircManager.sendRawCommand("PRIVMSG MemoServ :LIST") }
                Action { text: "Del...";  onTriggered: { messageInput.text = "/MS DEL "; messageInput.forceActiveFocus() } }
                Action { text: "Help";    onTriggered: ircManager.sendRawCommand("PRIVMSG MemoServ :HELP") }
            }

            // ── BotServ ──
            Menu {
                title: "BotServ"
                Action { text: "Add Bot..."; onTriggered: { messageInput.text = "/BS ADD "; messageInput.forceActiveFocus() } }
                Action { text: "Del Bot..."; onTriggered: { messageInput.text = "/BS DEL "; messageInput.forceActiveFocus() } }
                Action { text: "List Bots";  onTriggered: ircManager.sendRawCommand("PRIVMSG BotServ :LIST") }
                Action { text: "Help";       onTriggered: ircManager.sendRawCommand("PRIVMSG BotServ :HELP") }
            }

            MenuSeparator {}

            // ── Channel Settings (mode shortcuts) ──
            Menu {
                title: "Channel Settings"
                Action { text: "Invite Only (+i)";   onTriggered: { if (currentChannel) ircManager.sendRawCommand("MODE " + currentChannel + " +i") } }
                Action { text: "Moderated (+m)";     onTriggered: { if (currentChannel) ircManager.sendRawCommand("MODE " + currentChannel + " +m") } }
                Action { text: "Secret (+s)";        onTriggered: { if (currentChannel) ircManager.sendRawCommand("MODE " + currentChannel + " +s") } }
                Action { text: "Topic Lock (+t)";    onTriggered: { if (currentChannel) ircManager.sendRawCommand("MODE " + currentChannel + " +t") } }
                Action { text: "No External (+n)";   onTriggered: { if (currentChannel) ircManager.sendRawCommand("MODE " + currentChannel + " +n") } }
                Action { text: "Registered (+r)";    onTriggered: { if (currentChannel) ircManager.sendRawCommand("MODE " + currentChannel + " +r") } }
                Action { text: "Set Key (+k)...";    onTriggered: { messageInput.text = "/MODE " + currentChannel + " +k "; messageInput.forceActiveFocus() } }
                Action { text: "Set Limit (+l)...";  onTriggered: { messageInput.text = "/MODE " + currentChannel + " +l "; messageInput.forceActiveFocus() } }
                MenuSeparator {}
                Action { text: "Remove Invite Only"; onTriggered: { if (currentChannel) ircManager.sendRawCommand("MODE " + currentChannel + " -i") } }
                Action { text: "Remove Moderated";   onTriggered: { if (currentChannel) ircManager.sendRawCommand("MODE " + currentChannel + " -m") } }
                Action { text: "Remove Secret";      onTriggered: { if (currentChannel) ircManager.sendRawCommand("MODE " + currentChannel + " -s") } }
                Action { text: "Remove Key";         onTriggered: { if (currentChannel) ircManager.sendRawCommand("MODE " + currentChannel + " -k *") } }
                Action { text: "Remove Limit";       onTriggered: { if (currentChannel) ircManager.sendRawCommand("MODE " + currentChannel + " -l") } }
            }

            MenuSeparator {}

            // ── Network Info ──
            Menu {
                title: "Network Info"
                Action { text: "Links";    onTriggered: ircManager.sendRawCommand("LINKS") }
                Action { text: "Lusers";   onTriggered: ircManager.sendRawCommand("LUSERS") }
                Action { text: "MOTD";     onTriggered: ircManager.sendRawCommand("MOTD") }
                Action { text: "Admin";    onTriggered: ircManager.sendRawCommand("ADMIN") }
                Action { text: "Stats..."; onTriggered: { messageInput.text = "/STATS "; messageInput.forceActiveFocus() } }
                Action { text: "Trace..."; onTriggered: { messageInput.text = "/TRACE "; messageInput.forceActiveFocus() } }
                Action { text: "Version";  onTriggered: ircManager.sendRawCommand("VERSION") }
                Action { text: "Time";     onTriggered: ircManager.sendRawCommand("TIME") }
            }

            // ── Channel Stats ──
            Menu {
                title: "Channel Stats"
                Action { text: "Modes"; onTriggered: { if (currentChannel) ircManager.sendRawCommand("MODE " + currentChannel) } }
                Action { text: "Topic"; onTriggered: { if (currentChannel) ircManager.sendRawCommand("TOPIC " + currentChannel) } }
                Action { text: "Who";   onTriggered: { if (currentChannel) ircManager.sendRawCommand("WHO " + currentChannel) } }
                Action { text: "Names"; onTriggered: { if (currentChannel) ircManager.sendRawCommand("NAMES " + currentChannel) } }
            }

            // ── Oper Commands ──
            Menu {
                title: "Oper Commands"
                Action { text: "Oper...";    onTriggered: { messageInput.text = "/OPER "; messageInput.forceActiveFocus() } }
                Action { text: "Rehash";     onTriggered: ircManager.sendRawCommand("REHASH") }
                Action { text: "Restart";    onTriggered: ircManager.sendRawCommand("RESTART") }
                Action { text: "SQuit...";   onTriggered: { messageInput.text = "/SQUIT "; messageInput.forceActiveFocus() } }
                Action { text: "GLINE...";   onTriggered: { messageInput.text = "/GLINE "; messageInput.forceActiveFocus() } }
                Action { text: "Kill...";    onTriggered: { messageInput.text = "/KILL "; messageInput.forceActiveFocus() } }
                Action { text: "Wallops..."; onTriggered: { messageInput.text = "/WALLOPS "; messageInput.forceActiveFocus() } }
            }
        }

        // ═══ Help ═══
        Menu {
            title: "Help"
            Action { text: "Documentation";           onTriggered: Qt.openUrlExternally("https://github.com/lord3nd3r/NUchat") }
            Action { text: "Keyboard Shortcuts";     onTriggered: msgModel.addMessage("system", "Ctrl+Enter: Send | Ctrl+W: Close Tab | Ctrl+PgUp/PgDn: Switch Tabs") }
            Action { text: "Report a Bug";            onTriggered: Qt.openUrlExternally("https://github.com/lord3nd3r/NUchat/issues") }
            MenuSeparator {}
            Action { text: "Check for Updates";       onTriggered: Qt.openUrlExternally("https://github.com/lord3nd3r/NUchat/releases") }
            Action { text: "About NUchat...";         onTriggered: aboutDialog.open() }
        }
    }

    // ── Main Layout ──
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ═══ LEFT SIDEBAR ═══
        Rectangle {
            id: sidebarPanel
            Layout.preferredWidth: 180
            Layout.fillHeight: true
            color: theme.sidebarBg
            visible: root.prefShowServerTree

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Sidebar header
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28
                    color: theme.sidebarHeaderBg
                    Text {
                        anchors.centerIn: parent
                        text: "SERVERS"
                        color: theme.textMuted
                        font.pixelSize: 10
                        font.bold: true
                        font.letterSpacing: 1.5
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: theme.separator
                }

                ListView {
                    id: serverTree
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: channelListModel
                    clip: true
                    currentIndex: -1
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: Rectangle {
                        required property int index
                        required property string name
                        required property string entryType
                        required property bool hasUnread
                        required property bool hasHighlight
                        width: serverTree.width
                        height: entryType === "server" ? 30 : 26
                        color: serverTree.currentIndex === index ? theme.selectedBg
                             : delegateMouse.containsMouse ? theme.hoverBg : "transparent"

                        Row {
                            anchors.left: parent.left
                            anchors.leftMargin: entryType === "server" ? 8 : 22
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 6

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: entryType === "server" ? "\u25BE" : "\u2022"
                                color: entryType === "server" ? theme.textMuted : theme.nickVoice
                                font.pixelSize: entryType === "server" ? 10 : 10
                            }

                            Text {
                                id: channelLabel
                                anchors.verticalCenter: parent.verticalCenter
                                text: name
                                color: {
                                    if (serverTree.currentIndex === index) return theme.selectedText
                                    if (hasHighlight) return "#ff4444"   // red for nick mention / PM
                                    if (hasUnread) return "#ffffff"      // bright white for unread
                                    return theme.textSecondary
                                }
                                font.pixelSize: 12
                                font.bold: entryType === "server" || hasUnread || hasHighlight

                                // Pulse animation for nick highlights
                                SequentialAnimation on opacity {
                                    id: pulseAnim
                                    running: hasHighlight && serverTree.currentIndex !== index
                                    loops: Animation.Infinite
                                    NumberAnimation { from: 1.0; to: 0.4; duration: 800; easing.type: Easing.InOutSine }
                                    NumberAnimation { from: 0.4; to: 1.0; duration: 800; easing.type: Easing.InOutSine }
                                    onRunningChanged: if (!running) channelLabel.opacity = 1.0
                                }
                            }
                        }

                        MouseArea {
                            id: delegateMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                serverTree.currentIndex = index
                                if (entryType === "channel") {
                                    // Find the server this channel belongs to
                                    var srv = ""
                                    for (var k = index - 1; k >= 0; k--) {
                                        if (channelListModel.get(k).entryType === "server") {
                                            srv = channelListModel.get(k).name
                                            break
                                        }
                                    }
                                    root.currentServer = srv
                                    root.currentChannel = name
                                    ircManager.switchToChannel(srv, name)
                                    root.channelTopic = ircManager.channelTopic
                                    root.channelUsers = ircManager.channelUsers
                                } else {
                                    root.currentServer = name
                                    root.currentChannel = ""
                                    ircManager.switchToChannel(name, name)
                                }
                            }
                        }
                    }
                }
            }
        }

        // Sidebar / chat separator
        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            color: theme.separator
            visible: sidebarPanel.visible
        }

        // ═══ CENTER: Chat Area ═══
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ── Topic bar ──
            Rectangle {
                id: topicBarRect
                Layout.fillWidth: true
                Layout.minimumHeight: 30
                Layout.maximumHeight: 60
                Layout.preferredHeight: topicText.implicitHeight + 12
                color: theme.topicBg
                visible: root.prefShowTopicBar

                Text {
                    id: topicText
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: {
                        if (currentChannel === "") return "No channel selected"
                        var t = root.channelTopic
                        if (t && t !== "") return currentChannel + " — " + t
                        return currentChannel
                    }
                    textFormat: Text.RichText
                    color: theme.textMuted
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                    maximumLineCount: 3
                    elide: Text.ElideRight
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: theme.separator
                visible: topicBarRect.visible
            }

            // ── Message area ──
            ScrollView {
                id: chatScrollView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                TextArea {
                    id: chatArea
                    readOnly: true
                    selectByMouse: true
                    selectedTextColor: theme.highlightText
                    selectionColor: theme.highlight
                    color: theme.textPrimary
                    font.family: root.prefFontFamily
                    font.pixelSize: root.prefFontSize
                    wrapMode: TextEdit.Wrap
                    textFormat: TextEdit.RichText
                    background: Rectangle { color: theme.chatBg }
                    padding: 8

                    onLinkActivated: function(link) {
                        if (link.startsWith("nick://")) {
                            var nick = decodeURIComponent(link.substring(7))
                            nickContextMenu.targetNick = nick
                            nickContextMenu.popup()
                        } else {
                            Qt.openUrlExternally(link)
                        }
                    }

                    // Change cursor to hand when hovering a link
                    HoverHandler {
                        cursorShape: chatArea.hoveredLink !== "" ? Qt.PointingHandCursor : Qt.IBeamCursor
                    }

                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: function(eventPoint) {
                            var px = eventPoint.position.x
                            var py = eventPoint.position.y

                            // Check for link under cursor first
                            var link = chatArea.linkAt(px, py)
                            if (link && link !== "") {
                                if (link.startsWith("nick://")) {
                                    nickContextMenu.targetNick = decodeURIComponent(link.substring(7))
                                    nickContextMenu.popup()
                                } else {
                                    chatLinkMenu.targetUrl = link
                                    chatLinkMenu.popup()
                                }
                                return
                            }

                            // Then check for nick
                            var pos = chatArea.positionAt(px, py)
                            var fullText = chatArea.text
                            var lineStart = fullText.lastIndexOf('\n', pos - 1) + 1
                            var lineEnd = fullText.indexOf('\n', pos)
                            if (lineEnd < 0) lineEnd = fullText.length
                            var line = fullText.substring(lineStart, lineEnd)
                            var m = line.match(/<([~&@%+]?)(\S+?)>/)
                            if (m && m[2]) {
                                nickContextMenu.targetNick = m[2]
                                nickContextMenu.popup()
                            }
                        }
                    }

                    Connections {
                        target: msgModel
                        function onMessageAdded(formattedLine) {
                            chatArea.append(formattedLine)
                            chatArea.cursorPosition = chatArea.length
                        }
                        function onCleared() {
                            chatArea.text = ""
                        }
                    }

                    Component.onCompleted: {
                        text = msgModel.allFormattedText()
                        cursorPosition = length
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: theme.separator
            }

            // ── Input bar ──
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                color: theme.nickListBg

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 4

                    TextField {
                        id: messageInput
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        placeholderText: currentChannel !== ""
                            ? "Message " + currentChannel + "..."
                            : "Type /join #channel..."
                        placeholderTextColor: theme.placeholder
                        color: theme.textInput
                        font.family: root.prefFontFamily
                        font.pixelSize: root.prefFontSize
                        enabled: currentChannel !== "" || currentServer !== ""
                        background: Rectangle {
                            color: theme.inputBg
                            border.color: messageInput.activeFocus ? theme.inputBorderFocus : theme.inputBorder
                            border.width: 1
                            radius: 3
                        }
                        onAccepted: sendMessage()

                        // Tab completion for nicknames
                        property string tabPrefix: ""
                        property int tabIndex: -1
                        property var tabMatches: []

                        Keys.onTabPressed: function(event) {
                            event.accepted = true
                            if (!channelUsers || channelUsers.length === 0) return
                            var txt = messageInput.text
                            var curPos = messageInput.cursorPosition

                            // Find the word being typed
                            var before = txt.substring(0, curPos)
                            var wordStart = before.lastIndexOf(' ') + 1
                            var partial = before.substring(wordStart)
                            if (partial === "") return

                            // If this is a new tab cycle, build match list
                            if (messageInput.tabPrefix !== partial || messageInput.tabMatches.length === 0) {
                                messageInput.tabPrefix = partial
                                messageInput.tabIndex = 0
                                var matches = []
                                var lp = partial.toLowerCase()
                                for (var i = 0; i < channelUsers.length; i++) {
                                    var nick = channelUsers[i].replace(/^[@%+~&]/, '')
                                    if (nick.toLowerCase().indexOf(lp) === 0)
                                        matches.push(nick)
                                }
                                messageInput.tabMatches = matches
                            } else {
                                messageInput.tabIndex = (messageInput.tabIndex + 1) % messageInput.tabMatches.length
                            }

                            if (messageInput.tabMatches.length === 0) return
                            var completion = messageInput.tabMatches[messageInput.tabIndex]
                            var suffix = (wordStart === 0) ? ": " : " "
                            var after = txt.substring(curPos)
                            messageInput.text = txt.substring(0, wordStart) + completion + suffix + after
                            messageInput.cursorPosition = wordStart + completion.length + suffix.length
                            // Update tabPrefix to the completed nick so next tab cycles
                            messageInput.tabPrefix = completion
                        }

                        Keys.onPressed: function(event) {
                            if (event.key !== Qt.Key_Tab) {
                                messageInput.tabPrefix = ""
                                messageInput.tabMatches = []
                                messageInput.tabIndex = -1
                            }
                        }
                    }

                    Button {
                        id: sendButton
                        text: "Send"
                        Layout.preferredWidth: 60
                        Layout.fillHeight: true
                        enabled: messageInput.text !== "" && (currentChannel !== "" || messageInput.text.startsWith("/"))
                        onClicked: sendMessage()

                        background: Rectangle {
                            color: sendButton.enabled
                                ? (sendButton.down ? theme.buttonPressed : theme.buttonBg)
                                : theme.buttonDisabled
                            radius: 3
                        }
                        contentItem: Text {
                            text: sendButton.text
                            color: sendButton.enabled ? theme.buttonText : theme.buttonTextDisabled
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }
        }

        // Chat / nicklist separator
        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            color: theme.separator
            visible: root.prefShowUserList && currentChannel !== "" && currentChannel.startsWith("#")
        }

        // ═══ RIGHT: Nick List ═══
        Rectangle {
            id: nickListPanel
            Layout.preferredWidth: 160
            Layout.fillHeight: true
            color: theme.nickListBg
            visible: root.prefShowUserList && currentChannel !== "" && currentChannel.startsWith("#")

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Header
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28
                    color: theme.nickListHeaderBg
                    Text {
                        anchors.centerIn: parent
                        text: channelUsers ? channelUsers.length + " USERS" : "USERS"
                        color: theme.textMuted
                        font.pixelSize: 10
                        font.bold: true
                        font.letterSpacing: 1.5
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: theme.separator
                }

                // Nick list
                ListView {
                    id: nickList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    model: channelUsers || []

                    delegate: Rectangle {
                        required property int index
                        required property string modelData
                        width: nickList.width
                        height: 22
                        property string bareN: modelData.replace(/^[@%+~&]/, '')
                        property bool isSelected: root.selectedNicks.indexOf(bareN) !== -1
                        color: isSelected ? theme.highlight : (nickMouse.containsMouse ? theme.hoverBg : "transparent")

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData
                            color: {
                                // Color by prefix: @ = op, + = voice, % = halfop
                                if (modelData.startsWith("@")) return theme.nickOp
                                if (modelData.startsWith("%")) return theme.nickHalfOp
                                if (modelData.startsWith("+")) return theme.nickVoice
                                if (modelData.startsWith("~")) return theme.nickOwner
                                if (modelData.startsWith("&")) return theme.nickAdmin
                                return theme.nickNormal
                            }
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            width: parent.width - 16
                        }

                        MouseArea {
                            id: nickMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            onClicked: function(mouse) {
                                if (mouse.button === Qt.LeftButton) {
                                    var nick = bareN
                                    var sel = root.selectedNicks.slice()
                                    if (mouse.modifiers & Qt.ControlModifier) {
                                        // Ctrl+click: toggle this nick
                                        var idx = sel.indexOf(nick)
                                        if (idx !== -1) sel.splice(idx, 1); else sel.push(nick)
                                        root.selectedNicks = sel
                                    } else if (mouse.modifiers & Qt.ShiftModifier) {
                                        // Shift+click: range select from last clicked
                                        var users = channelUsers || []
                                        var from = root.lastClickedNickIndex
                                        var to = index
                                        if (from < 0) from = to
                                        var lo = Math.min(from, to), hi = Math.max(from, to)
                                        // keep existing selection from ctrl, add range
                                        for (var r = lo; r <= hi; r++) {
                                            var rn = root.bareNick(users[r])
                                            if (sel.indexOf(rn) === -1) sel.push(rn)
                                        }
                                        root.selectedNicks = sel
                                    } else {
                                        // Plain click: select only this nick
                                        root.selectedNicks = [nick]
                                    }
                                    root.lastClickedNickIndex = index
                                }
                            }
                            onPressed: function(mouse) {
                                if (mouse.button === Qt.RightButton) {
                                    var nick = bareN
                                    // If right-clicked nick isn't selected, select only it
                                    if (root.selectedNicks.indexOf(nick) === -1)
                                        root.selectedNicks = [nick]
                                    root.lastClickedNickIndex = index
                                    nickContextMenu.targetNick = nick
                                    nickContextMenu.popup()
                                }
                            }
                            onDoubleClicked: {
                                var nick = bareN
                                root.selectedNicks = [nick]
                                root.lastClickedNickIndex = index
                                messageInput.text = "/msg " + nick + " "
                                messageInput.forceActiveFocus()
                            }
                        }
                    }
                }

                // ── Channel mode buttons (HexChat-style) ──
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: theme.separator
                    visible: root.prefShowModeButtons
                }
                GridLayout {
                    visible: root.prefShowModeButtons
                    Layout.fillWidth: true
                    Layout.margins: 3
                    columns: 3
                    columnSpacing: 2
                    rowSpacing: 2

                    Repeater {
                        model: [
                            { label: "Op",   cmd: "+o" },
                            { label: "DeOp", cmd: "-o" },
                            { label: "Ban",  cmd: "ban" },
                            { label: "Kick", cmd: "kick" },
                            { label: "Voice",   cmd: "+v" },
                            { label: "DeVoice", cmd: "-v" }
                        ]
                        delegate: Button {
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.preferredHeight: 24
                            text: modelData.label
                            enabled: root.selectedNicks.length > 0 && currentChannel.startsWith("#")
                            onClicked: {
                                root.forEachSelectedNick(function(nick) {
                                    if (modelData.cmd === "kick")
                                        ircManager.sendRawCommand("KICK " + currentChannel + " " + nick)
                                    else if (modelData.cmd === "ban")
                                        ircManager.sendRawCommand("MODE " + currentChannel + " +b " + nick + "!*@*")
                                    else
                                        ircManager.sendRawCommand("MODE " + currentChannel + " " + modelData.cmd + " " + nick)
                                })
                            }
                            background: Rectangle {
                                color: parent.enabled ? (parent.down ? theme.buttonPressed : theme.buttonBg) : theme.buttonDisabled
                                radius: 2
                            }
                            contentItem: Text {
                                text: parent.text; color: parent.enabled ? theme.buttonText : theme.buttonTextDisabled
                                font.pixelSize: 10; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }
            }
        }
    }

    // ── Nick right-click context menu (nick list + chat area) ──
    Menu {
        id: nickContextMenu
        property string targetNick: ""

        palette.base: theme.menuBg
        palette.text: theme.menuText
        palette.highlight: theme.menuHighlight
        palette.highlightedText: theme.menuHighlightText

        Action { text: "Open Query"; onTriggered: { ircManager.openQuery(currentServer, nickContextMenu.targetNick); messageInput.forceActiveFocus() } }
        MenuSeparator {}
        Action { text: "WHOIS"; onTriggered: ircManager.sendRawCommand("WHOIS " + nickContextMenu.targetNick) }
        Action { text: "CTCP Version"; onTriggered: ircManager.sendRawCommand("PRIVMSG " + nickContextMenu.targetNick + " :\x01VERSION\x01") }
        Action { text: "CTCP Ping"; onTriggered: ircManager.sendRawCommand("PRIVMSG " + nickContextMenu.targetNick + " :\x01PING " + Date.now() + "\x01") }
        Action { text: "CTCP Time"; onTriggered: ircManager.sendRawCommand("PRIVMSG " + nickContextMenu.targetNick + " :\x01TIME\x01") }
        Action { text: "CTCP Finger"; onTriggered: ircManager.sendRawCommand("PRIVMSG " + nickContextMenu.targetNick + " :\x01FINGER\x01") }
        MenuSeparator {}
        Action { text: "Op"; onTriggered: forEachSelectedNick(function(n) { ircManager.sendRawCommand("MODE " + currentChannel + " +o " + n) }) }
        Action { text: "DeOp"; onTriggered: forEachSelectedNick(function(n) { ircManager.sendRawCommand("MODE " + currentChannel + " -o " + n) }) }
        Action { text: "HalfOp"; onTriggered: forEachSelectedNick(function(n) { ircManager.sendRawCommand("MODE " + currentChannel + " +h " + n) }) }
        Action { text: "DeHalfOp"; onTriggered: forEachSelectedNick(function(n) { ircManager.sendRawCommand("MODE " + currentChannel + " -h " + n) }) }
        Action { text: "Voice"; onTriggered: forEachSelectedNick(function(n) { ircManager.sendRawCommand("MODE " + currentChannel + " +v " + n) }) }
        Action { text: "DeVoice"; onTriggered: forEachSelectedNick(function(n) { ircManager.sendRawCommand("MODE " + currentChannel + " -v " + n) }) }
        MenuSeparator {}
        Action { text: "Invite..."; onTriggered: { inviteNickField.text = nickContextMenu.targetNick; inviteDialog.open() } }
        Action { text: "Slap"; onTriggered: forEachSelectedNick(function(n) { ircManager.sendMessage(currentChannel, "/me slaps " + n + " around a bit with a large trout") }) }
        MenuSeparator {}
        Action { text: "Kick"; onTriggered: forEachSelectedNick(function(n) { ircManager.sendRawCommand("KICK " + currentChannel + " " + n) }) }
        Action { text: "Ban"; onTriggered: forEachSelectedNick(function(n) { ircManager.sendRawCommand("MODE " + currentChannel + " +b " + n + "!*@*") }) }
        Action { text: "Kick + Ban"; onTriggered: forEachSelectedNick(function(n) { ircManager.sendRawCommand("MODE " + currentChannel + " +b " + n + "!*@*"); ircManager.sendRawCommand("KICK " + currentChannel + " " + n) }) }
        Action { text: "Ignore"; onTriggered: msgModel.addMessage("system", "Ignore list not yet implemented for " + selectedNicks.join(", ")) }
        MenuSeparator {}

        // ── ChanServ nick operations ──
        Menu {
            title: "ChanServ"
            Action { text: "Op";      onTriggered: { if (currentChannel) forEachSelectedNick(function(n) { ircManager.sendRawCommand("PRIVMSG ChanServ :OP " + currentChannel + " " + n) }) } }
            Action { text: "DeOp";    onTriggered: { if (currentChannel) forEachSelectedNick(function(n) { ircManager.sendRawCommand("PRIVMSG ChanServ :DEOP " + currentChannel + " " + n) }) } }
            Action { text: "HalfOp";  onTriggered: { if (currentChannel) forEachSelectedNick(function(n) { ircManager.sendRawCommand("PRIVMSG ChanServ :HALFOP " + currentChannel + " " + n) }) } }
            Action { text: "Voice";   onTriggered: { if (currentChannel) forEachSelectedNick(function(n) { ircManager.sendRawCommand("PRIVMSG ChanServ :VOICE " + currentChannel + " " + n) }) } }
            Action { text: "DeVoice"; onTriggered: { if (currentChannel) forEachSelectedNick(function(n) { ircManager.sendRawCommand("PRIVMSG ChanServ :DEVOICE " + currentChannel + " " + n) }) } }
            MenuSeparator {}
            Action { text: "Kick";      onTriggered: { if (currentChannel) forEachSelectedNick(function(n) { ircManager.sendRawCommand("PRIVMSG ChanServ :KICK " + currentChannel + " " + n) }) } }
            Action { text: "Ban";       onTriggered: { if (currentChannel) forEachSelectedNick(function(n) { ircManager.sendRawCommand("PRIVMSG ChanServ :BAN " + currentChannel + " " + n) }) } }
            Action { text: "Unban";     onTriggered: { if (currentChannel) forEachSelectedNick(function(n) { ircManager.sendRawCommand("PRIVMSG ChanServ :UNBAN " + currentChannel + " " + n) }) } }
            Action { text: "Kick+Ban";  onTriggered: { if (currentChannel) forEachSelectedNick(function(n) { ircManager.sendRawCommand("PRIVMSG ChanServ :KICKBAN " + currentChannel + " " + n) }) } }
            Action { text: "Quiet";     onTriggered: { if (currentChannel) forEachSelectedNick(function(n) { ircManager.sendRawCommand("PRIVMSG ChanServ :QUIET " + currentChannel + " " + n) }) } }
        }

        // ── OperServ nick operations ──
        Menu {
            title: "OperServ"
            Action { text: "Kill...";   onTriggered: { messageInput.text = "/KILL " + nickContextMenu.targetNick + " "; messageInput.forceActiveFocus() } }
            Action { text: "AKILL...";  onTriggered: { messageInput.text = "/OS AKILL ADD " + nickContextMenu.targetNick + "!*@* "; messageInput.forceActiveFocus() } }
            Action { text: "GLINE...";  onTriggered: { messageInput.text = "/GLINE " + nickContextMenu.targetNick + "!*@* "; messageInput.forceActiveFocus() } }
            Action { text: "KLINE...";  onTriggered: { messageInput.text = "/KLINE " + nickContextMenu.targetNick + "!*@* "; messageInput.forceActiveFocus() } }
        }
    }

    // ── Chat area link right-click menu ──
    Menu {
        id: chatLinkMenu
        property string targetUrl: ""

        palette.base: theme.menuBg
        palette.text: theme.menuText
        palette.highlight: theme.menuHighlight
        palette.highlightedText: theme.menuHighlightText

        Action {
            text: "Open URL"
            onTriggered: Qt.openUrlExternally(chatLinkMenu.targetUrl)
        }
        Action {
            text: "Copy Link"
            onTriggered: imgDownloader.copyToClipboard(chatLinkMenu.targetUrl)
        }
        MenuSeparator {}
        Action {
            text: "Download Image"
            enabled: {
                var u = chatLinkMenu.targetUrl.toLowerCase()
                return u.endsWith(".png") || u.endsWith(".jpg") || u.endsWith(".jpeg")
                    || u.endsWith(".gif") || u.endsWith(".webp") || u.endsWith(".bmp")
                    || u.endsWith(".svg") || u.endsWith(".tiff")
            }
            onTriggered: {
                if (imgDownloader.saveImageToDownloads(chatLinkMenu.targetUrl)) {
                    msgModel.addMessage("system", "Image saved to Downloads")
                } else {
                    // Not cached yet — download first, then try save
                    imgDownloader.download(chatLinkMenu.targetUrl)
                    msgModel.addMessage("system", "Downloading image… will save when ready")
                }
            }
        }
    }
    // ── Send message function ──
    function sendMessage() {
        var txt = messageInput.text
        if (txt === "") return
        // Allow /commands even without a channel selected
        if (txt.startsWith("/")) {
            var target = currentChannel !== "" ? currentChannel : (currentServer !== "" ? currentServer : "")
            ircManager.sendMessage(target, txt)
            messageInput.text = ""
            messageInput.forceActiveFocus()
            return
        }
        if (currentChannel !== "") {
            ircManager.sendMessage(currentChannel, txt)
            messageInput.text = ""
            messageInput.forceActiveFocus()
        }
    }

    // ── IRC event handlers ──
    Connections {
        target: ircManager
        function onServerRegistered(serverName) {
            root.currentServer = serverName
            refreshChannelList()
            // Auto-join pending channel after registration
            if (root.pendingAutoJoin !== "") {
                ircManager.joinChannel(root.pendingAutoJoin, "")
                root.pendingAutoJoin = ""
            }
        }
        function onChannelJoined(serverName, channel) {
            root.currentServer = serverName
            root.currentChannel = channel
            refreshChannelList()
            // Auto-select the joined channel in the sidebar
            for (var i = 0; i < channelListModel.count; i++) {
                if (channelListModel.get(i).name === channel &&
                    channelListModel.get(i).entryType === "channel") {
                    serverTree.currentIndex = i
                    break
                }
            }
        }
        function onChannelParted(serverName, channel) {
            refreshChannelList()
        }
        function onUnreadStateChanged() {
            updateUnreadStates()
        }
        function onCurrentNickChanged(nick) {
            // Could update a nick display somewhere
        }
        function onChannelTopicChanged(topic) {
            root.channelTopic = topic
        }
        function onChannelUsersChanged(users) {
            root.channelUsers = users
        }
    }

    // ── Dialogs ──
    NetworkListDialog   { id: networkListDialog }
    QuickConnectDialog  { id: quickConnectDialog }
    JoinChannelDialog   { id: joinChannelDialog }
    NickChangeDialog    { id: nickChangeDialog }
    ChannelListDialog   { id: channelListDialog }
    DccDialog           { id: dccDialog }
    RawLogDialog        { id: rawLogDialog }
    UrlGrabberDialog    { id: urlGrabberDialog }
    SearchDialog        { id: searchDialog }
    BanListDialog       { id: banListDialog }
    ChannelModeDialog   { id: channelModeDialog }
    TopicDialog         { id: topicDialog }
    AboutDialog         { id: aboutDialog }
    PreferencesDialog   { id: preferencesDialog }
    ScriptsDialog       { id: scriptsDialog }

    // ── Native file browser for loading Python scripts ──
    FileDialog {
        id: scriptFileDialog
        title: "Load Python Script"
        nameFilters: ["Python scripts (*.py)", "All files (*)"]
        currentFolder: typeof pyEngine !== 'undefined' ? "file://" + pyEngine.scriptsDirectory : ""
        onAccepted: {
            if (typeof pyEngine !== 'undefined') {
                var path = selectedFile.toString()
                // Strip file:// prefix
                if (path.startsWith("file://")) path = path.substring(7)
                pyEngine.loadScript(path)
            }
        }
    }

    // ── Inline mini-dialogs ──

    // Private message dialog
    Dialog {
        id: privateNickDialog; title: "Private Message"; width: 340; height: 140; modal: true; anchors.centerIn: parent
        background: Rectangle { color: theme.dialogBg; border.color: theme.dialogBorder; border.width: 1; radius: 6 }
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 16; spacing: 10
            RowLayout {
                spacing: 10; Layout.fillWidth: true
                Text { text: "Nickname:"; color: theme.textSecondary; font.pixelSize: 12 }
                TextField {
                    id: pmNickField; Layout.fillWidth: true; placeholderText: "nick"; placeholderTextColor: theme.placeholder
                    color: theme.menuText; font.pixelSize: 12; background: Rectangle { color: theme.dialogFieldBg; border.color: theme.dialogFieldBorder; radius: 2 }
                    onAccepted: pmOkBtn.clicked()
                }
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 8; Item { Layout.fillWidth: true }
                Button {
                    id: pmOkBtn; text: "OK"
                    onClicked: {
                        if (pmNickField.text !== "") {
                            ircManager.openQuery(currentServer, pmNickField.text)
                            messageInput.forceActiveFocus()
                            privateNickDialog.close()
                        }
                    }
                    background: Rectangle { color: parent.down ? theme.buttonPressed : theme.buttonBg; radius: 3 }
                    contentItem: Text { text: parent.text; color: theme.buttonText; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                Button {
                    text: "Cancel"
                    onClicked: privateNickDialog.close()
                    background: Rectangle { color: parent.down ? theme.dialogBorder : theme.buttonDisabled; radius: 3 }
                    contentItem: Text { text: parent.text; color: theme.textSecondary; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }
    }

    // Invite user dialog
    Dialog {
        id: inviteDialog; title: "Invite User"; width: 340; height: 140; modal: true; anchors.centerIn: parent
        background: Rectangle { color: theme.dialogBg; border.color: theme.dialogBorder; border.width: 1; radius: 6 }
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 16; spacing: 10
            RowLayout {
                spacing: 10; Layout.fillWidth: true
                Text { text: "Nickname:"; color: theme.textSecondary; font.pixelSize: 12 }
                TextField {
                    id: inviteNickField; Layout.fillWidth: true; placeholderText: "nick to invite"; placeholderTextColor: theme.placeholder
                    color: theme.menuText; font.pixelSize: 12; background: Rectangle { color: theme.dialogFieldBg; border.color: theme.dialogFieldBorder; radius: 2 }
                    onAccepted: inviteOkBtn.clicked()
                }
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 8; Item { Layout.fillWidth: true }
                Button {
                    id: inviteOkBtn; text: "Invite"
                    onClicked: {
                        if (inviteNickField.text !== "" && currentChannel !== "") {
                            ircManager.sendRawCommand("INVITE " + inviteNickField.text + " " + currentChannel)
                            inviteDialog.close()
                        }
                    }
                    background: Rectangle { color: parent.down ? theme.buttonPressed : theme.buttonBg; radius: 3 }
                    contentItem: Text { text: parent.text; color: theme.buttonText; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                Button {
                    text: "Cancel"
                    onClicked: inviteDialog.close()
                    background: Rectangle { color: parent.down ? theme.dialogBorder : theme.buttonDisabled; radius: 3 }
                    contentItem: Text { text: parent.text; color: theme.textSecondary; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }
    }

    // Server password dialog
    Dialog {
        id: serverPassDialog; title: "Server Password"; width: 340; height: 140; modal: true; anchors.centerIn: parent
        background: Rectangle { color: theme.dialogBg; border.color: theme.dialogBorder; border.width: 1; radius: 6 }
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 16; spacing: 10
            RowLayout {
                spacing: 10; Layout.fillWidth: true
                Text { text: "Password:"; color: theme.textSecondary; font.pixelSize: 12 }
                TextField {
                    id: serverPassField; Layout.fillWidth: true; echoMode: TextInput.Password
                    placeholderText: "server password"; placeholderTextColor: theme.placeholder
                    color: theme.menuText; font.pixelSize: 12; background: Rectangle { color: theme.dialogFieldBg; border.color: theme.dialogFieldBorder; radius: 2 }
                    onAccepted: passOkBtn.clicked()
                }
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 8; Item { Layout.fillWidth: true }
                Button {
                    id: passOkBtn; text: "Send"
                    onClicked: {
                        if (serverPassField.text !== "") {
                            ircManager.sendRawCommand("PASS " + serverPassField.text)
                            serverPassDialog.close()
                        }
                    }
                    background: Rectangle { color: parent.down ? theme.buttonPressed : theme.buttonBg; radius: 3 }
                    contentItem: Text { text: parent.text; color: theme.buttonText; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                Button {
                    text: "Cancel"
                    onClicked: serverPassDialog.close()
                    background: Rectangle { color: parent.down ? theme.dialogBorder : theme.buttonDisabled; radius: 3 }
                    contentItem: Text { text: parent.text; color: theme.textSecondary; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }
    }
}
