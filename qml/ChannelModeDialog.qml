import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: dlg
    title: "Channel Mode"
    width: 440
    height: 400
    modal: true
    anchors.centerIn: parent

    background: Rectangle { color: "#2b2b2b"; border.color: "#555"; border.width: 1; radius: 6 }
    header: Rectangle {
        height: 36; color: "#252526"; radius: 6
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 6; color: "#252526" }
        Text { anchors.centerIn: parent; text: "Channel Mode — " + root.currentChannel; color: "#ddd"; font.pixelSize: 14; font.bold: true }
    }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 16; spacing: 10

        GroupBox {
            Layout.fillWidth: true
            label: Text { text: " Channel Modes "; color: "#aaa"; font.pixelSize: 11 }
            background: Rectangle { color: "#222"; border.color: "#404040"; radius: 3; y: 10 }

            GridLayout {
                columns: 2
                columnSpacing: 16
                rowSpacing: 4
                anchors.fill: parent

                CheckBox { id: modeN; text: "+n  No external messages"; checked: true
                    contentItem: Text { text: modeN.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                CheckBox { id: modeT; text: "+t  Topic by ops only"; checked: true
                    contentItem: Text { text: modeT.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                CheckBox { id: modeI; text: "+i  Invite only"
                    contentItem: Text { text: modeI.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                CheckBox { id: modeM; text: "+m  Moderated"
                    contentItem: Text { text: modeM.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                CheckBox { id: modeS; text: "+s  Secret"
                    contentItem: Text { text: modeS.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                CheckBox { id: modeP; text: "+p  Private"
                    contentItem: Text { text: modeP.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                CheckBox { id: modeC; text: "+c  No colors"
                    contentItem: Text { text: modeC.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                CheckBox { id: modeR; text: "+R  Registered only"
                    contentItem: Text { text: modeR.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
            }
        }

        GroupBox {
            Layout.fillWidth: true
            label: Text { text: " Limits "; color: "#aaa"; font.pixelSize: 11 }
            background: Rectangle { color: "#222"; border.color: "#404040"; radius: 3; y: 10 }

            GridLayout {
                columns: 4
                columnSpacing: 10
                rowSpacing: 6
                anchors.fill: parent

                CheckBox { id: modeL; text: "+l  User limit:"
                    contentItem: Text { text: modeL.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                SpinBox { from: 1; to: 9999; value: 100; enabled: modeL.checked; Layout.preferredWidth: 100 }
                CheckBox { id: modeK; text: "+k  Key:"
                    contentItem: Text { text: modeK.text; color: "#ccc"; font.pixelSize: 12; leftPadding: 22 } }
                TextField {
                    Layout.fillWidth: true; enabled: modeK.checked
                    color: "#ddd"; font.pixelSize: 12; placeholderText: "channel key"; placeholderTextColor: "#666"
                    background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true; spacing: 8
            TextField {
                id: rawModeField; Layout.fillWidth: true
                placeholderText: "Raw mode string (e.g. +o nick)"; placeholderTextColor: "#666"
                color: "#ddd"; font.family: "monospace"; font.pixelSize: 12
                background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
            }
        }

        RowLayout {
            Layout.fillWidth: true; spacing: 8
            Item { Layout.fillWidth: true }
            Button {
                text: "Apply"
                onClicked: {
                    // Build mode string from checkboxes
                    var setModes = ""
                    var unsetModes = ""
                    var params = []
                    if (modeN.checked) setModes += "n"; else unsetModes += "n"
                    if (modeT.checked) setModes += "t"; else unsetModes += "t"
                    if (modeI.checked) setModes += "i"; else unsetModes += "i"
                    if (modeM.checked) setModes += "m"; else unsetModes += "m"
                    if (modeS.checked) setModes += "s"; else unsetModes += "s"
                    if (modeP.checked) setModes += "p"; else unsetModes += "p"
                    if (modeC.checked) setModes += "c"; else unsetModes += "c"
                    if (modeR.checked) setModes += "R"; else unsetModes += "R"
                    var modeStr = ""
                    if (setModes !== "") modeStr += "+" + setModes
                    if (unsetModes !== "") modeStr += "-" + unsetModes
                    if (modeStr !== "" && root.currentChannel !== "") {
                        ircManager.sendRawCommand("MODE " + root.currentChannel + " " + modeStr)
                    }
                    // Handle raw mode field too
                    if (rawModeField.text !== "" && root.currentChannel !== "") {
                        ircManager.sendRawCommand("MODE " + root.currentChannel + " " + rawModeField.text)
                    }
                    dlg.close()
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
