#pragma once

#include "dircont.h"

typedef DIRCONT DIRPATH;

bool dpt_move(DIRPATH *dpt, char *subdir);
void dpt_up(DIRPATH *dpt);

void dpt_init(DIRPATH *dpt, char *path);
DIRPATH *dpt_create(char *path);
char *dpt_string(DIRPATH *dpt, char *buf);