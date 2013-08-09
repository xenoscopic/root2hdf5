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
        // in which to write the code.  If the "compile" argument is true, then
        // this method will use the TSystem::CompileMacro method to generate and
        // load a shared library.  In this case, the provided code will not have
        // implicit access to the ROOT headers, so you need to include what you
        // need.  This is done instead of using TROOT::LoadMacro with a "+" sign
        // because the output directory of the compiled code can be controlled.
        // If the "compile" argument is false, this method will use the
        // TROOT::LoadMacro method, and will process the code without compiling
        // it, but cannot do things like generating dictionaries.  This method 
        // returns true on success and false on failure.
        bool process_long_line(const std::string & long_line,
                               bool compile = false);
    }
}
