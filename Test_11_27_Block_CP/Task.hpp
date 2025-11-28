#ifndef __TASK_HPP__
#define __TASK_HPP__
#include <iostream>
#include <functional>
#include <string>

using task_t = std::function<void(void)>;

void DownloadTask()
{
    std::cout << "I am a download task!" << std::endl;
}

void SQLTask()
{
    std::cout << "I am a SQL task!" << std::endl;
}

void DeleteTask()
{
    std::cout << "I am a Delete task!" << std::endl;
}

void SocketTask()
{
    std::cout << "I am a Socket Task!" << std::endl;
}

task_t tasks[] = {DownloadTask, SQLTask, DeleteTask, SocketTask};

class Task
{
public:
    Task(int left, int right)
        :_left(left)
        ,_right(right)
    {}

    Task() = default;

    void operator()()
    {
        _result = _left + _right;
    }

    std::string GetTask()
    {
        std::string task = "Task is " + std::to_string(_left) + " + " + std::to_string(_right) + " = ? "; 

        return task;
    }

    std::string DealResult()
    {
        std::string result = "Result is " + std::to_string(_left) + " + " + std::to_string(_right) + " = " + std::to_string(_result); 

        return result;
    }

private:
    int _left;
    int _right;
    int _result;
};




#endif