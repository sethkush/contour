import QtQuick 2.0
import QtQuick.Controls 1.0
import Contour.Terminal 1.0

ApplicationWindow
{
    visible: true
    width: 900
    height: 700
    title: qsTr("Minimal Qml")

    ContourTerminal
    {
        visible: true
        width: 800
        height: 600
    }
}
