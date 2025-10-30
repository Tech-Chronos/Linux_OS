#include <stdio.h>
#include <unistd.h>
int main()
{
    for (int i = 10; i > 0; --i)
    {
        printf("%2d\r",i);
        fflush(stdout);

        sleep(1);
    }

    return 0;
}
