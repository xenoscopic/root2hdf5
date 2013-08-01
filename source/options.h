#pragma once

// Boost includes
#include <boost/program_options.hpp>

namespace root2hdf5
{
    namespace options
    {
        // Global options map and convenience accessors
        extern boost::program_options::variables_map options;
        extern bool verbose;

        // Parsing methods
        void parse_command_line_options(int argc, char * argv[]);
    }
}
