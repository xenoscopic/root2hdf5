// Standard includes
#include <cstdlib>
#include <iostream>
#include <string>

// Boost includes
#include <boost/filesystem.hpp>

// ROOT includes
#include <TROOT.h>
#include <TSystem.h>
#include <TFile.h>

// HDF5 includes
#include <hdf5.h>

// root2hdf5 includes
#include "options.h"
#include "convert.h"

// Standard namespaces
using namespace std;

// Boost namespace aliases
namespace fs = boost::filesystem;

// root2hdf5 namespaces
using namespace root2hdf5::options;
using namespace root2hdf5::convert;


int main(int argc, char *argv[])
{
    // Set ROOT to batch mode so nothing pops up on the screen (not that it
    // should, but you never know with ROOT)
    gROOT->SetBatch(kTRUE);

    // Tell ROOT to do any compilation in a temporary directory
    gSystem->SetBuildDir(fs::temp_directory_path().native().c_str());

    // Parse command line options and create some convenient accessors
    parse_command_line_options(argc, argv);

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
        if(verbose)
        {
            cerr << "Unable to open input file: " << input_url << endl;
        }
        exit(EXIT_FAILURE);
    }

    // Open the output file
    hid_t output_file = H5Fcreate(output_url.c_str(),
                                  H5F_ACC_TRUNC,
                                  H5P_DEFAULT,
                                  H5P_DEFAULT);

    // Walk the input file and convert everything
    if(!convert(input_file, output_file))
    {
        exit(EXIT_FAILURE);
    }

    // Cleanup output resources
    if(H5Fclose(output_file) < 0)
    {
        if(verbose)
        {
            cerr << "ERROR: Closing output file failed" << endl;
        }
        exit(EXIT_FAILURE);
    }

    // Cleanup input resources
    input_file->Close();
    delete input_file;
    input_file = NULL;

    return 0;
}
