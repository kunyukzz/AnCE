#include "core/input.h"
#include "core/event.h"
#include "core/acmemory.h"
#include "core/logger.h"

typedef struct keyboard_state
{
    b8 keys[256];
} keyboard_state;

typedef struct mouse_state
{
    i16 x;
    i16 y;
    u8 buttons[BUTTON_MAX];
} mouse_state;

typedef struct input_state
{
    keyboard_state kbd_current;
    keyboard_state kbd_prev;
    mouse_state mouse_current;
    mouse_state mouse_prev;
} input_state;

static b8 initialized = FALSE;
static input_state state = {};

void input_initialize()
{
    ac_zero_memory_t(&state, sizeof(input_state));
    initialized = TRUE;
    ACINFO("Input subsystem initialized");
}

void input_update(f64 delta_time)
{
    if (!initialized)
        return;

    // copy current state tp previous state
    ac_copy_memory_t(&state.kbd_prev, &state.kbd_current, sizeof(keyboard_state));
    ac_copy_memory_t(&state.mouse_prev, &state.mouse_current, sizeof(mouse_state));
}

void input_shutdown()
{
    initialized = FALSE;
}

void input_process_key(keys key, b8 pressed)
{
    if (state.kbd_current.keys[key] != pressed)
    {
        // update internal state
        state.kbd_current.keys[key] = pressed;

        event_context context;
        context.data.u16[0] = key;
        ac_event_fire_t(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASE, 0, context);
    }
}

void input_process_button(buttons button, b8 pressed)
{
    if (state.mouse_current.buttons[button] != pressed)
    {
        state.mouse_current.buttons[button] = pressed;

        event_context context;
        context.data.u16[0] = button;
        ac_event_fire_t(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASE, 0, context);
    }
}

void input_process_mouse_move(i16 x, i16 y)
{
    if (state.mouse_current.x != x || state.mouse_current.y != y)
    {
        // NOTE: enable this to check mouse position.
        // ACDEBUG("Mouse Pos: %i, %i", x, y);

        // update internal state
        state.mouse_current.x = x;
        state.mouse_current.y = y;

        event_context context;
        context.data.u16[0] = x;
        context.data.u16[1] = y;
        ac_event_fire_t(EVENT_CODE_MOUSE_MOVE, 0, context);
    }
}

void input_process_mouse_wheel(i8 z_delta)
{
    event_context context;
    context.data.u8[0] = z_delta;
    ac_event_fire_t(EVENT_CODE_MOUSE_WHEEL, 0, context);
}

// ----------------- keyboar input --------------------
b8 input_key_down(keys key)
{
    if (!initialized)
        return FALSE;
    return state.kbd_current.keys[key] == TRUE;
}

b8 input_key_up(keys key)
{
    if (!initialized)
        return TRUE;
    return state.kbd_current.keys[key] == FALSE;
}

b8 input_was_key_down(keys key)
{
    if (!initialized)
        return FALSE;
    return state.kbd_prev.keys[key] == TRUE;
}

b8 input_was_key_up(keys key)
{
    if (!initialized)
        return TRUE;
    return state.kbd_prev.keys[key] == FALSE;
}

// ----------------- mouse input --------------------
b8 input_mouse_button_down(buttons button)
{
    if (!initialized)
        return FALSE;
    return state.mouse_current.buttons[button] == TRUE;
}

b8 input_mouse_button_up(buttons button)
{
    if (!initialized)
        return TRUE;
    return state.mouse_current.buttons[button] == FALSE;
}

b8 input_mouse_was_button_down(buttons button)
{
    if (!initialized)
        return FALSE;
    return state.mouse_prev.buttons[button] == TRUE;
}

b8 input_mouse_was_button_up(buttons button)
{
    if (!initialized)
        return TRUE;
    return state.mouse_prev.buttons[button] == FALSE;
}

void input_get_mouse_pos(i32* x, i32* y)
{
    if (!initialized)
    {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state.mouse_current.x;
    *y = state.mouse_current.y;
}

void input_get_prev_mouse_pos(i32* x, i32* y)
{
    if (!initialized)
    {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state.mouse_prev.x;
    *y = state.mouse_prev.y;
}
