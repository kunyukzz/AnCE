#include "renderer_frontend.h"

#include "renderer_backend.h"

#include "core/acmemory.h"
#include "core/logger.h"

static renderer_backend* backend = 0;

b8 renderer_initialize(const char* app_name, struct platform_state* plat_state)
{
    backend = ac_allocate_t(sizeof(renderer_backend), MEMTAG_RENDERER);
    backend->frame_number = 0;

    // TODO: make this configure-able.
    renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, plat_state, backend);

    if (!backend->initialize(backend, app_name, plat_state))
    {
        ACFATAL("Renderer backend failed to initialize, Shutting down.");
        return FALSE;
    }

    return TRUE;
}

void renderer_shutdown()
{
    backend->shutdown(backend);
    ac_free_t(backend, sizeof(renderer_backend), MEMTAG_RENDERER);
}

b8 renderer_begin_frame(f32 delta_time)
{
    return backend->begin_frame(backend, delta_time);
}

b8 renderer_end_frame(f32 delta_time)
{
    b8 result = backend->end_frame(backend, delta_time);
    backend->frame_number++;
    return result;
}

b8 renderer_draw_frame(render_packet* packet)
{
    if (renderer_begin_frame(packet->delta_time))
    {
        b8 result = renderer_end_frame(packet->delta_time);

        if (!result)
        {
            ACERROR("renderer_end_frame failed!. Application shutting down...");
            return FALSE;
        }
    }
    return TRUE;
}
