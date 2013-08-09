#include "cint.h"

// Standard includes
#include <iostream>
#include <fstream>

// Boost includes
// HACK: Need to define this macro to tell Boost not to use deprecated
// Boost.System constructs which result in unused-variable errors.
#ifndef BOOST_SYSTEM_NO_DEPRECATED
#define BOOST_SYSTEM_NO_DEPRECATED 1
#endif
#include <boost/filesystem.hpp>

// ROOT includes
#include <TROOT.h>
#include <TSystem.h>

// root2hdf5 includes
#include "options.h"


// Standard namespaces
using namespace std;

// Boost namespace aliases
namespace fs = boost::filesystem;

// root2hdf5 namespaces
using namespace root2hdf5::cint;
using namespace root2hdf5::options;


bool root2hdf5::cint::process_long_line(const string & long_line,
                                        bool compile)
{
    // Set the ACLiC build directory to a temporary directory so we don't
    // clutter our working directory with ROOT dictionaries
    static bool build_directory_set = false;
    static fs::path build_directory;
    if(!build_directory_set)
    {
        // Generate a build directory
        build_directory = fs::temp_directory_path();

        // Set the build directory path
        gSystem->SetBuildDir(build_directory.native().c_str());
    }

    // TODO: It might be better to add some more stringent error checking in
    // here, but most of these things are such robust, low-level OS things that
    // if they fail, the user's environment is probably already melting around
    // them, so just assume the temporary file creation/writing/deletion will
    // work.  However, still check LoadMacro's return code, because it is easy
    // and because we can't trust ROOT.

    // Generate a temporary path to write the line to, and just throw it in the
    // build directory to keep things clean.
    fs::path temp_path = build_directory 
                         / fs::unique_path("%%%%-%%%%-%%%%-%%%%.cpp");

    // Write the line to the temporary file.  It is very important that we
    // include a newline at the end or ROOT's crappy interpreter will give an
    // "unexpected EOF" error sporadically.
    ofstream output(temp_path.c_str(), ofstream::out);
    output << long_line << endl;
    output.close();

    // Process the line
    bool success = true;
    if(compile)
    {
        if(gSystem->CompileMacro(temp_path.native().c_str()) != 1)
        {
            success = false;
        }
    }
    else
    {
        if(gROOT->LoadMacro(temp_path.native().c_str()) < 0)
        {
            success = false;
        }
    }

    if(!success && verbose)
    {
        cerr << "ERROR: Unable to load temporary macro" << endl;
    }

    // Remove the file.  I know ROOT won't clean up after itself, but we can at
    // least clean up after ourselves.
    fs::remove(temp_path);
    
    return success;
}
