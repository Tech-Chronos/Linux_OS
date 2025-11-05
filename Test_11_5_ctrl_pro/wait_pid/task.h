#pragma once

#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <wait.h>
#include <functional>

#define N 3

using namespace std;

void MySQLSync();

void LoadLog();

void Network();