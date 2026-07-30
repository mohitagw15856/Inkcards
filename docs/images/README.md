# Brand and UI assets

| File | Purpose |
| ---- | ------- |
| `logo.svg` | Square app logo (flashcard stack with a face and a repeat loop). |
| `banner.svg` | Wide banner shown at the top of the main README. |
| `social-preview.svg` / `social-preview.png` | 1280x640 social/OpenGraph preview. |
| `screens/*.svg` / `screens/*.png` | Design mockups of the device UI. |

## Regenerating

The UI mockups are generated to match the layout constants in
`firmware/src/ui/`:

```sh
python3 docs/images/screens/generate_screens.py    # needs cairosvg for the PNGs
```

The banner, logo and social preview are hand-authored SVGs. To re-render the
social preview PNG after editing the SVG:

```sh
python3 -c "import cairosvg; cairosvg.svg2png(url='docs/images/social-preview.svg', write_to='docs/images/social-preview.png', output_width=1280, output_height=640)"
```

## Setting the social preview on GitHub

The social preview image is a repository setting, not something the code can
apply. To set it: repository **Settings -> General -> Social preview -> Edit**,
and upload `docs/images/social-preview.png` (1280x640).

## A note on fonts

The mockups embed the InkCards card text using a CJK-capable font at render
time, so `你好` shows correctly in the committed PNGs. On the real device, CJK
is rendered from CrossPoint-compatible SD-card `.cpfont` families, not embedded
fonts (see `ARCHITECTURE.md`).
