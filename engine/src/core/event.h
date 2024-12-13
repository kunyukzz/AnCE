#pragma once

#include "define.h"

typedef struct event_context
{
    union {
        i64 i64[2];
        u64 u64[2];
        f64 f64[2];

        i32 i32[4];
        u32 u32[4];
        f32 f32[4];

        i16 i16[8];
        u16 u16[8];

        i8 i8[16];
        u8 u8[16];

        char c[16];
    } data;
} event_context;

/* INFO:
 * Callback type definition for event handlers.
 * code: The event code to listen for.
 * sender: A pointer to the source triggering the event.
 * listener_inst: A pointer to the listener instance receiving the event.
 * data: Event-specific context/data passed along with the event.
 * Returns: True if the event was handled successfully; false otherwise.
 */
typedef b8 (*pfn_on_event)(u16 code, void* sender, void* listener_inst, event_context data);

b8 event_initialize();
void event_shutdown();

/* INFO:
 * Registers to listen when events are sent.
 * code: The event code to listen for.
 * listener: A pointer to the listener instance that will handle the event. This can be NULL.
 * on_event: The callback function to invoke when the event occurs.
 * Returns: True if the registration was successful, false otherwise.
 *
 * WARN: Event with multiple callback/listener will not register and will cause this to return FALSE
 */
ACAPI b8 ac_event_register_t(u16 code, void* listener, pfn_on_event on_event);


/* INFO:
 * Unregisters a callback for a specific event code.
 * code: The event code to stop listening for.
 * listener: A pointer to the listener instance that was registered. This can be NULL.
 * on_event: The callback function to remove from the event system.
 * Returns: True if the unregistration was successful, false otherwise.
 *
 * WARN: If the event or callback does not exist, this will return FALSE.
 */
ACAPI b8 ac_event_unregister_t(u16 code, void* listener, pfn_on_event on_event);


/* INFO:
 * Fires an event, notifying all registered listeners for the specified event code.
 * code: The event code to trigger.
 * sender: A pointer to the entity or system triggering the event. This can be NULL.
 * context: The event context, containing additional data for the event.
 * Returns: True if at least one listener handled the event, false otherwise.
 *
 * WARN: This is a synchronous call and may block if the callback is time-consuming and
 *       not passed to any more listeners.
 */
ACAPI b8 ac_event_fire_t(u16 code, void* sender, event_context context);


// system internal code. Application should use cpdes beyond 255.
typedef enum sys_event_code
{
    EVENT_CODE_APPLICATION_QUIT = 0x01, // shutdown application to next frame.
    EVENT_CODE_KEY_PRESSED = 0x02,      // keyboard key press
    EVENT_CODE_KEY_RELEASE = 0x03,      // keyboard key release
    EVENT_CODE_BUTTON_PRESSED = 0x04,   // mpuse button press
    EVENT_CODE_BUTTON_RELEASE = 0x05,   // mouse button release
    EVENT_CODE_MOUSE_MOVE = 0x06,       // mouse moved
    EVENT_CODE_MOUSE_WHEEL = 0x07,      // mouse wheel
    EVENT_CODE_RESIZED = 0x08,          // resize window
    MAX_EVENT_CODE = 0xFF
} sys_event_code;
