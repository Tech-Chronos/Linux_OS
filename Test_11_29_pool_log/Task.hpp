#pragma once

#include <iostream>
#include <string>
#include <queue>
#include <vector>
#include <functional>

#include <unistd.h>
#include <pthread.h>

using task_t = std::function<void(std::string& name)>;

namespace Task
{
    class BaseTask
    {
    public:
        void Task(std::string& name)
        {
            std::cout << name << " this is a base task!" << std::endl;
        }
    };

    class DownloadTask 
    {
    public:
        void Task(std::string& name) 
        {
            std::cout << name << " this is a Download task!" << std::endl;
        }
    };

    class SQLTask 
    {
    public:
        void Task(std::string& name) 
        {
            std::cout << name << " this is a SQLTask task!" << std::endl;
        }
    };

    class ModifyTask
    {
    public:
        void Task(std::string& name)
        {
            std::cout << name << " this is a ModifyTask task!" << std::endl;
        }
    };

    BaseTask* ba_tk = new BaseTask;
    DownloadTask* dl_tk = new DownloadTask;
    SQLTask* sq_tk = new SQLTask;
    ModifyTask* mod_tk = new ModifyTask;


    task_t basetask = std::bind(&BaseTask::Task, ba_tk, std::placeholders::_1);
    task_t downloadtask = std::bind(&DownloadTask::Task, dl_tk, std::placeholders::_1);
    task_t sqltask = std::bind(&SQLTask::Task, sq_tk, std::placeholders::_1);
    task_t modtask = std::bind(&ModifyTask::Task, mod_tk, std::placeholders::_1);

    //task_t tasks[] = {basetask, downloadtask, sqltask, modtask};
}


task_t tasks[] = {Task::basetask, Task::downloadtask, Task::sqltask, Task::modtask};