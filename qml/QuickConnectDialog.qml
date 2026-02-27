import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: dlg
    title: "Quick Connect"
    width: 420
    height: 320
    modal: true
    anchors.centerIn: parent

    background: Rectangle { color: "#2b2b2b"; border.color: "#555"; border.width: 1; radius: 6 }

    onOpened: {
        qcServer.text = appSettings.value("quickconnect/server", "irc.libera.chat")
        qcPort.text = appSettings.value("quickconnect/port", "6697")
        qcNick.text = appSettings.value("user/nickname", "NUchat_user")
        qcChannel.text = appSettings.value("quickconnect/channel", "#nuchat")
    }

    header: Rectangle {
        height: 36; color: "#252526"; radius: 6
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 6; color: "#252526" }
        Text { anchors.centerIn: parent; text: "Quick Connect"; color: "#ddd"; font.pixelSize: 14; font.bold: true }
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 16
        columns: 2
        columnSpacing: 10
        rowSpacing: 8

        Text { text: "Server:"; color: "#ccc"; font.pixelSize: 12 }
        TextField {
            id: qcServer; Layout.fillWidth: true; text: "irc.libera.chat"
            color: "#ddd"; font.pixelSize: 12; placeholderText: "hostname"
            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
        }

        Text { text: "Port:"; color: "#ccc"; font.pixelSize: 12 }
        TextField {
            id: qcPort; Layout.preferredWidth: 80; text: "6697"
            color: "#ddd"; font.pixelSize: 12; inputMethodHints: Qt.ImhDigitsOnly
            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
        }

        Text { text: "Nickname:"; color: "#ccc"; font.pixelSize: 12 }
        TextField {
            id: qcNick; Layout.fillWidth: true; text: "NUchat_user"
            color: "#ddd"; font.pixelSize: 12
            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
        }

        Text { text: "Password:"; color: "#ccc"; font.pixelSize: 12 }
        TextField {
            id: qcPass; Layout.fillWidth: true; echoMode: TextInput.Password
            color: "#ddd"; font.pixelSize: 12; placeholderText: "(optional)"; placeholderTextColor: "#666"
            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
        }

        Text { text: "Channel:"; color: "#ccc"; font.pixelSize: 12 }
        TextField {
            id: qcChannel; Layout.fillWidth: true; text: "#nuchat"
            color: "#ddd"; font.pixelSize: 12; placeholderText: "#channel"
            background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
        }

        CheckBox {
            id: qcSsl; Layout.columnSpan: 2
            checked: true
            indicator: Rectangle {
                width: 16; height: 16; radius: 2; color: qcSsl.checked ? "#0e639c" : "#333"; border.color: "#555"
                Text { anchors.centerIn: parent; text: qcSsl.checked ? "✓" : ""; color: "#fff"; font.pixelSize: 11 }
            }
            contentItem: Text { text: " Use SSL/TLS"; color: "#ccc"; font.pixelSize: 12; leftPadding: qcSsl.indicator.width + 6 }
        }

        Item { Layout.fillHeight: true; Layout.columnSpan: 2 }

        RowLayout {
            Layout.columnSpan: 2
            Layout.fillWidth: true
            spacing: 8
            Item { Layout.fillWidth: true }
            Button {
                text: "Connect"
                onClicked: {
                    ircManager.connectToServer(
                        qcServer.text,
                        parseInt(qcPort.text),
                        qcSsl.checked,
                        qcNick.text,
                        appSettings.value("user/username", qcNick.text),
                        appSettings.value("user/realname", "NUchat User"),
                        qcPass.text
                    )
                    // Save last used quick connect settings
                    appSettings.setValue("quickconnect/server", qcServer.text)
                    appSettings.setValue("quickconnect/port", qcPort.text)
                    appSettings.setValue("quickconnect/channel", qcChannel.text)
                    appSettings.sync()
                    // Auto-join channel after connect if specified
                    if (qcChannel.text !== "" && qcChannel.text !== "#") {
                        root.pendingAutoJoin = qcChannel.text
                    }
                    root.refreshChannelList()
                    dlg.close()
                }
                background: Rectangle { color: parent.down ? "#28a745" : "#218838"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#fff"; font.pixelSize: 12; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Cancel"
                onClicked: dlg.close()
                background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }
    }
}
