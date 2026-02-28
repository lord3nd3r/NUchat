import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: dlg
    title: "Ignore List"
    width: 480
    height: 400
    modal: true
    anchors.centerIn: parent

    background: Rectangle { color: "#2b2b2b"; border.color: "#555"; border.width: 1; radius: 6 }
    header: Rectangle {
        height: 36; color: "#252526"; radius: 6
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 6; color: "#252526" }
        Text { anchors.centerIn: parent; text: "Ignore List"; color: "#ddd"; font.pixelSize: 14; font.bold: true }
    }

    onOpened: refreshList()

    function refreshList() {
        ignoreModel.clear()
        var masks = ircManager.ignoreList()
        for (var i = 0; i < masks.length; i++) {
            ignoreModel.append({mask: masks[i]})
        }
    }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 12; spacing: 8

        Text {
            text: "Ignored nicks/masks — messages from matching users will be hidden."
            color: "#999"; font.pixelSize: 11; wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        // Add new ignore entry
        RowLayout {
            Layout.fillWidth: true; spacing: 6
            TextField {
                id: newIgnoreField
                Layout.fillWidth: true
                placeholderText: "nick or nick!user@host mask"
                color: "#ddd"; font.pixelSize: 12
                background: Rectangle { color: "#1e1e1e"; border.color: "#444"; border.width: 1; radius: 3 }
                onAccepted: addBtn.clicked()
            }
            Button {
                id: addBtn
                text: "Add"
                enabled: newIgnoreField.text.trim() !== ""
                onClicked: {
                    var mask = newIgnoreField.text.trim()
                    if (mask !== "") {
                        ircManager.addIgnore(mask)
                        newIgnoreField.text = ""
                        refreshList()
                    }
                }
                background: Rectangle { color: parent.enabled ? (parent.down ? "#1177bb" : "#0e639c") : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: parent.enabled ? "#fff" : "#777"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }

        Rectangle {
            Layout.fillWidth: true; height: 24; color: "#252526"
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                Text { text: "Mask"; color: "#aaa"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true }
            }
        }

        ListView {
            id: ignoreListView
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true; boundsBehavior: Flickable.StopAtBounds
            model: ListModel { id: ignoreModel }
            delegate: Rectangle {
                required property int index
                required property string mask
                width: ignoreListView.width; height: 28
                color: ignoreListView.currentIndex === index ? "#264f78" : (index % 2 === 0 ? "#2b2b2b" : "#2e2e2e")
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                    Text { text: mask; color: "#ccc"; font.pixelSize: 12; Layout.fillWidth: true; elide: Text.ElideRight }
                    Button {
                        text: "✕"
                        implicitWidth: 24; implicitHeight: 22
                        onClicked: {
                            ircManager.removeIgnore(mask)
                            dlg.refreshList()
                        }
                        background: Rectangle { color: parent.hovered ? "#b33" : "transparent"; radius: 3 }
                        contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    }
                }
                MouseArea {
                    anchors.fill: parent; z: -1
                    onClicked: ignoreListView.currentIndex = index
                }
            }

            Text {
                anchors.centerIn: parent
                visible: ignoreModel.count === 0
                text: "No ignored users"
                color: "#666"
                font.pixelSize: 13
            }
        }

        RowLayout {
            Layout.fillWidth: true; spacing: 8
            Text { text: ignoreModel.count + " ignored"; color: "#888"; font.pixelSize: 11 }
            Item { Layout.fillWidth: true }
            Button {
                text: "Clear All"
                enabled: ignoreModel.count > 0
                onClicked: { ircManager.clearIgnoreList(); refreshList() }
                background: Rectangle { color: parent.enabled ? (parent.down ? "#555" : "#444") : "#333"; radius: 3 }
                contentItem: Text { text: parent.text; color: parent.enabled ? "#ccc" : "#666"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Close"; onClicked: dlg.close()
                background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }
    }
}
