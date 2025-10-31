#include "pb.h"

double filesize1 = 100*1024*1024;
double filesize2 = 10*1024*1024;
double filesize3 = 220*1024*1024;
double filesize4 = 150*1024*1024;
double filesize5 = 90*1024*1024;


void download(double filesize, call_back call)
{
    // 文件大小
    //double filesize = 100*1024*1024;
    // 当前下载的大小
    double cur = 0;
    // 当前网络带宽
    int bind_width = 1024 * 1024;
    printf("download begin: cur = %.2lf\n", cur);
    while (cur <= filesize)
    {
        call(filesize, cur);
        cur += bind_width;
    }
    printf("\ndownload end: cur = %.2lf\n", cur);
}

int main()
{
    download(filesize1, processbar);
    download(filesize2, processbar);
    download(filesize3, processbar);
    download(filesize4, processbar);
    download(filesize5, processbar);
    return 0;
}
