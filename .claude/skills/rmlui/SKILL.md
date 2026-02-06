---
name: rmlui
description: RmlUI/RCSS syntax rules and differences from standard CSS. Use when editing .rcss or .rml files, or when the user asks about RmlUI styling.
---

# RmlUI Development Skill

RCSS is based on CSS2 with modifications. Many CSS properties work differently or don't exist.

## Critical Differences from CSS

### Layout & Sizing (Recent Lessons)

RmlUi does **not** behave like a web browser with an implicit HTML "user-agent stylesheet". If you don't explicitly set layout semantics for structural elements, containers can end up effectively shrink-to-content, producing tiny computed sizes and therefore wrong borders, alignment, and hitboxes.

#### Always set structural layout explicitly

- Prefer a shared base stylesheet that sets block defaults, e.g. `div { display: block; }`, `p { display: block; }`, `h1..h4 { display: block; }`, and `body { display: block; width: 100%; height: 100%; }`.
- In this repo, `ui/rcss/base.rcss` is the right place for those defaults. Ensure all real UI documents import it first.

#### Percent sizing depends on parent sizing

- `width: 100%` / `height: 100%` only work if the containing block resolves to a definite size.
- If an overlay/viewport chain is missing a definite size, descendants can collapse (eg. a panel computing to something like `172x19`), which makes borders and click hitboxes appear "broken".

#### Flexbox needs explicit constraints to avoid collapse

- In flex rows, one child can collapse another unless you pin constraints.
  - Use fixed `flex-basis` (eg. `flex: 0 0 220dp;`) for labels so they can't steal/lose space unpredictably.
  - Give interactive controls a definite `min-width` and/or `flex: 1 1 <basis>` so they remain usable.
- For container elements inside flex, explicitly set `display: block` and/or `width: 100%` when you want full-width layout.

### Border - NO STYLE KEYWORD
```css
/* WRONG */ border: 1dp solid #2a4060;
/* CORRECT */ border: 1dp #2a4060;

/* WRONG */ border: none;
/* CORRECT */ border-width: 0;
```

### Position Fixed - BROKEN
`position: fixed` = `absolute`. Does NOT position relative to viewport.
```css
/* WRONG */ .overlay { position: fixed; top: 0; left: 0; right: 0; bottom: 0; }
/* CORRECT */ body { width: 100%; height: 100%; }
            .overlay { width: 100%; height: 100%; }
```

### No Background Images
```css
/* WRONG */ background-image: url('bg.png');
/* CORRECT */ decorator: image(bg.png);
```

### Border Radius - No Percentages
```css
/* WRONG */ border-radius: 50%;
/* CORRECT */ border-radius: 6dp;
```

### Font Family - No Fallbacks
```css
/* WRONG */ font-family: LatoLatin, sans-serif;
/* CORRECT */ font-family: LatoLatin;
```

### Units
Use `dp` not `px`: `16dp`, `100%`, `1.5em`

### Other Behavioral Differences
- `z-index` applies to ALL elements (not just positioned)
- `:hover`, `:active`, `:focus` propagate to parents
- No pseudo-elements (`::before`, `::after`)
- Transitions only trigger on class/pseudo-class changes

## Unsupported Properties

| Property | Alternative |
|----------|-------------|
| `background-image` | `decorator: image()` |
| `text-shadow` | `font-effect: shadow()` or `glow()` |
| `text-align: justify` | Not supported |
| `line-height: normal` | Use explicit value |
| `content: ""` | Use child elements in RML |

## RCSS-Exclusive Features

### Decorators
```css
decorator: image(icon.png);
decorator: horizontal-gradient(#ff0000 #0000ff);
decorator: tiled-horizontal(btn-l, btn-c, btn-r);  /* 3-slice */
decorator: tiled-box(tl, t, tr, l, c, r, bl, b, br);  /* 9-slice */
```

### Font Effects
```css
font-effect: glow(2dp #ff0000);
font-effect: outline(2dp black);
font-effect: shadow(2dp 2dp #000);
```

### Sprite Sheets
```css
@spritesheet theme {
  src: sprites.png;
  icon-play: 0px 0px 32px 32px;
}
```

## Form Element Selectors

```css
input.text { }           /* <input type="text"/> */
input.range { }          /* <input type="range"/> (slider) */
input.checkbox:checked { }
input.range slidertrack { }
input.range sliderbar { }
select selectvalue { }
select selectbox option { }
```

## Data Binding Quick Reference

```html
{{ variable }}                              <!-- Text interpolation -->
<div data-if="condition">                   <!-- Conditional -->
<div data-for="item : list">                <!-- Loop -->
<input type="range" data-value="volume"/>   <!-- Two-way binding -->
<button data-event-click="action()">        <!-- Event handler -->
```

For full syntax: See [DATA_BINDING.md](DATA_BINDING.md)

## Project Structure

```
ui/rml/menus/     - Menu screens
ui/rml/hud/       - HUD overlays
ui/rcss/*.rcss    - Stylesheets
```

Fonts: `LatoLatin`, `OpenSans` (loaded in `rmlui/ui_manager.cpp`)

## Menu Actions (Project-Specific)

```html
<button data-action="navigate('options')">Options</button>
<button data-action="console('map e1m1')">Start</button>
```

## Debugging

- `ui_menu <path>` - Open RML document
- `ui_debugger` - Toggle visual debugger
- When debugging layout bugs, inspect computed element sizes first. If containers are tiny, fix the layout chain (explicit `display`, explicit parent sizing, and flex constraints) before tweaking borders/spacing.

## References

- [Property Index](https://mikke89.github.io/RmlUiDoc/pages/rcss/property_index.html)
- [Decorators](https://mikke89.github.io/RmlUiDoc/pages/rcss/decorators.html)
- [Data Binding](https://mikke89.github.io/RmlUiDoc/pages/data_bindings.html)
