import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: dlg
    title: "DCC Transfers"
    width: 560
    height: 380
    modal: true
    anchors.centerIn: parent

    background: Rectangle { color: "#2b2b2b"; border.color: "#555"; border.width: 1; radius: 6 }
    header: Rectangle {
        height: 36; color: "#252526"; radius: 6
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 6; color: "#252526" }
        Text { anchors.centerIn: parent; text: "DCC Transfers"; color: "#ddd"; font.pixelSize: 14; font.bold: true }
    }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 12; spacing: 8

        // Column headers
        Rectangle {
            Layout.fillWidth: true; height: 24; color: "#252526"
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                Text { text: "File"; color: "#aaa"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true }
                Text { text: "Size"; color: "#aaa"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 80 }
                Text { text: "Status"; color: "#aaa"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 80 }
                Text { text: "Speed"; color: "#aaa"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 80 }
                Text { text: "ETA"; color: "#aaa"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 60 }
            }
        }

        ListView {
            id: dccList
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true; boundsBehavior: Flickable.StopAtBounds
            model: ListModel { id: dccModel }

            delegate: Rectangle {
                required property int index
                required property string fileName
                required property string size
                required property string status
                required property string speed
                required property string eta
                width: dccList.width; height: 26
                color: dccList.currentIndex === index ? "#264f78" : (index % 2 === 0 ? "#2b2b2b" : "#2e2e2e")
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                    Text { text: fileName; color: "#ddd"; font.pixelSize: 12; Layout.fillWidth: true }
                    Text { text: size; color: "#ccc"; font.pixelSize: 12; Layout.preferredWidth: 80 }
                    Text {
                        text: status; font.pixelSize: 12; Layout.preferredWidth: 80
                        color: status === "Complete" ? "#6a9955" : (status === "Failed" ? "#f44747" : "#569cd6")
                    }
                    Text { text: speed; color: "#888"; font.pixelSize: 12; Layout.preferredWidth: 80 }
                    Text { text: eta; color: "#888"; font.pixelSize: 12; Layout.preferredWidth: 60 }
                }
                MouseArea { anchors.fill: parent; onClicked: dccList.currentIndex = index }
            }

            // empty state
            Text {
                visible: dccModel.count === 0
                anchors.centerIn: parent
                text: "No active transfers"
                color: "#666"; font.pixelSize: 13
            }
        }

        RowLayout {
            Layout.fillWidth: true; spacing: 8
            Button {
                text: "Send File..."
                onClicked: msgModel.addMessage("system", "DCC: File send dialog would open here")
                background: Rectangle { color: parent.down ? "#1177bb" : "#0e639c"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#fff"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Accept"
                enabled: dccList.currentIndex >= 0
                background: Rectangle { color: parent.enabled ? (parent.down ? "#28a745" : "#218838") : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: parent.enabled ? "#fff" : "#777"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Cancel"
                enabled: dccList.currentIndex >= 0
                background: Rectangle { color: parent.enabled ? (parent.down ? "#a02020" : "#802020") : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: parent.enabled ? "#fff" : "#777"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Item { Layout.fillWidth: true }
            Button {
                text: "Clear Finished"
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
