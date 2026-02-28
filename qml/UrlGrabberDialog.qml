import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: dlg
    title: "URL Grabber"
    width: 560
    height: 380
    modal: true
    anchors.centerIn: parent

    background: Rectangle { color: "#2b2b2b"; border.color: "#555"; border.width: 1; radius: 6 }
    header: Rectangle {
        height: 36; color: "#252526"; radius: 6
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 6; color: "#252526" }
        Text { anchors.centerIn: parent; text: "URL Grabber"; color: "#ddd"; font.pixelSize: 14; font.bold: true }
    }

    onOpened: urlList.refreshUrls()

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 12; spacing: 8

        Rectangle {
            Layout.fillWidth: true; height: 24; color: "#252526"
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                Text { text: "URL"; color: "#aaa"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true }
                Text { text: "Nick"; color: "#aaa"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 100 }
                Text { text: "Channel"; color: "#aaa"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 100 }
            }
        }

        ListView {
            id: urlList
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true; boundsBehavior: Flickable.StopAtBounds
            model: ListModel {
                id: urlModel
            }
            // Load grabbed URLs when dialog opens
            Component.onCompleted: refreshUrls()
            function refreshUrls() {
                urlModel.clear()
                var urls = ircManager.grabbedUrls()
                for (var i = 0; i < urls.length; i++) {
                    urlModel.append({url: urls[i].url, nick: urls[i].nick, channel: urls[i].channel})
                }
            }
            delegate: Rectangle {
                required property int index
                required property string url
                required property string nick
                required property string channel
                width: urlList.width; height: 24
                color: urlList.currentIndex === index ? "#264f78" : (index % 2 === 0 ? "#2b2b2b" : "#2e2e2e")
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                    Text { text: url; color: "#569cd6"; font.pixelSize: 12; Layout.fillWidth: true; elide: Text.ElideRight }
                    Text { text: nick; color: "#ccc"; font.pixelSize: 12; Layout.preferredWidth: 100 }
                    Text { text: channel; color: "#6a9955"; font.pixelSize: 12; Layout.preferredWidth: 100 }
                }
                MouseArea {
                    anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: urlList.currentIndex = index
                    onDoubleClicked: Qt.openUrlExternally(url)
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true; spacing: 8
            Text { text: urlModel.count + " URLs captured"; color: "#888"; font.pixelSize: 11 }
            Item { Layout.fillWidth: true }
            Button {
                text: "Open"
                enabled: urlList.currentIndex >= 0
                onClicked: if (urlList.currentIndex >= 0) Qt.openUrlExternally(urlModel.get(urlList.currentIndex).url)
                background: Rectangle { color: parent.enabled ? (parent.down ? "#1177bb" : "#0e639c") : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: parent.enabled ? "#fff" : "#777"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Clear"
                onClicked: { ircManager.clearGrabbedUrls(); urlModel.clear() }
                background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
            Button {
                text: "Close"; onClicked: dlg.close()
                background: Rectangle { color: parent.down ? "#555" : "#444"; radius: 3 }
                contentItem: Text { text: parent.text; color: "#ccc"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }
    }
}
