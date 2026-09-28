#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "qqqwm.h"
#include "config.h"

static client *client_list = {0}, *workspace_clients[NUMBER_OF_WS] = {0}, *current_client;

static int          window_x,     window_y,      num_lock_mask = 0;
static int          screen_width, screen_height, current_workspace = 1;
static unsigned int window_width, window_height;

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
        return;
    }

    current_client = c;
    XSetInputFocus(display, current_client->w, RevertToParent, CurrentTime);
}

void notify_enter(XEvent *e) {
    while(XCheckTypedEvent(display, EnterNotify, e));

    for win if (client_item->w == e->xcrossing.window) win_focus(client_item);
}

void notify_destroy(XEvent *e) {
    win_del(e->xdestroywindow.window);

    if (client_list) win_focus(client_list->prev);
}

void notify_motion(XEvent *e) {
    if (!mouse_event.subwindow || !current_client || current_client->f) return;

    while(XCheckTypedEvent(display, MotionNotify, e));

    int x_delta = e->xbutton.x_root - mouse_event.x_root;
    int y_delta = e->xbutton.y_root - mouse_event.y_root;

    XMoveResizeWindow(display, mouse_event.subwindow,
        window_x + (mouse_event.button == 1 ? x_delta : 0),
        window_y + (mouse_event.button == 1 ? y_delta : 0),
        MAX(1, window_width + (mouse_event.button == 3 ? x_delta : 0)),
        MAX(1, window_height + (mouse_event.button == 3 ? y_delta : 0)));
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

    win_size(e->xbutton.subwindow, &window_x, &window_y, &window_width, &window_height);
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
        new_client->prev        = client_list->prev;
        client_list->prev       = new_client;
        new_client->next        = client_list;

    } else {
        client_list = new_client;
        client_list->prev = client_list->next = client_list;
    }

    ws_save(current_workspace);
}

void win_del(Window w) {
    client *removed_client = 0;

    for win if (client_item->w == w) removed_client = client_item;

    if (!client_list || !removed_client)        return;
    if (removed_client->prev == removed_client) client_list = 0;
    if (client_list == removed_client)          client_list = removed_client->next;
    if (removed_client->next)                   removed_client->next->prev = removed_client->prev;
    if (removed_client->prev)                   removed_client->prev->next = removed_client->next;
    if (current_client == removed_client)       current_client = 0;

    free(removed_client);
    ws_save(current_workspace);
}

void win_kill(const Arg arg) {
    if (current_client) XKillClient(display, current_client->w);
}

void win_center(const Arg arg) {
    if (!current_client) return;

    win_size(current_client->w, &(int){0}, &(int){0},
        &window_width, &window_height);
    XMoveWindow(display, current_client->w,
        (screen_width - window_width) / 2,
        (screen_height - window_height) / 2);
}

void win_fs(const Arg arg) {
    if (!current_client) return;

    if ((current_client->f = current_client->f ? 0 : 1)) {
        win_size(current_client->w, &current_client->wx, &current_client->wy,
            &current_client->ww, &current_client->wh);
        XMoveResizeWindow(display, current_client->w, 0, 0,
            screen_width, screen_height);

    } else {
        XMoveResizeWindow(display, current_client->w,
            current_client->wx, current_client->wy,
            current_client->ww, current_client->wh);
    }
}

void win_to_ws(const Arg arg) {
    int previous_workspace = current_workspace;
    Window window;

    if (!current_client || arg.i == previous_workspace) return;
    window = current_client->w;

    ws_sel(arg.i);
    win_add(window);
    ws_save(arg.i);

    ws_sel(previous_workspace);
    win_del(window);
    XUnmapWindow(display, window);
    ws_save(previous_workspace);

    if (client_list) win_focus(client_list);
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

void ws_go(const Arg arg) {
    int previous_workspace = current_workspace;
    if (arg.i == current_workspace) return;

    ws_save(current_workspace);

    ws_sel(arg.i);
    for win XMapWindow(display, client_item->w);
    ws_sel(previous_workspace);
    for win XUnmapWindow(display, client_item->w);
    ws_sel(arg.i);

    if (client_list) win_focus(client_list); else current_client = 0;
}

void configure_request(XEvent *e) {
    XConfigureRequestEvent *ev = &e->xconfigurerequest;

    XConfigureWindow(display, ev->window, ev->value_mask, &(XWindowChanges) {
        .x = ev->x,
        .y = ev->y,
        .width  = ev->width,
        .height = ev->height,
        .sibling    = ev->above,
        .stack_mode = ev->detail
    });
}

void map_request(XEvent *e) {
    Window w = e->xmaprequest.window;

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
    execvp((char*)arg.com[0], (char**)arg.com);
    
    exit(111); /* Exit Failure */
}

void input_grab(Window grab_window) {
    unsigned int i;
    unsigned int j;
    unsigned int modifiers[] = {0, LockMask, num_lock_mask, num_lock_mask|LockMask}; 

    XModifierKeymap *modifier_map = XGetModifierMapping(display);
    KeyCode code;

    for (i = 0; i < 8; i++)
        for (int modifier_index = 0; modifier_index < modifier_map->max_keypermod; modifier_index++)
            if (modifier_map->modifiermap[i * modifier_map->max_keypermod + modifier_index] == XKeysymToKeycode(display, 0xff7f))
                num_lock_mask = (1 << i);

    XUngrabKey(display, AnyKey, AnyModifier, grab_window);

    for (i = 0; i < sizeof(keys)/sizeof(*keys); i++)
        if ((code = XKeysymToKeycode(display, keys[i].keysym)))
            for (j = 0; j < sizeof(modifiers)/sizeof(*modifiers); j++)
                XGrabKey(display, code, keys[i].mod | modifiers[j], grab_window, True, GrabModeAsync, GrabModeAsync);

    for (i = 1; i < 4; i += 2)
        for (j = 0; j < sizeof(modifiers)/sizeof(*modifiers); j++)
            XGrabButton(display, i, MOD | modifiers[j], grab_window, True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
                GrabModeAsync, GrabModeAsync, 0, 0);

    XFreeModifiermap(modifier_map);
}

int main(void) {
    XEvent ev;

    if (!(display = XOpenDisplay(0))) exit(1);

    signal(SIGCHLD, SIG_IGN);
    XSetErrorHandler(xerror);

    int s = DefaultScreen(display);
    root_window = RootWindow(display, s);

    screen_width = XDisplayWidth(display, s);
    screen_height = XDisplayHeight(display, s);

    XSelectInput(display, root_window, SubstructureRedirectMask);
    XDefineCursor(display, root_window, XCreateFontCursor(display, XC_left_ptr));

    input_grab(root_window);

    while (1 && !XNextEvent(display, &ev)) // 1 && will forever be here...
        if (events[ev.type]) events[ev.type](&ev);
}
