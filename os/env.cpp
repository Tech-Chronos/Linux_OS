//
// Created by dsj on 2026/3/5.
//

#include <iostream>

;

int main(int argc, char* argv[], char* environ[])
{
    for (int i = 0; i < argc; ++i)
    {
        std::cout << argv[i] << std::endl;
    }

    for (int i = 0; environ[i] != nullptr; ++i)
    {
        std::cout << environ[i] << std::endl;
    }

    return 0;
}