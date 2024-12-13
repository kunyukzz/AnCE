#include "core/event.h"
#include "container/dyn_array.h"
#include "core/acmemory.h"

typedef struct registered_event
{
    void* listener;
    pfn_on_event callback;
} registered_event;

typedef struct event_code_entry
{
    registered_event* events;
} event_code_entry;

#define MAX_MSG_CODES 8384

// state structure
typedef struct event_sys_state
{
    // lookup table for event codes.
    event_code_entry registered[MAX_MSG_CODES];
} event_sys_state;

static b8 is_initialized = FALSE;
static event_sys_state state;

b8 event_initialize()
{
    if (is_initialized == TRUE)
        return FALSE;

    is_initialized = FALSE;

    ac_zero_memory_t(&state, sizeof(state));

    is_initialized = TRUE;

    return TRUE;
}

void event_shutdown()
{
    for (u16 i = 0; i < MAX_MSG_CODES; ++i)
    {
        if (state.registered[i].events != 0)
        {
            ac_dyn_array_destroy_t(state.registered[i].events);
            state.registered[i].events = 0;
        }
    }
}

b8 ac_event_register_t(u16 code, void* listener, pfn_on_event on_event)
{
    if (is_initialized == FALSE)
        return FALSE;

    if (state.registered[code].events == 0)
        state.registered[code].events = ac_dyn_array_create_t(registered_event);

    u64 reg_count = ac_dyn_array_length_t(state.registered[code].events);
    for (u64 i = 0; i < reg_count; ++i)
    {
        if (state.registered[code].events[i].listener == listener)
        {
            return FALSE;
        }
    }

    registered_event event;
    event.listener = listener;
    event.callback = on_event;
    ac_dyn_array_push_t(state.registered[code].events, event);
    return TRUE;
}

b8 ac_event_unregister_t(u16 code, void* listener, pfn_on_event on_event)
{
    if (is_initialized == FALSE)
        return FALSE;

    if (state.registered[code].events == 0)
    {
        return FALSE;
    }

    u64 reg_count = ac_dyn_array_length_t(state.registered[code].events);
    for (u64 i = 0; i < reg_count; ++i)
    {
        registered_event e = state.registered[code].events[i];
        if (e.listener == listener && e.callback == on_event)
        {
            registered_event popped_event;
            ac_dyn_array_pop_at_t(state.registered[code].events, i, &popped_event);
            return TRUE;
        }
    }

    return FALSE;
}

b8 ac_event_fire_t(u16 code, void* sender, event_context context)
{
    if (is_initialized == FALSE)
        return FALSE;

    if (state.registered[code].events == 0)
        return FALSE;

    u64 reg_count = ac_dyn_array_length_t(state.registered[code].events);
    for (u64 i = 0; i < reg_count; ++i)
    {
        registered_event e = state.registered[code].events[i];
        if (e.callback(code, sender, e.listener, context))
            return TRUE;
    }

    return FALSE;
}
