#include <X11/Xlib.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define win (client *previous_client = 0, *client_item = client_list; client_item && previous_client != client_list->prev; previous_client = client_item, client_item = client_item->next)
#define win_size(W, gx, gy, gw, gh)                          
    XGetGeometry(display, W, &(Window){0}, gx, gy, gw, gh, &(unsigned int){0}, &(unsigned int){0})

#define mod_clean(mask) (mask & ~(num_lock_mask | LockMask) & \
        (ShiftMask | ControlMask | Mod1Mask | Mod2Mask | Mod3Mask | Mod4Mask | Mod5Mask))

#define ws_save(W) workspace_clients[W] = client_list
#define ws_sel(W)  client_list = workspace_clients[current_workspace = W]

typedef struct {
    const char** com;
    const int i;
    const Window w;
} Arg;

struct key {
    unsigned int mod;
    KeySym keysym;
    void (*function)(const Arg arg);
    const Arg arg;
};

typedef struct client {
    struct client *next, *prev;
    int f, wx, wy;
    unsigned int ww, wh;
    Window w;
} client;

void run(const Arg arg);
void key_press(XEvent *e);
void button_press(XEvent *e);
void button_release(XEvent *e);
void input_grab(Window grab_window);
void map_request(XEvent *e);
void configure_request(XEvent *e);
void mapping_notify(XEvent *e);
void notify_enter(XEvent *e);
void notify_motion(XEvent *e);
void notify_destroy(XEvent *e);
void win_add(Window w);
void win_del(Window w);
void win_kill(const Arg arg);
void win_fs(const Arg arg);
void win_focus(client *c);
void win_center(const Arg arg);
void win_prev(const Arg arg);
void win_next(const Arg arg);
void win_to_ws(const Arg arg);
void ws_go(const Arg arg);

static int xerror() { return 0; }
