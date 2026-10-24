#ifndef CONFIG_H
#define CONFIG_H

/* -- modifier ------------------------------------------------ */
#define MOD                 WLR_MODIFIER_ALT

/* -- terminal ------------------------------------------------ */
#define TERMINAL            "foot"

/* -- keybinds  ----------------------------------------------- */
#define KEYBIND_TERMINAL    XKB_KEY_Return
#define KEYBIND_CLOSE       XKB_KEY_q
#define KEYBIND_QUIT        XKB_KEY_e

/* -- geometry ------------------------------------------------- */
#define DEF_W               800
#define DEF_H               600
#define MIN_W               100
#define MIN_H               80

/* -- keyboard -------------------------------------------------- */
#define REPEAT_RATE         25
#define REPEAT_DELAY        600

/* -- cursor ---------------------------------------------------- */
#define CURSOR_SIZE         24
#define CURSOR_THEME        NULL

#endif