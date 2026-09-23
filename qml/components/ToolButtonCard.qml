import QtQuick
import QtQuick.Layouts

Rectangle {
    id: card

    property string title: ""
    property string subtitle: ""
    property string iconSource: ""

    signal clicked()

    height: 42
    radius: 8

    color: mouseArea.containsMouse ? "#EEF2FF" : "#FFFFFF"

    border.color:
        mouseArea.containsMouse
        ? "#5E7BFF"
        : "#E3E6EC"

    border.width: 1

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 10

        Image {
            source: card.iconSource

            sourceSize.width: 18
            sourceSize.height: 18

            Layout.preferredWidth: 18
            Layout.preferredHeight: 18
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Text {
                text: card.title

                color: "#202430"
                font.pixelSize: 12
                font.weight: Font.DemiBold

                elide: Text.ElideRight

                Layout.fillWidth: true
            }

            Text {
                visible: card.subtitle.length > 0

                text: card.subtitle

                color: "#8A94A6"
                font.pixelSize: 10

                elide: Text.ElideRight

                Layout.fillWidth: true
            }
        }
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent

        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor

        onClicked: card.clicked()
    }
}