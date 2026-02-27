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

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        // ── Information ──
        Text {
            text: typeof pyEngine !== 'undefined'
                  ? "Scripts directory: " + pyEngine.scriptsDirectory
                  : "Python scripting is not available (python3-dev not installed)"
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
                model: typeof pyEngine !== 'undefined' ? pyEngine.loadedScripts : []
                currentIndex: 0

                delegate: Rectangle {
                    required property int index
                    required property string modelData
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

                        Text {
                            text: modelData
                            color: "#ddd"
                            font.pixelSize: 13
                            font.bold: true
                        }
                        Text {
                            text: typeof pyEngine !== 'undefined' ? pyEngine.scriptInfo(modelData) : ""
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
                    text: "No scripts loaded.\nDrop .py files into the scripts folder."
                    color: "#666"
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        // ── Hook count ──
        Text {
            text: typeof pyEngine !== 'undefined'
                  ? pyEngine.hookCount() + " active hooks"
                  : ""
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
                target: typeof pyEngine !== 'undefined' ? pyEngine : null
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
                    if (typeof pyEngine !== 'undefined' && scriptList.currentIndex >= 0) {
                        var scripts = pyEngine.loadedScripts
                        if (scriptList.currentIndex < scripts.length) {
                            pyEngine.unloadScript(scripts[scriptList.currentIndex])
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
                    if (typeof pyEngine !== 'undefined' && scriptList.currentIndex >= 0) {
                        var scripts = pyEngine.loadedScripts
                        if (scriptList.currentIndex < scripts.length) {
                            pyEngine.reloadScript(scripts[scriptList.currentIndex])
                        }
                    }
                }
                background: Rectangle { color: parent.enabled ? (parent.down ? "#1177bb" : "#0e639c") : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: parent.enabled ? "#fff" : "#777"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Reload All"
                onClicked: { if (typeof pyEngine !== 'undefined') pyEngine.reloadAll() }
                background: Rectangle { color: parent.down ? "#1177bb" : "#0e639c"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#fff"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Open Folder"
                onClicked: { if (typeof pyEngine !== 'undefined') pyEngine.openScriptsFolder() }
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
