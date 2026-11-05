#ifndef CONFIG_H
#define CONFIG_H

#define DEF_W               800
#define DEF_H               600
#define MIN_W               100
#define MIN_H               80

#define REPEAT_RATE         25
#define REPEAT_DELAY        600

#define CURSOR_SIZE         24
#define CURSOR_THEME        NULL

#define WALLPAPER           ".config/wall.ff"

#define XKB_RULES           NULL
#define XKB_MODEL           NULL
#define XKB_LAYOUT          "us,ru"
#define XKB_VARIANT         NULL
#define XKB_OPTIONS         "grp:win_space_toggle"

#define MODKEY              WLR_MODIFIER_ALT
#define SHIFT               WLR_MODIFIER_SHIFT
#define CTRL                WLR_MODIFIER_CTRL
#define LOGO                WLR_MODIFIER_LOGO

static const char *termcmd[] UNUSED = { "foot", NULL };

static const struct key keys[] UNUSED = {
	/* mod              keysym           func          arg */
	{ MODKEY,           XKB_KEY_Return,  spawn,        {.v = termcmd} },
	{ MODKEY,           XKB_KEY_q,       killclient,   {0} },
	{ MODKEY,           XKB_KEY_e,       quit,         {0} },
};

static const struct button buttons[] UNUSED = {
	/* mod              button      func          arg */
	{ MODKEY,           BTN_LEFT,   move_mouse,   {0} },
	{ MODKEY,           BTN_RIGHT,  resize_mouse, {0} },
};

#endif
