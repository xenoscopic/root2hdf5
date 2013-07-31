// Standard includes
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>

// Boost includes
#include <boost/program_options.hpp>

// ROOT includes
#include <TChain.h>

// HDF5 includes
#include "H5Cpp.h"


// Standard namespaces
using namespace std;

// Boost namespace aliases
namespace po = boost::program_options;

// HDF5 namespaces
#ifndef H5_NO_NAMESPACE
using namespace H5;
#endif


// Command line parsing
po::variables_map parse_command_line_options(int argc, char * argv[])
{
    // Set up named arguments which are visible to the user
    po::options_description visible(
        "usage: root2hdf5 [options] <input-url> [<input-url>...] <output-url>"
    );
    visible.add_options()
        ("verbose,v", "Print output of file operations.")
        ("help,h", "Print this message and exit.")
    ;

    // Set up hidden arguments.  Anything we want to be positional needs to be
    // specified in here.
    po::options_description hidden("hidden options");
    hidden.add_options()
        ("urls,u",
            po::value< vector<string> >()->multitoken(),
            "URLs for input/output")
    ;

    // Create combined options
    po::options_description all("all options");
    all.add(visible).add(hidden);

    // Set up positional arguments
    po::positional_options_description p;
    p.add("urls", -1);

    // Do the actual parsing
    po::variables_map vm;
    try 
    {
        po::store(
            po::command_line_parser(argc, argv)
                .options(all)
                .positional(p)
                .run(),
            vm
        );
        po::notify(vm);
        
        // Print help if requested
        if(vm.count("help"))
        {
            cout << visible << endl;
            exit(EXIT_SUCCESS);
        }

        // Print help if no (or not enough) paths were specified
        if(vm.count("urls") == 0
           || vm["urls"].as< vector<string> >().size() < 2)
        {
            cout << visible << endl;
            exit(EXIT_FAILURE);
        }
    }
    catch(std::exception& e)
    {
        cerr << "Couldn't parse command line options: " << e.what() << endl;
        cerr << visible << endl;
        exit(EXIT_FAILURE);
    }
    catch(...)
    {
        cerr << "Couldn't parse command line options, not sure why." << endl;
        cerr << visible << endl;
        exit(EXIT_FAILURE);
    }
    
    return vm;
}


int main(int argc, char *argv[])
{
    // Parse command line options
    po::variables_map options = parse_command_line_options(argc, argv);

    // Open the first file specified and use that as a map to seek out any TTree
    // objects
    




    return 0;
}
