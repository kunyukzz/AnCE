#include <core/assertion.h>
#include <core/logger.h>
#include <core/app.h>

int main(void)
{
    application_config config;
    config.pos_x = 100;
    config.pos_y = 100;
    config.width = 800;
    config.height = 600;
    config.title = "Test Engine";

    application_create(&config);
    application_run();
    return 0;
}
