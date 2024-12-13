#include "game.h"

#include <engine_entry.h>
#include <core/acmemory.h>

b8 create_game(game* out_game)
{
    out_game->app_config.pos_x = 100;
    out_game->app_config.pos_y = 100;
    out_game->app_config.width = 800;
    out_game->app_config.height = 600;
    out_game->app_config.title = "Test Engine";

    out_game->update = game_update;
    out_game->initialize = game_initialize;
    out_game->render = game_render;
    out_game->on_resize = game_on_resize;

    out_game->state = ac_allocate_t(sizeof(game_state), MEMTAG_GAME);

    return TRUE;
}
