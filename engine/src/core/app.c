#include "app.h"
#include "logger.h"
#include "platform/platform.h"

typedef struct
{
    b8 is_running;
    b8 is_suspend;
    platform_state platform;
    i16 width;
    i16 height;
    i16 last_time;
} application_state;

static b8 initialized = FALSE;
static application_state app_state;

b8 application_create(application_config* config)
{
    if (initialized)
    {
        ACERROR("Application already running");
        return FALSE;
    }

    init_log();

    ACFATAL("Test Message: %f", 20.0f);
    ACERROR("Test Message: %f", 20.0f);
    ACWARN("Test Message: %f", 20.0f);
    ACINFO("Test Message: %f", 20.0f);
    ACDEBUG("Test Message: %f", 20.0f);
    ACTRACE("Test Message: %f", 20.0f);

    app_state.is_running = TRUE;
    app_state.is_suspend = FALSE;

    if (!platform_startup(&app_state.platform, config->title, config->pos_x, config->pos_y, config->width, config->height))
        return FALSE;

    initialized = TRUE;
    return TRUE;
}

b8 application_run()
{
    while (app_state.is_running)
    {
        if (!platform_push_msg(&app_state.platform))
            app_state.is_running = FALSE;
    }

    app_state.is_running = FALSE;
    platform_shutdown(&app_state.platform);

    return TRUE;
}
