import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import OpenView3D 1.0

ApplicationWindow {
    id: root
    width: 1280
    height: 760
    visible: true
    title: "OpenView3D"

    property var appController
    property bool transformExpanded: false

    color: "#F7F8FA"

    FileDialog {
        id: modelDialog
        title: "Select OBJ Model"
        nameFilters: ["OBJ files (*.obj)", "All files (*)"]
        onAccepted: root.appController.loadModel(selectedFile.toString())
    }

    FileDialog {
        id: textureDialog
        title: "Select Texture"
        nameFilters: ["DDS (*.dds)", "BMP (*.bmp)", "All files (*)"]
        onAccepted: root.appController.loadTexture(selectedFile.toString())
    }

    component ToolButtonCard: Rectangle {
        id: card

        property string title: ""
        property string subtitle: ""
        property string iconSource: ""

        signal clicked()

        height: 42
        radius: 8
        color: mouseArea.containsMouse ? "#EEF2FF" : "#FFFFFF"
        border.color: mouseArea.containsMouse ? "#5E7BFF" : "#E3E6EC"
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

    component LabeledSlider: ColumnLayout {
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
                sliderBlock.valueNumber = value
                sliderBlock.moved(value)
            }

            onPressedChanged: {
                if (!pressed) {
                    sliderBlock.valueNumber = value
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

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: "#FFFFFF"
            border.color: "#E6E8EF"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 14

                Text {
                    text: "OpenView3D"
                    color: "#1D2433"
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                }

                Rectangle {
                    width: 1
                    height: 20
                    color: "#E6E8EF"
                }

                Text {
                    text: "Model Viewer / CAD Preview"
                    color: "#697386"
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }

                Rectangle {
                    radius: 12
                    height: 26
                    width: 84
                    color: "#EEF2FF"

                    Text {
                        anchors.centerIn: parent
                        text: "Design"
                        color: "#5E7BFF"
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.preferredWidth: 56
                Layout.fillHeight: true
                color: "#FFFFFF"
                border.color: "#E6E8EF"

                ColumnLayout {
                    anchors.top: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.topMargin: 14
                    spacing: 14

                    Repeater {
                        model: [
                            "qrc:/qt/qml/OpenView3D/qml/icons/cursor.svg",
                            "qrc:/qt/qml/OpenView3D/qml/icons/cube.svg",
                            "qrc:/qt/qml/OpenView3D/qml/icons/rotate.svg",
                            "qrc:/qt/qml/OpenView3D/qml/icons/zoom.svg"
                        ]

                        Rectangle {
                            width: 34
                            height: 34
                            radius: 8
                            color: index === 1 ? "#EEF2FF" : "transparent"
                            border.color: index === 1 ? "#5E7BFF" : "transparent"

                            Image {
                                anchors.centerIn: parent
                                source: modelData
                                sourceSize.width: 18
                                sourceSize.height: 18
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.preferredWidth: 256
                Layout.fillHeight: true
                color: "#FAFBFC"
                border.color: "#E6E8EF"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 14

                    Text {
                        text: "Scene Controls"
                        color: "#1D2433"
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }

                    Text {
                        text: "Load a model and adjust the camera."
                        color: "#7A8495"
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    ToolButtonCard {
                        Layout.fillWidth: true
                        title: "Load OBJ Model"
                        subtitle: "Choose mesh file"
                        iconSource: "qrc:/qt/qml/OpenView3D/qml/icons/cube.svg"
                        onClicked: modelDialog.open()
                    }

                    ToolButtonCard {
                        Layout.fillWidth: true
                        title: "Load Texture"
                        subtitle: "DDS or BMP"
                        iconSource: "qrc:/qt/qml/OpenView3D/qml/icons/texture.svg"
                        onClicked: textureDialog.open()
                    }

                    ToolButtonCard {
                        Layout.fillWidth: true
                        title: "Reset Camera"
                        subtitle: "Default view"
                        iconSource: "qrc:/qt/qml/OpenView3D/qml/icons/reset.svg"
                        onClicked: root.appController.resetCamera()
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#E6E8EF"
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        radius: 8
                        color: transformMouse.containsMouse ? "#EEF2FF" : "transparent"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 8

                            Text {
                                text: "Transform"
                                color: "#1D2433"
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                                Layout.fillWidth: true
                            }

                            Text {
                                text: root.transformExpanded ? "⌃" : "⌄"
                                color: "#5E7BFF"
                                font.pixelSize: 16
                                font.weight: Font.DemiBold
                            }
                        }

                        MouseArea {
                            id: transformMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.transformExpanded = !root.transformExpanded
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        visible: root.transformExpanded

                        LabeledSlider {
                            Layout.fillWidth: true
                            label: "Zoom"
                            fromValue: 0.2
                            toValue: 5.0
                            valueNumber: root.appController ? root.appController.zoom : 1.0

                            onMoved: function(value) {
                                root.appController.zoom = value
                            }
                        }

                        LabeledSlider {
                            Layout.fillWidth: true
                            label: "X Rotation"
                            fromValue: -180
                            toValue: 180
                            valueNumber: root.appController ? root.appController.rotationX : 0

                            onMoved: function(value) {
                                root.appController.rotationX = value
                            }
                        }

                        LabeledSlider {
                            Layout.fillWidth: true
                            label: "Y Rotation"
                            fromValue: -180
                            toValue: 180
                            valueNumber: root.appController ? root.appController.rotationY : 0

                            onMoved: function(value) {
                                root.appController.rotationY = value
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#E6E8EF"
                    }

                    Text {
                        text: "Model Path"
                        color: "#1D2433"
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 76
                        radius: 8
                        color: "#FFFFFF"
                        border.color: "#E3E6EC"

                        TextEdit {
                            anchors.fill: parent
                            anchors.margins: 10

                            text: root.appController && root.appController.modelPath.length > 0
                                  ? root.appController.modelPath
                                  : "No model selected"

                            color: "#667085"
                            font.pixelSize: 10
                            wrapMode: TextEdit.WrapAnywhere
                            readOnly: true
                            selectByMouse: true
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#F4F5F7"

                Rectangle {
                    id: viewportContainer
                    anchors.fill: parent
                    anchors.margins: 18
                    radius: 12
                    color: "#F4F4F4"
                    border.color: "#D8DCE5"
                    border.width: 1
                    clip: true

                    focus: true
                    activeFocusOnTab: true

                    Component.onCompleted: forceActiveFocus()

                    Keys.onPressed: function(event) {
                        if (!root.appController)
                            return

                        if (event.key === Qt.Key_Left) {
                            root.appController.rotationY -= 5
                            event.accepted = true
                        } else if (event.key === Qt.Key_Right) {
                            root.appController.rotationY += 5
                            event.accepted = true
                        } else if (event.key === Qt.Key_Up) {
                            root.appController.rotationX -= 5
                            event.accepted = true
                        } else if (event.key === Qt.Key_Down) {
                            root.appController.rotationX += 5
                            event.accepted = true
                        }
                    }

                    WheelHandler {
                        target: null

                        onWheel: function(event) {
                            if (!root.appController)
                                return

                            var step = event.angleDelta.y > 0 ? 0.1 : -0.1
                            var newZoom = root.appController.zoom + step
                            root.appController.zoom = Math.max(0.2, Math.min(5.0, newZoom))
                        }
                    }

                    OpenGLViewport {
                        id: viewport
                        anchors.fill: parent

                        modelPath: root.appController ? root.appController.modelPath : ""
                        texturePath: root.appController ? root.appController.texturePath : ""

                        zoom: root.appController ? root.appController.zoom : 1.0
                        rotationX: root.appController ? root.appController.rotationX : 0
                        rotationY: root.appController ? root.appController.rotationY : 0
                    }

                    TapHandler {
                        onTapped: viewportContainer.forceActiveFocus()
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.margins: 12
                        width: 160
                        height: 28
                        radius: 14
                        color: "#CCFFFFFF"
                        border.color: "#E3E6EC"

                        Text {
                            anchors.centerIn: parent
                            text: "CAD View"
                            color: "#333846"
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                        }
                    }
                }
            }
        }
    }
}