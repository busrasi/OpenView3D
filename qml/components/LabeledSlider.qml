import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ColumnLayout {
    id: sliderBlock

    property string label: ""
    property real valueNumber: 0
    property real fromValue: 0
    property real toValue: 100

    signal moved(real value)

    spacing: 6

    RowLayout {
        Layout.fillWidth: true

        Text {
            text: sliderBlock.label
            color: "#333846"
            font.pixelSize: 11
            font.weight: Font.Medium
            Layout.fillWidth: true
        }

        Text {
            text: Math.round(sliderBlock.valueNumber * 100) / 100
            color: "#667085"
            font.pixelSize: 11
        }
    }

    Slider {
        id: slider
        Layout.fillWidth: true
        from: sliderBlock.fromValue
        to: sliderBlock.toValue
        value: sliderBlock.valueNumber
        live: true

        onMoved: {

            sliderBlock.moved(value)
        }

        onPressedChanged: {
            if (!pressed) {

                sliderBlock.moved(value)
            }
        }

        background: Rectangle {
            x: slider.leftPadding
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            width: slider.availableWidth
            height: 3
            radius: 2
            color: "#D9DEE8"

            Rectangle {
                width: slider.visualPosition * parent.width
                height: parent.height
                radius: 2
                color: "#5E7BFF"
            }
        }

        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + slider.availableHeight / 2 - height / 2

            implicitWidth: 12
            implicitHeight: 12
            radius: 6
            color: slider.pressed ? "#405BFF" : "#5E7BFF"
            border.color: "#FFFFFF"
            border.width: 2

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.NoButton
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
            }
        }
    }
}
