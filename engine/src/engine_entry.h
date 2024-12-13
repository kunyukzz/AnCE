#pragma once

#include "core/app.h"
#include "core/logger.h"
#include "core/acmemory.h"
#include "engine_game.h"

extern b8 create_game(game* out_game);

int main(void)
{
    memory_initialize();

    game game_inst;
    if (!create_game(&game_inst))
    {
        ACFATAL("Could not create game")
        return -1;
    }

    if (!game_inst.initialize || !game_inst.update || !game_inst.render || !game_inst.on_resize)
    {
        ACFATAL("The Game function's pointer must be assigned!!");
        return -2;
    }

    // Initialized
    if (!application_create(&game_inst))
    {
        ACFATAL("Application Failed to create!");
        return 1;
    }

    // Game loop
    if (!application_run())
    {
        ACINFO("Application not shutdown gracefully");
        return -2;
    }

    memory_shutdown();

    return 0;
}
