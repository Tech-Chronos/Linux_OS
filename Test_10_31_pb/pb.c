#include "pb.h"
#define STYLE '='


//void processbar()
//{
//    char buf[101] = {'\0'};
//    const char* str = "-\\|/";
//    size_t len = strlen(str);
//
//    memset(buf, 0, sizeof buf);
//
//    int cnt = 0;
//
//    while (cnt < 100)
//    {
//        buf[cnt] = STYLE;
//        ++cnt;
//
//        printf("[%-100s][%-3d%%][%c]\r",buf,cnt,str[cnt % len]);
//
//        fflush(stdout);
//        usleep(50000);
//    }
//    printf("\n");
//}


void processbar(double filesize, double cur)
{
    char buf[101] = {'\0'};
    const char* str = "-\\|/";
    size_t len = strlen(str);

    memset(buf, 0, sizeof buf);
    
    double rate = (double)(cur * 100.0) / filesize; 

    int cnt = 0;
    int loop_count = (int)rate;

    while (cnt < loop_count)
    {
        buf[cnt] = STYLE;
        ++cnt;
        
    }

        printf("[%-100s][%-.3lf%%][%c]\r", buf, rate, str[cnt % len]);
        fflush(stdout); 
        usleep(20000);

}
