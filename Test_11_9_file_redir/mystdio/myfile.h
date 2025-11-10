#pragma once
#include <iostream>
#include <stdio.h>

#include <cstring>
#include <cstdlib>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SIZE 1024
#define LINEFLUSH 1
#define FULLFLUSH 2

using namespace std;

struct myfile
{
    int _fileno;

    int flush_mod;

    char buf[SIZE];  
    
    int capacity;
    int pos;
};


myfile* myopen(const char* path, char* flags);

ssize_t mywrite(myfile* file, const char* buf, size_t size);

void myflush(myfile* file);

void myfclose(myfile* file);

