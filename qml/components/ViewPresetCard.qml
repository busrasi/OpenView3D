import QtQuick
import QtQuick.Layouts

Rectangle {
    id: card

    property string presetName: ""
    property string iconSource: ""
    property bool selected: false

    signal clicked()

    Layout.fillWidth: true
    Layout.preferredHeight: 89

    radius: 10

    color: selected ? "#EEF2FF" : mouseArea.containsMouse ? "#F7F9FF" : "#FFFFFF"

    border.width: selected ? 2 : 1
    border.color: selected ? "#5E7BFF" : mouseArea.containsMouse ? "#AEBBFF" : "#D8DCE5"

    Column {
        anchors.centerIn: parent
        spacing: 8

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            source: card.iconSource
            sourceSize.width: 34
            sourceSize.height: 34
            width: 34
            height: 34
            fillMode: Image.PreserveAspectFit
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: card.presetName
            color: "#1D2433"
            font.pixelSize: 11
            font.weight: Font.Medium
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