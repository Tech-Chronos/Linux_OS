#pragma once

#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef void (*call_back)(double filesize, double cur);

void processbar(double filesize, double cur);

