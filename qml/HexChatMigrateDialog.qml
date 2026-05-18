import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: dlg
    title: "Import HexChat Scripts"
    width: 460
    modal: true
    anchors.centerIn: parent
    closePolicy: Popup.NoAutoClose

    // Emitted after the user makes a choice (imported or skipped)
    signal done()

    background: Rectangle { color: "#2b2b2b"; border.color: "#555"; border.width: 1; radius: 6 }
    header: Rectangle {
        height: 38; color: "#252526"; radius: 6
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 6; color: "#252526" }
        Text {
            anchors.centerIn: parent
            text: "Import HexChat Scripts"
            color: "#ddd"; font.pixelSize: 14; font.bold: true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 14

        Text {
            Layout.fillWidth: true
            text: "HexChat scripts were found in <b>~/.config/hexchat/addons/</b>."
            color: "#ccc"; font.pixelSize: 13; wrapMode: Text.WordWrap
            textFormat: Text.RichText
        }

        Text {
            Layout.fillWidth: true
            text: "Would you like to copy them into your NUchat scripts folder so they load automatically?"
            color: "#ccc"; font.pixelSize: 12; wrapMode: Text.WordWrap
        }

        Rectangle {
            Layout.fillWidth: true
            height: scriptListColumn.implicitHeight + 16
            color: "#1e1e1e"; border.color: "#404040"; radius: 3
            clip: true

            Column {
                id: scriptListColumn
                anchors { left: parent.left; right: parent.right; top: parent.top; margins: 8 }
                spacing: 3

                Repeater {
                    model: appSettings.hexchatScriptFiles()
                    delegate: Text {
                        text: "• " + modelData
                        color: "#aaa"; font.pixelSize: 11; font.family: "monospace"
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: "Scripts that already exist in NUchat's scripts folder will not be overwritten."
            color: "#888"; font.pixelSize: 11; wrapMode: Text.WordWrap; font.italic: true
        }

        // Result label (shown after import)
        Text {
            id: resultLabel
            Layout.fillWidth: true
            color: "#4ec9b0"; font.pixelSize: 12; wrapMode: Text.WordWrap
            visible: false
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Item { Layout.fillWidth: true }

            Button {
                id: btnSkip
                text: "Not Now"
                background: Rectangle {
                    color: btnSkip.down ? "#555" : "#444"
                    radius: 3
                }
                contentItem: Text {
                    text: "Not Now"; color: "#ccc"; font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    appSettings.setValue("app/hexchatMigratePrompted", true)
                    appSettings.sync()
                    dlg.done()
                    dlg.close()
                }
            }

            Button {
                id: btnImport
                text: "Import Scripts"
                background: Rectangle {
                    color: btnImport.down ? "#1177bb" : "#0e639c"
                    radius: 3
                }
                contentItem: Text {
                    text: btnImport.text; color: "#fff"; font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    var n = appSettings.importHexChatScripts()
                    appSettings.setValue("app/hexchatMigratePrompted", true)
                    appSettings.sync()
                    if (typeof pyEngine !== "undefined")
                        pyEngine.reloadAll()
                    if (typeof luaEngine !== "undefined")
                        luaEngine.reloadAll()
                    resultLabel.text = n > 0
                        ? n + " script(s) imported. They will load on next startup."
                        : "No new scripts were copied (they may already exist)."
                    resultLabel.visible = true
                    btnImport.enabled = false
                    btnSkip.text = "Close"
                }
            }
        }
    }
}
