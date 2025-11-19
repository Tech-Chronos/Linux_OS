#include <iostream>
#include <sys/types.h>
#include <signal.h>

// ./mykill killsignal pid
int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cout << "[USAGE METHOD] -> " << argv[0] << " killsignal pidnum" << std::endl;
        exit(-1);
    }

    int killsignal = std::stoi(argv[1]);
    int pid = std::stoi(argv[2]);

    kill(pid, killsignal);

    return 0;
}