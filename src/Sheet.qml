import QtQuick

// A dimmed window with one card in the middle. A click outside the card dismisses it.
Rectangle {
    id: sheet
    default property alias content: column.data
    signal dismissed()
    anchors.fill: parent
    visible: false
    color: Qt.rgba(0, 0, 0, 0.35)

    MouseArea { anchors.fill: parent; onClicked: sheet.dismissed() }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width - 32, 340)
        height: column.implicitHeight + 40
        radius: pal.radius.xl
        color: pal.bgElevated

        MouseArea { anchors.fill: parent }
        Column { id: column; x: 20; y: 20; width: parent.width - 40; spacing: 16 }
    }
}
