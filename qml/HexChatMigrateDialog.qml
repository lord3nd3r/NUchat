import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: dlg
    title: "Migrate from HexChat"
    width: 500
    modal: true
    anchors.centerIn: parent
    closePolicy: Popup.NoAutoClose

    signal done()

    // Called externally to trigger a network-list reload after import
    property var networkListModel: null

    background: Rectangle { color: "#2b2b2b"; border.color: "#555"; border.width: 1; radius: 6 }
    header: Rectangle {
        height: 38; color: "#252526"; radius: 6
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 6; color: "#252526" }
        Text {
            anchors.centerIn: parent
            text: "Migrate from HexChat"
            color: "#ddd"; font.pixelSize: 14; font.bold: true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        Text {
            Layout.fillWidth: true
            text: "NUchat found your HexChat configuration. Select what you'd like to import:"
            color: "#ccc"; font.pixelSize: 13; wrapMode: Text.WordWrap
        }

        // ── Scripts ──
        Rectangle {
            Layout.fillWidth: true
            height: scriptsSection.implicitHeight + 16
            color: "#1e1e1e"; border.color: chkScripts.checked ? "#0e639c" : "#404040"; radius: 4

            ColumnLayout {
                id: scriptsSection
                anchors { left: parent.left; right: parent.right; top: parent.top; margins: 10 }
                spacing: 6

                RowLayout {
                    spacing: 8
                    CheckBox {
                        id: chkScripts
                        checked: appSettings.hexchatScriptsExist()
                        enabled: appSettings.hexchatScriptsExist()
                        contentItem: Text {
                            text: "<b>Scripts</b> — ~/.config/hexchat/addons/"
                            color: chkScripts.enabled ? "#ddd" : "#666"
                            font.pixelSize: 12; leftPadding: 22
                            textFormat: Text.RichText
                        }
                    }
                }

                Repeater {
                    model: appSettings.hexchatScriptFiles()
                    delegate: Text {
                        Layout.leftMargin: 24
                        text: "  • " + modelData
                        color: "#888"; font.pixelSize: 11; font.family: "monospace"
                        visible: chkScripts.checked
                    }
                }

                Text {
                    visible: !appSettings.hexchatScriptsExist()
                    text: "  No scripts found."
                    color: "#666"; font.pixelSize: 11; font.italic: true
                }
            }
        }

        // ── Config / Networks ──
        Rectangle {
            Layout.fillWidth: true
            height: configSection.implicitHeight + 16
            color: "#1e1e1e"; border.color: chkConfig.checked ? "#0e639c" : "#404040"; radius: 4

            ColumnLayout {
                id: configSection
                anchors { left: parent.left; right: parent.right; top: parent.top; margins: 10 }
                spacing: 4

                RowLayout {
                    spacing: 8
                    CheckBox {
                        id: chkConfig
                        checked: appSettings.hexchatConfigExists()
                        enabled: appSettings.hexchatConfigExists()
                        contentItem: Text {
                            text: "<b>Networks &amp; Identity</b> — servlist.conf + hexchat.conf"
                            color: chkConfig.enabled ? "#ddd" : "#666"
                            font.pixelSize: 12; leftPadding: 22
                            textFormat: Text.RichText
                        }
                    }
                }

                Text {
                    Layout.leftMargin: 8
                    text: "  Imports your saved server list and nickname/username/realname."
                    color: "#888"; font.pixelSize: 11; font.italic: true
                    visible: chkConfig.checked
                }
                Text {
                    visible: !appSettings.hexchatConfigExists()
                    text: "  No HexChat config found."
                    color: "#666"; font.pixelSize: 11; font.italic: true
                }
            }
        }

        // Result text
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
                background: Rectangle { color: btnSkip.down ? "#555" : "#444"; radius: 3 }
                contentItem: Text {
                    text: btnSkip.text; color: "#ccc"; font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
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
                enabled: chkScripts.checked || chkConfig.checked
                text: "Import"
                background: Rectangle {
                    color: btnImport.enabled ? (btnImport.down ? "#1177bb" : "#0e639c") : "#333"
                    radius: 3
                }
                contentItem: Text {
                    text: "Import"; color: btnImport.enabled ? "#fff" : "#666"; font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    var messages = []

                    if (chkScripts.checked) {
                        var n = appSettings.importHexChatScripts()
                        if (typeof pyEngine !== "undefined") pyEngine.reloadAll()
                        if (typeof luaEngine !== "undefined") luaEngine.reloadAll()
                        messages.push(n > 0 ? n + " script(s) imported." : "Scripts: none new (already exist).")
                    }

                    if (chkConfig.checked) {
                        // Import identity
                        var identity = appSettings.importHexChatIdentity()
                        if (identity.nick)     { appSettings.setValue("user/nickname", identity.nick);     appSettings.sync() }
                        if (identity.username) { appSettings.setValue("user/username", identity.username); appSettings.sync() }
                        if (identity.realname) { appSettings.setValue("user/realname", identity.realname); appSettings.sync() }

                        // Import networks — merge into existing network list
                        var imported = appSettings.importHexChatNetworks()
                        if (imported.length > 0) {
                            var existing = appSettings.value("networks/list", "[]")
                            var arr = []
                            try { arr = JSON.parse(existing) } catch(e) { arr = [] }
                            var existingNames = arr.map(function(x) { return x.network.toLowerCase() })
                            var added = 0
                            for (var i = 0; i < imported.length; i++) {
                                var net = imported[i]
                                if (existingNames.indexOf(net.network.toLowerCase()) < 0) {
                                    arr.push(net)
                                    added++
                                }
                            }
                            appSettings.setValue("networks/list", JSON.stringify(arr))
                            appSettings.sync()
                            messages.push(added + " network(s) added, " + (imported.length - added) + " already existed.")
                        } else {
                            messages.push("Networks: no servlist.conf found.")
                        }

                        if (identity.nick || identity.username || identity.realname)
                            messages.push("Identity imported.")
                    }

                    appSettings.setValue("app/hexchatMigratePrompted", true)
                    appSettings.sync()

                    resultLabel.text = messages.join(" ")
                    resultLabel.visible = true
                    btnImport.enabled = false
                    chkScripts.enabled = false
                    chkConfig.enabled = false
                    btnSkip.text = "Close"
                }
            }
        }
    }
}
