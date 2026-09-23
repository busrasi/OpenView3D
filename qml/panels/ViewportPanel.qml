import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenView3D 1.0

Rectangle {
    id: panel

    property var appController
    property real viewportPanX: 0.0
    property real viewportPanY: 0.0
    property string draggedAssetPath: ""
    property string draggedAssetType: ""

    signal panChanged(real panX, real panY)
    signal activePresetChanged(string presetName)

    color: "#F4F5F7"

    Connections {
        target: panel.appController

        function onViewCountChanged() {
            Qt.callLater(function() {
                panel.forceViewportFocus()
            })
        }

        function onActiveViewIndexChanged() {
            Qt.callLater(function() {
                panel.forceViewportFocus()
            })
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 8

        Rectangle {
            id: viewportContainer

            Layout.fillWidth: true
            Layout.fillHeight: true

            radius: 12
            color: "#F4F4F4"
            border.color: "#D8DCE5"
            border.width: 1
            clip: true

            focus: true
            activeFocusOnTab: true

            Component.onCompleted: forceActiveFocus()

            Keys.onPressed: function(event) {
                if (!panel.appController)
                    return

                panel.activePresetChanged("Custom")

                if (event.key === Qt.Key_Left) {
                    panel.appController.rotationY -= 5
                    event.accepted = true
                } else if (event.key === Qt.Key_Right) {
                    panel.appController.rotationY += 5
                    event.accepted = true
                } else if (event.key === Qt.Key_Up) {
                    panel.appController.rotationX -= 5
                    event.accepted = true
                } else if (event.key === Qt.Key_Down) {
                    panel.appController.rotationX += 5
                    event.accepted = true
                }
            }

            WheelHandler {
                target: null

                onWheel: function(event) {
                    if (!panel.appController)
                        return

                    var step = event.angleDelta.y > 0 ? 0.1 : -0.1
                    var newZoom = panel.appController.zoom + step
                    panel.appController.zoom = Math.max(0.2, Math.min(5.0, newZoom))
                }
            }

            OpenGLViewport {
                id: viewport
                anchors.fill: parent

                modelPath: panel.appController ? panel.appController.modelPath : ""
                texturePath: panel.appController ? panel.appController.texturePath : ""

                zoom: panel.appController ? panel.appController.zoom : 1.0
                rotationX: panel.appController ? panel.appController.rotationX : 0
                rotationY: panel.appController ? panel.appController.rotationY : 0

                panX: panel.viewportPanX
                panY: panel.viewportPanY
            }

            DragHandler {
                id: objectDragHandler
                target: null
                acceptedButtons: Qt.LeftButton

                property real startPanX: 0.0
                property real startPanY: 0.0

                onActiveChanged: {
                    if (active) {
                        viewportContainer.forceActiveFocus()
                        startPanX = panel.viewportPanX
                        startPanY = panel.viewportPanY
                    }
                }

                onTranslationChanged: {
                    if (!active)
                        return

                    const newPanX = startPanX + translation.x * 0.005
                    const newPanY = startPanY - translation.y * 0.005

                    panel.panChanged(newPanX, newPanY)
                }
            }

            HoverHandler {
                id: viewportHover
                cursorShape: objectDragHandler.active
                             ? Qt.ClosedHandCursor
                             : Qt.OpenHandCursor
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
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                hoverEnabled: true

                property bool dragInsideViewport: false

                onContainsMouseChanged: {
                    dragInsideViewport = containsMouse

                    if (containsMouse && panel.draggedAssetPath.length > 0) {
                        console.log("DRAG OVER VIEWPORT:", panel.draggedAssetPath, panel.draggedAssetType)
                    }
                }

                onReleased: {
                    if (!dragInsideViewport)
                        return

                    if (!panel.appController)
                        return

                    if (panel.draggedAssetPath.length === 0)
                        return

                    console.log("DROP ON VIEWPORT:", panel.draggedAssetPath, panel.draggedAssetType)

                    if (panel.draggedAssetType === "model") {
                        panel.appController.loadModel(panel.draggedAssetPath)
                    } else if (panel.draggedAssetType === "texture") {
                        panel.appController.loadTexture(panel.draggedAssetPath)
                    }
                }
            }
        }

        Rectangle {
            id: viewTabBar

            Layout.fillWidth: true
            Layout.preferredHeight: 34

            radius: 10
            color: "#00FFFFFF"
            border.color: "#E6E8EF"
            border.width: 1
            clip: true

            RowLayout {
                anchors.fill: parent
                spacing: 0

                Repeater {
                    model: panel.appController ? panel.appController.viewCount : 1

                    Rectangle {
                        id: tabItem

                        Layout.preferredWidth: Math.max(
                            96,
                            Math.min(
                                136,
                                (viewTabBar.width - 44)
                                / Math.max(1, panel.appController ? panel.appController.viewCount : 1)
                            )
                        )

                        Layout.fillHeight: true
                        radius: 0

                        property bool selected: panel.appController
                                                && index === panel.appController.activeViewIndex
                        property bool hovered: tabMouse.containsMouse

                        color: selected ? "#F7F9FF"
                              : hovered ? "#F5F7FB"
                              : "transparent"

                        // Main tab click layer stays behind the close button.
                        MouseArea {
                            id: tabMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            z: 0

                            onClicked: {
                                if (panel.appController)
                                    panel.appController.activeViewIndex = index

                                panel.forceViewportFocus()
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 8
                            spacing: 8
                            z: 2

                            Text {
                                text: "View " + (index + 1)

                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter

                                color: tabItem.selected ? "#2F5BFF" : "#4B5565"
                                font.pixelSize: 11
                                font.weight: tabItem.selected ? Font.DemiBold : Font.Medium
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }

                            Rectangle {
                                id: closeButton

                                visible: panel.appController
                                         && panel.appController.viewCount > 1

                                Layout.preferredWidth: 18
                                Layout.preferredHeight: 18
                                Layout.alignment: Qt.AlignVCenter

                                radius: 9
                                color: closeMouse.containsMouse ? "#E8EDFF" : "transparent"
                                border.color: closeMouse.containsMouse ? "#B8C4FF" : "transparent"
                                border.width: 1
                                z: 10

                                Text {
                                    anchors.centerIn: parent
                                    text: "×"
                                    color: closeMouse.containsMouse ? "#2F5BFF" : "#8A94A6"
                                    font.pixelSize: 13
                                    font.weight: Font.DemiBold
                                    y: -1
                                }

                                MouseArea {
                                    id: closeMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    z: 20

                                    onClicked: function(mouse) {
                                        mouse.accepted = true

                                        const controller = panel.appController
                                        const viewIndex = index

                                        if (controller && controller.viewCount > 1) {
                                            console.log("Close view clicked:", viewIndex)
                                            controller.closeView(viewIndex)
                                        }

                                        // Do not force focus here.
                                        // Closing a tab destroys/rebuilds the delegate, so calling panel/viewport
                                        // after closeView() can hit a destroyed QML scope.
                                    }
                                }
                            }
                        }

                        Rectangle {
                            visible: tabItem.selected

                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12

                            height: 2
                            radius: 10
                            color: "#2F5BFF"
                            z: 3
                        }
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 42
                    Layout.fillHeight: true
                    color: "transparent"

                    Rectangle {
                        anchors.centerIn: parent

                        width: 28
                        height: 28
                        radius: 10

                        visible: panel.appController
                                 ? panel.appController.viewCount < 15
                                 : true

                        color: addViewMouse.containsMouse ? "#EEF2FF" : "#FFFFFF"
                        border.color: addViewMouse.containsMouse ? "#5E7BFF" : "#D8DCE5"
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: "+"
                            color: "#4F6BFF"
                            font.pixelSize: 18
                            font.weight: Font.Medium
                            y: -1
                        }

                        MouseArea {
                            id: addViewMouse

                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor

                            onClicked: {
                                if (panel.appController)
                                    panel.appController.addView()

                                panel.forceViewportFocus()
                            }
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                }
            }
        }    }

    function forceViewportFocus() {
        viewportContainer.forceActiveFocus()
    }
}