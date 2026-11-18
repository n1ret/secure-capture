import QtQuick
import QtQuick.Controls as QQC2
import org.kde.plasma.workspace.dbus

QQC2.Button {
    id: picker

    // The item that receives the picked ID, e.g. a TextField. Anything with a `text` property works.
    property Item target
    // Set this from the parent, e.g. to root.kwinAvailable
    property bool available: true
    // True while KWin is waiting for the user to click a window
    readonly property alias picking: internal.picking

    // Emitted with the ID after it has been written to `target`
    signal picked(string identifier)

    function pick() {
        if (internal.picking)
            return;
        internal.picking = true;

        const pending = SessionBus.asyncCall({
            "service": "org.kde.KWin",
            "path": "/KWin",
            "iface": "org.kde.KWin",
            "member": "queryWindowInfo",
            "arguments": []
        });

        pending.finished.connect(() => {
            internal.picking = false;
            // Esc cancels with an error reply, which is normal, so it is ignored
            if (!pending.isError) {
                const info = pending.value || {};
                const id = String(info.desktopFile) || String(info.resourceClass) || "";
                if (id) {
                    if (picker.target) {
                        picker.target.text = id;
                        picker.target.forceActiveFocus();
                    }
                    picker.picked(id);
                }
            }
            pending.destroy();
        });
    }

    QtObject {
        id: internal
        property bool picking: false
    }

    icon.name: "color-picker"
    enabled: available && !picking
    onClicked: pick()

    QQC2.ToolTip.text: picking ? i18n("Click a window… (Esc to cancel)") : i18n("Pick a window")
    QQC2.ToolTip.visible: hovered || picking
}
