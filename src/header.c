#define header_funcs
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>
#include <fcntl.h>
#include "socket.h"
#include "server.h"
#include "response.h"
#include "header.h"

#define STR_LEN(hdr) (sizeof(hdr) - 1)
#define min(a, b) ((a) < (b) ? (a) : (b))

void print_header(const struct header *h)
{
    if (h == NULL)
        return;

    printf("=== HTTP Header ===\n");

    printf("Method: %s\n", h->Method == GET ? "GET" :
                           h->Method == POST ? "POST" : "UNKNOWN");

    printf("Target: %s\n", h->Target);
    printf("Version: %s\n", h->Version);

    printf("Host: %s\n", h->Host);
    printf("Connection: %s\n", h->Connection);
    printf("Content-Length: %d\n", h->Content_Length);
    printf("Cache-Control: %s\n", h->Cache_Control);
    printf("sec-ch-ua-platform: %s\n", h->sec_ch_ua_platform);
    printf("Upgrade-Insecure-Requests: %s\n",
           h->Upgrade_Insecure_Requests ? "true" : "false");
    printf("Content-Type: %s\n", h->Content_Type);
    printf("Origin: %s\n", h->Origin);
    printf("User-Agent: %s\n", h->User_Agent);
    printf("sec-ch-ua: %s\n", h->sec_ch_ua);
    printf("sec-ch-ua-mobile: %s\n",
           h->sec_ch_ua_mobile ? "true" : "false");
    printf("Accept: %s\n", h->Accept);
    printf("Sec-GPC: %s\n", h->Sec_GPC ? "true" : "false");
    printf("Accept-Language: %s\n", h->Accept_Language);
    printf("Sec-Fetch-Site: %s\n", h->Sec_Fetch_Site);
    printf("Sec-Fetch-Mode: %s\n", h->Sec_Fetch_Mode);
    printf("Sec-Fetch-User: %s\n",
           h->Sec_Fetch_User ? "true" : "false");
    printf("Sec-Fetch-Dest: %s\n", h->Sec_Fetch_Dest);
    printf("Referer: %s\n", h->Referer);
    printf("Accept-Encoding: %s\n", h->Accept_Encoding);

    printf("\nCookies:\n");
    for (int i = 0; i < 16; i++)
    {
        if (h->Cookies[i].name[0] == '\0')
            break;
        printf("  %s = %s\n", h->Cookies[i].name, h->Cookies[i].value);
    }

    printf("\nGET Parameters:\n");
    for (int i = 0; i < 16; i++)
    {
        if (h->Get_Data[i].name[0] == '\0')
            break;
        printf("  %s = %s\n", h->Get_Data[i].name, h->Get_Data[i].value);
    }

    printf("\nPOST Parameters:\n");
    for (int i = 0; i < 16; i++)
    {
        if (h->Post_Data[i].name[0] == '\0')
            break;
        printf("  %s = %s\n", h->Post_Data[i].name, h->Post_Data[i].value);
    }

    printf("===================\n");
}

static void print_data(struct header* data)
{
    printf("%d, %s, %s\n", data->Method, data->Target, data->Version);

    printf("cookies:\n");
    for (int i = 0; i < 16; i++) {
        if (data->Cookies[i].name[0] == '\0'){
            break;
        }
        printf("%s = %s\n", data->Cookies[i].name, data->Cookies[i].value);
    }

    printf("GET:\n");
    for (int i = 0; i < 16; i++) {
        if (data->Get_Data[i].name[0] == '\0'){
            break;
        }
        printf("%s = %s\n", data->Get_Data[i].name, data->Get_Data[i].value);
    }
    
    printf("POST:\n");
    for (int i = 0; i < 16; i++) {
        if (data->Post_Data[i].name[0] == '\0'){
            printf("\n");
            break;
        }
        printf("%s = %s\n", data->Post_Data[i].name, data->Post_Data[i].value);
    }
}

int parse(char* query, struct item* items, const char separator, int max_items)
{
    int i = 0;

    while (query && *query && i < max_items) {
        while (*query == ' ' || *query == '\r' || *query == '\n' || *query == '\t')
            query++;

        char* next = strchr(query, separator);
        if (next && items) {
            *next = '\0';
        }

        char* eq = strchr(query, '=');
        if (eq) {
            if(items){
                *eq = '\0';

                snprintf(items[i].name,
                        sizeof(items[i].name),
                        "%s",
                        query);

                snprintf(items[i].value,
                        sizeof(items[i].value),
                        "%s",
                        eq + 1);
            }

            i++;
        }

        if (!next)
            break;

        query = next + 1;
    }

    return i;
}

static int request_line(char* line, struct header* data)
{
    if (!line || !data)
        return -1;

    char *method_end = strchr(line, ' ');
    if (!method_end)
        return -1;

    if (strncmp(line, "GET ", 4) == 0)
        data->Method = GET;
    else if (strncmp(line, "POST ", 5) == 0)
        data->Method = POST;
    else
        return -1;
        
    char* target = method_end + 1;
    char* version = strchr(target, ' ');

    if (!version)
        return -1;

    *version = '\0';
    version++;

    char* query = strchr(target, '?');

    if (query) {
        *query = '\0';
        query++;

        data->n_get = parse(query, NULL, '&', INT_MAX);
        data->Get_Data = malloc(sizeof(struct item)*data->n_get);
        //data->Get_Data = query;
        parse(query, data->Get_Data, '&', data->n_get);
    }

    snprintf(data->Target, sizeof(data->Target), "%s", target);
    snprintf(data->Version, sizeof(data->Version), "%s", version);

    return 0;
}

static int headers(struct header* data)
{
    char* line;

    char cookie_str[] = "Cookie: ";
    char host_str[] = "Host: ";
    char connection_str[] = "Connection: ";
    char content_length_str[] = "Content-Length: ";
    char cache_control_str[] = "Cache-Control: ";
    char sec_ch_ua_platform_str[] = "sec-ch-ua-platform: ";
    char upgrade_insecure_requests_str[] = "Upgrade-Insecure-Requests: ";
    char content_type_str[] = "Content-Type: ";
    char origin_str[] = "Origin: ";
    char user_agent_str[] = "User-Agent: ";
    char sec_ch_ua_str[] = "sec-ch-ua: ";
    char sec_ch_ua_mobile_str[] = "sec-ch-ua-mobile: ";
    char accept_str[] = "Accept: ";
    char sec_gpc_str[] = "Sec-GPC: ";
    char accept_language_str[] = "Accept-Language: ";
    char sec_fetch_site_str[] = "Sec-Fetch-Site: ";
    char sec_fetch_mode_str[] = "Sec-Fetch-Mode: ";
    char sec_fetch_user_str[] = "Sec-Fetch-User: ";
    char sec_fetch_dest_str[] = "Sec-Fetch-Dest: ";
    char referer_str[] = "Referer: ";
    char accept_encoding_str[] = "Accept-Encoding: ";

    while ((line = strtok(NULL, "\r\n"))) {
        if (strncmp(line, cookie_str, STR_LEN(cookie_str)) == 0){
            data->n_cookies = parse(line + STR_LEN(cookie_str), NULL, ';', INT_MAX);
            data->Cookies = malloc(sizeof(struct item)*data->n_cookies);
            //data->Cookies = line + STR_LEN(cookie_str);
            parse(line + STR_LEN(cookie_str), data->Cookies, ';', data->n_cookies);
        }
        else if (strncmp(line, host_str, STR_LEN(host_str)) == 0)
            snprintf(data->Host, sizeof(data->Host), "%s", line + STR_LEN(host_str));
        else if (strncmp(line, connection_str, STR_LEN(connection_str)) == 0)
            snprintf(data->Connection, sizeof(data->Connection), "%s", line + STR_LEN(connection_str));
        else if (strncmp(line, content_length_str, STR_LEN(content_length_str)) == 0)
            data->Content_Length = atoi(line + STR_LEN(content_length_str));
        else if (strncmp(line, cache_control_str, STR_LEN(cache_control_str)) == 0)
            snprintf(data->Cache_Control, sizeof(data->Cache_Control), "%s", line + STR_LEN(cache_control_str));
        else if (strncmp(line, sec_ch_ua_platform_str, STR_LEN(sec_ch_ua_platform_str)) == 0)
            snprintf(data->sec_ch_ua_platform, sizeof(data->sec_ch_ua_platform), "%s", line + STR_LEN(sec_ch_ua_platform_str));
        else if (strncmp(line, upgrade_insecure_requests_str, STR_LEN(upgrade_insecure_requests_str)) == 0)
            data->Upgrade_Insecure_Requests = atoi(line + STR_LEN(upgrade_insecure_requests_str));
        else if (strncmp(line, content_type_str, STR_LEN(content_type_str)) == 0)
            snprintf(data->Content_Type, sizeof(data->Content_Type), "%s", line + STR_LEN(content_type_str));
        else if (strncmp(line, origin_str, STR_LEN(origin_str)) == 0)
            snprintf(data->Origin, sizeof(data->Origin), "%s", line + STR_LEN(origin_str));
        else if (strncmp(line, user_agent_str, STR_LEN(user_agent_str)) == 0)
            snprintf(data->User_Agent, sizeof(data->User_Agent), "%s", line + STR_LEN(user_agent_str));
        else if (strncmp(line, sec_ch_ua_str, STR_LEN(sec_ch_ua_str)) == 0)
            snprintf(data->sec_ch_ua, sizeof(data->sec_ch_ua), "%s", line + STR_LEN(sec_ch_ua_str));
        else if (strncmp(line, sec_ch_ua_mobile_str, STR_LEN(sec_ch_ua_mobile_str)) == 0)
            data->sec_ch_ua_mobile = (strcmp(line + STR_LEN(sec_ch_ua_mobile_str), "?1") == 0);
        else if (strncmp(line, accept_str, STR_LEN(accept_str)) == 0)
            snprintf(data->Accept, sizeof(data->Accept), "%s", line + STR_LEN(accept_str));
        else if (strncmp(line, sec_gpc_str, STR_LEN(sec_gpc_str)) == 0)
            data->Sec_GPC = atoi(line + STR_LEN(sec_gpc_str));
        else if (strncmp(line, accept_language_str, STR_LEN(accept_language_str)) == 0)
            snprintf(data->Accept_Language, sizeof(data->Accept_Language), "%s", line + STR_LEN(accept_language_str));
        else if (strncmp(line, sec_fetch_site_str, STR_LEN(sec_fetch_site_str)) == 0)
            snprintf(data->Sec_Fetch_Site, sizeof(data->Sec_Fetch_Site), "%s", line + STR_LEN(sec_fetch_site_str));
        else if (strncmp(line, sec_fetch_mode_str, STR_LEN(sec_fetch_mode_str)) == 0)
            snprintf(data->Sec_Fetch_Mode, sizeof(data->Sec_Fetch_Mode), "%s", line + STR_LEN(sec_fetch_mode_str));
        else if (strncmp(line, sec_fetch_user_str, STR_LEN(sec_fetch_user_str)) == 0)
            data->Sec_Fetch_User = (strcmp(line + STR_LEN(sec_fetch_user_str), "?1") == 0);
        else if (strncmp(line, sec_fetch_dest_str, STR_LEN(sec_fetch_dest_str)) == 0)
            snprintf(data->Sec_Fetch_Dest, sizeof(data->Sec_Fetch_Dest), "%s", line + STR_LEN(sec_fetch_dest_str));
        else if (strncmp(line, referer_str, STR_LEN(referer_str)) == 0)
            snprintf(data->Referer, sizeof(data->Referer), "%s", line + STR_LEN(referer_str));
        else if (strncmp(line, accept_encoding_str, STR_LEN(accept_encoding_str)) == 0)
            snprintf(data->Accept_Encoding, sizeof(data->Accept_Encoding), "%s", line + STR_LEN(accept_encoding_str));
    }

    return 0;
}

static int process_body(char* body, struct header* data)
{
    if(data->Method == POST){
        data->n_post = min(parse(body, NULL, '&', INT_MAX),16);
        data->Post_Data = malloc(sizeof(struct item)*data->n_post);
        //data->Post_Data = body;
        parse(body, data->Post_Data, '&', data->n_post);
        return 0;
    }
    return 0;
}

struct header get_header_data(const char* buffer, ssize_t size)
{
    struct header data = {0};
    char* copy = strdup(buffer);
    if (!copy)
        return (struct header){0};
    
    char* header_end = strstr(copy, "\r\n\r\n");
    char* body = NULL;
    if (header_end) {
        *header_end = '\0';
        body = header_end + 4;
    }

    char* line = strtok(copy, "\r\n");

    if (request_line(line, &data)){
        free(copy);
        return (struct header){0};
    }

    if(headers(&data)){
        free(copy);
        return (struct header){0};
    }

    if(header_end && process_body(body, &data)){
        free(copy);
        return (struct header){0};
    }
    
    //print_data(&data);

    free(copy);
    return data;
};
