import QtQuick

// The Omarchy theme's colors under the names the UI uses. Every other file reads
// `pal`, never a hex value.
QtObject {
    function c(key, fallback) { return theme.colors[key] !== undefined ? theme.colors[key] : fallback }
    function a(col, alpha) { return Qt.rgba(col.r, col.g, col.b, alpha) }

    readonly property color bg:             c("background", "#1c1c1e")
    readonly property color bgElevated:     c("lighter_background", "#2c2c2e")
    readonly property color label:          c("foreground", "#f5f5f7")
    readonly property color labelSecondary: c("dark_foreground", "#a1a1a6")
    readonly property color tint:           theme.accent
    readonly property color fill:           a(label, 0.06)
    readonly property color fillPressed:    a(label, 0.12)
    readonly property color danger:         c("red", "#ff453a")

    readonly property var radius: ({ sm: 6, md: 10, lg: 14, xl: 20 })
    readonly property string family: "Inter"
}
