import QtQuick 2.15
import QtQuick.Controls 2.15

Dialog {
    id: dialog
    title: "Settings"
    width: 420
    height: 260
    modal: true
    anchors.centerIn: parent

    background: Rectangle {
        color: "#2b2b2b"
        border.color: "#555"
        border.width: 1
        radius: 6
    }

    header: Rectangle {
        height: 36
        color: "#252526"
        radius: 6
        // square off bottom corners
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 6; color: "#252526" }
        Text {
            anchors.centerIn: parent
            text: "Settings"
            color: "#ddd"
            font.pixelSize: 14
            font.bold: true
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Text {
            text: "Theme"
            color: "#ccc"
            font.bold: true
            font.pixelSize: 13
        }
        ComboBox {
            id: themeCombo
            width: parent.width
            model: themeManager.availableThemes()
            onCurrentTextChanged: {
                if (currentText !== "")
                    themeManager.loadTheme(":/themes/" + currentText)
            }
        }

        Item { width: 1; height: 20 }

        Button {
            text: "Close"
            anchors.right: parent.right
            onClicked: dialog.close()
            background: Rectangle {
                color: parent.down ? "#1177bb" : "#0e639c"
                radius: 3
            }
            contentItem: Text {
                text: "Close"
                color: "#fff"
                font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}
