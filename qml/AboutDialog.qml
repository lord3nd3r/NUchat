import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: dlg
    title: "About NUchat"
    width: 400
    height: 300
    modal: true
    anchors.centerIn: parent

    background: Rectangle { color: "#2b2b2b"; border.color: "#555"; border.width: 1; radius: 6 }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 20; spacing: 12

        Text {
            text: "NUchat"
            color: "#569cd6"; font.pixelSize: 28; font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }
        Text {
            text: "Version " + appVersion
            color: "#aaa"; font.pixelSize: 13
            Layout.alignment: Qt.AlignHCenter
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#404040" }

        Text {
            text: "A modern, cross-platform IRC client built with\nQt6 and QML."
            color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true; wrapMode: Text.Wrap
        }
        Text {
            text: "Features: SASL authentication, SSL/TLS,\nmultiple server support, DCC transfers,\nplugin system, scriptable via JavaScript."
            color: "#aaa"; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true; wrapMode: Text.Wrap
        }

        Item { Layout.fillHeight: true }

        Text {
            text: "© 2026 NUchat Contributors"
            color: "#888"; font.pixelSize: 11
            Layout.alignment: Qt.AlignHCenter
        }
        Text {
            text: "Licensed under the GNU General Public License v3"
            color: "#666"; font.pixelSize: 10
            Layout.alignment: Qt.AlignHCenter
        }

        Button {
            text: "Close"
            Layout.alignment: Qt.AlignHCenter
            onClicked: dlg.close()
            background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
            contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        }
    }
}
