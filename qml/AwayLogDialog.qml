import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: dlg
    title: "Away Log"
    width: 600
    height: 420
    modal: true
    anchors.centerIn: parent

    background: Rectangle { color: "#2b2b2b"; border.color: "#555"; border.width: 1; radius: 6 }
    header: Rectangle {
        height: 36; color: "#252526"; radius: 6
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 6; color: "#252526" }
        Text { anchors.centerIn: parent; text: "Away Log"; color: "#ddd"; font.pixelSize: 14; font.bold: true }
    }

    onOpened: refreshLog()

    function refreshLog() {
        awayModel.clear()
        var entries = ircManager.awayLog()
        for (var i = 0; i < entries.length; i++) {
            awayModel.append({
                timestamp: entries[i].timestamp,
                nick: entries[i].nick,
                channel: entries[i].channel,
                message: entries[i].message
            })
        }
    }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 12; spacing: 8

        Text {
            text: ircManager.isAway ? "You are currently away" : "You are not away"
            color: ircManager.isAway ? "#d19a66" : "#98c379"
            font.pixelSize: 12
        }

        Rectangle {
            Layout.fillWidth: true; height: 24; color: "#252526"
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                Text { text: "Time"; color: "#aaa"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 70 }
                Text { text: "Nick"; color: "#aaa"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 100 }
                Text { text: "Channel"; color: "#aaa"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 120 }
                Text { text: "Message"; color: "#aaa"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true }
            }
        }

        ListView {
            id: awayList
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true; boundsBehavior: Flickable.StopAtBounds
            model: ListModel { id: awayModel }
            delegate: Rectangle {
                required property int index
                required property string timestamp
                required property string nick
                required property string channel
                required property string message
                width: awayList.width; height: 24
                color: index % 2 === 0 ? "#2b2b2b" : "#2e2e2e"
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                    Text { text: timestamp; color: "#888"; font.pixelSize: 12; Layout.preferredWidth: 70 }
                    Text { text: nick; color: "#61afef"; font.pixelSize: 12; Layout.preferredWidth: 100; elide: Text.ElideRight }
                    Text { text: channel; color: "#6a9955"; font.pixelSize: 12; Layout.preferredWidth: 120; elide: Text.ElideRight }
                    Text { text: message; color: "#ccc"; font.pixelSize: 12; Layout.fillWidth: true; elide: Text.ElideRight }
                }
            }

            Text {
                anchors.centerIn: parent
                visible: awayModel.count === 0
                text: "No messages received while away"
                color: "#666"
                font.pixelSize: 13
            }
        }

        RowLayout {
            Layout.fillWidth: true; spacing: 8
            Text { text: awayModel.count + " message(s)"; color: "#888"; font.pixelSize: 11 }
            Item { Layout.fillWidth: true }
            Button {
                text: "Clear"
                onClicked: { ircManager.clearAwayLog(); awayModel.clear() }
                background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Close"; onClicked: dlg.close()
                background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }
    }
}
