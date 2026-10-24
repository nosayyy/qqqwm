project('qqqwm', 'c',
  version: '0.1.0',
  license: 'MIT',
  meson_version: '>=0.60.0',
  default_options: [
    'c_std=c11',
    'warning_level=3',
  ],
)

add_project_arguments(
  '-D_POSIX_C_SOURCE=200809L',
  '-DWLR_USE_UNSTABLE',
  '-Wno-unused-parameter',
  '-Wshadow',
  '-Wpointer-arith',
  '-Wcast-qual',
  language: 'c',
)

wlroots_dep        = dependency('wlroots-0.20')
wayland_server_dep = dependency('wayland-server')
xkbcommon_dep      = dependency('xkbcommon')
wayland_protos_dep = dependency('wayland-protocols')
libinput_dep       = dependency('libinput')

wayland_scanner = find_program('wayland-scanner')

wl_protocol_dir = wayland_protos_dep.get_variable(pkgconfig: 'pkgdatadir')
xdg_shell_xml   = wl_protocol_dir / 'stable/xdg-shell/xdg-shell.xml'

xdg_shell_header = custom_target(
  'xdg-shell-protocol.h',
  input:  xdg_shell_xml,
  output: 'xdg-shell-protocol.h',
  command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
)

xdg_shell_code = custom_target(
  'xdg-shell-protocol.c',
  input:  xdg_shell_xml,
  output: 'xdg-shell-protocol.c',
  command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
)

src = files(
  'src/main.c',
  'src/utils.c',
  'src/client.c',
  'src/output.c',
  'src/input.c',
)

inc = include_directories('src', '.')

executable('qqqwm', src, xdg_shell_header, xdg_shell_code,
  include_directories: inc,
  dependencies: [
    wlroots_dep,
    wayland_server_dep,
    xkbcommon_dep,
    libinput_dep,
  ],
  install: true,
)
