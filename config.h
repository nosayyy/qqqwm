#ifndef CONFIG_H
#define CONFIG_H

#define MOD Mod4Mask

const char* menu[] = {"dmenu_run", 0};
const char* term[] = {"st",        0};

const char* briup[]   = {"bri", "10", "+", 0};
const char* bridown[] = {"bri", "10", "-", 0};

const char* volup[]   = {"amixer", "sset", "Master", "5%+", 0};
const char* voldown[] = {"amixer", "sset", "Master", "5%-", 0};
const char* volmute[] = {"amixer", "sset", "Master", "toggle", 0};

static struct key keys[] = {
    {MOD,      XK_f,   win_fs,     {0}},
    {MOD,      XK_q,   win_kill,   {0}},
    {MOD,      XK_c,   win_center, {0}},

    {Mod1Mask,           XK_Tab, win_next, {0}},
    {Mod1Mask|ShiftMask, XK_Tab, win_prev, {0}},

    {MOD, XK_d,      run, {.com = menu}},
    {MOD, XK_Return, run, {.com = term}},

    {0,   XF86XK_AudioRaiseVolume,  run, {.com = volup}},
    {0,   XF86XK_AudioLowerVolume,  run, {.com = voldown}},
    {0,   XF86XK_AudioMute,         run, {.com = volmute}},

    {0,   XF86XK_MonBrightnessUp,   run, {.com = briup}},
    {0,   XF86XK_MonBrightnessDown, run, {.com = bridown}},
};

#endif
