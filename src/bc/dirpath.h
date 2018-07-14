#pragma once

#include "dircont.h"

typedef struct __dircont DIRPATH;

bool dpt_move(DIRPATH *dpt, const char *subdir);
bool dpt_up(DIRPATH *dpt);

void dpt_init(DIRPATH *dpt, const char *path);
//DIRPATH *dpt_create(const char *path);
char *dpt_string(DIRPATH *dpt, char *buf);
