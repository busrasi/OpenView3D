import QtQuick
import QtQuick.Layouts

Item {
    id: header

    property string title: ""
    property bool showDivider: false

    implicitHeight: 24

    RowLayout {
        anchors.fill: parent
        spacing: 8

        Text {
            text: header.title

            color: "#1D2433"

            font.pixelSize: 13
            font.weight: Font.DemiBold
        }

        Rectangle {
            visible: header.showDivider

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter

            height: 1

            color: "#E6E8EF"
        }
    }
}