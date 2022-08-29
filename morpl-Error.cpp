#include "morpl-Error.h"

namespace moRPL
{
    bool checkCLError(cl_int error_code, const char *file, const int line)
    {
        if (error_code != CL_SUCCESS)
        {
            spd::error("{} : {} : OpenCL Runtime API error, error code : {}", file, line, error_code);
            // exit(EXIT_FAILURE);
            return false;
        }
        else
            return true;
    }

    void log(const char *text)
    {
        spd::info(text);
    }
}