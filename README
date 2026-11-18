DESCRIPTION
  Lightweight window manager for Wayland.
  Written in C with settings loaded from environment variables.

FEATURES
  - Focus follows mouse
  - Environment-configurable keyboard and mouse shortcuts
  - Multiple keyboard layouts via XKB
  - Farbfeld wallpaper, no image library needed

BUILD
  meson setup build
  meson compile -C build
  meson install -C build

REQUIREMENTS
  - Wayland development libraries: wayland, wayland-protocols,
    wayland-scanner, wlroots, libinput, libxkbcommon
  - GCC/Clang compiler
  - POSIX environment

CONFIGURATION
  Settings are read at startup from environment variables. Examples:

    export QQQWM_DEF_W=1000
    export QQQWM_DEF_H=700
    export QQQWM_XKB_LAYOUT="us,ru"
    export QQQWM_XKB_OPTIONS="grp:alt_shift_toggle"
    export QQQWM_MODKEY=alt       # alt, logo, shift, ctrl
    export QQQWM_KEY_TERMINAL=Return
    export QQQWM_KEY_KILL=q
    export QQQWM_KEY_QUIT=e
    export QQQWM_TERMINAL=foot
    export QQQWM_BUTTON_MOVE=1    # BTN_LEFT
    export QQQWM_BUTTON_RESIZE=3  # BTN_RIGHT
    export QQQWM_WALLPAPER=.config/wall.ff

  Key names use XKB keysym names. Mouse button values are Linux input
  button numbers (1=left, 2=middle, 3=right). Unset variables use defaults.

USAGE
  Start from .profile or a display manager, for example:
    exec qqqwm
