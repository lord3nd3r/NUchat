import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: dlg
    title: "Change Nickname"
    width: 340
    height: 160
    modal: true
    anchors.centerIn: parent

    background: Rectangle { color: "#2b2b2b"; border.color: "#555"; border.width: 1; radius: 6 }
    header: Rectangle {
        height: 36; color: "#252526"; radius: 6
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 6; color: "#252526" }
        Text { anchors.centerIn: parent; text: "Change Nickname"; color: "#ddd"; font.pixelSize: 14; font.bold: true }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        RowLayout {
            spacing: 10; Layout.fillWidth: true
            Text { text: "New nick:"; color: "#ccc"; font.pixelSize: 12 }
            TextField {
                id: nickField; Layout.fillWidth: true; text: "NUchat_user"
                color: "#ddd"; font.pixelSize: 12
                background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                onAccepted: okBtn.clicked()
            }
        }
        Item { Layout.fillHeight: true }
        RowLayout {
            Layout.fillWidth: true; spacing: 8
            Item { Layout.fillWidth: true }
            Button {
                id: okBtn; text: "OK"
                onClicked: {
                    if (nickField.text !== "") {
                        ircManager.changeNick(nickField.text)
                        dlg.close()
                    }
                }
                background: Rectangle { color: parent.down ? "#1177bb" : "#0e639c"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#fff"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Cancel"; onClicked: dlg.close()
                background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }
    }
}
