#ifndef ERROR_H
#define ERROR_H

#include <CL/cl.h>
#include <spdlog/spdlog.h>
#include <stdlib.h>
#include <string>

namespace spd = spdlog;

namespace moRPL
{
    bool checkCLError(cl_int error_code, const char *file, const int line);

    void log(const char* text);
}

#endif