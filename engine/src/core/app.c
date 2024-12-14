#include "core/app.h"
#include "engine_game.h"

#include "core/logger.h"

#include "core/acmemory.h"
#include "core/event.h"
#include "core/input.h"
#include "platform/platform.h"
#include <core/clock.h>

#include "renderer/renderer_frontend.h"

typedef struct
{
    game* game_inst;
    b8 is_running;
    b8 is_suspend;
    platform_state platform;
    i16 width;
    i16 height;
    clock clock;
    i16 last_time;
} application_state;

static b8 initialized = FALSE;
static application_state app_state;

// Event handler
b8 application_on_event(u16 code, void* sender, void* listener, event_context context);
b8 application_on_key(u16 code, void* sender, void* listener, event_context context);

b8 application_create(game* game_inst)
{
    if (initialized)
    {
        ACERROR("Application already running");
        return FALSE;
    }

    app_state.game_inst = game_inst;
    init_log();
    input_initialize();

    //ACFATAL("Test Message: %f", 20.0f);
    //ACERROR("Test Message: %f", 20.0f);
    //ACWARN("Test Message: %f", 20.0f);
    //ACINFO("Test Message: %f", 20.0f);
    //ACDEBUG("Test Message: %f", 20.0f);
    //ACTRACE("Test Message: %f", 20.0f);

    app_state.is_running = TRUE;
    app_state.is_suspend = FALSE;

    if (!event_initialize())
    {
        ACERROR("Event system failed to Initialized!");
        return FALSE;
    }

    ac_event_register_t(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    ac_event_register_t(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    ac_event_register_t(EVENT_CODE_KEY_RELEASE, 0, application_on_key);

    if (!platform_startup(&app_state.platform,
                          game_inst->app_config.title,
                          game_inst->app_config.pos_x,
                          game_inst->app_config.pos_y,
                          game_inst->app_config.width,
                          game_inst->app_config.height))
        return FALSE;

    // renderer start
    if (!renderer_initialize(game_inst->app_config.title, &app_state.platform))
    {
        ACFATAL("Failed to initialize renderer");
        return FALSE;
    }

    // game start
    if (!app_state.game_inst->initialize(app_state.game_inst))
    {
        ACFATAL("Game failed to Initialize");
        return FALSE;
    }

    app_state.game_inst->on_resize(app_state.game_inst, app_state.width, app_state.height);

    initialized = TRUE;
    return TRUE;
}

b8 application_run()
{
    clock_start(&app_state.clock);
    clock_update(&app_state.clock);
    app_state.last_time = app_state.clock.elapsed;
    f64 running_time = 0;
    u8 frame_count = 0;
    f64 target_frame_seconds = 1.0f / 60;

    ACINFO(ac_get_memory_usage_t());

    while (app_state.is_running)
    {
        if (!platform_push_msg(&app_state.platform))
            app_state.is_running = FALSE;

        if (!app_state.is_suspend)
        {
            // updating time and get delta time
            clock_update(&app_state.clock);
            f64 current_time = app_state.clock.elapsed;
            f64 delta = (current_time - app_state.last_time);
            f64 frame_start_time = platform_get_absolute_time();

            if (!app_state.game_inst->update(app_state.game_inst, (f32)delta))
            {
                ACFATAL("Game Update Failed, Shutting Down.");
                app_state.is_running = FALSE;
                break;
            }

            if (!app_state.game_inst->render(app_state.game_inst, (f32)delta))
            {
                ACFATAL("Game Render Failed, Shutting Down");
                app_state.is_running = FALSE;
                break;
            }

            // TEST:
            render_packet packet;
            packet.delta_time = delta;
            renderer_draw_frame(&packet);

            // calculate how long frame took
            f64 frame_end_time = platform_get_absolute_time();
            f64 frame_elapsed_time = frame_end_time - frame_start_time;
            running_time += frame_elapsed_time;
            f64 remaining_seconds = target_frame_seconds - frame_elapsed_time;
            if (remaining_seconds > 0)
            {
                u64 remaining_ms = (remaining_seconds * 1000);
                b8 limit = FALSE;
                if (remaining_ms > 0 && limit)
                    platform_sleep(remaining_ms - 1);
                frame_count++;
            }

            input_update(delta);
            app_state.last_time = current_time;
        }
    }

    app_state.is_running = FALSE;

    ac_event_unregister_t(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    ac_event_unregister_t(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    ac_event_unregister_t(EVENT_CODE_KEY_RELEASE, 0, application_on_key);
    event_shutdown();
    input_shutdown();
    renderer_shutdown();

    platform_shutdown(&app_state.platform);
    return TRUE;
}

b8 application_on_event(u16 code, void* sender, void* listener, event_context context)
{
    switch (code)
    {
    case EVENT_CODE_APPLICATION_QUIT: {
        ACINFO("EVENT_CODE_APPLICATION_QUIT received!. Shutting Down.");
        app_state.is_running = FALSE;
        return TRUE;
    }
    }

    ACDEBUG("LALALAL");
    return FALSE;
}

b8 application_on_key(u16 code, void* sender, void* listener, event_context context)
{
    if (code == EVENT_CODE_KEY_PRESSED)
    {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_ESCAPE)
        {
            event_context data = {};
            ac_event_fire_t(EVENT_CODE_APPLICATION_QUIT, 0, data);
            return TRUE;
        }
        else if (key_code == KEY_A)
        {
            ACDEBUG("Explicit! A Key Pressed");
        }
        else
        {
            ACDEBUG(" '%c' key pressed in window", key_code);
        }
    }
    else if (code == EVENT_CODE_KEY_RELEASE)
    {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_B)
        {
            ACDEBUG("Explicit! B key Released");
        }
        else
        {
            ACDEBUG(" '%c' key being released", key_code);
        }
    }

    return FALSE;
}
