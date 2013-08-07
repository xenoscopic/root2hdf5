#pragma once

// Standard includes
#include <string>


namespace root2hdf5
{
    namespace cint
    {
        // This method provides a convenient interface for executing code via
        // CINT which might be too long for the TROOT::ProcessLine method (which
        // has a limit of 2043 characters).  The method creates a temporary file
        // in which to write the code, has CINT load the file from disk, and
        // then cleans up the temporary file.  This method returns true on
        // success and false on failure.
        bool process_long_line(const std::string & long_line);
    }
}
