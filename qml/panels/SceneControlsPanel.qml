import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import OpenView3D 1.0
import "../components"

Rectangle {
    id: panel

    property var appController
    property bool transformExpanded: false
    property string activePreset: "Front"

    signal requestModelDialog()
    signal requestTextureDialog()
    signal requestImageTo3DDialog()
    signal requestResetCamera()
    signal requestViewportFocus()
    signal presetSelected(string name, real rotationX, real rotationY)
    signal assetSelected(string assetPath, string assetType)
    signal dragStarted(string assetPath, string assetType)
    signal dragFinished()

    color: "#FAFBFC"
    border.color: "#E6E8EF"

    ScrollView {
        id: sceneScroll
        anchors.fill: parent
        clip: true

        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        contentWidth: sceneControlsColumn.width + 32

        ColumnLayout {
            id: sceneControlsColumn
            width: sceneScroll.width - 32
            x: 16
            y: 16
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
                onClicked: panel.requestModelDialog()
            }

            ToolButtonCard {
                Layout.fillWidth: true
                title: "Load Texture"
                subtitle: "PNG / JPG / BMP"
                iconSource: "qrc:/qt/qml/OpenView3D/qml/icons/texture.svg"
                onClicked: panel.requestTextureDialog()
            }

            GenerateModelCard {
                Layout.fillWidth: true
                onGenerateClicked: panel.requestImageTo3DDialog()
            }

            ToolButtonCard {
                Layout.fillWidth: true
                title: "Reset Camera"
                subtitle: "Default view"
                iconSource: "qrc:/qt/qml/OpenView3D/qml/icons/reset.svg"
                onClicked: panel.requestResetCamera()
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#E6E8EF"
            }

            SectionHeader {
                title: "View Presets"
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 8
                rowSpacing: 8

                Repeater {
                    model: [
                        { name: "Front", x: 0,   y: 0,   icon: "qrc:/qt/qml/OpenView3D/qml/icons/view_front.svg" },
                        { name: "Right", x: 0,   y: -90, icon: "qrc:/qt/qml/OpenView3D/qml/icons/view_right.svg" },
                        { name: "Top",   x: -90, y: 0,   icon: "qrc:/qt/qml/OpenView3D/qml/icons/view_top.svg" },
                        { name: "Iso",   x: -30, y: -45, icon: "qrc:/qt/qml/OpenView3D/qml/icons/view_iso.svg" }
                    ]

                    ViewPresetCard {
                        required property var modelData

                        presetName: modelData.name
                        iconSource: modelData.icon
                        selected: panel.activePreset === modelData.name

                        onClicked: {
                            panel.activePreset = modelData.name
                            panel.presetSelected(modelData.name, modelData.x, modelData.y)
                            panel.requestViewportFocus()
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#E6E8EF"
            }

            SectionHeader {
                title: "Assets"
            }

            AssetBrowser {
                Layout.fillWidth: true

                onAssetSelected: function(assetPath, assetType) {
                    panel.assetSelected(assetPath, assetType)
                }

                onDragStarted: function(assetPath, assetType) {
                    panel.dragStarted(assetPath, assetType)
                }

                onDragFinished: {
                    panel.dragFinished()
                }
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
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Text {
                        text: panel.transformExpanded ? "˄" : "˅"
                        color: "#5E7BFF"
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        Layout.preferredWidth: 20
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    }
                }

                MouseArea {
                    id: transformMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: panel.transformExpanded = !panel.transformExpanded
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 12
                visible: panel.transformExpanded

                LabeledSlider {
                    Layout.fillWidth: true
                    label: "Zoom"
                    fromValue: 0.2
                    toValue: 5.0
                    valueNumber: panel.appController ? panel.appController.zoom : 1.0
                    onMoved: function(value) {
                        if (panel.appController)
                            panel.appController.zoom = value
                    }
                }

                LabeledSlider {
                    Layout.fillWidth: true
                    label: "X Rotation"
                    fromValue: -180
                    toValue: 180
                    valueNumber: panel.appController ? panel.appController.rotationX : 0
                    onMoved: function(value) {
                        if (!panel.appController)
                            return

                        panel.activePreset = "Custom"
                        panel.appController.rotationX = value
                    }
                }

                LabeledSlider {
                    Layout.fillWidth: true
                    label: "Y Rotation"
                    fromValue: -180
                    toValue: 180
                    valueNumber: panel.appController ? panel.appController.rotationY : 0
                    onMoved: function(value) {
                        if (!panel.appController)
                            return

                        panel.activePreset = "Custom"
                        panel.appController.rotationY = value
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#E6E8EF"
            }

            SectionHeader {
                title: "Model Path"
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
                    text: panel.appController && panel.appController.modelPath.length > 0
                          ? panel.appController.modelPath
                          : "No model selected"
                    color: "#667085"
                    font.pixelSize: 10
                    wrapMode: TextEdit.WrapAnywhere
                    readOnly: true
                    selectByMouse: true
                }
            }

            Item {
                Layout.preferredHeight: 16
            }
        }
    }
}