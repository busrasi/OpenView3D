import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import OpenView3D 1.0
import "components"
import "panels"

ApplicationWindow {
    id: root

    width: 1280
    height: 760
    visible: true
    title: "OpenView3D"
    color: "#F7F8FA"

    property var appController
    property bool transformExpanded: false
    property real viewportPanX: 0.0
    property real viewportPanY: 0.0
    property string activePreset: "Front"
    property string draggedAssetPath: ""
    property string draggedAssetType: ""

    FileDialog {
        id: modelDialog
        title: "Select OBJ Model"
        fileMode: FileDialog.OpenFile
        nameFilters: ["OBJ files (*.obj)", "All files (*)"]

        onAccepted: {
            if (root.appController)
                root.appController.loadModel(selectedFile.toString())
        }
    }

    FileDialog {
        id: textureDialog
        title: "Select Texture"
        fileMode: FileDialog.OpenFile
        nameFilters: [
            "Images (*.png *.jpg *.jpeg *.bmp)",
            "DDS (*.dds)",
            "All files (*)"
        ]

        onAccepted: {
            if (root.appController)
                root.appController.loadTexture(selectedFile.toString())
        }
    }

    FileDialog {
        id: imageTo3DDialog
        title: "Select Image for 3D Generation"
        fileMode: FileDialog.OpenFile
        nameFilters: [
            "Images (*.png *.jpg *.jpeg *.bmp *.webp)",
            "All files (*)"
        ]

        onAccepted: {
            if (root.appController)
                root.appController.generateModel(selectedFile.toString())
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

            SceneControlsPanel {
                id: sceneControlsPanel

                Layout.preferredWidth: 256
                Layout.fillHeight: true

                appController: root.appController
                activePreset: root.activePreset
                transformExpanded: root.transformExpanded

                onRequestModelDialog: modelDialog.open()
                onRequestTextureDialog: textureDialog.open()
                onRequestImageTo3DDialog: imageTo3DDialog.open()

                onRequestResetCamera: {
                    if (!root.appController)
                        return

                    root.appController.resetCamera()
                    root.viewportPanX = 0.0
                    root.viewportPanY = 0.0
                    root.activePreset = "Front"
                    root.transformExpanded = false
                }

                onPresetSelected: function(name, rotationX, rotationY) {
                    root.activePreset = name

                    if (root.appController) {
                        root.appController.rotationX = rotationX
                        root.appController.rotationY = rotationY
                    }
                }

                onRequestViewportFocus: {
                    viewportPanel.forceViewportFocus()
                }

                onTransformExpandedChanged: {
                    root.transformExpanded = transformExpanded
                }

                onDragStarted: function(path, type) {
                    console.log("MAIN DRAG START:", path, type)
                    root.draggedAssetPath = path
                    root.draggedAssetType = type
                }

                onDragFinished: {
                    console.log("MAIN DRAG FINISHED")
                }
            }

            ViewportPanel {
                id: viewportPanel

                Layout.fillWidth: true
                Layout.fillHeight: true

                appController: root.appController
                viewportPanX: root.viewportPanX
                viewportPanY: root.viewportPanY
                draggedAssetPath: root.draggedAssetPath
                draggedAssetType: root.draggedAssetType

                onPanChanged: function(panX, panY) {
                    root.viewportPanX = panX
                    root.viewportPanY = panY
                }

                onActivePresetChanged: function(presetName) {
                    root.activePreset = presetName
                }
            }
        }
    }
}