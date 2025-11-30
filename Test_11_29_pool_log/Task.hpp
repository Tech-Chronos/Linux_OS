#pragma once
#include "Log.hpp"

#include <iostream>
#include <string>
#include <queue>
#include <vector>
#include <functional>

#include <unistd.h>
#include <pthread.h>

using task_t = std::function<void(std::string& name)>;
Log lg;

namespace Task
{
    class BaseTask
    {
    public:
        void Task(std::string& name)
        {
            //std::cout << name << " this is a base task!" << std::endl;
            LOG(DEBUG, "%s this is a base task!\n", name.c_str());
        }
    };

    class DownloadTask 
    {
    public:
        void Task(std::string& name) 
        {
            //std::cout << name << " this is a Download task!" << std::endl;
            LOG(DEBUG, "%s this is a Download task!\n", name.c_str());
        }
    };

    class SQLTask 
    {
    public:
        void Task(std::string& name) 
        {
            //std::cout << name << " this is a SQLTask task!" << std::endl;
            LOG(DEBUG, "%s this is a SQLTask task!\n", name.c_str());

        }
    };

    class ModifyTask
    {
    public:
        void Task(std::string& name)
        {
            //std::cout << name << " this is a ModifyTask task!" << std::endl;
            LOG(DEBUG, "%s this is a ModifyTask task!\n", name.c_str());

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