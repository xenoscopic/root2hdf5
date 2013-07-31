// Standard includes
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

// Boost includes
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

// ROOT includes
#include <TClass.h>
#include <TDirectory.h>
#include <TROOT.h>
#include <TFile.h>
#include <TList.h>
#include <TKey.h>

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


// Command line parsing and convenient accessor variables
// NOTE: Yes, I hate global variables, but let's be honest, command line options
// *are* global, so I think this is one case where it is okay, and certainly
// more convenient than passing them to every single method that needs them.
po::variables_map options;
bool verbose = false;
void parse_command_line_options(int argc, char * argv[])
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

void walk_and_convert(TDirectory *directory, CommonFG *group)
{
    // Generate a list of keys in the directory
    TIter next_key(directory->GetListOfKeys());

    // Go through the keys, handling them by their type
    TKey *key;
    while((key = (TKey *)next_key()))
    {
        // Grab the object and its type
        TObject *object = key->ReadObj();
        TClass *object_type = object->IsA();

        // Switch based on type
        if(object_type->InheritsFrom(TDirectory::Class()))
        {
            // This is a ROOT directory, so first create a corresponding HDF5
            // group and then recurse into it
            Group new_group = group->createGroup(key->GetName());
            walk_and_convert((TDirectory *)key->ReadObj(), &new_group);
        }
        else
        {
            // This type is currently unhandled

        }
    }
}


int main(int argc, char *argv[])
{
    // Set ROOT to batch mode so nothing pops up on the screen (not that it
    // should, but you never know with ROOT)
    gROOT->SetBatch(kTRUE);

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
        cerr << "Unable to open input file: " << input_url << endl;
        exit(EXIT_FAILURE);
    }

    // Open the output file
    H5File output_file(output_url, H5F_ACC_TRUNC);

    // Walk the input file and convert everything
    walk_and_convert(input_file, &output_file);

    // Cleanup input resources
    input_file->Close();
    delete input_file;
    input_file = NULL;    

    return 0;
}
