import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: dlg
    title: "Raw Log"
    width: 640
    height: 440
    modal: true
    anchors.centerIn: parent

    background: Rectangle { color: "#1e1e1e"; border.color: "#555"; border.width: 1; radius: 6 }
    header: Rectangle {
        height: 36; color: "#252526"; radius: 6
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 6; color: "#252526" }
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12
            Text { text: "Raw IRC Log"; color: "#ddd"; font.pixelSize: 14; font.bold: true }
            Item { Layout.fillWidth: true }
            CheckBox {
                id: autoScroll; checked: true
                indicator: Rectangle { width: 14; height: 14; radius: 2; color: autoScroll.checked ? "#0e639c" : "#333"; border.color: "#555"; Text { anchors.centerIn: parent; text: autoScroll.checked ? "✓" : ""; color: "#fff"; font.pixelSize: 10 } }
                contentItem: Text { text: " Auto-scroll"; color: "#aaa"; font.pixelSize: 11; leftPadding: autoScroll.indicator.width + 4 }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 8; spacing: 6

        ScrollView {
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true

            ListView {
                id: rawLogList
                model: ListModel {
                    id: rawLogModel
                }
                boundsBehavior: Flickable.StopAtBounds
                delegate: Text {
                    required property string direction
                    required property string line
                    width: rawLogList.width
                    text: direction + " " + line
                    color: direction === ">>" ? "#569cd6" : "#6a9955"
                    font.family: "monospace"; font.pixelSize: 11
                    wrapMode: Text.Wrap
                    padding: 1
                }
            }
        }

        Connections {
            target: ircManager
            function onRawLineReceived(direction, line) {
                rawLogModel.append({direction: direction, line: line})
                if (autoScroll.checked) {
                    rawLogList.positionViewAtEnd()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true; spacing: 6
            TextField {
                id: rawInput; Layout.fillWidth: true
                placeholderText: "Send raw IRC command..."; placeholderTextColor: "#666"
                color: "#ddd"; font.family: "monospace"; font.pixelSize: 12
                background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                onAccepted: {
                    if (text !== "") {
                        ircManager.sendRawCommand(text)
                        text = ""
                    }
                }
            }
            Button {
                text: "Send"
                onClicked: rawInput.accepted()
                background: Rectangle { color: parent.down ? "#1177bb" : "#0e639c"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#fff"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Clear"
                onClicked: rawLogModel.clear()
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
