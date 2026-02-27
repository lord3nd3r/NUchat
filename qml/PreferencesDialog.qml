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

    // Load saved user info when dialog opens
    onOpened: {
        prefNick.text = appSettings.value("user/nickname", "NUchat_user")
        prefNick2.text = appSettings.value("user/nickname2", "NUchat_user2")
        prefNick3.text = appSettings.value("user/nickname3", "NUchat_user3")
        prefUsername.text = appSettings.value("user/username", "nuchat")
        prefRealname.text = appSettings.value("user/realname", "NUchat User")
        prefQuitMsg.text = appSettings.value("user/quitMessage", "Leaving (NUchat)")
        prefPartMsg.text = appSettings.value("user/partMessage", "")
        prefAwayMsg.text = appSettings.value("user/awayMessage", "I'm away")
    }

    // Save user info
    function saveSettings() {
        appSettings.setValue("user/nickname", prefNick.text)
        appSettings.setValue("user/nickname2", prefNick2.text)
        appSettings.setValue("user/nickname3", prefNick3.text)
        appSettings.setValue("user/username", prefUsername.text)
        appSettings.setValue("user/realname", prefRealname.text)
        appSettings.setValue("user/quitMessage", prefQuitMsg.text)
        appSettings.setValue("user/partMessage", prefPartMsg.text)
        appSettings.setValue("user/awayMessage", prefAwayMsg.text)
        appSettings.sync()
    }

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
                    CheckBox { text: "Show server tree"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Show user list in channels"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Show topic bar"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Show status bar"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Show mode buttons"
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Open new tabs in background"
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Confirm on close when connected"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Minimize to system tray"
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
                    CheckBox { text: "Strip mIRC colors from messages"
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Show nick colors"; checked: true
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
                            model: ["Monospace", "DejaVu Sans Mono", "Liberation Mono", "Courier New", "Consolas", "Fira Code"]
                            Layout.preferredWidth: 200
                        }
                    }
                    RowLayout {
                        spacing: 8
                        Text { text: "Font size:"; color: "#ccc"; font.pixelSize: 12 }
                        SpinBox { from: 8; to: 24; value: 12; Layout.preferredWidth: 100 }
                    }
                    CheckBox { text: "Show timestamps"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    RowLayout {
                        spacing: 8
                        Text { text: "Timestamp format:"; color: "#ccc"; font.pixelSize: 12 }
                        ComboBox { model: ["[HH:mm:ss]", "[HH:mm]", "[h:mm AP]", "HH:mm:ss"]; Layout.preferredWidth: 160 }
                    }
                    RowLayout {
                        spacing: 8
                        Text { text: "Max scrollback lines:"; color: "#ccc"; font.pixelSize: 12 }
                        SpinBox { from: 100; to: 100000; value: 10000; stepSize: 500; Layout.preferredWidth: 120 }
                    }
                    CheckBox { text: "Indent wrapped text"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Enable text highlighting"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    RowLayout {
                        spacing: 8
                        Text { text: "Extra highlight words:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
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
                    CheckBox { text: "Enable spell check"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Nick completion with Tab"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    RowLayout {
                        spacing: 8
                        Text { text: "Nick completion suffix:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
                            text: ":"; Layout.preferredWidth: 60; color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                    }
                    CheckBox { text: "Show nick completion menu"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Input history per channel"
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    RowLayout {
                        spacing: 8
                        Text { text: "Max input history:"; color: "#ccc"; font.pixelSize: 12 }
                        SpinBox { from: 10; to: 1000; value: 100; Layout.preferredWidth: 100 }
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
                    CheckBox { text: "Auto-reconnect on disconnect"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    RowLayout {
                        spacing: 8
                        Text { text: "Reconnect delay (seconds):"; color: "#ccc"; font.pixelSize: 12 }
                        SpinBox { from: 1; to: 300; value: 10; Layout.preferredWidth: 100 }
                    }
                    RowLayout {
                        spacing: 8
                        Text { text: "Max reconnect attempts:"; color: "#ccc"; font.pixelSize: 12 }
                        SpinBox { from: 0; to: 100; value: 10; Layout.preferredWidth: 100 }
                    }
                    CheckBox { text: "Auto-join channels on connect"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Use global user info for all servers"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    Text { text: "Proxy:"; color: "#ccc"; font.pixelSize: 13; font.bold: true }
                    RowLayout {
                        spacing: 8
                        Text { text: "Type:"; color: "#ccc"; font.pixelSize: 12 }
                        ComboBox { model: ["None", "SOCKS4", "SOCKS5", "HTTP CONNECT"]; Layout.preferredWidth: 160 }
                    }
                    RowLayout {
                        spacing: 8
                        Text { text: "Host:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
                            Layout.fillWidth: true; placeholderText: "proxy.example.com"; placeholderTextColor: "#666"
                            color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                        Text { text: "Port:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
                            Layout.preferredWidth: 60; placeholderText: "1080"; placeholderTextColor: "#666"
                            color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                    }
                    CheckBox { text: "Proxy requires authentication"
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
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
                    Text { text: "Default authentication method for new networks:"; color: "#aaa"; font.pixelSize: 12 }
                    RowLayout {
                        spacing: 8
                        Text { text: "Method:"; color: "#ccc"; font.pixelSize: 12 }
                        ComboBox {
                            model: ["None", "SASL PLAIN", "SASL EXTERNAL (cert)", "SASL SCRAM-SHA-256",
                                    "SASL ECDSA-NIST256P", "NickServ (/msg)", "NickServ (/nickserv)",
                                    "Server Password", "CERTFP (client cert)"]
                            Layout.preferredWidth: 260
                        }
                    }
                    Text { text: "NickServ:"; color: "#ccc"; font.pixelSize: 13; font.bold: true }
                    GridLayout {
                        columns: 2; columnSpacing: 10; rowSpacing: 6; Layout.fillWidth: true
                        Text { text: "NickServ command:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
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
                    CheckBox { text: "Accept invalid SSL certificates"
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
                            Layout.fillWidth: true; text: "~/Downloads"; color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                        Button {
                            text: "Browse"
                            background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                            contentItem: Text { text: "Browse"; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                    }
                    CheckBox { text: "Auto-accept DCC sends"
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    RowLayout {
                        spacing: 8
                        Text { text: "Max file size (MB, 0=unlimited):"; color: "#ccc"; font.pixelSize: 12 }
                        SpinBox { from: 0; to: 10000; value: 0; Layout.preferredWidth: 120 }
                    }
                    Text { text: "DCC IP:"; color: "#ccc"; font.pixelSize: 13; font.bold: true }
                    RowLayout {
                        spacing: 8
                        Text { text: "IP method:"; color: "#ccc"; font.pixelSize: 12 }
                        ComboBox { model: ["Auto-detect", "Get from server (DCC)", "Specify manually"]; Layout.preferredWidth: 200 }
                    }
                    RowLayout {
                        spacing: 8
                        Text { text: "Manual IP:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
                            Layout.fillWidth: true; placeholderText: "0.0.0.0"; placeholderTextColor: "#666"
                            color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                    }
                    RowLayout {
                        spacing: 8
                        Text { text: "Port range:"; color: "#ccc"; font.pixelSize: 12 }
                        SpinBox { from: 1024; to: 65535; value: 1024; Layout.preferredWidth: 100 }
                        Text { text: "to"; color: "#ccc"; font.pixelSize: 12 }
                        SpinBox { from: 1024; to: 65535; value: 5000; Layout.preferredWidth: 100 }
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
                    CheckBox { text: "Enable chat logging"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    RowLayout {
                        spacing: 8
                        Text { text: "Log directory:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
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
                        ComboBox { model: ["Plain text", "HTML", "JSON"]; Layout.preferredWidth: 160 }
                    }
                    CheckBox { text: "Log timestamps"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Log private messages"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Create separate log per channel"; checked: true
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
                    CheckBox { text: "Desktop notifications on highlight"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Desktop notifications on private message"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Flash taskbar on activity"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Show unread count in tray icon"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Highlight on nick mention"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    RowLayout {
                        spacing: 8
                        Text { text: "Notification timeout (ms):"; color: "#ccc"; font.pixelSize: 12 }
                        SpinBox { from: 0; to: 30000; value: 5000; stepSize: 500; Layout.preferredWidth: 120 }
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
                    CheckBox { text: "Enable sounds"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Play sound on highlight"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Play sound on private message"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Play sound on connection"
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    RowLayout {
                        spacing: 8
                        Text { text: "Beep command:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
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
                    CheckBox { text: "Make URLs clickable"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Auto-grab URLs to URL Grabber"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    RowLayout {
                        spacing: 8
                        Text { text: "Browser command:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
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
                            model: ["UTF-8", "ISO-8859-1 (Latin-1)", "ISO-8859-15", "Windows-1252",
                                    "KOI8-R", "Shift_JIS", "EUC-JP", "GB2312", "Big5"]
                            Layout.preferredWidth: 220
                        }
                    }
                    CheckBox { text: "Identify to services before joining channels"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Enable IPv6"
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    CheckBox { text: "Enable CAP negotiation"; checked: true
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    Text { text: "Perform on connect:"; color: "#ccc"; font.pixelSize: 12 }
                    Rectangle {
                        Layout.fillWidth: true; height: 80; color: "#333"; border.color: "#555"; radius: 2
                        TextEdit {
                            anchors.fill: parent; anchors.margins: 6
                            color: "#ddd"; font.family: "monospace"; font.pixelSize: 12
                            text: "/mode %n +x\n/msg NickServ IDENTIFY password"
                            wrapMode: Text.Wrap
                        }
                    }
                    CheckBox { text: "Show raw IRC in server tab"
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                    RowLayout {
                        spacing: 8
                        Text { text: "Command character:"; color: "#ccc"; font.pixelSize: 12 }
                        TextField {
                            text: "/"; Layout.preferredWidth: 40; color: "#ddd"; font.pixelSize: 12
                            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                        }
                    }
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
