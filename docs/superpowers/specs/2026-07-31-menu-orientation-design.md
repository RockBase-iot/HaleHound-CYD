# Menu Orientation Design

## Objective

Make the application menus usable in all four display rotations on both
ESP32-2432S028 (`esp32-cyd`) and NM-CYD-C5 (`nm-cyd-c5`) without changing the
default portrait experience or the existing feature dispatch and VALHALLA
policy behavior.

## Root Cause

The TFT driver changes its runtime width and height after `setRotation()`, but
the legacy menu code continues to use compile-time `CYD_SCREEN_WIDTH` and
`CYD_SCREEN_HEIGHT` values. Main-menu drawing, submenu drawing, touch hit
testing, the mode banner, and the PIN lock target therefore disagree with the
physical 320x240 landscape surface. Submenus also use a fixed single-column
28-pixel pitch, which puts the last entries below the landscape viewport.

## Scope

- Main menu drawing and touch targets.
- All category submenus and the nested AirTag submenu.
- Rotation settings screen and its touch targets.
- Home-screen VALHALLA/BLUE TEAM banner and PIN lock target.
- Pure host-testable layout calculations.

Feature operation screens are outside this change. They can be audited
separately because they contain module-specific coordinate systems.

## Layout Rules

### Main Menu

- Use two columns in portrait and landscape.
- Keep the existing column-major category order: indices 0-4 in the left
  column and 5-9 in the right column.
- Calculate margins, gaps, row height, and button width from `tft.width()` and
  `tft.height()`.
- Reserve the bottom 22 pixels for the persisted VALHALLA/BLUE TEAM banner.

### Submenus

- Use one column when `height >= width`.
- Use two columns when `width > height`.
- Use column-major order so sequential items remain top-to-bottom before
  moving to the second column.
- Keep `Back` as the final item in the item array; it remains visible and uses
  the same layout and hit-test rules as every other entry.
- Support at least the current maximum of ten entries at 240x320 and 320x240.

### Rotation Screen

- Use one column for four options in portrait.
- Use a two-by-two option grid in landscape.
- Recalculate option rectangles after applying a new rotation.

## Shared Layout Contract

`MenuLayout` is a pure value object. It receives the runtime screen size, item
count, layout kind, content bounds, margins, and gaps. It returns an item
rectangle and provides hit testing. Drawing and touch handling must consume the
same rectangles; no menu handler may reconstruct its own X/Y formula.

Invalid indexes return an empty rectangle. Empty or invalid layouts return no
hit. Rectangle boundaries are half-open (`x <= point < x + width`) so adjacent
items cannot both match one touch.

## Rendering

- Center icons and labels inside their runtime rectangles.
- Clear an item's full rectangle before redrawing selected or unselected state.
- Center the skull watermark in the available runtime viewport without using
  negative coordinates.
- Position the mode banner and lock icon from runtime width and height.
- Preserve existing theme colors, icons, labels, safe-mode blocking, and
  VALHALLA/BLUE TEAM policy logic.

## Verification

- Native layout tests cover 240x320 and 320x240 main menus, submenus with 1,
  9, and 10 entries, rotation options, bounds, non-overlap, and hit-test
  round trips.
- Build both `esp32-cyd` and `nm-cyd-c5`.
- Run target-contract and architecture-boundary audits.
- Hardware verification remains required for physical touch calibration and
  visual appearance on both boards.
