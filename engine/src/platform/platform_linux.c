#include "platform/platform.h"

#if ACPLATFORM_LINUX

#include "core/logger.h"

#include <X11/XKBlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <sys/time.h>
#include <xcb/xcb.h>

#if _POSIX_C_SOURCE >= 200809L
#include <time.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    Display* display;
    xcb_connection_t* connection;
    xcb_window_t window;
    xcb_screen_t screen;
    xcb_atom_t wm_protocol;
    xcb_atom_t wm_delete_win;
} internal_state;

b8 platform_startup(platform_state* plat_state, const char* app_name, i32 x, i32 y, i32 width, i32 height)
{
    plat_state->internal_state = malloc(sizeof(internal_state));
    internal_state* state = (internal_state*)plat_state->internal_state;

    // Connect to X server
    state->display = XOpenDisplay(NULL);
    XAutoRepeatOff(state->display);

    // receive connection from display
    state->connection = XGetXCBConnection(state->display);
    if (xcb_connection_has_error(state->connection))
    {
        ACFATAL("Failed to connect to server via XCB.");
        return FALSE;
    }

    // get data from X server
    const struct xcb_setup_t* setup = xcb_get_setup(state->connection);

    xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);
    int screen = 0;
    for (i32 s = screen; s > 0; s--)
    {
        xcb_screen_next(&it);
    }
    state->screen = *it.data;
    state->window = xcb_generate_id(state->connection);

    // register event type
    u32 event_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

    // listen input for mouse and keyboard
    u32 event_values = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
                       XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    // sent value to XCB (bg color and events)
    u32 value_list[] = { state->screen.black_pixel, event_values };

    // create the actual window
    xcb_void_cookie_t cookie = xcb_create_window(state->connection,
                                                 XCB_COPY_FROM_PARENT, // depth
                                                 state->window,
                                                 state->screen.root, // parent
                                                 x,
                                                 y,
                                                 width,
                                                 height,
                                                 0,
                                                 XCB_WINDOW_CLASS_INPUT_OUTPUT, // class
                                                 state->screen.root_visual,     // visual
                                                 event_mask,
                                                 value_list);

    // window properties
    xcb_change_property(
      state->connection, XCB_PROP_MODE_REPLACE, state->window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(app_name), app_name);

    // notify server when windpw manager try to destroy windpw
    xcb_intern_atom_cookie_t wm_delete_cookie = xcb_intern_atom(state->connection, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
    xcb_intern_atom_cookie_t wm_protocol_cookie = xcb_intern_atom(state->connection, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* wm_delete_reply = xcb_intern_atom_reply(state->connection, wm_delete_cookie, NULL);
    xcb_intern_atom_reply_t* wm_protocol_reply = xcb_intern_atom_reply(state->connection, wm_protocol_cookie, NULL);

    state->wm_delete_win = wm_delete_reply->atom;
    state->wm_protocol = wm_protocol_reply->atom;

    xcb_change_property(state->connection, XCB_PROP_MODE_REPLACE, state->window, wm_protocol_reply->atom, 4, 32, 1, &wm_delete_reply->atom);

    // map window to screen
    xcb_map_window(state->connection, state->window);

    // flush stream
    i32 stream = xcb_flush(state->connection);
    if (stream <= 0)
    {
        ACFATAL("Error when flushing stream: %d", stream);
        return FALSE;
    }

    return TRUE;
}

void platform_shutdown(platform_state* plat_state)
{
    internal_state* state = (internal_state*)plat_state->internal_state;
    XAutoRepeatOn(state->display);
    xcb_destroy_window(state->connection, state->window);
}

b8 platform_push_msg(platform_state* plat_state)
{
    internal_state* state = (internal_state*)plat_state->internal_state;

    xcb_generic_event_t* event;
    xcb_client_message_event_t* cm;

    b8 quit_flag = FALSE;

    while (event != 0)
    {
        event = xcb_poll_for_event(state->connection);
        if (event == 0)
        {
            break;
        }

        // check with bitwise operation. idk why linux using this
        switch (event->response_type & -0x80)
        {
        case XCB_KEY_PRESS:
        case XCB_KEY_RELEASE: {
            // TODO: keyboard input process
        }
        break;
        case XCB_BUTTON_PRESS:
        case XCB_BUTTON_RELEASE: {
            // TODO: mouse button input process
        }
        case XCB_MOTION_NOTIFY:
            break;
        case XCB_CONFIGURE_NOTIFY: {
            // TODO:implementing window resize
        }
        case XCB_CLIENT_MESSAGE: {
            cm = (xcb_client_message_event_t*)event;

            if (cm->data.data32[0] == state->wm_delete_win)
                quit_flag = TRUE;
        }
        break;
        default:
            break;
        }
        free(event);
    }
    return !quit_flag;
}

void* platform_allocated(u64 size, b8 aligned)
{
    return malloc(size);
}

void platform_free(void* block, b8 aligned)
{
    free(block);
}

void* platform_zero_mem(void* block, u64 size)
{
    return memset(block, 0, size);
}

void* platform_copy_mem(void* dest, const void* source, u64 size)
{
    return memcpy(dest, source, size);
}

void* platform_set_mem(void* dest, i32 value, u64 size)
{
    return memset(dest, value, size);
}

void platform_console_write(const char* msg, u8 color)
{
    // FATAL, ERROR, WARN, INFO, DEBUG, TRACE
    const char* color_string[] = { "0;41", "1;31", "1;33", "1;32", "1;34", "1;30" };
    printf("\033[%sm%s\033[0m", color_string[color], msg);
}

void platform_console_write_error(const char* msg, u8 color)
{
    // FATAL, ERROR, WARN, INFO, DEBUG, TRACE
    const char* color_string[] = { "0;41", "1;31", "1;33", "1;32", "1;34", "1;30" };
    printf("\033[%sm%s\033[0m", color_string[color], msg);
}

f64 platform_get_absolute_time()
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec + now.tv_nsec * 0.000000001;
}

void platform_sleep(u64 ms)
{
#if _POSIX_C_SOURCE >= 200809L
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000 * 1000;
#else
    if (ms >= 1000)
    {
        sleep(ms / 1000);
    }
    usleep((ms % 1000) * 1000)
#endif
}

#endif // ACPLATFORM_LINUX

