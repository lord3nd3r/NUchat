import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: dlg
    title: "Network List — NUchat"
    width: 640
    height: 700
    modal: true
    anchors.centerIn: parent

    background: Rectangle { color: "#2b2b2b"; border.color: "#555"; border.width: 1; radius: 6 }

    header: Rectangle {
        height: 38; color: "#252526"; radius: 6
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 6; color: "#252526" }
        Text { anchors.centerIn: parent; text: "Network List"; color: "#ddd"; font.pixelSize: 14; font.bold: true }
    }

    // ── Persistence helpers ──
    function saveNetworks() {
        var arr = []
        for (var i = 0; i < networkModel.count; i++) {
            var item = networkModel.get(i)
            arr.push({
                network: item.network, server: item.server, port: item.port, ssl: item.ssl,
                saslMethod: item.saslMethod, saslUser: item.saslUser, saslPass: item.saslPass,
                useGlobalNick: item.useGlobalNick, customNick: item.customNick,
                customUser: item.customUser, customReal: item.customReal,
                serverPass: item.serverPass, isZnc: item.isZnc,
                zncUser: item.zncUser, zncPass: item.zncPass, zncNetwork: item.zncNetwork
            })
        }
        appSettings.setValue("networks/list", JSON.stringify(arr))
        appSettings.sync()
    }

    function loadNetworks() {
        var json = appSettings.value("networks/list", "")
        if (json === "" || json === undefined) return  // keep hardcoded defaults
        try {
            var arr = JSON.parse(json)
            if (!Array.isArray(arr) || arr.length === 0) return
            networkModel.clear()
            for (var i = 0; i < arr.length; i++) {
                var n = arr[i]
                networkModel.append({
                    network: n.network || "", server: n.server || "", port: n.port || 6667, ssl: !!n.ssl,
                    saslMethod: n.saslMethod || "None", saslUser: n.saslUser || "", saslPass: n.saslPass || "",
                    useGlobalNick: n.useGlobalNick !== false, customNick: n.customNick || "",
                    customUser: n.customUser || "", customReal: n.customReal || "",
                    serverPass: n.serverPass || "", isZnc: !!n.isZnc,
                    zncUser: n.zncUser || "", zncPass: n.zncPass || "", zncNetwork: n.zncNetwork || ""
                })
            }
            networkList.currentIndex = 0
        } catch (e) {
            console.warn("Failed to load saved networks:", e)
        }
    }

    Component.onCompleted: loadNetworks()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        // ── Global Nickname ──
        RowLayout {
            Layout.fillWidth: true; spacing: 8
            Text { text: "Global Nickname:"; color: "#ccc"; font.pixelSize: 12 }
            TextField {
                id: nickField; Layout.fillWidth: true
                text: appSettings.value("user/nickname", "NUchat_user")
                placeholderText: "Your nickname"; placeholderTextColor: "#666"
                color: "#ddd"; font.pixelSize: 12
                background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                onEditingFinished: { appSettings.setValue("user/nickname", text); appSettings.sync() }
            }
        }

        // ── Saved networks list ──
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#1e1e1e"
            border.color: "#404040"
            radius: 3

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 0

                ListView {
                    id: networkList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: ListModel {
                        id: networkModel
                        ListElement { network: "Libera.Chat"; server: "irc.libera.chat"; port: 6697; ssl: true; saslMethod: "PLAIN"; saslUser: ""; saslPass: ""; useGlobalNick: true; customNick: ""; customUser: ""; customReal: ""; serverPass: ""; isZnc: false; zncUser: ""; zncPass: ""; zncNetwork: "" }
                        ListElement { network: "OFTC"; server: "irc.oftc.net"; port: 6697; ssl: true; saslMethod: "None"; saslUser: ""; saslPass: ""; useGlobalNick: true; customNick: ""; customUser: ""; customReal: ""; serverPass: ""; isZnc: false; zncUser: ""; zncPass: ""; zncNetwork: "" }
                        ListElement { network: "EFnet"; server: "irc.efnet.org"; port: 6667; ssl: false; saslMethod: "None"; saslUser: ""; saslPass: ""; useGlobalNick: true; customNick: ""; customUser: ""; customReal: ""; serverPass: ""; isZnc: false; zncUser: ""; zncPass: ""; zncNetwork: "" }
                        ListElement { network: "DALnet"; server: "irc.dal.net"; port: 6697; ssl: true; saslMethod: "None"; saslUser: ""; saslPass: ""; useGlobalNick: true; customNick: ""; customUser: ""; customReal: ""; serverPass: ""; isZnc: false; zncUser: ""; zncPass: ""; zncNetwork: "" }
                        ListElement { network: "Undernet"; server: "irc.undernet.org"; port: 6667; ssl: false; saslMethod: "None"; saslUser: ""; saslPass: ""; useGlobalNick: true; customNick: ""; customUser: ""; customReal: ""; serverPass: ""; isZnc: false; zncUser: ""; zncPass: ""; zncNetwork: "" }
                        ListElement { network: "Rizon"; server: "irc.rizon.net"; port: 6697; ssl: true; saslMethod: "None"; saslUser: ""; saslPass: ""; useGlobalNick: true; customNick: ""; customUser: ""; customReal: ""; serverPass: ""; isZnc: false; zncUser: ""; zncPass: ""; zncNetwork: "" }
                        ListElement { network: "IRCnet"; server: "open.ircnet.net"; port: 6667; ssl: false; saslMethod: "None"; saslUser: ""; saslPass: ""; useGlobalNick: true; customNick: ""; customUser: ""; customReal: ""; serverPass: ""; isZnc: false; zncUser: ""; zncPass: ""; zncNetwork: "" }
                        ListElement { network: "QuakeNet"; server: "irc.quakenet.org"; port: 6667; ssl: false; saslMethod: "None"; saslUser: ""; saslPass: ""; useGlobalNick: true; customNick: ""; customUser: ""; customReal: ""; serverPass: ""; isZnc: false; zncUser: ""; zncPass: ""; zncNetwork: "" }
                        ListElement { network: "Snoonet"; server: "irc.snoonet.org"; port: 6697; ssl: true; saslMethod: "None"; saslUser: ""; saslPass: ""; useGlobalNick: true; customNick: ""; customUser: ""; customReal: ""; serverPass: ""; isZnc: false; zncUser: ""; zncPass: ""; zncNetwork: "" }
                        ListElement { network: "freenode (legacy)"; server: "chat.freenode.net"; port: 6697; ssl: true; saslMethod: "None"; saslUser: ""; saslPass: ""; useGlobalNick: true; customNick: ""; customUser: ""; customReal: ""; serverPass: ""; isZnc: false; zncUser: ""; zncPass: ""; zncNetwork: "" }
                    }
                    currentIndex: 0

                    // ── Drag-and-drop state ──
                    property int dragFromIndex: -1
                    property int dropToIndex: -1

                    delegate: Item {
                        id: delegateRoot
                        required property int index
                        required property string network
                        required property string server
                        required property int port
                        required property bool ssl
                        width: networkList.width
                        height: 28

                        // Drop indicator line
                        Rectangle {
                            anchors.left: parent.left; anchors.right: parent.right
                            anchors.top: parent.top
                            height: 2; color: "#569cd6"; z: 10
                            visible: networkList.dropToIndex === delegateRoot.index && networkList.dragFromIndex !== -1 && networkList.dragFromIndex !== delegateRoot.index
                        }

                        // Background + click handling (bottom layer)
                        Rectangle {
                            id: delegateBg
                            anchors.fill: parent
                            color: networkList.currentIndex === delegateRoot.index ? "#264f78"
                                 : rowMouse.containsMouse ? "#333" : "transparent"
                            opacity: (networkList.dragFromIndex === delegateRoot.index) ? 0.4 : 1.0
                        }

                        // Click area covers the full row but sits below the drag handle
                        MouseArea {
                            id: rowMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            z: 1
                            onClicked: networkList.currentIndex = delegateRoot.index
                            onDoubleClicked: { networkList.currentIndex = delegateRoot.index; connectBtn.clicked() }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            z: 2

                            // Drag handle — sits on top of rowMouse
                            Text {
                                text: "\u2807"
                                color: "#888"
                                font.pixelSize: 16
                                Layout.preferredWidth: 18
                                verticalAlignment: Text.AlignVCenter

                                MouseArea {
                                    id: dragArea
                                    anchors.fill: parent
                                    cursorShape: Qt.OpenHandCursor
                                    preventStealing: true

                                    property bool held: false
                                    property real startY: 0

                                    onPressed: function(mouse) {
                                        held = true
                                        startY = mouse.y
                                        networkList.dragFromIndex = delegateRoot.index
                                        networkList.currentIndex = delegateRoot.index
                                        dragProxy.text = delegateRoot.network
                                        dragProxy.y = delegateRoot.mapToItem(networkList, 0, 0).y
                                        dragProxy.visible = true
                                        cursorShape = Qt.ClosedHandCursor
                                    }
                                    onReleased: function(mouse) {
                                        if (held && networkList.dragFromIndex >= 0 && networkList.dropToIndex >= 0
                                            && networkList.dragFromIndex !== networkList.dropToIndex) {
                                            networkModel.move(networkList.dragFromIndex, networkList.dropToIndex, 1)
                                            networkList.currentIndex = networkList.dropToIndex
                                            dlg.saveNetworks()
                                        }
                                        held = false
                                        networkList.dragFromIndex = -1
                                        networkList.dropToIndex = -1
                                        dragProxy.visible = false
                                        cursorShape = Qt.OpenHandCursor
                                    }
                                    onPositionChanged: function(mouse) {
                                        if (!held) return
                                        var posInList = dragArea.mapToItem(networkList, mouse.x, mouse.y)
                                        dragProxy.y = posInList.y - 13
                                        var targetIdx = Math.floor(posInList.y / 28)
                                        targetIdx = Math.max(0, Math.min(targetIdx, networkModel.count - 1))
                                        networkList.dropToIndex = targetIdx
                                    }
                                }
                            }
                            Text { text: delegateRoot.network; color: "#ddd"; font.pixelSize: 12; Layout.fillWidth: true }
                            Text { text: delegateRoot.server + ":" + delegateRoot.port; color: "#888"; font.pixelSize: 11 }
                            Text { text: delegateRoot.ssl ? "\uD83D\uDD12" : ""; font.pixelSize: 11; Layout.preferredWidth: 16 }
                        }
                    }

                    // Floating drag proxy
                    Rectangle {
                        id: dragProxy
                        visible: false
                        width: networkList.width - 8
                        height: 26
                        x: 4
                        color: "#264f78"
                        radius: 3
                        opacity: 0.85
                        z: 100
                        property string text: ""
                        Text { anchors.centerIn: parent; text: dragProxy.text; color: "#fff"; font.pixelSize: 12; font.bold: true }
                    }
                }
            }
        }

        // ── Edit section for selected network ──
        GroupBox {
            Layout.fillWidth: true
            label: Text { text: " Server Details "; color: "#aaa"; font.pixelSize: 11 }
            background: Rectangle { color: "#222"; border.color: "#404040"; radius: 3; y: 10 }

            GridLayout {
                columns: 4
                anchors.fill: parent
                columnSpacing: 8
                rowSpacing: 6

                Text { text: "Network:"; color: "#ccc"; font.pixelSize: 12 }
                TextField {
                    id: editNetwork; Layout.fillWidth: true; Layout.columnSpan: 3
                    text: networkList.currentIndex >= 0 ? networkModel.get(networkList.currentIndex).network : ""
                    color: "#ddd"; font.pixelSize: 12
                    background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                }

                Text { text: "Server:"; color: "#ccc"; font.pixelSize: 12 }
                TextField {
                    id: editServer; Layout.fillWidth: true
                    text: networkList.currentIndex >= 0 ? networkModel.get(networkList.currentIndex).server : ""
                    color: "#ddd"; font.pixelSize: 12
                    background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                }
                Text { text: "Port:"; color: "#ccc"; font.pixelSize: 12 }
                TextField {
                    id: editPort; Layout.preferredWidth: 70
                    text: networkList.currentIndex >= 0 ? networkModel.get(networkList.currentIndex).port : ""
                    color: "#ddd"; font.pixelSize: 12; inputMethodHints: Qt.ImhDigitsOnly
                    background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                }

                Text { text: "SSL/TLS:"; color: "#ccc"; font.pixelSize: 12 }
                CheckBox {
                    id: editSsl; Layout.columnSpan: 3
                    checked: networkList.currentIndex >= 0 ? networkModel.get(networkList.currentIndex).ssl : false
                    indicator: Rectangle {
                        width: 16; height: 16; radius: 2; color: editSsl.checked ? "#0e639c" : "#333"; border.color: "#555"
                        Text { anchors.centerIn: parent; text: editSsl.checked ? "✓" : ""; color: "#fff"; font.pixelSize: 11 }
                    }
                    contentItem: Text { text: " Use SSL/TLS encryption"; color: "#ccc"; font.pixelSize: 12; leftPadding: editSsl.indicator.width + 6 }
                }

                Text { text: "Password:"; color: "#ccc"; font.pixelSize: 12 }
                TextField {
                    id: editServerPass; Layout.fillWidth: true; Layout.columnSpan: 3
                    text: networkList.currentIndex >= 0 ? networkModel.get(networkList.currentIndex).serverPass : ""
                    echoMode: TextInput.Password; placeholderText: "Server password (optional)"; placeholderTextColor: "#666"
                    color: "#ddd"; font.pixelSize: 12
                    background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                }

                Text { text: "SASL:"; color: "#ccc"; font.pixelSize: 12 }
                ComboBox {
                    id: editSasl; Layout.columnSpan: 3; Layout.preferredWidth: 200
                    model: ["None", "PLAIN", "EXTERNAL", "SCRAM-SHA-256", "ECDSA-NIST256P-CHALLENGE"]
                    currentIndex: {
                        if (networkList.currentIndex < 0) return 0
                        var m = networkModel.get(networkList.currentIndex).saslMethod
                        var idx = ["None", "PLAIN", "EXTERNAL", "SCRAM-SHA-256", "ECDSA-NIST256P-CHALLENGE"].indexOf(m)
                        return idx >= 0 ? idx : 0
                    }
                }

                Text { text: "SASL User:"; color: "#ccc"; font.pixelSize: 12; visible: editSasl.currentIndex > 0 }
                TextField {
                    id: editSaslUser; Layout.fillWidth: true; visible: editSasl.currentIndex > 0
                    text: networkList.currentIndex >= 0 ? networkModel.get(networkList.currentIndex).saslUser : ""
                    placeholderText: "username"; placeholderTextColor: "#666"
                    color: "#ddd"; font.pixelSize: 12
                    background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                }
                Text { text: "SASL Pass:"; color: "#ccc"; font.pixelSize: 12; visible: editSasl.currentIndex === 1 }
                TextField {
                    id: editSaslPass; Layout.preferredWidth: 160; visible: editSasl.currentIndex === 1
                    text: networkList.currentIndex >= 0 ? networkModel.get(networkList.currentIndex).saslPass : ""
                    echoMode: TextInput.Password; placeholderText: "password"; placeholderTextColor: "#666"
                    color: "#ddd"; font.pixelSize: 12
                    background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                }

                // ── ZNC Bouncer ──
                Rectangle { Layout.columnSpan: 4; Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#404040"; Layout.topMargin: 4; Layout.bottomMargin: 2 }

                CheckBox {
                    id: editIsZnc; Layout.columnSpan: 4
                    checked: networkList.currentIndex >= 0 ? networkModel.get(networkList.currentIndex).isZnc : false
                    indicator: Rectangle {
                        width: 16; height: 16; radius: 2; color: editIsZnc.checked ? "#0e639c" : "#333"; border.color: "#555"
                        Text { anchors.centerIn: parent; text: editIsZnc.checked ? "\u2713" : ""; color: "#fff"; font.pixelSize: 11 }
                    }
                    contentItem: Text { text: " Connect via ZNC bouncer"; color: "#ccc"; font.pixelSize: 12; leftPadding: editIsZnc.indicator.width + 6 }
                }

                Text { text: "ZNC User:"; color: editIsZnc.checked ? "#ccc" : "#666"; font.pixelSize: 12; visible: editIsZnc.checked }
                TextField {
                    id: editZncUser; Layout.fillWidth: true; Layout.columnSpan: 3; visible: editIsZnc.checked
                    text: networkList.currentIndex >= 0 ? networkModel.get(networkList.currentIndex).zncUser : ""
                    placeholderText: "ZNC username"; placeholderTextColor: "#666"
                    color: "#ddd"; font.pixelSize: 12
                    background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                }

                Text { text: "ZNC Pass:"; color: editIsZnc.checked ? "#ccc" : "#666"; font.pixelSize: 12; visible: editIsZnc.checked }
                TextField {
                    id: editZncPass; Layout.fillWidth: true; Layout.columnSpan: 3; visible: editIsZnc.checked
                    text: networkList.currentIndex >= 0 ? networkModel.get(networkList.currentIndex).zncPass : ""
                    echoMode: TextInput.Password; placeholderText: "ZNC password"; placeholderTextColor: "#666"
                    color: "#ddd"; font.pixelSize: 12
                    background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                }

                Text { text: "ZNC Network:"; color: editIsZnc.checked ? "#ccc" : "#666"; font.pixelSize: 12; visible: editIsZnc.checked }
                TextField {
                    id: editZncNetwork; Layout.fillWidth: true; Layout.columnSpan: 3; visible: editIsZnc.checked
                    text: networkList.currentIndex >= 0 ? networkModel.get(networkList.currentIndex).zncNetwork : ""
                    placeholderText: "Network name (e.g. libera, optional)"; placeholderTextColor: "#666"
                    color: "#ddd"; font.pixelSize: 12
                    background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                }

                // ── Per-network identity ──
                Rectangle { Layout.columnSpan: 4; Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#404040"; Layout.topMargin: 4; Layout.bottomMargin: 2 }

                Text { text: "Identity:"; color: "#aaa"; font.pixelSize: 11; font.bold: true; Layout.columnSpan: 4 }

                CheckBox {
                    id: editUseGlobal; Layout.columnSpan: 4
                    checked: networkList.currentIndex >= 0 ? networkModel.get(networkList.currentIndex).useGlobalNick : true
                    indicator: Rectangle {
                        width: 16; height: 16; radius: 2; color: editUseGlobal.checked ? "#0e639c" : "#333"; border.color: "#555"
                        Text { anchors.centerIn: parent; text: editUseGlobal.checked ? "✓" : ""; color: "#fff"; font.pixelSize: 11 }
                    }
                    contentItem: Text { text: " Use global nickname / identity"; color: "#ccc"; font.pixelSize: 12; leftPadding: editUseGlobal.indicator.width + 6 }
                }

                Text { text: "Nickname:"; color: editUseGlobal.checked ? "#666" : "#ccc"; font.pixelSize: 12; visible: !editUseGlobal.checked }
                TextField {
                    id: editCustomNick; Layout.fillWidth: true; Layout.columnSpan: 3; visible: !editUseGlobal.checked
                    text: networkList.currentIndex >= 0 ? networkModel.get(networkList.currentIndex).customNick : ""
                    placeholderText: "Network-specific nick"; placeholderTextColor: "#666"
                    color: "#ddd"; font.pixelSize: 12; enabled: !editUseGlobal.checked
                    background: Rectangle { color: editUseGlobal.checked ? "#2a2a2a" : "#333"; border.color: "#555"; radius: 2 }
                }

                Text { text: "Username:"; color: editUseGlobal.checked ? "#666" : "#ccc"; font.pixelSize: 12; visible: !editUseGlobal.checked }
                TextField {
                    id: editCustomUser; Layout.fillWidth: true; Layout.columnSpan: 3; visible: !editUseGlobal.checked
                    text: networkList.currentIndex >= 0 ? networkModel.get(networkList.currentIndex).customUser : ""
                    placeholderText: "Network-specific username"; placeholderTextColor: "#666"
                    color: "#ddd"; font.pixelSize: 12; enabled: !editUseGlobal.checked
                    background: Rectangle { color: editUseGlobal.checked ? "#2a2a2a" : "#333"; border.color: "#555"; radius: 2 }
                }

                Text { text: "Real name:"; color: editUseGlobal.checked ? "#666" : "#ccc"; font.pixelSize: 12; visible: !editUseGlobal.checked }
                TextField {
                    id: editCustomReal; Layout.fillWidth: true; Layout.columnSpan: 3; visible: !editUseGlobal.checked
                    text: networkList.currentIndex >= 0 ? networkModel.get(networkList.currentIndex).customReal : ""
                    placeholderText: "Network-specific real name"; placeholderTextColor: "#666"
                    color: "#ddd"; font.pixelSize: 12; enabled: !editUseGlobal.checked
                    background: Rectangle { color: editUseGlobal.checked ? "#2a2a2a" : "#333"; border.color: "#555"; radius: 2 }
                }
            }
        }

        // ── Buttons ──
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Button {
                text: "Add Network"
                onClicked: {
                    networkModel.append({network: "New Network", server: "irc.example.com", port: 6697, ssl: true, saslMethod: "None", saslUser: "", saslPass: "", useGlobalNick: true, customNick: "", customUser: "", customReal: "", serverPass: "", isZnc: false, zncUser: "", zncPass: "", zncNetwork: ""})
                    networkList.currentIndex = networkModel.count - 1
                    dlg.saveNetworks()
                }
                background: Rectangle { color: parent.down ? "#1177bb" : "#0e639c"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#fff"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Remove"
                enabled: networkList.currentIndex >= 0
                onClicked: { if (networkList.currentIndex >= 0) { networkModel.remove(networkList.currentIndex); dlg.saveNetworks() } }
                background: Rectangle { color: parent.enabled ? (parent.down ? "#a02020" : "#802020") : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: parent.enabled ? "#fff" : "#777"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Save"
                enabled: networkList.currentIndex >= 0
                onClicked: {
                    var i = networkList.currentIndex
                    if (i >= 0) {
                        networkModel.setProperty(i, "network", editNetwork.text)
                        networkModel.setProperty(i, "server", editServer.text)
                        networkModel.setProperty(i, "port", parseInt(editPort.text) || 6667)
                        networkModel.setProperty(i, "ssl", editSsl.checked)
                        networkModel.setProperty(i, "serverPass", editServerPass.text)
                        networkModel.setProperty(i, "saslMethod", editSasl.currentText)
                        networkModel.setProperty(i, "saslUser", editSaslUser.text)
                        networkModel.setProperty(i, "saslPass", editSaslPass.text)
                        networkModel.setProperty(i, "isZnc", editIsZnc.checked)
                        networkModel.setProperty(i, "zncUser", editZncUser.text)
                        networkModel.setProperty(i, "zncPass", editZncPass.text)
                        networkModel.setProperty(i, "zncNetwork", editZncNetwork.text)
                        networkModel.setProperty(i, "useGlobalNick", editUseGlobal.checked)
                        networkModel.setProperty(i, "customNick", editCustomNick.text)
                        networkModel.setProperty(i, "customUser", editCustomUser.text)
                        networkModel.setProperty(i, "customReal", editCustomReal.text)
                        dlg.saveNetworks()
                    }
                }
                background: Rectangle { color: parent.enabled ? (parent.down ? "#1177bb" : "#0e639c") : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: parent.enabled ? "#fff" : "#777"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }

            Item { Layout.fillWidth: true }

            Button {
                id: connectBtn
                text: "Connect"
                enabled: networkList.currentIndex >= 0
                onClicked: {
                    if (networkList.currentIndex >= 0) {
                        var nw = networkModel.get(networkList.currentIndex)
                        // Determine nick/user/realname: per-network or global
                        var useNick, useUser, useReal
                        if (nw.useGlobalNick || nw.customNick === "") {
                            useNick = nickField.text !== "" ? nickField.text : appSettings.value("user/nickname", "NUchat_user")
                        } else {
                            useNick = nw.customNick
                        }
                        if (nw.useGlobalNick || nw.customUser === "") {
                            useUser = appSettings.value("user/username", "nuchat")
                        } else {
                            useUser = nw.customUser
                        }
                        if (nw.useGlobalNick || nw.customReal === "") {
                            useReal = appSettings.value("user/realname", "NUchat User")
                        } else {
                            useReal = nw.customReal
                        }
                        // Build server password: ZNC format or plain
                        var serverPassword = ""
                        if (nw.isZnc && nw.zncUser !== "" && nw.zncPass !== "") {
                            if (nw.zncNetwork !== "") {
                                serverPassword = nw.zncUser + "/" + nw.zncNetwork + ":" + nw.zncPass
                            } else {
                                serverPassword = nw.zncUser + ":" + nw.zncPass
                            }
                        } else if (nw.serverPass !== "") {
                            serverPassword = nw.serverPass
                        }
                        ircManager.connectToServer(
                            nw.server,
                            nw.port,
                            nw.ssl,
                            useNick,
                            useUser,
                            useReal,
                            serverPassword
                        )
                        root.refreshChannelList()
                        dlg.close()
                    }
                }
                background: Rectangle { color: parent.enabled ? (parent.down ? "#28a745" : "#218838") : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: parent.enabled ? "#fff" : "#777"; font.pixelSize: 12; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
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
