import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

Item {
    property alias text: label.text

    height: label.implicitHeight
    Layout.fillWidth: true

    QQC2.Label {
        id: label

        anchors.horizontalCenter: parent.horizontalCenter
        font.bold: true
    }
}
