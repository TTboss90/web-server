#pragma once
#include <stdbool.h>

enum Method
{
    GET = 1,
    POST
};

struct item
{
    char name[16];
    char value[256];
};

struct header
{
    enum Method Method;
    char Target[1024];
    char Version[16];

    char Host[128];
    char Connection[32];
    int Content_Length;
    char Cache_Control[64];
    char sec_ch_ua_platform[32];
    bool Upgrade_Insecure_Requests;
    char Content_Type[64];
    char Origin[1024];
    char User_Agent[512];
    char sec_ch_ua[128];
    bool sec_ch_ua_mobile;
    char Accept[512];
    bool Sec_GPC;
    char Accept_Language[32];
    char Sec_Fetch_Site[32];
    char Sec_Fetch_Mode[32];
    bool Sec_Fetch_User;
    char Sec_Fetch_Dest[32];
    char Referer[512];
    char Accept_Encoding[64];

    struct item* Cookies;
    int n_cookies;
    struct item* Post_Data;
    int n_post;
    struct item* Get_Data;
    int n_get;
};

#ifdef header_funcs
struct header get_header_data(const char* buffer, ssize_t size);
void print_header(const struct header *h);
int parse(char* query, struct item* items, const char separator, int max_items);
#endif