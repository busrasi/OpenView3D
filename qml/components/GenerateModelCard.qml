import QtQuick
import QtQuick.Layouts

Rectangle {
    id: card

    signal generateClicked()

    Layout.fillWidth: true
    Layout.preferredHeight: 42

    radius: 10
    color: mouseArea.containsMouse ? "#EEF2FF" : "#FFFFFF"
    border.color: mouseArea.containsMouse ? "#5E7BFF" : "#D8DCE5"
    border.width: 1

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 9

        Text {
            text: "✨"
            font.pixelSize: 14
            Layout.preferredWidth: 20
            horizontalAlignment: Text.AlignHCenter
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Text {
                text: "Image to 3D Model"
                color: "#1D2433"
                font.pixelSize: 12
                font.weight: Font.DemiBold
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Text {
                text: "Upload image and generate OBJ"
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

        onClicked: card.generateClicked()
    }
}