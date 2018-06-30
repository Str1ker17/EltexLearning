#pragma once

#include "dircont.h"

typedef struct __dircont DIRPATH;

bool dpt_move(DIRPATH *dpt, char *subdir);
bool dpt_up(DIRPATH *dpt);

void dpt_init(DIRPATH *dpt, char *path);
DIRPATH *dpt_create(char *path);
char *dpt_string(DIRPATH *dpt, char *buf);