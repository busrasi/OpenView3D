import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    id: browser

    property bool expanded: true

    signal assetSelected(string assetPath, string assetType)
    signal dragStarted(string assetPath, string assetType)
    signal dragFinished()

    Layout.fillWidth: true
    Layout.preferredHeight: expanded ? 190 : 42

    radius: 10
    color: "#FFFFFF"
    border.color: "#D8DCE5"
    border.width: 1
    clip: true

    Column {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        Rectangle {
            width: parent.width
            height: 26
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                spacing: 8

                Text {
                    text: browser.expanded ? "Ë„" : "Ë…"
                    color: "#5E7BFF"
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    Layout.preferredWidth: 18
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                Text {
                    text: "Project Assets"
                    color: "#1D2433"
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    Layout.fillWidth: true
                    verticalAlignment: Text.AlignVCenter
                }

                Text {
                    text: "local"
                    color: "#8A94A6"
                    font.pixelSize: 10
                    verticalAlignment: Text.AlignVCenter
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: browser.expanded = !browser.expanded
            }
        }

        Rectangle {
            visible: browser.expanded
            width: parent.width
            height: 1
            color: "#EEF0F4"
        }

        ListView {
            visible: browser.expanded
            width: parent.width
            height: 136
            clip: true
            spacing: 4

            model: [
                {
                    name: "vase.obj",
                    type: "model",
                    path: "qrc:/qt/qml/OpenView3D/resources/model_vase/model.obj"
                },
                {
                    name: "capsule.obj",
                    type: "model",
                    path: "qrc:/qt/qml/OpenView3D/resources/models/capsule.obj"
                },
                {
                    name: "texture_0.png",
                    type: "texture",
                    path: "qrc:/qt/qml/OpenView3D/resources/model_vase/texture_0.png"
                },
                {
                    name: "capsule0.png",
                    type: "texture",
                    path: "qrc:/qt/qml/OpenView3D/resources/models/capsule0.png"
                }
            ]
            delegate: Item {
                id: assetItem

                required property var modelData

                width: ListView.view.width
                height: 32

                Rectangle {
                    id: assetRow
                    anchors.fill: parent
                    radius: 7

                    color: assetMouse.containsMouse ? "#F5F8FF" : "transparent"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        spacing: 8

                        Rectangle {
                            Layout.preferredWidth: 22
                            Layout.preferredHeight: 22
                            radius: 6

                            color: modelData.type === "model" ? "#EEF2FF" : "#F5F7FB"
                            border.color: modelData.type === "model" ? "#C9D2FF" : "#D8DCE5"

                            Text {
                                anchors.centerIn: parent
                                text: modelData.type === "model" ? "M" : "T"
                                color: modelData.type === "model" ? "#5E7BFF" : "#667085"
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            Text {
                                text: modelData.name
                                color: "#333846"
                                font.pixelSize: 11
                                font.weight: Font.Medium
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Text {
                                text: modelData.type === "model" ? "Mesh Asset" : "Texture Asset"
                                color: "#8A94A6"
                                font.pixelSize: 9
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }
                    }
                }

                Rectangle {
                    id: dragGhost

                    width: assetRow.width
                    height: assetRow.height
                    radius: 8

                    color: "#FFFFFF"
                    border.color: "#5E7BFF"
                    border.width: 1

                    visible: false
                    opacity: 0.92

                    Text {
                        anchors.centerIn: parent
                        text: modelData.name
                        color: "#1D2433"
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                    }

                    Drag.active: assetMouse.drag.active
                    Drag.hotSpot.x: width / 2
                    Drag.hotSpot.y: height / 2

                    Drag.mimeData: {
                        "assetPath": modelData.path,
                        "assetType": modelData.type
                    }
                }

                MouseArea {
                    id: assetMouse

                    anchors.fill: parent
                    hoverEnabled: true

                    drag.target: dragGhost
                    drag.axis: Drag.XAndYAxis
                    drag.threshold: 6

                    cursorShape: drag.active ? Qt.ClosedHandCursor : Qt.PointingHandCursor

                    onPressed: {
                        dragGhost.visible = true
                    }

                    onReleased: {
                        dragGhost.visible = false
                    }

                    onClicked: {
                        browser.assetSelected(modelData.path, modelData.type)
                    }
                }
            }
        }
    }
}