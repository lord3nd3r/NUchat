import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: dlg
    title: "Join Channel"
    width: 380
    height: 200
    modal: true
    anchors.centerIn: parent

    background: Rectangle { color: "#2b2b2b"; border.color: "#555"; border.width: 1; radius: 6 }
    header: Rectangle {
        height: 36; color: "#252526"; radius: 6
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 6; color: "#252526" }
        Text { anchors.centerIn: parent; text: "Join Channel"; color: "#ddd"; font.pixelSize: 14; font.bold: true }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        GridLayout {
            columns: 2; columnSpacing: 10; rowSpacing: 8; Layout.fillWidth: true

            Text { text: "Channel:"; color: "#ccc"; font.pixelSize: 12 }
            TextField {
                id: joinChan; Layout.fillWidth: true; text: "#"
                color: "#ddd"; font.pixelSize: 12; placeholderText: "#channel"; placeholderTextColor: "#666"
                background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                onAccepted: joinBtn.clicked()
            }
            Text { text: "Key:"; color: "#ccc"; font.pixelSize: 12 }
            TextField {
                id: joinKey; Layout.fillWidth: true; echoMode: TextInput.Password
                color: "#ddd"; font.pixelSize: 12; placeholderText: "(optional)"; placeholderTextColor: "#666"
                background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
            }
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true; spacing: 8
            Item { Layout.fillWidth: true }
            Button {
                id: joinBtn; text: "Join"
                onClicked: {
                    if (joinChan.text !== "" && joinChan.text !== "#") {
                        ircManager.joinChannel(joinChan.text, joinKey.text)
                        dlg.close()
                    }
                }
                background: Rectangle { color: parent.down ? "#28a745" : "#218838"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#fff"; font.pixelSize: 12; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Cancel"; onClicked: dlg.close()
                background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }
    }
}
