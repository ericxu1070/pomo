import QtQuick

Text {
    property string role: "body"
    // Pixel size, weight, letter spacing in px.
    readonly property var spec: ({
        title2: [20, Font.DemiBold, -0.3], title3: [17, Font.DemiBold, -0.17],
        headline: [14, Font.DemiBold, -0.07], body: [14, Font.Normal, 0],
        callout: [13, Font.Normal, 0], footnote: [12, Font.Normal, 0.06] })
    font.family: pal.family
    font.pixelSize: spec[role][0]
    font.weight: spec[role][1]
    font.letterSpacing: spec[role][2]
    color: pal.label
    textFormat: Text.PlainText
}
