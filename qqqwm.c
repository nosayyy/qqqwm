#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include <signal.h>
#include <unistd.h>

#include "qqqwm.h"
#include "config.h"

static client *client_list = {0}, *current_client;

static int          window_x,     window_y,      num_lock_mask = 0;
static int          screen_width, screen_height;
static unsigned int window_width, window_height;

static int          drag_x, drag_y;
static unsigned int drag_width, drag_height;

static Display *display;
static Window root_window;

static XButtonEvent mouse_event;

static void (*events[LASTEvent])(XEvent *e) = {
    [KeyPress]                 =key_press,
    [ButtonPress]           =button_press,
    [ButtonRelease]       =button_release,
    [ConfigureRequest] =configure_request,
    [MapRequest]             =map_request,
    [MappingNotify]       =mapping_notify,
    [EnterNotify]           =notify_enter,
    [DestroyNotify]       =notify_destroy,
    [MotionNotify]         =notify_motion,
};

void win_focus(client *c) {
    if (!c) {
        current_client = 0;
        XSetInputFocus(display, root_window, RevertToPointerRoot, CurrentTime);
        return;
    }

    current_client = c;
    XSetInputFocus(display, current_client->w, RevertToPointerRoot, CurrentTime);
}

void notify_enter(XEvent *e) {
    while(XCheckTypedEvent(display, EnterNotify, e));

    if (e->xcrossing.mode != NotifyNormal || e->xcrossing.detail == NotifyInferior) return;

    for win if (client_item->w == e->xcrossing.window) win_focus(client_item);
}

void notify_destroy(XEvent *e) {
    Window w = e->xdestroywindow.window;
    int was_current = current_client && current_client->w == w;

    if (mouse_event.subwindow == w) mouse_event.subwindow = 0;

    win_del(w);

    if (was_current) win_focus(client_list ? client_list->prev : 0);
}

void notify_motion(XEvent *e) {
    if (!mouse_event.subwindow) return;
    
    while(XCheckTypedEvent(display, MotionNotify, e));
    for win if (client_item->w == mouse_event.subwindow && client_item->f) return;
    int x_delta = e->xmotion.x_root - mouse_event.x_root;
    int y_delta = e->xmotion.y_root - mouse_event.y_root;

    XMoveResizeWindow(display, mouse_event.subwindow,
        drag_x + (mouse_event.button == 1 ? x_delta : 0),
        drag_y + (mouse_event.button == 1 ? y_delta : 0),
        MAX(1, (int)drag_width  + (mouse_event.button == 3 ? x_delta : 0)),
        MAX(1, (int)drag_height + (mouse_event.button == 3 ? y_delta : 0)));
}

void key_press(XEvent *e) {
    KeySym key_symbol = XkbKeycodeToKeysym(display, e->xkey.keycode, 0, 0);

    for (unsigned int key_index=0; key_index<sizeof(keys)/sizeof(*keys); ++key_index)
        if (keys[key_index].keysym == key_symbol &&
            mod_clean(keys[key_index].mod) == mod_clean(e->xkey.state))
            keys[key_index].function(keys[key_index].arg);
}

void button_press(XEvent *e) {
    if (!e->xbutton.subwindow) return;

    win_size(e->xbutton.subwindow, &drag_x, &drag_y, &drag_width, &drag_height);
    XRaiseWindow(display, e->xbutton.subwindow);
    mouse_event = e->xbutton;
}

void button_release(XEvent *e) {
    mouse_event.subwindow = 0;
}

void win_add(Window w) {
    client *new_client;

    if (!(new_client = (client *) calloc(1, sizeof(client)))) exit(1);
    new_client->w = w;

    if (client_list) {
        client_list->prev->next = new_client;
        new_client->prev = client_list->prev;
        client_list->prev = new_client;
        new_client->next = client_list;
    } else {
        client_list = new_client;
        client_list->prev = client_list->next = client_list;
    }
}

void win_del(Window w) {
    client *removed_client = 0;

    for win if (client_item->w == w) removed_client = client_item;

    if (!client_list || !removed_client)        return;
    if (removed_client->prev == removed_client) client_list = 0;
    if (client_list == removed_client)          client_list = removed_client->next;

    removed_client->next->prev = removed_client->prev;
    removed_client->prev->next = removed_client->next;

    if (current_client == removed_client) current_client = 0;
    free(removed_client);
}

void win_kill(const Arg arg) {
    if (current_client) XKillClient(display, current_client->w);
}

void win_center(const Arg arg) {
    if (!current_client) return;

    win_size(current_client->w, &(int){0}, &(int){0}, &window_width, &window_height);
    XMoveWindow(display, current_client->w,
        (screen_width - window_width) / 2,
        (screen_height - window_height) / 2);
}

void win_fs(const Arg arg) {
    if (!current_client) return;

    if ((current_client->f = current_client->f ? 0 : 1)) {
        win_size(current_client->w, &current_client->wx, &current_client->wy, &current_client->ww, &current_client->wh);
        XMoveResizeWindow(display, current_client->w, 0, 0, screen_width, screen_height);
        XRaiseWindow(display, current_client->w);
    } else {
        XMoveResizeWindow(display, current_client->w,
            current_client->wx, current_client->wy,
            current_client->ww, current_client->wh);
    }
}

void win_prev(const Arg arg) {
    if (!current_client) return;

    XRaiseWindow(display, current_client->prev->w);
    win_focus(current_client->prev);
}

void win_next(const Arg arg) {
    if (!current_client) return;

    XRaiseWindow(display, current_client->next->w);
    win_focus(current_client->next);
}

void configure_request(XEvent *e) {
    XConfigureRequestEvent *ev = &e->xconfigurerequest;

    XConfigureWindow(display, ev->window, ev->value_mask & ~CWSibling, &(XWindowChanges) {
        .x = ev->x,
        .y = ev->y,
        .width  = ev->width,
        .height = ev->height,
        .border_width = ev->border_width,
        .stack_mode = ev->detail
    });
}

void map_request(XEvent *e) {
    Window w = e->xmaprequest.window;

    for win if (client_item->w == w) {
        XMapWindow(display, w);
        win_focus(client_item);
        return;
    }

    XSelectInput(display, w, StructureNotifyMask|EnterWindowMask);
    win_size(w, &window_x, &window_y, &window_width, &window_height);
    win_add(w);
    current_client = client_list->prev;

    if (window_x + window_y == 0) win_center((Arg){0});

    XMapWindow(display, w);
    win_focus(client_list->prev);
}

void mapping_notify(XEvent *e) {
    XMappingEvent *ev = &e->xmapping;

    if (ev->request == MappingKeyboard || ev->request == MappingModifier) {
        XRefreshKeyboardMapping(ev);
        input_grab(root_window);
    }
}

void run(const Arg arg) {
    if (fork()) return;
    if (display) close(ConnectionNumber(display));

    setsid();
    signal(SIGCHLD, SIG_DFL);
    execvp((char*)arg.com[0], (char**)arg.com);
    
    _exit(111);
}

void input_grab(Window grab_window) {
    unsigned int i;
    unsigned int j;

    XModifierKeymap *modifier_map = XGetModifierMapping(display);
    KeyCode code;
    KeyCode num_lock_code = XKeysymToKeycode(display, XK_Num_Lock);
    num_lock_mask = 0;

    if (modifier_map && num_lock_code)
        for (i = 0; i < 8; i++)
            for (int modifier_index = 0; modifier_index < modifier_map->max_keypermod; modifier_index++)
                if (modifier_map->modifiermap[i * modifier_map->max_keypermod + modifier_index] == num_lock_code)
                    num_lock_mask = (1 << i);

    unsigned int modifiers[] = {0, LockMask, num_lock_mask, num_lock_mask|LockMask};

    XUngrabKey(display, AnyKey, AnyModifier, grab_window);
    XUngrabButton(display, AnyButton, AnyModifier, grab_window);

    for (i = 0; i < sizeof(keys)/sizeof(*keys); i++)
        if ((code = XKeysymToKeycode(display, keys[i].keysym)))
            for (j = 0; j < sizeof(modifiers)/sizeof(*modifiers); j++)
                XGrabKey(display, code, keys[i].mod | modifiers[j], grab_window, True, GrabModeAsync, GrabModeAsync);

    for (i = 1; i < 4; i += 2)
        for (j = 0; j < sizeof(modifiers)/sizeof(*modifiers); j++)
            XGrabButton(display, i, MOD | modifiers[j], grab_window, True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
                GrabModeAsync, GrabModeAsync, 0, 0);

    if (modifier_map) XFreeModifiermap(modifier_map);
}

int main(void) {
    XEvent ev;

    if (!(display = XOpenDisplay(0))) exit(1);

    signal(SIGCHLD, SIG_IGN);

    int s = DefaultScreen(display);
    root_window = RootWindow(display, s);

    screen_width = XDisplayWidth(display, s);
    screen_height = XDisplayHeight(display, s);

    XSetErrorHandler(xerror_start);
    XSelectInput(display, root_window, SubstructureRedirectMask);
    XSync(display, False);
    XSetErrorHandler(xerror);
    XDefineCursor(display, root_window, XCreateFontCursor(display, XC_left_ptr));

    input_grab(root_window);

    while (!XNextEvent(display, &ev))
        if (ev.type < LASTEvent && events[ev.type]) events[ev.type](&ev);
}
