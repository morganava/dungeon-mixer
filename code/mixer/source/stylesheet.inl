// scrollbar
STYLE_SHEET_ENTRY(scrollbarSize, 20.0, sdf::none)
STYLE_SHEET_ENTRY(scrollbarMinLength, 40.0, sdf::none)
STYLE_SHEET_ENTRY(scrollbarForeColor, color::rgb(0x60,0x60,0x60), sdf::none)
STYLE_SHEET_ENTRY(scrollbarActiveColor, color::rgb(0xc0,0x60,0x00), sdf::none)
STYLE_SHEET_ENTRY(scrollbarBackColor, color::rgb(0xc0,0xc0,0xc0), sdf::none)
STYLE_SHEET_ENTRY(scrollbarCornerRadius, 5.0, sdf::none)
STYLE_SHEET_ENTRY(scrollbarStep, 45.0, sdf::none)
// size
STYLE_SHEET_ENTRY(size, vec2(0.0, 0.0), sdf::geometry)
// margin
STYLE_SHEET_ENTRY(margin, edges{}, sdf::geometry)
// border
STYLE_SHEET_ENTRY(borderSize, 2.0, sdf::geometry)
STYLE_SHEET_ENTRY(borderColor, color::rgb(0x00,0x00,0x00), sdf::none)
STYLE_SHEET_ENTRY(borderRadius, corners{}, sdf::geometry)
// padding
STYLE_SHEET_ENTRY(padding, edges{}, sdf::geometry)
// background
STYLE_SHEET_ENTRY(backgroundColor, color::rgb(0xff,0xff,0xff), sdf::none)
STYLE_SHEET_ENTRY(backgroundImage, image_handle::invalid, sdf::none)
STYLE_SHEET_ENTRY(backgroundImageMode, background_image_mode::stretch, sdf::none)
// font
STYLE_SHEET_ENTRY(fontFamily, font_handle::invalid, sdf::none)
STYLE_SHEET_ENTRY(fontColor, color::rgb(0x00,0x00,0x00), sdf::none)
STYLE_SHEET_ENTRY(fontSize, 24.0, sdf::none)
STYLE_SHEET_ENTRY(placeholderFontColor, color::rgb(0x60,0x60,0x60), sdf::none)
STYLE_SHEET_ENTRY(selectedFontColor, color::rgb(0xFF, 0xFF, 0xFF), sdf::none)
STYLE_SHEET_ENTRY(selectionColor, color::rgb(0xFF,0x88,0x00), sdf::none)
// text
STYLE_SHEET_ENTRY(textHalign, halign::left, sdf::none)
STYLE_SHEET_ENTRY(textValign, valign::middle, sdf::none)
