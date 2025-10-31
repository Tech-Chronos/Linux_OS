#include <stdio.h>

int sum(int num)
{
    int ret = 0;
    for (int i = 0; i < num; ++i)
    {
        ret += i;
    }

    return ret;
}

int main()
{
    int cnt = 101;
    int ret = sum(cnt);
    printf("%d\n",ret);
    return 0;
}
