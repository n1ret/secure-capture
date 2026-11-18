import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid
import org.kde.plasma.workspace.dbus

PlasmoidItem {
    id: root

    readonly property string serviceName: "org.kde.KWin"
    readonly property string objectPath: "/SecureCapture"
    readonly property string interfaceName: "org.kde.SecureCapture"
    readonly property bool kwinAvailable: serviceWatcher.registered
    readonly property var backendValues: backendProperties.properties
    readonly property bool backendAvailable: kwinAvailable && Boolean(backendValues.backendAvailable)
    readonly property bool currentActive: kwinAvailable && Boolean(backendValues.active)
    readonly property bool manualEnabled: kwinAvailable && Boolean(backendValues.manualEnabled)
    readonly property bool manualDisabled: kwinAvailable && Boolean(backendValues.manualDisabled)
    readonly property bool protectDesktop: kwinAvailable && Boolean(backendValues.protectDesktop)
    readonly property bool protectPanels: kwinAvailable && Boolean(backendValues.protectPanels)
    readonly property bool protectTabSwitcher: kwinAvailable && Boolean(backendValues.protectTabSwitcher)
    property string patternText
    property string triggerText
    property bool nextTriggerEnables: true

    function send(member, args) {
        if (!backendAvailable) {
            console.warn("SecureCapture backend unavailable");
            return;
        }

        const pending = SessionBus.asyncCall({
            "service": root.serviceName,
            "path": root.objectPath,
            "iface": root.interfaceName,
            "member": member,
            "arguments": args
        });

        pending.finished.connect(() => {
            if (pending.isError) {
                const error = pending.error

                console.error(
                    "D-Bus call failed:", member,
                    "\nName:", error.name,
                    "\nMessage:", error.message
                )
            }

            pending.destroy();
        });
    }

    function setProperty(member, value) {
        send(member, [value]);
    }

    function setEnabled(value) {
        setProperty("setEnabled", value);
    }

    function setDisabled(value) {
        setProperty("setDisabled", value);
    }

    function addPattern(text) {
        const value = text.trim();
        if (!value)
            return false;

        const values = (backendValues.allowlist || []).slice();
        if (!values.includes(value)) {
            values.push(value);
            setProperty("setAllowlist", values);
        }
        return true;
    }

    function removePattern(index) {
        const values = (backendValues.allowlist || []).slice();
        values.splice(index, 1);
        setProperty("setAllowlist", values);
    }

    function addTrigger(text) {
        const value = text.trim();
        if (!value)
            return false;

        const values = (backendValues.triggers || []).slice();
        const rule = value + "\t" + (nextTriggerEnables ? "enable" : "disable");
        if (!values.includes(rule)) {
            values.push(rule);
            setProperty("setTriggers", values);
        }
        return true;
    }

    function removeTrigger(index) {
        const values = (backendValues.triggers || []).slice();
        values.splice(index, 1);
        setProperty("setTriggers", values);
    }

    DBusServiceWatcher {
        id: serviceWatcher

        watchedService: root.serviceName
    }

    Properties {
        id: backendProperties

        busType: BusType.Session
        service: root.serviceName
        path: root.objectPath
        iface: root.interfaceName
    }

    Connections {
        function onRegisteredChanged() {
            if (serviceWatcher.registered)
                backendProperties.updateAll();
        }

        target: serviceWatcher
    }

    compactRepresentation: Item {
        readonly property int iconMargin: -3

        Kirigami.Icon {
            anchors {
                top: parent.top
                topMargin: iconMargin
                left: parent.left
                leftMargin: iconMargin
                right: parent.right
                rightMargin: iconMargin
            }
            source: "preferences-desktop-screensaver"
            opacity: root.backendAvailable ? 1 : 0.45
        }

        Rectangle {
            anchors {
                top: icon.bottom
                bottom: parent.bottom
                horizontalCenter: parent.horizontalCenter
            }

            width: parent.width * 0.8
            height: 2
            radius: 2

            color: !root.backendAvailable ? "#808080"
                : root.currentActive ? "#35c759"
                : "#ff453a"
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent

            acceptedButtons: Qt.LeftButton | Qt.RightButton

            onClicked: mouse => {
                if (mouse.button === Qt.LeftButton) {
                    root.expanded = !root.expanded
                } else if (mouse.button === Qt.RightButton) {
                    if (root.backendAvailable) {
                        root.setEnabled(!root.manualEnabled)
                    }
                }
            }
        }
    }

    fullRepresentation: QQC2.Pane { padding: Kirigami.Units.gridUnit;
                                    topPadding: 0; bottomPadding: 0;
                                    contentItem: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        Header {
            text: i18n("Status: ") + (
                !root.backendAvailable ? i18n("Backend unavailable") :
                root.manualDisabled ? i18n("Disabled manually") :
                root.manualEnabled ? i18n("Enabled manually") :
                root.currentActive ? i18n("Enabled automatically") :
                i18n("Disabled")
            )
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Row {
            spacing: Kirigami.Units.largeSpacing
            anchors.horizontalCenter: parent.horizontalCenter

            QQC2.CheckBox {
                text: i18n("Enabled")
                enabled: root.backendAvailable
                checked: root.manualEnabled
                onClicked: {
                    root.setEnabled(checked)
                    if (checked) root.setDisabled(false)
                }
            }

            QQC2.CheckBox {
                text: i18n("Force disabled")
                enabled: root.backendAvailable
                checked: root.manualDisabled
                onClicked: {    
                    root.setDisabled(checked)
                    if (checked) root.setEnabled(false)
                }
            }
        }

        RowLayout {
            anchors {
                left: parent.left
                right: parent.right
                leftMargin: parent.padding
                rightMargin: parent.padding
            }

            QQC2.CheckBox {
                text: i18n("Desktop")
                enabled: root.backendAvailable
                checked: !root.protectDesktop
                onToggled: root.setProperty("setProtectDesktop", !checked)
            }

            Item { Layout.fillWidth: true }

            QQC2.CheckBox {
                text: i18n("Plasma panels")
                enabled: root.backendAvailable
                checked: !root.protectPanels
                onToggled: root.setProperty("setProtectPanels", !checked)
            }

            Item { Layout.fillWidth: true }

            QQC2.CheckBox {
                text: i18n("Tab Switcher")
                enabled: root.backendAvailable
                checked: !root.protectTabSwitcher
                onToggled: root.setProperty("setProtectTabSwitcher", !checked)
            }
        }

        Separator {}

        Label {
            text: i18n("Allowlisted application patterns")
        }

        RowLayout {
            Layout.fillWidth: true

            IdentifierPicker {
                target: patternField
                available: root.kwinAvailable
            }

            QQC2.TextField {
                id: patternField

                function submit() {
                    if (root.addPattern(text))
                        text = "";
                }

                Layout.fillWidth: true
                enabled: root.backendAvailable
                placeholderText: i18n("org.example.App or *browser*")
                onAccepted: submit()
            }

            SquareButton {
                text: "+"
                enabled: root.backendAvailable && patternField.text.trim().length > 0
                onClicked: patternField.submit()
            }
        }

        ScrollableList {
            Layout.preferredHeight: 2

            Repeater {
                model: root.backendValues.allowlist || []

                delegate: RowLayout {
                    Rectangle {
                        Layout.fillWidth: true
                        height: removeButton.implicitHeight
                        radius: Kirigami.Units.cornerRadius
                        color: Kirigami.Theme.alternateBackgroundColor

                        QQC2.Label {
                            text: modelData
                            elide: Text.ElideRight

                            anchors {
                                left: parent.left
                                right: parent.right
                                leftMargin: Kirigami.Units.largeSpacing
                                rightMargin: Kirigami.Units.smallSpacing
                                verticalCenter: parent.verticalCenter
                            }

                            QQC2.ToolTip.text: text
                            QQC2.ToolTip.visible: truncated && hoverHandler.hovered
                            HoverHandler { id: hoverHandler }
                        }

                        TapHandler {
                            onTapped: {
                                patternField.text = modelData
                                patternField.forceActiveFocus()

                                root.removePattern(index)
                            }
                        }
                    }

                    SquareButton {
                        id: removeButton
                        text: "×"
                        onClicked: root.removePattern(index)
                    }
                }
            }
        }

        Separator {}

        Label {
            text: i18n("Automatic activation rules")
        }

        RowLayout {
            Layout.fillWidth: true

            IdentifierPicker {
                target: triggerField
                available: root.kwinAvailable
            }

            SquareButton {
                Layout.preferredHeight: triggerField.implicitHeight

                text: root.nextTriggerEnables ? "🟢" : "🔴"
                onClicked: root.nextTriggerEnables = !root.nextTriggerEnables
            }

            QQC2.TextField {
                id: triggerField

                function submit() {
                    if (root.addTrigger(text))
                        text = "";
                }

                Layout.fillWidth: true
                enabled: root.backendAvailable
                placeholderText: i18n("Application pattern")
                onAccepted: submit()
            }

            SquareButton {
                text: "+"
                enabled: root.backendAvailable && triggerField.text.trim().length > 0
                onClicked: triggerField.submit()
            }
        }

        ScrollableList {
            Layout.preferredHeight: 1

            Repeater {
                model: root.backendValues.triggers || []

                delegate: RowLayout {
                    Rectangle {
                        Layout.fillWidth: true
                        height: removeButton.implicitHeight
                        radius: Kirigami.Units.cornerRadius
                        color: Kirigami.Theme.alternateBackgroundColor

                        QQC2.Label {
                            text: {
                                let [pattern, state] = modelData.split("\t");
                                return (state === "enable" ? "🟢" : "🔴") + "   " + pattern;
                            }
                            elide: Text.ElideRight

                            anchors {
                                left: parent.left
                                right: parent.right
                                leftMargin: Kirigami.Units.largeSpacing
                                rightMargin: Kirigami.Units.smallSpacing
                                verticalCenter: parent.verticalCenter
                            }

                            QQC2.ToolTip.text: text
                            QQC2.ToolTip.visible: truncated && hoverHandler.hovered
                            HoverHandler { id: hoverHandler }
                        }

                        TapHandler {
                            onTapped: {
                                let [pattern, state] = modelData.split("\t")

                                root.nextTriggerEnables = state === "enable"
                                triggerField.text = pattern
                                triggerField.forceActiveFocus()

                                root.removeTrigger(index)
                            }
                        }
                    }

                    SquareButton {
                        id: removeButton
                        text: "×"
                        onClicked: root.removeTrigger(index)
                    }
                }
            }
        }
    } }
}
