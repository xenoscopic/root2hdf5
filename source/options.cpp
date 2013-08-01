#include "options.h"

// Standard includes
#include <iostream>
#include <cstdlib>


// Standard namespaces
using namespace std;

// Boost namespace aliases
namespace po = boost::program_options;

// root2hdf5 namespaces
using namespace root2hdf5::options;


// Global variable declarations
namespace root2hdf5
{
    namespace options
    {
        po::variables_map options;
        bool verbose = false;
    }
}


// Parsing methods
void root2hdf5::options::parse_command_line_options(int argc, char * argv[])
{
    // Set up named arguments which are visible to the user
    po::options_description options_specification(
        "usage: root2hdf5 [options] <input-url> <output-url>"
    );
    options_specification.add_options()
        ("input-url,i",
            po::value<string>()->value_name("<input-url>")->required(),
            "Input URL")
        ("output-url,o",
            po::value<string>()->value_name("<output-url>")->required(),
            "Output URL")
        ("overwrite,O", "Overwrite the output path.")
        ("verbose,v", "Print output of file operations.")
        ("help,h", "Print this message and exit.")
    ;

    // Set up positional arguments
    po::positional_options_description p;
    p.add("input-url", 1);
    p.add("output-url", 1);

    // Do the actual parsing
    try 
    {
        po::store(
            po::command_line_parser(argc, argv)
                .options(options_specification)
                .positional(p)
                .run(),
            options
        );
        po::notify(options);
        
        // Print help if requested
        if(options.count("help"))
        {
            cout << options_specification << endl;
            exit(EXIT_SUCCESS);
        }

        // Print help if no (or not enough) paths were specified
        if(options.count("input-url") == 0 || options.count("output-url") == 0)
        {
            cout << options_specification << endl;
            exit(EXIT_FAILURE);
        }

        // Set up convenience accessors
        verbose = options.count("verbose");
    }
    catch(std::exception& e)
    {
        cerr << "Couldn't parse command line options: " << e.what() << endl;
        cerr << options_specification << endl;
        exit(EXIT_FAILURE);
    }
    catch(...)
    {
        cerr << "Couldn't parse command line options, not sure why." << endl;
        cerr << options_specification << endl;
        exit(EXIT_FAILURE);
    }
}
