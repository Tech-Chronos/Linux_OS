#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


using namespace std;

int main()
{
    //int fd = open("log.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    printf("hello printf\n");
    fprintf(stdout, "%s", "hello fprintf\n");

    const char* message = "hello write\n";
    write(1, message, strlen(message));

    fork();

    return 0;
}
