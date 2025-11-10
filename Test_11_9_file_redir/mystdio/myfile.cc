#include "myfile.h"

myfile *myopen(const char *path, char *flags)
{
    int fd = -1;
    if (strcmp(flags, "r") == 0)
    {
        fd = open(path, O_RDONLY);
        if (fd < 0)
            cerr << "open error" << endl;
    }
    else if (strcmp(flags, "w") == 0)
    {
        fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0)
            cerr << "open error" << endl;
    }
    else if (strcmp(flags, "a") == 0)
    {
        fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0666);
        if (fd < 0)
            cerr << "open error" << endl;
    }

    myfile *file = (myfile *)malloc(sizeof(myfile));
    file->_fileno = fd;

    if (fd == 1 || fd == 2)
    {
        file->flush_mod = LINEFLUSH;
    }
    else
    {
        file->flush_mod = FULLFLUSH;
    }

    file->capacity = SIZE;
    file->pos = 0;

    return file;
}

void myflush(myfile *file)
{
    if (file->pos != 0)
    {
        write(file->_fileno, file->buf, SIZE);

        //cout << "this is write" << endl;
        file->pos = 0;
    }
}

ssize_t mywrite(myfile *file, const char *buf, size_t size)
{
    // 固定长度的数组
    memcpy(file->buf, buf, size);

    file->pos += size;
    if (file->buf[file->pos - 1] == '\n' && file->flush_mod == LINEFLUSH)
    {
        myflush(file);
        cout << "LINEFLUSH" << endl;
    }
    else if (file->pos == file->capacity && file->flush_mod == FULLFLUSH)
    {
        myflush(file);
        cout << "FULLFLUSH" << endl;
    }

    return size;
}

void myfclose(myfile *file)
{
    myflush(file);

    close(file->_fileno);
}
