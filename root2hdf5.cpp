// Standard includes
#include <cstdlib>
#include <iostream>
#include <string>

// Boost includes
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

// ROOT includes
#include <TROOT.h>
#include <TFile.h>

// HDF5 includes
#include "H5Cpp.h"


// Standard namespaces
using namespace std;

// Boost namespace aliases
namespace po = boost::program_options;
namespace fs = boost::filesystem;

// HDF5 namespaces
#ifndef H5_NO_NAMESPACE
using namespace H5;
#endif


// Command line parsing
po::variables_map parse_command_line_options(int argc, char * argv[])
{
    // Set up named arguments which are visible to the user
    po::options_description options(
        "usage: root2hdf5 [options] <input-url> <output-url>"
    );
    options.add_options()
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
    po::variables_map vm;
    try 
    {
        po::store(
            po::command_line_parser(argc, argv)
                .options(options)
                .positional(p)
                .run(),
            vm
        );
        po::notify(vm);
        
        // Print help if requested
        if(vm.count("help"))
        {
            cout << options << endl;
            exit(EXIT_SUCCESS);
        }

        // Print help if no (or not enough) paths were specified
        if(vm.count("input-url") == 0 || vm.count("output-url") == 0)
        {
            cout << options << endl;
            exit(EXIT_FAILURE);
        }
    }
    catch(std::exception& e)
    {
        cerr << "Couldn't parse command line options: " << e.what() << endl;
        cerr << options << endl;
        exit(EXIT_FAILURE);
    }
    catch(...)
    {
        cerr << "Couldn't parse command line options, not sure why." << endl;
        cerr << options << endl;
        exit(EXIT_FAILURE);
    }
    
    return vm;
}


int main(int argc, char *argv[])
{
    // Set ROOT to batch mode so nothing pops up on the screen (not that it
    // should, but you never know with ROOT)
    gROOT->SetBatch(kTRUE);

    // Parse command line options and create some convenient accessors
    po::variables_map options = parse_command_line_options(argc, argv);
    bool verbose = options.count("verbose") > 0;

    // Grab file paths (parse_command_line_options will have validated them)
    string input_url = options["input-url"].as<string>();
    string output_url = options["output-url"].as<string>();

    // Check if the output path exists.  If it does, and it is a directory, then
    // the user has likely made a mistake, so bail.  If it does and it is a
    // file, then check if the user has specified the overwrite option, and in
    // that case, proceed.
    fs::path output_path(output_url);
    bool exists = fs::exists(output_path);
    bool is_dir = exists && fs::is_directory(output_path);
    if(exists)
    {
        if(is_dir)
        {
            cerr << "Output path is a directory, manually delete if you would "
                 << "like to overwrite it" << endl;
            exit(EXIT_FAILURE);
        }
        else if(options.count("overwrite") == 0)
        {
            cout << "Output path exists.  Specify the \"--overwrite\" option "
                 << "if you would like to overwrite it" << endl;
            exit(EXIT_FAILURE);
        }
    }
    
    // Print path information if requested
    if(verbose)
    {
        cout << "Converting " << input_url << " -> " << output_url << endl;
    }

    // Open the input file
    TFile *input_file = TFile::Open(input_url.c_str(), "READ");
    if(input_file == NULL)
    {
        cerr << "Unable to open input file: " << input_url << endl;
        exit(EXIT_FAILURE);
    }

    // Open the output file
    H5File *output_file = new H5File(output_url, H5F_ACC_TRUNC);



    // Cleanup output resources
    delete output_file;
    output_file = NULL;

    // Cleanup input resources
    input_file->Close();
    delete input_file;
    input_file = NULL;    

    return 0;
}
