# Third-party code vendored into InkCards firmware

The rendering stack under this directory is vendored from
[CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader)
(MIT licence, compatible with this project's MIT licence), commit
`d0b70b3f9f8120df145a081028ef2ed2b4a49a30` (develop):

| Directory | Upstream | Purpose |
|---|---|---|
| `GfxRenderer/` | `lib/GfxRenderer/` | 1-bit framebuffer renderer: text, shapes, bitmaps |
| `EpdFont/` | `lib/EpdFont/` (headers and sources only; the 19 MB `builtinFonts/` are not vendored, InkCards loads fonts from SD card) | Compressed antialiased font engine |
| `MiniBidi/` | `lib/MiniBidi/` | Bidirectional text support used by GfxRenderer |
| `Memory/` | `lib/Memory/` | `makeUniqueNoThrow` and scratch-buffer helpers |
| `InflateReader/` | `lib/InflateReader/` | Streaming inflate used by SD-card font loading |
| `Utf8/` | `lib/Utf8/` | UTF-8 iteration and composition tables |
| `uzlib/` | `lib/uzlib/` | Deflate implementation backing InflateReader |

`inkcards_core/` is InkCards' own portable logic, not vendored.

The device layer (`Hal*`, SDK hardware libraries) is NOT vendored here; it
comes from [inkkit](https://github.com/mohitagw15856/inkkit), pinned in
`platformio.ini`.
