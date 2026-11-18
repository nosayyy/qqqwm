#ifndef CONFIG_H
#define CONFIG_H

/* This file intentionally contains only compile-time constants.
 * All user settings are loaded from environment variables by config.c. */

#include <stdint.h>

struct config {
	int def_w;
	int def_h;
	int min_w;
	int min_h;
	int repeat_rate;
	int repeat_delay;
	int cursor_size;
	const char *cursor_theme;
	const char *wallpaper;
	const char *xkb_rules;
	const char *xkb_model;
	const char *xkb_layout;
	const char *xkb_variant;
	const char *xkb_options;
	const char *terminal;
	uint32_t modkey;
	uint32_t terminal_key;
	uint32_t kill_key;
	uint32_t quit_key;
	uint32_t move_button;
	uint32_t resize_button;
};

extern struct config cfg;

void config_load(void);

#endif
