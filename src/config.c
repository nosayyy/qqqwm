#include "common.h"

#include <errno.h>

struct config cfg;

static int env_int(const char *name, int fallback, int lo, int hi) {
	const char *value = getenv(name);
	char *end;
	long number;

	if (!value || !*value)
		return fallback;
	errno = 0;
	number = strtol(value, &end, 10);
	if (errno || *end || number < lo || number > hi)
		return fallback;
	return (int)number;
}

static const char *env_str(const char *name, const char *fallback) {
	const char *value = getenv(name);
	return value && *value ? value : fallback;
}

static uint32_t env_mod(const char *name) {
	const char *value = getenv(name);
	if (!value || !strcmp(value, "alt"))
		return WLR_MODIFIER_ALT;
	if (!strcmp(value, "logo") || !strcmp(value, "super"))
		return WLR_MODIFIER_LOGO;
	if (!strcmp(value, "shift"))
		return WLR_MODIFIER_SHIFT;
	if (!strcmp(value, "ctrl") || !strcmp(value, "control"))
		return WLR_MODIFIER_CTRL;
	return WLR_MODIFIER_ALT;
}

static xkb_keysym_t env_keysym(const char *name, xkb_keysym_t fallback) {
	const char *value = getenv(name);
	xkb_keysym_t sym;

	if (!value || !*value)
		return fallback;
	sym = xkb_keysym_from_name(value, XKB_KEYSYM_NO_FLAGS);
	return sym == XKB_KEY_NoSymbol ? fallback : sym;
}

void config_load(void) {
	cfg.def_w = env_int("QQQWM_DEF_W", 800, 100, 10000);
	cfg.def_h = env_int("QQQWM_DEF_H", 600, 80, 10000);
	cfg.min_w = env_int("QQQWM_MIN_W", 100, 1, 10000);
	cfg.min_h = env_int("QQQWM_MIN_H", 80, 1, 10000);
	cfg.repeat_rate = env_int("QQQWM_REPEAT_RATE", 25, 0, 1000);
	cfg.repeat_delay = env_int("QQQWM_REPEAT_DELAY", 600, 0, 10000);
	cfg.cursor_size = env_int("QQQWM_CURSOR_SIZE", 24, 1, 512);
	cfg.cursor_theme = env_str("QQQWM_CURSOR_THEME", NULL);
	cfg.wallpaper = env_str("QQQWM_WALLPAPER", ".config/wall.ff");
	cfg.xkb_rules = env_str("QQQWM_XKB_RULES", NULL);
	cfg.xkb_model = env_str("QQQWM_XKB_MODEL", NULL);
	cfg.xkb_layout = env_str("QQQWM_XKB_LAYOUT", "us,ru");
	cfg.xkb_variant = env_str("QQQWM_XKB_VARIANT", NULL);
	cfg.xkb_options = env_str("QQQWM_XKB_OPTIONS", "grp:alt_shift_toggle");
	cfg.terminal = env_str("QQQWM_TERMINAL", "foot");
	cfg.modkey = env_mod("QQQWM_MODKEY");
	cfg.terminal_key = env_keysym("QQQWM_KEY_TERMINAL", XKB_KEY_Return);
	cfg.kill_key = env_keysym("QQQWM_KEY_KILL", XKB_KEY_q);
	cfg.quit_key = env_keysym("QQQWM_KEY_QUIT", XKB_KEY_e);
	cfg.move_button = (uint32_t)env_int("QQQWM_BUTTON_MOVE", BTN_LEFT, 1, 255);
	cfg.resize_button = (uint32_t)env_int("QQQWM_BUTTON_RESIZE", BTN_RIGHT, 1, 255);
}
