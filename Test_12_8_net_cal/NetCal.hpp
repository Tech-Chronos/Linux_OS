#pragma once
#include <iostream>

#include "Protocol.hpp"
#include "IOService.hpp"

class Calculator
{
public:
    Calculator()
    {
    }

    std::shared_ptr<Response> Calculate(std::shared_ptr<Request> req)
    {
        std::shared_ptr<Response> resp;
        switch (req->GetOper())
        {
        case '+':
        {
            resp->_result = req->GetX() + req->GetY();
            resp->_exit_code = 0;
            resp->_desc = "add successfully";
            break;
        }
        case '-':
        {
            resp->_result = req->GetX() - req->GetY();
            resp->_exit_code = 0;
            resp->_desc = "sub successfully";
            break;
        }
        case '*':
        {
            resp->_result = req->GetX() * req->GetY();
            resp->_exit_code = 0;
            resp->_desc = "multiple successfully";
            break;
        }
        case '/':
        {
            if (req->GetY() == 0)
            {
                resp->_desc = "div zero";
                resp->_exit_code = 1;
                resp->_result = 0;
                break;
            }
            resp->_result = (req->GetX() * 1.0) / req->GetY();
            resp->_exit_code = 0;
            resp->_desc = "div successfully";
            break;
        }
        case '%':
        {
            if (req->GetY() == 0)
            {
                resp->_desc = "mod zero";
                resp->_exit_code = 2;
                resp->_result = 0;
                break;
            }
            resp->_result = req->GetX() % req->GetY();
            resp->_exit_code = 0;
            resp->_desc = "mod successfully";
            break;
        }
        default:
            resp->_desc = "invalid input";
            resp->_exit_code = 3;
            resp->_result = 0;
            break;
        }

        return resp;
    }

    ~Calculator()
    {
    }

private:
};
