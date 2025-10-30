#include "progress_bar.h"

#define STYLE '#'
#define line1 '-'
#define line2 '\\'
#define line3 '/'


void process_bar()
{
    char buf[101];
    char line[3] = {line1, line2, line3};

    memset(buf, 0, sizeof(buf));
    
    int sec = 0;
    scanf("%d",&sec);

    int cnt = 0;
    while (cnt <  sec)
    {
        ++cnt;
        if (cnt == sec)
        {
            for (int i = 0; i < 100; ++i)
            {
                buf[i] = STYLE;
            }
        }
        else 
        {
           for (int i = 0; i < cnt * (100.0 / sec); ++i)
           {
               buf[i] = STYLE;
           }
        }
        printf("[%-100s][%.2lf%%][%c]\r", buf,(double)((cnt * 1.0 / sec) * 100.0) , line[cnt % 3]);
        
        fflush(stdout); 
       
        usleep(10000);
    }
    printf("\n");
}
