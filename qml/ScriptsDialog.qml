import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: dlg
    title: "Scripts — NUchat"
    width: 560
    height: 480
    modal: true
    anchors.centerIn: parent

    background: Rectangle { color: "#2b2b2b"; border.color: "#555"; border.width: 1; radius: 6 }

    header: Rectangle {
        height: 38; color: "#252526"; radius: 6
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 6; color: "#252526" }
        Text { anchors.centerIn: parent; text: "Loaded Scripts"; color: "#ddd"; font.pixelSize: 14; font.bold: true }
    }

    // Combined script model
    ListModel {
        id: combinedScripts
    }

    // Update combined model when engines change
    function updateScriptList() {
        combinedScripts.clear()
        
        if (typeof pyEngine !== 'undefined' && pyEngine !== null) {
            var pyScripts = pyEngine.loadedScripts
            for (var i = 0; i < pyScripts.length; i++) {
                combinedScripts.append({
                    "filename": pyScripts[i],
                    "engine": "python",
                    "info": pyEngine.scriptInfo(pyScripts[i])
                })
            }
        }
        
        if (typeof luaEngine !== 'undefined' && luaEngine !== null) {
            var luaScripts = luaEngine.loadedScripts
            for (var j = 0; j < luaScripts.length; j++) {
                combinedScripts.append({
                    "filename": luaScripts[j],
                    "engine": "lua",
                    "info": luaEngine.scriptInfo(luaScripts[j])
                })
            }
        }
    }

    Component.onCompleted: updateScriptList()

    Connections {
        target: (typeof pyEngine !== 'undefined' && pyEngine !== null) ? pyEngine : null
        function onLoadedScriptsChanged() { updateScriptList() }
    }

    Connections {
        target: (typeof luaEngine !== 'undefined' && luaEngine !== null) ? luaEngine : null
        function onLoadedScriptsChanged() { updateScriptList() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        // ── Information ──
        Text {
            text: {
                var parts = []
                if (typeof pyEngine !== 'undefined' && pyEngine !== null)
                    parts.push("Python")
                if (typeof luaEngine !== 'undefined' && luaEngine !== null)
                    parts.push("Lua")
                
                if (parts.length === 0)
                    return "No scripting engines available (python3-dev and/or lua not installed)"
                
                var dir = (typeof pyEngine !== 'undefined' && pyEngine !== null) ? pyEngine.scriptsDirectory
                         : (typeof luaEngine !== 'undefined' && luaEngine !== null) ? luaEngine.scriptsDirectory : ""
                return "Scripts directory: " + dir + " (" + parts.join(", ") + ")"
            }
            color: "#999"; font.pixelSize: 11; wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        // ── Script list ──
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#1e1e1e"
            border.color: "#404040"
            radius: 3

            ListView {
                id: scriptList
                anchors.fill: parent
                anchors.margins: 1
                clip: true
                model: combinedScripts
                currentIndex: 0

                delegate: Rectangle {
                    required property int index
                    required property string filename
                    required property string engine
                    required property string info
                    width: scriptList.width
                    height: 50
                    color: scriptList.currentIndex === index ? "#264f78"
                         : slMouse.containsMouse ? "#333" : "transparent"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        anchors.topMargin: 6
                        anchors.bottomMargin: 6
                        spacing: 2

                        RowLayout {
                            spacing: 6
                            Text {
                                text: filename
                                color: "#ddd"
                                font.pixelSize: 13
                                font.bold: true
                            }
                            Rectangle {
                                width: 40
                                height: 16
                                radius: 3
                                color: engine === "python" ? "#3776ab" : "#000080"
                                Text {
                                    anchors.centerIn: parent
                                    text: engine === "python" ? "PY" : "LUA"
                                    color: "#fff"
                                    font.pixelSize: 9
                                    font.bold: true
                                }
                            }
                        }
                        Text {
                            text: info
                            color: "#999"
                            font.pixelSize: 11
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    MouseArea {
                        id: slMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: scriptList.currentIndex = index
                    }
                }

                // Empty state
                Text {
                    anchors.centerIn: parent
                    visible: scriptList.count === 0
                    text: "No scripts loaded.\nDrop .py or .lua files into the scripts folder."
                    color: "#666"
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        // ── Hook count ──
        Text {
            text: {
                var total = 0
                if (typeof pyEngine !== 'undefined' && pyEngine !== null)
                    total += pyEngine.hookCount()
                // Note: Lua engine doesn't expose hookCount() yet
                return total > 0 ? total + " active hooks" : ""
            }
            color: "#888"; font.pixelSize: 11
        }

        // ── Status messages ──
        Text {
            id: scriptStatus
            text: ""
            color: "#4ec9b0"
            font.pixelSize: 11
            Layout.fillWidth: true
            wrapMode: Text.Wrap

            Connections {
                target: (typeof pyEngine !== 'undefined' && pyEngine !== null) ? pyEngine : null
                function onScriptMessage(msg) { scriptStatus.text = msg }
            }
        }

        // ── Buttons ──
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Button {
                text: "Load..."
                onClicked: scriptFileDialog.open()
                background: Rectangle { color: parent.down ? "#1177bb" : "#0e639c"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#fff"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Unload"
                enabled: scriptList.currentIndex >= 0 && scriptList.count > 0
                onClicked: {
                    if (scriptList.currentIndex >= 0 && scriptList.currentIndex < combinedScripts.count) {
                        var item = combinedScripts.get(scriptList.currentIndex)
                        if (item.engine === "python" && typeof pyEngine !== 'undefined' && pyEngine !== null) {
                            pyEngine.unloadScript(item.filename)
                        } else if (item.engine === "lua" && typeof luaEngine !== 'undefined' && luaEngine !== null) {
                            luaEngine.unloadScript(item.filename)
                        }
                    }
                }
                background: Rectangle { color: parent.enabled ? (parent.down ? "#a02020" : "#802020") : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: parent.enabled ? "#fff" : "#777"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Reload"
                enabled: scriptList.currentIndex >= 0 && scriptList.count > 0
                onClicked: {
                    if (scriptList.currentIndex >= 0 && scriptList.currentIndex < combinedScripts.count) {
                        var item = combinedScripts.get(scriptList.currentIndex)
                        if (item.engine === "python" && typeof pyEngine !== 'undefined' && pyEngine !== null) {
                            pyEngine.reloadScript(item.filename)
                        } else if (item.engine === "lua" && typeof luaEngine !== 'undefined' && luaEngine !== null) {
                            // Lua doesn't have reloadScript, use unload + load
                            luaEngine.unloadScript(item.filename)
                            luaEngine.loadScript(luaEngine.scriptsDirectory + "/" + item.filename)
                        }
                    }
                }
                background: Rectangle { color: parent.enabled ? (parent.down ? "#1177bb" : "#0e639c") : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: parent.enabled ? "#fff" : "#777"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Reload All"
                onClicked: {
                    if (typeof pyEngine !== 'undefined' && pyEngine !== null) pyEngine.reloadAll()
                    if (typeof luaEngine !== 'undefined' && luaEngine !== null) luaEngine.reloadAll()
                }
                background: Rectangle { color: parent.down ? "#1177bb" : "#0e639c"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#fff"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Open Folder"
                onClicked: {
                    if (typeof pyEngine !== 'undefined' && pyEngine !== null)
                        pyEngine.openScriptsFolder()
                    else if (typeof luaEngine !== 'undefined' && luaEngine !== null)
                        luaEngine.openScriptsFolder()
                }
                background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Close"
                onClicked: dlg.close()
                background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }
    }
}
