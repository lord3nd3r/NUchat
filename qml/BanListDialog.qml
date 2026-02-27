import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: dlg
    title: "Ban List"
    width: 520
    height: 380
    modal: true
    anchors.centerIn: parent

    background: Rectangle { color: "#2b2b2b"; border.color: "#555"; border.width: 1; radius: 6 }
    header: Rectangle {
        height: 36; color: "#252526"; radius: 6
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 6; color: "#252526" }
        Text { anchors.centerIn: parent; text: "Ban List — " + root.currentChannel; color: "#ddd"; font.pixelSize: 14; font.bold: true }
    }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 12; spacing: 8

        // Column headers
        Rectangle {
            Layout.fillWidth: true; height: 24; color: "#252526"
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                Text { text: "Mask"; color: "#aaa"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true }
                Text { text: "Set By"; color: "#aaa"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 120 }
                Text { text: "Date"; color: "#aaa"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 120 }
            }
        }

        ListView {
            id: banListView
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true; boundsBehavior: Flickable.StopAtBounds
            model: ListModel {
                id: banModel
            }
            delegate: Rectangle {
                required property int index
                required property string mask
                required property string setBy
                required property string date
                width: banListView.width; height: 24
                color: banListView.currentIndex === index ? "#264f78" : (index % 2 === 0 ? "#2b2b2b" : "#2e2e2e")
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                    Text { text: mask; color: "#f44747"; font.pixelSize: 12; font.family: "monospace"; Layout.fillWidth: true }
                    Text { text: setBy; color: "#ccc"; font.pixelSize: 12; Layout.preferredWidth: 120 }
                    Text { text: date; color: "#888"; font.pixelSize: 11; Layout.preferredWidth: 120 }
                }
                MouseArea { anchors.fill: parent; onClicked: banListView.currentIndex = index }
            }
        }

        RowLayout {
            Layout.fillWidth: true; spacing: 8
            TextField {
                id: banMask; Layout.fillWidth: true
                placeholderText: "*!*@hostname"; placeholderTextColor: "#666"
                color: "#ddd"; font.pixelSize: 12; font.family: "monospace"
                background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
            }
            Button {
                text: "Add Ban"
                onClicked: {
                    if (banMask.text !== "" && root.currentChannel !== "") {
                        ircManager.sendRawCommand("MODE " + root.currentChannel + " +b " + banMask.text)
                        banModel.append({mask: banMask.text, setBy: "you", date: Qt.formatDateTime(new Date(), "yyyy-MM-dd hh:mm")})
                        banMask.text = ""
                    }
                }
                background: Rectangle { color: parent.down ? "#a02020" : "#802020"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#fff"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Remove"
                enabled: banListView.currentIndex >= 0
                onClicked: {
                    if (banListView.currentIndex >= 0 && root.currentChannel !== "") {
                        var m = banModel.get(banListView.currentIndex).mask
                        ircManager.sendRawCommand("MODE " + root.currentChannel + " -b " + m)
                        banModel.remove(banListView.currentIndex)
                    }
                }
                background: Rectangle { color: parent.enabled ? (parent.down ? "#555" : "#444") : "#333"; radius: 3 }
                contentItem: Text { text: parent.text; color: parent.enabled ? "#ccc" : "#666"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }

        RowLayout {
            Layout.fillWidth: true; spacing: 8
            Text { text: banModel.count + " bans"; color: "#888"; font.pixelSize: 11 }
            Item { Layout.fillWidth: true }
            Button {
                text: "Refresh"
                onClicked: {
                    if (root.currentChannel !== "") {
                        banModel.clear()
                        ircManager.sendRawCommand("MODE " + root.currentChannel + " +b")
                    }
                }
                background: Rectangle { color: parent.down ? "#1177bb" : "#0e639c"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#fff"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Close"; onClicked: dlg.close()
                background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }
    }
}
