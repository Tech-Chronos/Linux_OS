#include <iostream>
#include <cassert>
using namespace std;

void lower_to_upper(char *str, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        if (str[i] >= 'a' && str[i] <= 'z')
        {
            str[i] = str[i] - 32;
        }
        // putchar(str[i]);
    }
}

void test1()
{
    char str[] = "abcd";

    lower_to_upper(str, sizeof(str) / sizeof(char));

    cout << str << endl;
    printf("%s\n", str);

    int arr[] = {10, 20, 30};
    cout << arr << endl;
}

int my_strlen1(const char *str)
{
    const char *ch = str;
    int num = 0;
    while (*ch != '\0')
    {
        ++num;
        ++ch;
    }

    return num;
}

int my_strlen2(const char *str)
{
    if (*str == '\0')
    {
        return 0;
    }

    return 1 + my_strlen2(++str);
}

int my_strlen3(const char *str)
{
    const char *ch = str;
    while (*ch != '\0')
    {
        ++ch;
    }

    return ch - str;
}

void func2()
{
    int i = my_strlen3("hello worldmmmm");
    cout << i << endl;
}

char *my_strcpy(const char *src, char *dest)
{
    char *ret = dest;
    while (*src != '\0')
    {
        *dest = *src;
        ++src;
        ++dest;
    }
    *dest = '\0';

    return ret;
}

void func3()
{
    const char *str1 = "hello world!";
    char *str2 = (char *)malloc(sizeof(char) * 100);

    char *ret = my_strcpy(str1, str2);

    cout << ret << endl;
}

// 追加
char *my_strcat(char *dest, const char *src)
{
    char *ret = dest;
    assert(dest != NULL);
    assert(src != NULL);
    while (*dest)
    {
        dest++;
    }
    while ((*dest++ = *src++))
    {
        ;
    }
    return ret;
}

int main()
{
    // func1();
    // func2();
    // func3();

    return 0;
}