#pragma once
#include "header.h"

char* http_err(const int code, const char* text);
void* build_response(char* buffer, struct header* header);