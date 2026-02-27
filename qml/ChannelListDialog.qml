import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: dlg
    title: "Channel List"
    width: 560
    height: 420
    modal: true
    anchors.centerIn: parent

    background: Rectangle { color: "#2b2b2b"; border.color: "#555"; border.width: 1; radius: 6 }
    header: Rectangle {
        height: 36; color: "#252526"; radius: 6
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 6; color: "#252526" }
        Text { anchors.centerIn: parent; text: "Channel List"; color: "#ddd"; font.pixelSize: 14; font.bold: true }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            Layout.fillWidth: true; spacing: 8
            TextField {
                id: filterField; Layout.fillWidth: true
                placeholderText: "Search channels..."; placeholderTextColor: "#666"
                color: "#ddd"; font.pixelSize: 12
                background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
            }
            Text { text: "Min users:"; color: "#ccc"; font.pixelSize: 12 }
            SpinBox {
                id: minUsers; from: 0; to: 9999; value: 2
                editable: true; Layout.preferredWidth: 100
            }
            Button {
                text: "Refresh"
                onClicked: {
                    chanListModel.clear()
                    ircManager.sendRawCommand("LIST")
                }
                background: Rectangle { color: parent.down ? "#1177bb" : "#0e639c"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#fff"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }

        // Column headers
        Rectangle {
            Layout.fillWidth: true; height: 24; color: "#252526"
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                Text { text: "Channel"; color: "#aaa"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 200 }
                Text { text: "Users"; color: "#aaa"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 60 }
                Text { text: "Topic"; color: "#aaa"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true }
            }
        }

        ListView {
            id: chanListView
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true; boundsBehavior: Flickable.StopAtBounds

            model: ListModel {
                id: chanListModel
            }

            delegate: Rectangle {
                required property int index
                required property string channel
                required property int users
                required property string topic
                width: chanListView.width; height: 26
                color: chanListView.currentIndex === index ? "#264f78"
                     : clMouse.containsMouse ? "#333" : (index % 2 === 0 ? "#2b2b2b" : "#2e2e2e")

                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                    Text { text: channel; color: "#569cd6"; font.pixelSize: 12; Layout.preferredWidth: 200 }
                    Text { text: users.toString(); color: "#ccc"; font.pixelSize: 12; Layout.preferredWidth: 60; horizontalAlignment: Text.AlignRight }
                    Text { text: topic; color: "#aaa"; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
                }
                MouseArea {
                    id: clMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: chanListView.currentIndex = index
                    onDoubleClicked: {
                        var ch = chanListModel.get(index).channel
                        ircManager.joinChannel(ch, "")
                        dlg.close()
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true; spacing: 8
            Text { text: chanListModel.count + " channels shown"; color: "#888"; font.pixelSize: 11 }
            Item { Layout.fillWidth: true }
            Button {
                text: "Join Selected"
                enabled: chanListView.currentIndex >= 0
                onClicked: {
                    var ch = chanListModel.get(chanListView.currentIndex).channel
                    ircManager.joinChannel(ch, "")
                    dlg.close()
                }
                background: Rectangle { color: parent.enabled ? (parent.down ? "#28a745" : "#218838") : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: parent.enabled ? "#fff" : "#777"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Close"; onClicked: dlg.close()
                background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }
    }
}
