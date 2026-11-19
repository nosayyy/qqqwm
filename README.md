
<h1 align="center">QQQWM</h1>

<p align="center">
  <strong>A minimalistic Wayland window manager written in pure C</strong>
</p>

<p align="center">
  <img alt="C" src="https://img.shields.io/badge/C-11-blue"/>
  <img alt="Wayland" src="https://img.shields.io/badge/Wayland-native-1a73e8"/>
  <img alt="wlroots" src="https://img.shields.io/badge/wlroots-0.20-purple"/>
  <img alt="License" src="https://img.shields.io/badge/license-MIT-green"/>
</p>

---

## About the project

**QQQWM** is a lightweight tiling-adjacent compositor for Wayland, written in C. Windows are placed in the center of the screen, can be moved and resized with the mouse, and all controls are fully configurable through environment variables.

The project uses [wlroots](https://gitlab.freedesktop.org/wlroots/wlroots), XDG Shell, and XKB.

## Features

- 🖱️ **Focus follows mouse** — focus switches when the cursor is hovered
- ⌨️ **Configurable keybindings** — no source code editing required
- 🌍 **Multiple XKB layouts** — layout switching and options
- 🖼️ **Farbfeld wallpaper** — image loading without an additional graphics library
- 🪟 **Move and resize with the mouse**
- 🎨 **Configurable theme and cursor size**
- ⚡ **Small and understandable C codebase**

## Requirements

To build, you will need:

- Wayland
- `wayland-protocols`
- `wayland-scanner`
- wlroots 0.20
- libinput
- libxkbcommon
- Meson and Ninja
- GCC or Clang
- A POSIX-compatible system

Example dependency installation on Arch Linux:

```bash
sudo pacman -S wayland wayland-protocols wlroots libinput libxkbcommon meson ninja gcc
```

## Build and installation

```bash
git clone https://github.com/nosayyy/qqqwm.git
cd qqqwm

meson setup build
meson compile -C build
sudo meson install -C build
```

To build into a regular directory:

```bash
meson setup build
meson compile -C build
./build/qqqwm
```

## Running

Make sure the `XDG_RUNTIME_DIR` variable is set:

```bash
export XDG_RUNTIME_DIR=/run/user/$(id -u)
exec qqqwm
```

Add to `~/.profile`:

```bash
export XDG_RUNTIME_DIR=/run/user/$(id -u)
exec qqqwm
```

## Hotkeys

Hotkeys are formed as:

```text
QQQWM_MODKEY + key
```

| Action | Variable | Default |
|---|---|---|
| Open terminal | `QQQWM_KEY_TERMINAL` | `Return` |
| Close current window | `QQQWM_KEY_KILL` | `q` |
| Quit QQQWM | `QQQWM_KEY_QUIT` | `e` |

For example:

```bash
export QQQWM_MODKEY=logo
export QQQWM_TERMINAL=kitty
export QQQWM_KEY_TERMINAL=Return
export QQQWM_KEY_KILL=q
export QQQWM_KEY_QUIT=e
```

## Mouse controls

Hold the modifier and use a mouse button over a window:

| Action | Variable | Value |
|---|---|---|
| Move window | `QQQWM_BUTTON_MOVE` | `1` — left button |
| Resize window | `QQQWM_BUTTON_RESIZE` | `3` — right button |

Values correspond to Linux input button codes.

```bash
export QQQWM_MODKEY=alt
export QQQWM_BUTTON_MOVE=1
export QQQWM_BUTTON_RESIZE=3
```

## Configuration

All parameters are loaded at startup. Invalid or empty values are replaced with default values.

### Window sizes

| Variable | Default | Description |
|---|---:|---|
| `QQQWM_DEF_W` | `800` | Width of a new window |
| `QQQWM_DEF_H` | `600` | Height of a new window |
| `QQQWM_MIN_W` | `100` | Minimum width |
| `QQQWM_MIN_H` | `80` | Minimum height |

### Keyboard

| Variable | Default | Description |
|---|---|---|
| `QQQWM_MODKEY` | `alt` | Modifier for keybindings |
| `QQQWM_REPEAT_RATE` | `25` | Key repeat rate |
| `QQQWM_REPEAT_DELAY` | `600` | Repeat delay, ms |
| `QQQWM_XKB_RULES` | — | XKB rules |
| `QQQWM_XKB_MODEL` | — | Keyboard model |
| `QQQWM_XKB_LAYOUT` | `us,ru` | Keyboard layouts |
| `QQQWM_XKB_VARIANT` | — | Layout variant |
| `QQQWM_XKB_OPTIONS` | `grp:alt_shift_toggle` | XKB options |

Valid modifier values:

- `alt`
- `logo` or `super`
- `shift`
- `ctrl` or `control`

Example:

```bash
export QQQWM_MODKEY=super
export QQQWM_XKB_LAYOUT="us,ru"
export QQQWM_XKB_OPTIONS="grp:alt_shift_toggle"
export QQQWM_REPEAT_RATE=40
export QQQWM_REPEAT_DELAY=400
```

### Application and cursor

| Variable | Default | Description |
|---|---|---|
| `QQQWM_TERMINAL` | `foot` | Terminal executable |
| `QQQWM_CURSOR_THEME` | — | Cursor theme name |
| `QQQWM_CURSOR_SIZE` | `24` | Cursor size |
| `QQQWM_WALLPAPER` | `.config/wall.ff` | Path to the Farbfeld wallpaper relative to `$HOME` |

Example:

```bash
export QQQWM_TERMINAL=kitty
export QQQWM_CURSOR_THEME=Adwaita
export QQQWM_CURSOR_SIZE=24
export QQQWM_WALLPAPER=.config/wallpapers/background.ff
```

## Complete configuration example

```bash
# Window sizes
export QQQWM_DEF_W=1000
export QQQWM_DEF_H=700
export QQQWM_MIN_W=300
export QQQWM_MIN_H=200

# Keyboard
export QQQWM_MODKEY=super
export QQQWM_XKB_LAYOUT="us,ru"
export QQQWM_XKB_OPTIONS="grp:alt_shift_toggle"
export QQQWM_REPEAT_RATE=40
export QQQWM_REPEAT_DELAY=400

# Hotkeys
export QQQWM_TERMINAL=foot
export QQQWM_KEY_TERMINAL=Return
export QQQWM_KEY_KILL=q
export QQQWM_KEY_QUIT=e

# Mouse
export QQQWM_BUTTON_MOVE=1
export QQQWM_BUTTON_RESIZE=3

# Cursor and background
export QQQWM_CURSOR_THEME=Adwaita
export QQQWM_CURSOR_SIZE=24
export QQQWM_WALLPAPER=.config/wall.ff
```

## Farbfeld wallpaper

The wallpaper must be in [Farbfeld](https://tools.suckless.org/farbfeld/) format and located relative to `$HOME`.

```bash
mkdir -p ~/.config
ffmpeg -i wallpaper.png ~/.config/wall.ff
```

> Conversion is only possible if utilities supporting the Farbfeld format are available.

```bash
export QQQWM_WALLPAPER=.config/wall.ff
```

The image will be centered and scaled while preserving aspect ratio.

## Project structure

```text
.
├── meson.build
├── LICENSE.md
└── src
    ├── main.c      — Wayland compositor startup
    ├── config.c    — reading environment variables
    ├── input.c     — keyboard, mouse, and bindings
    ├── client.c    — XDG client management
    ├── output.c    — displays and Farbfeld wallpaper
    ├── utils.c     — utility functions
    └── common.h    — common project interface
```

## Limitations

The current branch has several important limitations:

- only one physical monitor is supported;
- additional monitor outputs are ignored;
- there is no built-in bar;
- there is no workspaces model;
- windows have a fixed geometric position when they appear;
- only XDG top-level surfaces are managed.

## License

The project is distributed under the MIT License. See [LICENSE.md](LICENSE.md) for details.
