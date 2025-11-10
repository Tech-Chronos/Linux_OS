#include "myfile.h"

int main()
{
    myfile* file = myopen("./log.txt", "a");

    const char* message = "hello world\n";

    cout << file->_fileno << endl;

    cout << mywrite(file, message, strlen(message)) << endl;

    myflush(file);

    myfclose(file);

    return 0;
}