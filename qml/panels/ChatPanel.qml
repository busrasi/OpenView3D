import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: panel
    objectName: "aiChatPanel"
    required property var controller
    color: "#FFFFFF"
    border.color: "#D8DCE5"
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10
        Label { text: "AI Assistant"; font.pixelSize: 18; font.bold: true }
        Label { text: panel.controller.providerName; color: "#465FC0" }
        Label {
            Layout.fillWidth: true
            text: "Selected: " + (panel.controller.selectedModelName || "No model selected")
            wrapMode: Text.WrapAnywhere
            textFormat: Text.PlainText
        }
        ListView {
            id: history
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 10
            model: panel.controller.messages
            onCountChanged: Qt.callLater(positionViewAtEnd)
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                required property var modelData
                width: history.width
                height: message.implicitHeight + 20
                radius: 6
                color: modelData.role === "user" ? "#EEF2FF" : "#F4F5F7"
                Label {
                    id: message
                    anchors.fill: parent
                    anchors.margins: 10
                    text: (modelData.role === "user" ? "You: " : "Assistant: ") + modelData.text
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                }
            }
        }
        Label {
            Layout.fillWidth: true
            visible: panel.controller.error.length > 0
            text: panel.controller.error
            textFormat: Text.PlainText
            wrapMode: Text.WrapAnywhere
            color: "#B42318"
        }
        RowLayout {
            visible: panel.controller.busy
            BusyIndicator { running: panel.controller.busy; Layout.preferredWidth: 26; Layout.preferredHeight: 26 }
            Label { text: "Working..." }
        }
        TextField {
            id: input
            objectName: "aiChatInput"
            Layout.fillWidth: true
            placeholderText: "Make a hole through the center"
            maximumLength: 4000
            enabled: !panel.controller.busy
            onAccepted: panel.submit()
        }
        Button {
            text: "Send"
            Layout.fillWidth: true
            enabled: !panel.controller.busy && input.text.trim().length > 0
            onClicked: panel.submit()
        }
    }
    function submit() {
        if (panel.controller.busy || !input.text.trim().length) return
        panel.controller.send(input.text)
        input.clear()
    }
}
