import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs

Dialog {
    id: dlg
    title: "DCC Transfers"
    width: 620
    height: 420
    modal: true
    anchors.centerIn: parent

    background: Rectangle { color: theme.dialogBg; border.color: theme.dialogBorder; border.width: 1; radius: 6 }
    header: Rectangle {
        height: 36; color: theme.dialogHeaderBg; radius: 6
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 6; color: theme.dialogHeaderBg }
        Text { anchors.centerIn: parent; text: "DCC Transfers"; color: theme.textPrimary; font.pixelSize: 14; font.bold: true }
    }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 12; spacing: 8

        // Column headers
        Rectangle {
            Layout.fillWidth: true; height: 24; color: theme.dialogHeaderBg
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                Text { text: "Dir"; color: theme.textMuted; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 36 }
                Text { text: "Nick"; color: theme.textMuted; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 80 }
                Text { text: "File"; color: theme.textMuted; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true }
                Text { text: "Size"; color: theme.textMuted; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 70 }
                Text { text: "Progress"; color: theme.textMuted; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 70 }
                Text { text: "Speed"; color: theme.textMuted; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 70 }
                Text { text: "ETA"; color: theme.textMuted; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 50 }
            }
        }

        ListView {
            id: dccList
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true; boundsBehavior: Flickable.StopAtBounds
            model: dccManager
            currentIndex: -1

            delegate: Rectangle {
                required property int index
                required property string transferId
                required property string nick
                required property string fileName
                required property string fileSize
                required property real progress
                required property string status
                required property string speed
                required property string eta
                required property string direction
                width: dccList.width; height: 30
                color: dccList.currentIndex === index ? theme.highlight : (index % 2 === 0 ? theme.dialogBg : theme.sidebarBg)

                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; spacing: 4

                    // Direction indicator
                    Text {
                        text: direction === "Send" ? "↑" : "↓"
                        color: direction === "Send" ? "#e5c07b" : "#61afef"
                        font.pixelSize: 13; font.bold: true
                        Layout.preferredWidth: 36
                    }

                    // Nick
                    Text { text: nick; color: theme.nickNormal; font.pixelSize: 12; Layout.preferredWidth: 80; elide: Text.ElideRight }

                    // Filename
                    Text { text: fileName; color: theme.textPrimary; font.pixelSize: 12; Layout.fillWidth: true; elide: Text.ElideMiddle }

                    // Size
                    Text { text: fileSize; color: theme.textSecondary; font.pixelSize: 12; Layout.preferredWidth: 70 }

                    // Progress bar + percentage
                    Item {
                        Layout.preferredWidth: 70; Layout.preferredHeight: 16
                        Rectangle {
                            anchors.fill: parent; color: theme.inputBg; radius: 3
                            Rectangle {
                                width: parent.width * progress; height: parent.height
                                color: status === "Complete" ? "#6a9955" : (status === "Failed" ? "#f44747" : "#569cd6")
                                radius: 3
                                Behavior on width { NumberAnimation { duration: 200 } }
                            }
                            Text {
                                anchors.centerIn: parent
                                text: status
                                color: "#fff"; font.pixelSize: 9; font.bold: true
                            }
                        }
                    }

                    // Speed
                    Text { text: speed; color: theme.textMuted; font.pixelSize: 11; Layout.preferredWidth: 70 }

                    // ETA
                    Text { text: eta; color: theme.textMuted; font.pixelSize: 11; Layout.preferredWidth: 50 }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: dccList.currentIndex = index
                }
            }

            // Empty state
            Text {
                visible: dccManager.count === 0
                anchors.centerIn: parent
                text: "No active transfers"
                color: theme.textMuted; font.pixelSize: 13
            }
        }

        // Action buttons
        RowLayout {
            Layout.fillWidth: true; spacing: 8

            Button {
                text: "Send File..."
                onClicked: fileDialog.open()
                background: Rectangle { color: parent.down ? "#1177bb" : theme.buttonBg; radius: 3 }
                contentItem: Text { text: parent.text; color: theme.buttonText; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Accept"
                enabled: dccList.currentIndex >= 0 && dccList.currentItem && dccList.currentItem.status === "Pending"
                onClicked: {
                    if (dccList.currentItem)
                        dccManager.acceptTransfer(dccList.currentItem.transferId)
                }
                background: Rectangle { color: parent.enabled ? (parent.down ? "#28a745" : "#218838") : theme.buttonDisabled; radius: 3 }
                contentItem: Text { text: parent.text; color: parent.enabled ? "#fff" : theme.buttonTextDisabled; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Cancel"
                enabled: dccList.currentIndex >= 0 && dccList.currentItem
                         && dccList.currentItem.status !== "Complete"
                         && dccList.currentItem.status !== "Cancelled"
                onClicked: {
                    if (dccList.currentItem)
                        dccManager.cancelTransfer(dccList.currentItem.transferId)
                }
                background: Rectangle { color: parent.enabled ? (parent.down ? "#a02020" : "#802020") : theme.buttonDisabled; radius: 3 }
                contentItem: Text { text: parent.text; color: parent.enabled ? "#fff" : theme.buttonTextDisabled; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Item { Layout.fillWidth: true }
            Button {
                text: "Clear Finished"
                onClicked: dccManager.clearFinished()
                background: Rectangle { color: parent.down ? theme.buttonPressed : theme.buttonBg; radius: 3 }
                contentItem: Text { text: parent.text; color: theme.buttonText; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Close"; onClicked: dlg.close()
                background: Rectangle { color: parent.down ? theme.buttonPressed : theme.buttonBg; radius: 3 }
                contentItem: Text { text: parent.text; color: theme.buttonText; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }

        // Download directory info
        RowLayout {
            Layout.fillWidth: true; spacing: 4
            Text { text: "Download to:"; color: theme.textMuted; font.pixelSize: 11 }
            Text { text: dccManager.downloadDir; color: theme.textSecondary; font.pixelSize: 11; elide: Text.ElideMiddle; Layout.fillWidth: true }
        }
    }

    // File picker for sending
    FileDialog {
        id: fileDialog
        title: "Select File to Send"
        onAccepted: {
            // Need a target nick — use selected nick from nicklist if available
            var targetNick = root.selectedNicks.length > 0 ? root.selectedNicks[0] : ""
            if (targetNick === "") {
                msgModel.addMessage("system", "DCC: Select a nick in the nick list first, then send")
                return
            }
            var path = selectedFile.toString().replace("file://", "")
            dccManager.sendFile(targetNick, path)
        }
    }
}
