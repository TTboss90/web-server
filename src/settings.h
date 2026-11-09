#pragma once

struct settings_t
{
    int port;
    char url[32];
};

extern struct settings_t settings;

void init_settings();
