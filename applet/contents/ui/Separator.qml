import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Separator {
    Layout.fillWidth: true
    Layout.leftMargin: -parent.parent.leftPadding / 2
    Layout.rightMargin: -parent.parent.rightPadding / 2

    anchors {
        left: parent.left
        leftMargin: parent.padding / 2
    }
}
