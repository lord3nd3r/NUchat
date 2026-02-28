import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: dlg
    title: "Preferences"
    width: 640
    height: 520
    modal: true
    anchors.centerIn: parent

    // helper: read a bool setting (QSettings returns strings after restart)
    function boolSetting(key, def) {
        var v = appSettings.value(key, def)
        return v === true || v === "true"
    }

    // helper: read an int setting
    function intSetting(key, def) {
        var v = appSettings.value(key, def)
        return parseInt(v) || def
    }

    // Load ALL saved settings when dialog opens
    onOpened: {
        // User Info (tab 4)
        prefNick.text  = appSettings.value("user/nickname", "NUchat_user")
        prefNick2.text = appSettings.value("user/nickname2", "NUchat_user2")
        prefNick3.text = appSettings.value("user/nickname3", "NUchat_user3")
        prefUsername.text  = appSettings.value("user/username", "nuchat")
        prefRealname.text  = appSettings.value("user/realname", "NUchat User")
        prefQuitMsg.text   = appSettings.value("user/quitMessage", "Leaving (NUchat)")
        prefPartMsg.text   = appSettings.value("user/partMessage", "")
        prefAwayMsg.text   = appSettings.value("user/awayMessage", "I'm away")

        // Interface (tab 0)
        chkShowServerTree.checked   = boolSetting("ui/showServerTree", true)
        chkShowUserList.checked     = boolSetting("ui/showUserList", true)
        chkShowTopicBar.checked     = boolSetting("ui/showTopicBar", true)
        chkShowModeButtons.checked  = boolSetting("ui/showModeButtons", true)
        chkShowStatusBar.checked    = boolSetting("ui/showStatusBar", true)
        chkTabsInBackground.checked = boolSetting("ui/tabsInBackground", false)
        chkConfirmClose.checked     = boolSetting("ui/confirmClose", true)
        chkMinToTray.checked        = boolSetting("ui/minimizeToTray", false)

        // Colors (tab 1)
        chkStripColors.checked      = boolSetting("ui/stripMircColors", false)
        chkShowNickColors.checked   = boolSetting("ui/showNickColors", true)

        // Text & Fonts (tab 2)
        cboFontFamily.currentIndex  = intSetting("ui/fontFamilyIndex", 0)
        spnFontSize.value           = intSetting("ui/fontSize", 12)
        chkShowTimestamps.checked   = boolSetting("ui/showTimestamps", true)
        cboTimestampFmt.currentIndex = intSetting("ui/timestampFmtIndex", 0)
        spnScrollback.value         = intSetting("ui/maxScrollback", 10000)
        chkIndentWrap.checked       = boolSetting("ui/indentWrap", true)
        chkHighlighting.checked     = boolSetting("ui/textHighlighting", true)
        txtHighlightWords.text      = appSettings.value("ui/highlightWords", "")

        // Input (tab 3)
        chkSpellCheck.checked       = boolSetting("input/spellCheck", true)
        chkTabCompletion.checked    = boolSetting("input/tabCompletion", true)
        txtCompletionSuffix.text    = appSettings.value("input/completionSuffix", ":")
        chkCompletionMenu.checked   = boolSetting("input/completionMenu", true)
        chkHistoryPerChannel.checked = boolSetting("input/historyPerChannel", false)
        spnMaxHistory.value         = intSetting("input/maxHistory", 100)

        // Connection (tab 5)
        chkAutoReconnect.checked    = boolSetting("conn/autoReconnect", true)
        spnReconnectDelay.value     = intSetting("conn/reconnectDelay", 10)
        spnMaxReconnect.value       = intSetting("conn/maxReconnectAttempts", 10)
        chkAutoJoin.checked         = boolSetting("conn/autoJoin", true)
        chkGlobalUserInfo.checked   = boolSetting("conn/globalUserInfo", true)
        cboProxyType.currentIndex   = intSetting("conn/proxyTypeIndex", 0)
        txtProxyHost.text           = appSettings.value("conn/proxyHost", "")
        txtProxyPort.text           = appSettings.value("conn/proxyPort", "")
        chkProxyAuth.checked        = boolSetting("conn/proxyAuth", false)
        txtProxyUser.text           = appSettings.value("conn/proxyUser", "")
        txtProxyPass.text           = appSettings.value("conn/proxyPass", "")

        // SASL (tab 6)
        cboAuthMethod.currentIndex  = intSetting("auth/methodIndex", 0)
        txtNickServCmd.text         = appSettings.value("auth/nickservCmd", "/msg NickServ IDENTIFY %p")
        txtCertFile.text            = appSettings.value("auth/certFile", "")
        txtKeyFile.text             = appSettings.value("auth/keyFile", "")
        chkAcceptInvalidSSL.checked = boolSetting("auth/acceptInvalidSSL", false)

        // DCC (tab 7)
        txtDccDownloadDir.text      = appSettings.value("dcc/downloadDir", "~/Downloads")
        chkDccAutoAccept.checked    = boolSetting("dcc/autoAccept", false)
        spnDccMaxSize.value         = intSetting("dcc/maxSize", 0)
        cboDccIpMethod.currentIndex = intSetting("dcc/ipMethodIndex", 0)
        txtDccManualIp.text         = appSettings.value("dcc/manualIp", "")
        spnDccPortLow.value         = intSetting("dcc/portLow", 1024)
        spnDccPortHigh.value        = intSetting("dcc/portHigh", 5000)

        // Logging (tab 8)
        chkEnableLogging.checked    = boolSetting("log/enable", true)
        txtLogDir.text              = appSettings.value("log/directory", "~/.config/NUchat/logs")
        cboLogFormat.currentIndex   = intSetting("log/formatIndex", 0)
        chkLogTimestamps.checked    = boolSetting("log/timestamps", true)
        chkLogPMs.checked           = boolSetting("log/privateMessages", true)
        chkLogPerChannel.checked    = boolSetting("log/perChannel", true)

        // Notifications (tab 9)
        chkNotifyHighlight.checked  = boolSetting("notify/highlight", true)
        chkNotifyPM.checked         = boolSetting("notify/privateMessage", true)
        chkFlashTaskbar.checked     = boolSetting("notify/flashTaskbar", true)
        chkTrayUnread.checked       = boolSetting("notify/trayUnread", true)
        chkHighlightNick.checked    = boolSetting("notify/highlightNick", true)
        spnNotifyTimeout.value      = intSetting("notify/timeout", 5000)

        // Sounds (tab 10)
        chkEnableSounds.checked     = boolSetting("sound/enable", true)
        chkSoundHighlight.checked   = boolSetting("sound/highlight", true)
        chkSoundPM.checked          = boolSetting("sound/privateMessage", true)
        chkSoundConnect.checked     = boolSetting("sound/connection", false)
        txtBeepCmd.text             = appSettings.value("sound/beepCommand", "")

        // URL Handlers (tab 12)
        chkClickableUrls.checked    = boolSetting("url/clickable", true)
        chkUrlGrabber.checked       = boolSetting("url/autoGrab", true)
        chkShowInlineImages.checked = boolSetting("ui/showInlineImages", true)
        txtBrowserCmd.text          = appSettings.value("url/browserCmd", "xdg-open %s")

        // Advanced (tab 16)
        cboEncoding.currentIndex    = intSetting("adv/encodingIndex", 0)
        chkIdentifyFirst.checked    = boolSetting("adv/identifyFirst", true)
        chkIPv6.checked             = boolSetting("adv/ipv6", false)
        chkCAP.checked              = boolSetting("adv/capNegotiation", true)
        txtPerformCmds.text         = appSettings.value("adv/performCommands", "/mode %n +x")
        chkShowRawIRC.checked       = boolSetting("adv/showRawIRC", false)
        txtCmdChar.text             = appSettings.value("adv/cmdChar", "/")

        // Plugin/Script dirs (tabs 14, 15)
        txtPluginDir.text           = appSettings.value("plugin/directory", "~/.config/NUchat/plugins")
        txtScriptDir.text           = appSettings.value("script/directory", "~/.config/NUchat/scripts")
    }

    // Save all settings (called on OK)
    function saveSettings() {
        // User info
        appSettings.setValue("user/nickname", prefNick.text)
        appSettings.setValue("user/nickname2", prefNick2.text)
        appSettings.setValue("user/nickname3", prefNick3.text)
        appSettings.setValue("user/username", prefUsername.text)
        appSettings.setValue("user/realname", prefRealname.text)
        appSettings.setValue("user/quitMessage", prefQuitMsg.text)
        appSettings.setValue("user/partMessage", prefPartMsg.text)
        appSettings.setValue("user/awayMessage", prefAwayMsg.text)

        // Text fields that only save on OK (not live)
        appSettings.setValue("input/completionSuffix", txtCompletionSuffix.text)
        appSettings.setValue("conn/proxyHost", txtProxyHost.text)
        appSettings.setValue("conn/proxyPort", txtProxyPort.text)
        appSettings.setValue("conn/proxyUser", txtProxyUser.text)
        appSettings.setValue("conn/proxyPass", txtProxyPass.text)
        appSettings.setValue("auth/nickservCmd", txtNickServCmd.text)
        appSettings.setValue("auth/certFile", txtCertFile.text)
        appSettings.setValue("auth/keyFile", txtKeyFile.text)
        appSettings.setValue("dcc/downloadDir", txtDccDownloadDir.text)
        appSettings.setValue("dcc/manualIp", txtDccManualIp.text)
        appSettings.setValue("log/directory", txtLogDir.text)
        appSettings.setValue("sound/beepCommand", txtBeepCmd.text)
        appSettings.setValue("url/browserCmd", txtBrowserCmd.text)
        appSettings.setValue("ui/highlightWords", txtHighlightWords.text)
        appSettings.setValue("adv/performCommands", txtPerformCmds.text)
        appSettings.setValue("adv/cmdChar", txtCmdChar.text)
        appSettings.setValue("plugin/directory", txtPluginDir.text)
        appSettings.setValue("script/directory", txtScriptDir.text)
        appSettings.sync()
    }

    // helper: save a setting and sync immediately (for checkboxes, spinboxes, combos)
    function saveSetting(key, val) { appSettings.setValue(key, val); appSettings.sync() }

    background: Rectangle { color: "#2b2b2b"; border.color: "#555"; border.width: 1; radius: 6 }
    header: Rectangle {
        height: 38; color: "#252526"; radius: 6
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 6; color: "#252526" }
        Text { anchors.centerIn: parent; text: "Preferences"; color: "#ddd"; font.pixelSize: 14; font.bold: true }
    }

    RowLayout {
        anchors.fill: parent; anchors.margins: 0; spacing: 0

        Rectangle {
            Layout.preferredWidth: 160; Layout.fillHeight: true; color: "#1e1e1e"
            ListView {
                id: prefCatList
                anchors.fill: parent; anchors.topMargin: 8; anchors.bottomMargin: 8
                clip: true; currentIndex: 0
                model: ListModel {
                    ListElement { name: "Interface" }
                    ListElement { name: "Colors" }
                    ListElement { name: "Text & Fonts" }
                    ListElement { name: "Input" }
                    ListElement { name: "User Info" }
                    ListElement { name: "Connection" }
                    ListElement { name: "SASL / Auth" }
                    ListElement { name: "DCC" }
                    ListElement { name: "Logging" }
                    ListElement { name: "Notifications" }
                    ListElement { name: "Sounds" }
                    ListElement { name: "Auto-Replace" }
                    ListElement { name: "URL Handlers" }
                    ListElement { name: "Text Events" }
                    ListElement { name: "Plugins" }
                    ListElement { name: "Scripts" }
                    ListElement { name: "Advanced" }
                    ListElement { name: "Shortcuts" }
                }
                delegate: Rectangle {
                    required property int index
                    required property string name
                    width: prefCatList.width; height: 28
                    color: prefCatList.currentIndex === index ? "#264f78"
                         : catMouse.containsMouse ? "#333" : "transparent"
                    Text {
                        anchors.left: parent.left; anchors.leftMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        text: name; color: prefCatList.currentIndex === index ? "#fff" : "#ccc"
                        font.pixelSize: 12
                    }
                    MouseArea {
                        id: catMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: prefCatList.currentIndex = index
                    }
                }
            }
        }

        Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#404040" }

        StackLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            currentIndex: prefCatList.currentIndex

            // 0 — Interface
            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width; spacing: 8
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: 16

                    Text { text: "Interface"; color: "#ddd"; font.pixelSize: 16; font.bold: true }
                    CheckBox { id: chkShowServerTree; text: "Show server tree"
                        onCheckedChanged: { appSettings.setValue("ui/showServerTree", checked); appSettings.sync(); root.prefShowServerTree = checked }
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkShowUserList; text: "Show user list in channels"
                        onCheckedChanged: { appSettings.setValue("ui/showUserList", checked); appSettings.sync(); root.prefShowUserList = checked }
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkShowTopicBar; text: "Show topic bar"
                        onCheckedChanged: { appSettings.setValue("ui/showTopicBar", checked); appSettings.sync(); root.prefShowTopicBar = checked }
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkShowStatusBar; text: "Show status bar"
                        onCheckedChanged: saveSetting("ui/showStatusBar", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkShowModeButtons; text: "Show mode buttons"
                        onCheckedChanged: { saveSetting("ui/showModeButtons", checked); root.prefShowModeButtons = checked }
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkTabsInBackground; text: "Open new tabs in background"
                        onCheckedChanged: saveSetting("ui/tabsInBackground", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkConfirmClose; text: "Confirm on close when connected"
                        onCheckedChanged: saveSetting("ui/confirmClose", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkMinToTray; text: "Minimize to system tray"
                        onCheckedChanged: saveSetting("ui/minimizeToTray", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                }
            }

            // 1 — Colors
            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width; spacing: 8
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: 16

                    Text { text: "Colors / Theme"; color: "#ddd"; font.pixelSize: 16; font.bold: true }
                    RowLayout {
                        spacing: 8
                        Text { text: "Theme:"; color: "#ccc"; font.pixelSize: 12 }
                        ComboBox {
                            model: themeManager.availableThemes()
                            Layout.preferredWidth: 200
                            onCurrentTextChanged: if (currentText !== "") themeManager.loadTheme(":/themes/" + currentText)
                        }
                    }
                    Text { text: "mIRC Colors:"; color: "#ccc"; font.pixelSize: 13; font.bold: true }
                    GridLayout {
                        columns: 8; rowSpacing: 4; columnSpacing: 4
                        Repeater {
                            model: ["#fff","#000","#00007f","#009300","#f00","#7f0000","#9c009c","#fc7f00",
                                    "#ffff00","#00fc00","#009393","#00ffff","#0000fc","#ff00ff","#7f7f7f","#d2d2d2"]
                            delegate: Rectangle { width: 24; height: 24; color: modelData; border.color: "#666"; radius: 2 }
                        }
                    }
                    CheckBox { id: chkStripColors; text: "Strip mIRC colors from messages"
                        onCheckedChanged: { appSettings.setValue("ui/stripMircColors", checked); appSettings.sync(); root.prefStripColors = checked }
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkShowNickColors; text: "Show nick colors"
                        onCheckedChanged: { appSettings.setValue("ui/showNickColors", checked); appSettings.sync(); root.prefShowNickColors = checked }
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                }
            }

            // 2 — Text & Fonts
            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width; spacing: 8
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: 16

                    Text { text: "Text & Fonts"; color: "#ddd"; font.pixelSize: 16; font.bold: true }
                    RowLayout {
                        spacing: 8
                        Text { text: "Font family:"; color: "#ccc"; font.pixelSize: 12 }
                        ComboBox {
                            id: cboFontFamily
                            model: ["Monospace", "DejaVu Sans Mono", "Liberation Mono", "Courier New", "Consolas", "Fira Code"]
                            Layout.preferredWidth: 200
                            onCurrentIndexChanged: { saveSetting("ui/fontFamilyIndex", currentIndex); root.prefFontFamily = currentText }
                        }
                    }
                    RowLayout {
                        spacing: 8
                        Text { text: "Font size:"; color: "#ccc"; font.pixelSize: 12 }
                        SpinBox {
                            id: spnFontSize; from: 8; to: 24; value: 12; Layout.preferredWidth: 100
                            onValueChanged: { saveSetting("ui/fontSize", value); root.prefFontSize = value }
                        }
                    }
                    CheckBox { id: chkShowTimestamps; text: "Show timestamps"
                        onCheckedChanged: { saveSetting("ui/showTimestamps", checked); root.prefShowTimestamps = checked }
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    RowLayout {
                        spacing: 8
                        Text { text: "Timestamp format:"; color: "#ccc"; font.pixelSize: 12 }
                        ComboBox {
                            id: cboTimestampFmt
                            model: ["[HH:mm:ss]", "[HH:mm]", "[h:mm AP]", "HH:mm:ss"]
                            Layout.preferredWidth: 160
                            onCurrentIndexChanged: saveSetting("ui/timestampFmtIndex", currentIndex)
                        }
                    }
                    RowLayout {
                        spacing: 8
                        Text { text: "Max scrollback lines:"; color: "#ccc"; font.pixelSize: 12 }
                        SpinBox {
                            id: spnScrollback; from: 100; to: 100000; value: 10000; stepSize: 500; Layout.preferredWidth: 120
                            onValueChanged: saveSetting("ui/maxScrollback", value)
                        }
                    }
                    CheckBox { id: chkIndentWrap; text: "Indent wrapped text"
                        onCheckedChanged: saveSetting("ui/indentWrap", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkHighlighting; text: "Enable text highlighting"
                        onCheckedChanged: saveSetting("ui/textHighlighting", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    RowLayout {
                        spacing: 8
                        Text { text: "Extra highlight words:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
                            id: txtHighlightWords
                            Layout.fillWidth: true; placeholderText: "comma-separated"; placeholderTextColor: "#666"
                            color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                    }
                }
            }

            // 3 — Input
            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width; spacing: 8
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: 16

                    Text { text: "Input"; color: "#ddd"; font.pixelSize: 16; font.bold: true }
                    CheckBox { id: chkSpellCheck; text: "Enable spell check"
                        onCheckedChanged: saveSetting("input/spellCheck", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkTabCompletion; text: "Nick completion with Tab"
                        onCheckedChanged: saveSetting("input/tabCompletion", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    RowLayout {
                        spacing: 8
                        Text { text: "Nick completion suffix:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
                            id: txtCompletionSuffix
                            text: ":"; Layout.preferredWidth: 60; color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                    }
                    CheckBox { id: chkCompletionMenu; text: "Show nick completion menu"
                        onCheckedChanged: saveSetting("input/completionMenu", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkHistoryPerChannel; text: "Input history per channel"
                        onCheckedChanged: saveSetting("input/historyPerChannel", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    RowLayout {
                        spacing: 8
                        Text { text: "Max input history:"; color: "#ccc"; font.pixelSize: 12 }
                        SpinBox {
                            id: spnMaxHistory; from: 10; to: 1000; value: 100; Layout.preferredWidth: 100
                            onValueChanged: saveSetting("input/maxHistory", value)
                        }
                    }
                }
            }

            // 4 — User Info
            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width; spacing: 8
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: 16

                    Text { text: "User Info"; color: "#ddd"; font.pixelSize: 16; font.bold: true }
                    GridLayout {
                        columns: 2; columnSpacing: 10; rowSpacing: 8; Layout.fillWidth: true
                        Text { text: "Nickname:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField { id: prefNick; Layout.fillWidth: true; color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 } }
                        Text { text: "Second nick:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField { id: prefNick2; Layout.fillWidth: true; color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 } }
                        Text { text: "Third nick:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField { id: prefNick3; Layout.fillWidth: true; color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 } }
                        Text { text: "Username:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField { id: prefUsername; Layout.fillWidth: true; color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 } }
                        Text { text: "Real name:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField { id: prefRealname; Layout.fillWidth: true; color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 } }
                        Text { text: "Quit message:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField { id: prefQuitMsg; Layout.fillWidth: true; color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 } }
                        Text { text: "Part message:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField { id: prefPartMsg;
                            Layout.fillWidth: true; placeholderText: "(same as quit)"; placeholderTextColor: "#666"
                            color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                        Text { text: "Away message:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField { id: prefAwayMsg; Layout.fillWidth: true; color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 } }
                    }
                }
            }

            // 5 — Connection
            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width; spacing: 8
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: 16

                    Text { text: "Connection"; color: "#ddd"; font.pixelSize: 16; font.bold: true }
                    CheckBox { id: chkAutoReconnect; text: "Auto-reconnect on disconnect"
                        onCheckedChanged: saveSetting("conn/autoReconnect", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    RowLayout {
                        spacing: 8
                        Text { text: "Reconnect delay (seconds):"; color: "#ccc"; font.pixelSize: 12 }
                        SpinBox { id: spnReconnectDelay; from: 1; to: 300; value: 10; Layout.preferredWidth: 100
                            onValueChanged: saveSetting("conn/reconnectDelay", value) }
                    }
                    RowLayout {
                        spacing: 8
                        Text { text: "Max reconnect attempts:"; color: "#ccc"; font.pixelSize: 12 }
                        SpinBox { id: spnMaxReconnect; from: 0; to: 100; value: 10; Layout.preferredWidth: 100
                            onValueChanged: saveSetting("conn/maxReconnectAttempts", value) }
                    }
                    CheckBox { id: chkAutoJoin; text: "Auto-join channels on connect"
                        onCheckedChanged: saveSetting("conn/autoJoin", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkGlobalUserInfo; text: "Use global user info for all servers"
                        onCheckedChanged: saveSetting("conn/globalUserInfo", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    Text { text: "Proxy:"; color: "#ccc"; font.pixelSize: 13; font.bold: true }
                    RowLayout {
                        spacing: 8
                        Text { text: "Type:"; color: "#ccc"; font.pixelSize: 12 }
                        ComboBox { id: cboProxyType; model: ["None", "SOCKS4", "SOCKS5", "HTTP CONNECT"]; Layout.preferredWidth: 160
                            onCurrentIndexChanged: saveSetting("conn/proxyTypeIndex", currentIndex) }
                    }
                    RowLayout {
                        spacing: 8
                        Text { text: "Host:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
                            id: txtProxyHost
                            Layout.fillWidth: true; placeholderText: "proxy.example.com"; placeholderTextColor: "#666"
                            color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                        Text { text: "Port:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
                            id: txtProxyPort
                            Layout.preferredWidth: 60; placeholderText: "1080"; placeholderTextColor: "#666"
                            color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                    }
                    CheckBox { id: chkProxyAuth; text: "Proxy requires authentication"
                        onCheckedChanged: saveSetting("conn/proxyAuth", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    RowLayout {
                        spacing: 8
                        visible: chkProxyAuth.checked
                        Text { text: "User:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
                            id: txtProxyUser
                            Layout.fillWidth: true; placeholderText: "username"; placeholderTextColor: "#666"
                            color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                        Text { text: "Pass:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
                            id: txtProxyPass
                            Layout.fillWidth: true; placeholderText: "password"; placeholderTextColor: "#666"
                            color: "#ddd"; font.pixelSize: 12; echoMode: TextInput.Password
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                    }
                }
            }

            // 6 — SASL / Auth
            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width; spacing: 8
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: 16

                    Text { text: "SASL / Authentication"; color: "#ddd"; font.pixelSize: 16; font.bold: true }
                    Text { text: "Authentication is configured per-network in the Network List.\nSettings below are defaults for new networks."; color: "#aaa"; font.pixelSize: 12; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    RowLayout {
                        spacing: 8
                        Text { text: "Method:"; color: "#ccc"; font.pixelSize: 12 }
                        ComboBox {
                            id: cboAuthMethod
                            model: ["None", "SASL PLAIN", "SASL EXTERNAL (cert)", "SASL SCRAM-SHA-256",
                                    "SASL ECDSA-NIST256P", "NickServ (/msg)", "NickServ (/nickserv)",
                                    "Server Password", "CERTFP (client cert)"]
                            Layout.preferredWidth: 260
                            onCurrentIndexChanged: saveSetting("auth/methodIndex", currentIndex)
                        }
                    }
                    Text { text: "NickServ:"; color: "#ccc"; font.pixelSize: 13; font.bold: true }
                    GridLayout {
                        columns: 2; columnSpacing: 10; rowSpacing: 6; Layout.fillWidth: true
                        Text { text: "NickServ command:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
                            id: txtNickServCmd
                            text: "/msg NickServ IDENTIFY %p"; Layout.fillWidth: true
                            color: "#ddd"; font.family: "monospace"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                    }
                    Text { text: "Client Certificate:"; color: "#ccc"; font.pixelSize: 13; font.bold: true }
                    GridLayout {
                        columns: 2; columnSpacing: 10; rowSpacing: 6; Layout.fillWidth: true
                        Text { text: "Cert file (PEM):"; color: "#ccc"; font.pixelSize: 12 }
                        RowLayout {
                            Layout.fillWidth: true; spacing: 6
                            TextField {
                                id: txtCertFile
                                Layout.fillWidth: true; placeholderText: "/path/to/client.pem"; placeholderTextColor: "#666"
                                color: "#ddd"; font.pixelSize: 12
                                background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                            }
                            Button {
                                text: "Browse"
                                background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                                contentItem: Text { text: "Browse"; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            }
                        }
                        Text { text: "Key file (PEM):"; color: "#ccc"; font.pixelSize: 12 }
                        RowLayout {
                            Layout.fillWidth: true; spacing: 6
                            TextField {
                                id: txtKeyFile
                                Layout.fillWidth: true; placeholderText: "/path/to/client.key"; placeholderTextColor: "#666"
                                color: "#ddd"; font.pixelSize: 12
                                background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                            }
                            Button {
                                text: "Browse"
                                background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                                contentItem: Text { text: "Browse"; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            }
                        }
                    }
                    CheckBox { id: chkAcceptInvalidSSL; text: "Accept invalid SSL certificates"
                        onCheckedChanged: saveSetting("auth/acceptInvalidSSL", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                }
            }

            // 7 — DCC
            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width; spacing: 8
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: 16

                    Text { text: "DCC Settings"; color: "#ddd"; font.pixelSize: 16; font.bold: true }
                    RowLayout {
                        spacing: 8
                        Text { text: "Download folder:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
                            id: txtDccDownloadDir
                            Layout.fillWidth: true; text: "~/Downloads"; color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                        Button {
                            text: "Browse"
                            background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                            contentItem: Text { text: "Browse"; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                    }
                    CheckBox { id: chkDccAutoAccept; text: "Auto-accept DCC sends"
                        onCheckedChanged: saveSetting("dcc/autoAccept", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    RowLayout {
                        spacing: 8
                        Text { text: "Max file size (MB, 0=unlimited):"; color: "#ccc"; font.pixelSize: 12 }
                        SpinBox { id: spnDccMaxSize; from: 0; to: 10000; value: 0; Layout.preferredWidth: 120
                            onValueChanged: saveSetting("dcc/maxSize", value) }
                    }
                    Text { text: "DCC IP:"; color: "#ccc"; font.pixelSize: 13; font.bold: true }
                    RowLayout {
                        spacing: 8
                        Text { text: "IP method:"; color: "#ccc"; font.pixelSize: 12 }
                        ComboBox { id: cboDccIpMethod; model: ["Auto-detect", "Get from server (DCC)", "Specify manually"]; Layout.preferredWidth: 200
                            onCurrentIndexChanged: saveSetting("dcc/ipMethodIndex", currentIndex) }
                    }
                    RowLayout {
                        spacing: 8
                        Text { text: "Manual IP:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
                            id: txtDccManualIp
                            Layout.fillWidth: true; placeholderText: "0.0.0.0"; placeholderTextColor: "#666"
                            color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                    }
                    RowLayout {
                        spacing: 8
                        Text { text: "Port range:"; color: "#ccc"; font.pixelSize: 12 }
                        SpinBox { id: spnDccPortLow; from: 1024; to: 65535; value: 1024; Layout.preferredWidth: 100
                            onValueChanged: saveSetting("dcc/portLow", value) }
                        Text { text: "to"; color: "#ccc"; font.pixelSize: 12 }
                        SpinBox { id: spnDccPortHigh; from: 1024; to: 65535; value: 5000; Layout.preferredWidth: 100
                            onValueChanged: saveSetting("dcc/portHigh", value) }
                    }
                }
            }

            // 8 — Logging
            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width; spacing: 8
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: 16

                    Text { text: "Logging"; color: "#ddd"; font.pixelSize: 16; font.bold: true }
                    CheckBox { id: chkEnableLogging; text: "Enable chat logging"
                        onCheckedChanged: saveSetting("log/enable", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    RowLayout {
                        spacing: 8
                        Text { text: "Log directory:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
                            id: txtLogDir
                            Layout.fillWidth: true; text: "~/.config/NUchat/logs"; color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                        Button {
                            text: "Browse"
                            background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                            contentItem: Text { text: "Browse"; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                    }
                    RowLayout {
                        spacing: 8
                        Text { text: "Log format:"; color: "#ccc"; font.pixelSize: 12 }
                        ComboBox { id: cboLogFormat; model: ["Plain text", "HTML", "JSON"]; Layout.preferredWidth: 160
                            onCurrentIndexChanged: saveSetting("log/formatIndex", currentIndex) }
                    }
                    CheckBox { id: chkLogTimestamps; text: "Log timestamps"
                        onCheckedChanged: saveSetting("log/timestamps", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkLogPMs; text: "Log private messages"
                        onCheckedChanged: saveSetting("log/privateMessages", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkLogPerChannel; text: "Create separate log per channel"
                        onCheckedChanged: saveSetting("log/perChannel", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                }
            }

            // 9 — Notifications
            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width; spacing: 8
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: 16

                    Text { text: "Notifications"; color: "#ddd"; font.pixelSize: 16; font.bold: true }
                    CheckBox { id: chkNotifyHighlight; text: "Desktop notifications on highlight"
                        onCheckedChanged: saveSetting("notify/highlight", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkNotifyPM; text: "Desktop notifications on private message"
                        onCheckedChanged: saveSetting("notify/privateMessage", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkFlashTaskbar; text: "Flash taskbar on activity"
                        onCheckedChanged: saveSetting("notify/flashTaskbar", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkTrayUnread; text: "Show unread count in tray icon"
                        onCheckedChanged: saveSetting("notify/trayUnread", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkHighlightNick; text: "Highlight on nick mention"
                        onCheckedChanged: saveSetting("notify/highlightNick", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    RowLayout {
                        spacing: 8
                        Text { text: "Notification timeout (ms):"; color: "#ccc"; font.pixelSize: 12 }
                        SpinBox { id: spnNotifyTimeout; from: 0; to: 30000; value: 5000; stepSize: 500; Layout.preferredWidth: 120
                            onValueChanged: saveSetting("notify/timeout", value) }
                    }
                }
            }

            // 10 — Sounds
            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width; spacing: 8
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: 16

                    Text { text: "Sounds"; color: "#ddd"; font.pixelSize: 16; font.bold: true }
                    CheckBox { id: chkEnableSounds; text: "Enable sounds"
                        onCheckedChanged: saveSetting("sound/enable", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkSoundHighlight; text: "Play sound on highlight"
                        onCheckedChanged: saveSetting("sound/highlight", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkSoundPM; text: "Play sound on private message"
                        onCheckedChanged: saveSetting("sound/privateMessage", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkSoundConnect; text: "Play sound on connection"
                        onCheckedChanged: saveSetting("sound/connection", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    RowLayout {
                        spacing: 8
                        Text { text: "Beep command:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
                            id: txtBeepCmd
                            Layout.fillWidth: true; placeholderText: "aplay /path/to/sound.wav"; placeholderTextColor: "#666"
                            color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                    }
                }
            }

            // 11 — Auto-Replace
            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width; spacing: 8
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: 16

                    Text { text: "Auto-Replace"; color: "#ddd"; font.pixelSize: 16; font.bold: true }
                    Text { text: "Automatically replace text as you type:"; color: "#aaa"; font.pixelSize: 12 }
                    Rectangle {
                        Layout.fillWidth: true; Layout.fillHeight: true; color: "#1e1e1e"; border.color: "#404040"; radius: 3
                        ListView {
                            anchors.fill: parent; anchors.margins: 4; clip: true
                            model: ListModel {
                                ListElement { pattern: ":)"; replacement: "emoji_smile" }
                                ListElement { pattern: ":("; replacement: "emoji_sad" }
                                ListElement { pattern: ":D"; replacement: "emoji_grin" }
                                ListElement { pattern: "teh"; replacement: "the" }
                            }
                            delegate: RowLayout {
                                required property string pattern
                                required property string replacement
                                width: parent ? parent.width : 0; spacing: 10; height: 24
                                Text { text: pattern; color: "#569cd6"; font.family: "monospace"; font.pixelSize: 12; Layout.preferredWidth: 80 }
                                Text { text: "->"; color: "#888"; font.pixelSize: 12 }
                                Text { text: replacement; color: "#6a9955"; font.pixelSize: 12; Layout.fillWidth: true }
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true; spacing: 8
                        TextField {
                            id: arPat; Layout.preferredWidth: 100; placeholderText: "pattern"; placeholderTextColor: "#666"
                            color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                        Text { text: "->"; color: "#888"; font.pixelSize: 14 }
                        TextField {
                            id: arRep; Layout.fillWidth: true; placeholderText: "replacement"; placeholderTextColor: "#666"
                            color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                        Button {
                            text: "Add"
                            background: Rectangle { color: parent.down ? "#1177bb" : "#0e639c"; radius: 3 }
                            contentItem: Text { text: "Add"; color: "#fff"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                    }
                }
            }

            // 12 — URL Handlers
            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width; spacing: 8
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: 16

                    Text { text: "URL Handlers"; color: "#ddd"; font.pixelSize: 16; font.bold: true }
                    CheckBox { id: chkClickableUrls; text: "Make URLs clickable"
                        onCheckedChanged: saveSetting("url/clickable", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkUrlGrabber; text: "Auto-grab URLs to URL Grabber"
                        onCheckedChanged: saveSetting("url/grabber", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkShowInlineImages; text: "Show inline images from links"
                        onCheckedChanged: saveSetting("ui/showInlineImages", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    RowLayout {
                        spacing: 8
                        Text { text: "Browser command:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
                            id: txtBrowserCmd
                            Layout.fillWidth: true; text: "xdg-open %s"
                            color: "#ddd"; font.family: "monospace"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                    }
                }
            }

            // 13 — Text Events
            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width; spacing: 8
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: 16

                    Text { text: "Text Events"; color: "#ddd"; font.pixelSize: 16; font.bold: true }
                    Text { text: "Customize how IRC events are displayed:"; color: "#aaa"; font.pixelSize: 12 }
                    Rectangle {
                        Layout.fillWidth: true; Layout.fillHeight: true; color: "#1e1e1e"; border.color: "#404040"; radius: 3
                        ListView {
                            anchors.fill: parent; anchors.margins: 4; clip: true
                            model: ListModel {
                                ListElement { event: "Channel Message"; format: "%C4%n%O  %1" }
                                ListElement { event: "Join"; format: "%C3***%O  %n (%h) has joined %c" }
                                ListElement { event: "Part"; format: "%C3***%O  %n has left %c (%2)" }
                                ListElement { event: "Quit"; format: "%C5***%O  %n has quit (%2)" }
                                ListElement { event: "Kick"; format: "%C5***%O  %n was kicked by %2 (%3)" }
                                ListElement { event: "Nick Change"; format: "%C3***%O  %n is now known as %2" }
                                ListElement { event: "Topic Change"; format: "%C3***%O  %n changed topic to: %2" }
                                ListElement { event: "Action"; format: "%C6*%O  %n %1" }
                                ListElement { event: "Notice"; format: "%C5-%n-%O  %1" }
                                ListElement { event: "CTCP"; format: "%C4***%O  Received CTCP %1 from %n" }
                            }
                            delegate: RowLayout {
                                required property string event
                                required property string format
                                width: parent ? parent.width : 0; spacing: 10; height: 24
                                Text { text: event; color: "#ccc"; font.pixelSize: 12; Layout.preferredWidth: 140 }
                                Text { text: format; color: "#6a9955"; font.family: "monospace"; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
                            }
                        }
                    }
                }
            }

            // 14 — Plugins
            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width; spacing: 8
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: 16

                    Text { text: "Plugins"; color: "#ddd"; font.pixelSize: 16; font.bold: true }
                    Text { text: "Loaded plugins:"; color: "#aaa"; font.pixelSize: 12 }
                    Rectangle {
                        Layout.fillWidth: true; height: 160; color: "#1e1e1e"; border.color: "#404040"; radius: 3
                        ListView {
                            anchors.fill: parent; anchors.margins: 4; clip: true
                            model: ListModel {
                                ListElement { plugin: "ExamplePlugin"; version: "1.0"; desc: "Example NUchat plugin" }
                            }
                            delegate: RowLayout {
                                required property string plugin
                                required property string version
                                required property string desc
                                width: parent ? parent.width : 0; height: 24; spacing: 8
                                Text { text: plugin; color: "#569cd6"; font.pixelSize: 12; font.bold: true; Layout.preferredWidth: 140 }
                                Text { text: "v" + version; color: "#888"; font.pixelSize: 11; Layout.preferredWidth: 40 }
                                Text { text: desc; color: "#ccc"; font.pixelSize: 12; Layout.fillWidth: true; elide: Text.ElideRight }
                            }
                        }
                    }
                    RowLayout {
                        spacing: 8
                        Button {
                            text: "Load Plugin..."
                            background: Rectangle { color: parent.down ? "#1177bb" : "#0e639c"; radius: 3 }
                            contentItem: Text { text: "Load Plugin..."; color: "#fff"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                        Button {
                            text: "Unload"
                            background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                            contentItem: Text { text: "Unload"; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                        Button {
                            text: "Reload"
                            background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                            contentItem: Text { text: "Reload"; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                    }
                    RowLayout {
                        spacing: 8
                        Text { text: "Plugin directory:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
                            id: txtPluginDir
                            Layout.fillWidth: true; text: "~/.config/NUchat/plugins"; color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                    }
                }
            }

            // 15 — Scripts
            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width; spacing: 8
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: 16

                    Text { text: "Scripts"; color: "#ddd"; font.pixelSize: 16; font.bold: true }
                    Text { text: "JavaScript scripts loaded by the scripting engine:"; color: "#aaa"; font.pixelSize: 12 }
                    Rectangle {
                        Layout.fillWidth: true; height: 140; color: "#1e1e1e"; border.color: "#404040"; radius: 3
                        Text { anchors.centerIn: parent; text: "No scripts loaded"; color: "#666"; font.pixelSize: 12 }
                    }
                    RowLayout {
                        spacing: 8
                        Button {
                            text: "Load Script..."
                            background: Rectangle { color: parent.down ? "#1177bb" : "#0e639c"; radius: 3 }
                            contentItem: Text { text: "Load Script..."; color: "#fff"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                        Button {
                            text: "Unload"
                            background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                            contentItem: Text { text: "Unload"; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                        Button {
                            text: "Open Script Editor"
                            background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                            contentItem: Text { text: "Open Script Editor"; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                    }
                    RowLayout {
                        spacing: 8
                        Text { text: "Script directory:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
                            id: txtScriptDir
                            Layout.fillWidth: true; text: "~/.config/NUchat/scripts"; color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                    }
                }
            }

            // 16 — Advanced
            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width; spacing: 8
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: 16

                    Text { text: "Advanced"; color: "#ddd"; font.pixelSize: 16; font.bold: true }
                    RowLayout {
                        spacing: 8
                        Text { text: "Encoding:"; color: "#ccc"; font.pixelSize: 12 }
                        ComboBox {
                            id: cboEncoding
                            model: ["UTF-8", "ISO-8859-1 (Latin-1)", "ISO-8859-15", "Windows-1252",
                                    "KOI8-R", "Shift_JIS", "EUC-JP", "GB2312", "Big5"]
                            Layout.preferredWidth: 220
                            onCurrentIndexChanged: saveSetting("adv/encodingIndex", currentIndex)
                        }
                    }
                    CheckBox { id: chkIdentifyFirst; text: "Identify to services before joining channels"
                        onCheckedChanged: saveSetting("adv/identifyFirst", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkIPv6; text: "Enable IPv6"
                        onCheckedChanged: saveSetting("adv/ipv6", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { id: chkCAP; text: "Enable CAP negotiation"
                        onCheckedChanged: saveSetting("adv/cap", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    Text { text: "Perform on connect:"; color: "#ccc"; font.pixelSize: 12 }
                    Rectangle {
                        Layout.fillWidth: true; height: 80; color: "#333"; border.color: "#555"; radius: 2
                        TextEdit {
                            id: txtPerformCmds
                            anchors.fill: parent; anchors.margins: 6
                            color: "#ddd"; font.family: "monospace"; font.pixelSize: 12
                            wrapMode: Text.Wrap
                        }
                    }
                    CheckBox { id: chkShowRawIRC; text: "Show raw IRC in server tab"
                        onCheckedChanged: saveSetting("adv/showRaw", checked)
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    RowLayout {
                        spacing: 8
                        Text { text: "Command character:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
                            id: txtCmdChar
                            text: "/"; Layout.preferredWidth: 40; color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                    }
                }
            }

            // 17 — Shortcuts
            ScrollView {
                clip: true
                ColumnLayout {
                    width: parent.width; spacing: 8
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                    anchors.margins: 16

                    Text { text: "Keyboard Shortcuts"; color: "#ddd"; font.pixelSize: 16; font.bold: true }
                    Text { text: "Customize key bindings. Clear a field to disable a shortcut.\nUse Qt key sequence format (e.g. Ctrl+Shift+K)."; color: "#aaa"; font.pixelSize: 12; wrapMode: Text.Wrap; Layout.fillWidth: true }

                    component ShortcutRow: RowLayout {
                        property string label
                        property string settingKey
                        property string defaultSeq
                        spacing: 8
                        Text { text: label; color: "#ccc"; font.pixelSize: 12; Layout.preferredWidth: 150 }
                        TextField {
                            id: seqField
                            Layout.fillWidth: true; color: "#ddd"; font.pixelSize: 12; placeholderText: defaultSeq; placeholderTextColor: "#666"
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                            text: appSettings.value(settingKey, defaultSeq) || defaultSeq
                        }
                        Button {
                            text: "Reset"
                            onClicked: seqField.text = defaultSeq
                            background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                            contentItem: Text { text: "Reset"; color: "#ccc"; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                        Connections {
                            target: seqField
                            function onTextEdited() { appSettings.setValue(settingKey, seqField.text); appSettings.sync() }
                        }
                    }

                    ShortcutRow { label: "Close Tab";      settingKey: "shortcut/closeTab";     defaultSeq: "Ctrl+W" }
                    ShortcutRow { label: "Previous Tab";    settingKey: "shortcut/prevTab";      defaultSeq: "Ctrl+PgUp" }
                    ShortcutRow { label: "Next Tab";        settingKey: "shortcut/nextTab";      defaultSeq: "Ctrl+PgDown" }
                    ShortcutRow { label: "Join Channel";    settingKey: "shortcut/joinChannel";  defaultSeq: "Ctrl+J" }
                    ShortcutRow { label: "Quick Connect";   settingKey: "shortcut/quickConnect"; defaultSeq: "Ctrl+N" }
                    ShortcutRow { label: "Disconnect";      settingKey: "shortcut/disconnect";   defaultSeq: "Ctrl+D" }
                    ShortcutRow { label: "Channel List";    settingKey: "shortcut/channelList";  defaultSeq: "Ctrl+L" }
                    ShortcutRow { label: "Raw Log";         settingKey: "shortcut/rawLog";       defaultSeq: "Ctrl+R" }
                    ShortcutRow { label: "Search";          settingKey: "shortcut/search";       defaultSeq: "Ctrl+F" }
                    ShortcutRow { label: "Preferences";     settingKey: "shortcut/preferences";  defaultSeq: "Ctrl+P" }
                    ShortcutRow { label: "Network List";    settingKey: "shortcut/networkList";  defaultSeq: "Ctrl+Shift+N" }
                    ShortcutRow { label: "Change Nick";     settingKey: "shortcut/nickChange";   defaultSeq: "Ctrl+K" }
                    ShortcutRow { label: "Toggle Away";     settingKey: "shortcut/away";         defaultSeq: "Ctrl+Shift+A" }
                    ShortcutRow { label: "Scripts";         settingKey: "shortcut/scripts";      defaultSeq: "Ctrl+Shift+S" }
                    ShortcutRow { label: "URL Grabber";     settingKey: "shortcut/urlGrabber";   defaultSeq: "Ctrl+U" }
                }
            }
        }
    }

    footer: Rectangle {
        height: 44; color: "#252526"
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12; spacing: 8
            Item { Layout.fillWidth: true }
            Button {
                text: "OK"
                onClicked: { dlg.saveSettings(); dlg.close() }
                background: Rectangle { color: parent.down ? "#1177bb" : "#0e639c"; radius: 3 }
                contentItem: Text { text: "OK"; color: "#fff"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Cancel"
                onClicked: dlg.close()
                background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                contentItem: Text { text: "Cancel"; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }
    }
}
