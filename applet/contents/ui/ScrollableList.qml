import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

QQC2.ScrollView {
    id: scroll

    default property alias content: column.data

    property real maxHeight: Kirigami.Units.gridUnit * 5

    readonly property bool scrollable: column.implicitHeight > maxHeight

    Layout.fillHeight: true
    Layout.fillWidth: true
    Layout.rightMargin: scrollable ? -QQC2.ScrollBar.vertical.width : 0

    padding: 0

    ColumnLayout {
        id: column

        width: scroll.availableWidth
        spacing: Kirigami.Units.smallSpacing
    }

    QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff
    QQC2.ScrollBar.vertical {
        background: null
        width: Kirigami.Units.smallSpacing * 3
        contentItem: Item {
            implicitHeight: 30
            Rectangle {
                anchors { top: parent.top; bottom: parent.bottom; right: parent.right }
                width: Kirigami.Units.smallSpacing
                radius: width / 2
                color: Kirigami.Theme.textColor
                opacity: parent.parent.pressed ? 0.6 : parent.parent.hovered ? 0.45 : 0.3
            }
        }
    }
}
