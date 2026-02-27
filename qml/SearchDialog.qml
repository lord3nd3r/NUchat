import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: dlg
    title: "Search Text"
    width: 400
    height: 160
    modal: true
    anchors.centerIn: parent

    background: Rectangle { color: "#2b2b2b"; border.color: "#555"; border.width: 1; radius: 6 }
    header: Rectangle {
        height: 36; color: "#252526"; radius: 6
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 6; color: "#252526" }
        Text { anchors.centerIn: parent; text: "Search Text"; color: "#ddd"; font.pixelSize: 14; font.bold: true }
    }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 16; spacing: 10

        RowLayout {
            Layout.fillWidth: true; spacing: 8
            TextField {
                id: searchField; Layout.fillWidth: true
                placeholderText: "Search..."; placeholderTextColor: "#666"
                color: "#ddd"; font.pixelSize: 12
                background: Rectangle { color: "#333"; border.color: "#555"; radius: 2 }
                onAccepted: findBtn.clicked()
            }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: 12
            CheckBox {
                id: caseSensitive
                indicator: Rectangle { width: 16; height: 16; radius: 2; color: caseSensitive.checked ? "#0e639c" : "#333"; border.color: "#555"; Text { anchors.centerIn: parent; text: caseSensitive.checked ? "✓" : ""; color: "#fff"; font.pixelSize: 11 } }
                contentItem: Text { text: " Case sensitive"; color: "#ccc"; font.pixelSize: 12; leftPadding: caseSensitive.indicator.width + 6 }
            }
            CheckBox {
                id: regexSearch
                indicator: Rectangle { width: 16; height: 16; radius: 2; color: regexSearch.checked ? "#0e639c" : "#333"; border.color: "#555"; Text { anchors.centerIn: parent; text: regexSearch.checked ? "✓" : ""; color: "#fff"; font.pixelSize: 11 } }
                contentItem: Text { text: " Regex"; color: "#ccc"; font.pixelSize: 12; leftPadding: regexSearch.indicator.width + 6 }
            }
            CheckBox {
                id: searchBackwards
                indicator: Rectangle { width: 16; height: 16; radius: 2; color: searchBackwards.checked ? "#0e639c" : "#333"; border.color: "#555"; Text { anchors.centerIn: parent; text: searchBackwards.checked ? "✓" : ""; color: "#fff"; font.pixelSize: 11 } }
                contentItem: Text { text: " Backwards"; color: "#ccc"; font.pixelSize: 12; leftPadding: searchBackwards.indicator.width + 6 }
            }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: 8
            Item { Layout.fillWidth: true }
            Button {
                id: findBtn; text: "Find"
                onClicked: {
                    var term = searchField.text
                    if (term === "") return
                    var allText = chatArea.text
                    var haystack = caseSensitive.checked ? allText : allText.toLowerCase()
                    var needle = caseSensitive.checked ? term : term.toLowerCase()
                    var pos = haystack.lastIndexOf(needle)
                    if (pos >= 0) {
                        chatArea.select(pos, pos + term.length)
                        chatArea.forceActiveFocus()
                    } else {
                        msgModel.addMessage("system", "No results found for: " + term)
                    }
                    dlg.close()
                }
                background: Rectangle { color: parent.down ? "#1177bb" : "#0e639c"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#fff"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Close"; onClicked: dlg.close()
                background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }
    }
}
