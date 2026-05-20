import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// ── Nick List Panel ──
// Extracted from main.qml for modularity.
// Uses context properties (ircManager, theme) and explicit properties for state.
Rectangle {
    id: nickListRoot

    // ── Properties from parent ──
    property var theme
    property var channelUsers
    property string currentChannel
    property var selectedNicks: []
    property int lastClickedNickIndex: -1
    property bool showModeButtons: false
    property var messageInputRef: null     // reference to message input for focus

    // ── Signals (emitted back to parent) ──
    signal selectedNicksUpdated(var nicks)
    signal lastClickedIndexUpdated(int idx)
    signal nickContextMenuRequested(string nick)

    // ── Helper ──
    function bareNick(n) { return n ? n.replace(/^[@%+~&]/, '') : "" }
    function forEachSelectedNick(callback) {
        for (var i = 0; i < selectedNicks.length; i++) callback(selectedNicks[i])
    }

    color: theme.nickListBg

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: theme.nickListHeaderBg
            Text {
                anchors.centerIn: parent
                text: channelUsers ? channelUsers.length + " USERS" : "USERS"
                color: theme.textMuted
                font.pixelSize: 10
                font.bold: true
                font.letterSpacing: 1.5
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: theme.separator
        }

        // Nick list
        ListView {
            id: nickList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: channelUsers || []

            delegate: Rectangle {
                required property int index
                required property string modelData
                width: nickList.width
                height: 22
                property string bareN: nickListRoot.bareNick(modelData)
                property bool isSelected: nickListRoot.selectedNicks.indexOf(bareN) !== -1
                color: isSelected ? theme.highlight : (nickMouse.containsMouse ? theme.hoverBg : "transparent")

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData
                    color: {
                        if (modelData.startsWith("@")) return theme.nickOp
                        if (modelData.startsWith("%")) return theme.nickHalfOp
                        if (modelData.startsWith("+")) return theme.nickVoice
                        if (modelData.startsWith("~")) return theme.nickOwner
                        if (modelData.startsWith("&")) return theme.nickAdmin
                        return theme.nickNormal
                    }
                    font.pixelSize: 12
                    elide: Text.ElideRight
                    width: parent.width - 16
                }

                MouseArea {
                    id: nickMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: function(mouse) {
                        if (mouse.button === Qt.LeftButton) {
                            var nick = bareN
                            var sel = nickListRoot.selectedNicks.slice()
                            if (mouse.modifiers & Qt.ControlModifier) {
                                var idx = sel.indexOf(nick)
                                if (idx !== -1) sel.splice(idx, 1); else sel.push(nick)
                                nickListRoot.selectedNicksUpdated(sel)
                            } else if (mouse.modifiers & Qt.ShiftModifier) {
                                var users = channelUsers || []
                                var from = nickListRoot.lastClickedNickIndex
                                var to = index
                                if (from < 0) from = to
                                var lo = Math.min(from, to), hi = Math.max(from, to)
                                for (var r = lo; r <= hi; r++) {
                                    var rn = nickListRoot.bareNick(users[r])
                                    if (sel.indexOf(rn) === -1) sel.push(rn)
                                }
                                nickListRoot.selectedNicksUpdated(sel)
                            } else {
                                nickListRoot.selectedNicksUpdated([nick])
                            }
                            nickListRoot.lastClickedIndexUpdated(index)
                        }
                    }
                    onPressed: function(mouse) {
                        if (mouse.button === Qt.RightButton) {
                            var nick = bareN
                            if (nickListRoot.selectedNicks.indexOf(nick) === -1)
                                nickListRoot.selectedNicksUpdated([nick])
                            nickListRoot.lastClickedIndexUpdated(index)
                            nickListRoot.nickContextMenuRequested(nick)
                        }
                    }
                    onDoubleClicked: {
                        var nick = bareN
                        nickListRoot.selectedNicksUpdated([nick])
                        nickListRoot.lastClickedIndexUpdated(index)
                        if (messageInputRef) {
                            messageInputRef.text = "/msg " + nick + " "
                            messageInputRef.forceActiveFocus()
                        }
                    }
                }
            }
        }

        // ── Channel mode buttons (HexChat-style) ──
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: theme.separator
            visible: nickListRoot.showModeButtons
        }
        GridLayout {
            visible: nickListRoot.showModeButtons
            Layout.fillWidth: true
            Layout.margins: 3
            columns: 3
            columnSpacing: 2
            rowSpacing: 2

            Repeater {
                model: [
                    { label: "Op",   cmd: "+o" },
                    { label: "DeOp", cmd: "-o" },
                    { label: "Ban",  cmd: "ban" },
                    { label: "Kick", cmd: "kick" },
                    { label: "Voice",   cmd: "+v" },
                    { label: "DeVoice", cmd: "-v" }
                ]
                delegate: Button {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 24
                    text: modelData.label
                    enabled: nickListRoot.selectedNicks.length > 0 && currentChannel.startsWith("#")
                    onClicked: {
                        nickListRoot.forEachSelectedNick(function(nick) {
                            if (modelData.cmd === "kick")
                                ircManager.sendRawCommand("KICK " + currentChannel + " " + nick)
                            else if (modelData.cmd === "ban")
                                ircManager.sendRawCommand("MODE " + currentChannel + " +b " + nick + "!*@*")
                            else
                                ircManager.sendRawCommand("MODE " + currentChannel + " " + modelData.cmd + " " + nick)
                        })
                    }
                    background: Rectangle {
                        color: parent.enabled ? (parent.down ? theme.buttonPressed : theme.buttonBg) : theme.buttonDisabled
                        radius: 2
                    }
                    contentItem: Text {
                        text: parent.text; color: parent.enabled ? theme.buttonText : theme.buttonTextDisabled
                        font.pixelSize: 10; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }
}
