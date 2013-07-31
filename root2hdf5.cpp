// Standard includes
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <map>

// Boost includes
#include <boost/assign.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

// ROOT includes
#include <TClass.h>
#include <TDirectory.h>
#include <TROOT.h>
#include <TFile.h>
#include <TList.h>
#include <TKey.h>
#include <TTree.h>
#include <TBranch.h>
#include <TLeaf.h>

// HDF5 includes
#include <H5Cpp.h>


// Standard namespaces
using namespace std;

// Boost namespaces
using namespace boost::assign;

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


map<string, PredType> root_type_name_to_hdf5_type = 
map_list_of
    ("Bool_t", PredType::NATIVE_HBOOL)
    ("Char_t", PredType::NATIVE_SCHAR)
    ("UChar_t", PredType::NATIVE_UCHAR)
    ("Short_t", PredType::NATIVE_INT16)
    ("UShort_t", PredType::NATIVE_UINT16)
    ("Int_t", PredType::NATIVE_INT32)
    ("UInt_t", PredType::NATIVE_UINT32)
    ("Long_t", PredType::NATIVE_INT64)
    ("Long64_t", PredType::NATIVE_INT64)
    ("ULong_t", PredType::NATIVE_UINT64)
    ("ULong64_t", PredType::NATIVE_UINT64)
    ("Float_t", PredType::NATIVE_FLOAT)
    ("Double_t", PredType::NATIVE_DOUBLE)
;


void create_dataset_from_tree_in_group(TTree *tree,
                                       CommonFG *group)
{
    // First, get the name for our dataset
    string name(tree->GetName());
    group = NULL;

    // Second, we need to define the dataspace (i.e. dimensions) of the
    // dataset, which in this case is just rank 1 with the number of
    // entries in the TTree
    hsize_t length = tree->GetEntries();
    DataSpace dataspace(1, &length);

    // Third, we need to construct the datatype.  To do this, we need to iterate
    // over the branches of the TTree, figure out their type, map it to an HDF5
    // type, and allocate a buffer to store the branch value in.  The good thing
    // is that we can use the same buffer for SetBranchAddress and HDF5's data
    // filling.
    map<string, const DataType &> branch_to_hdf5_type;
    map<string, size_t> branch_to_offset;

    TIter next_branch(tree->GetListOfBranches());
    TBranch *branch = NULL;
    while((branch = (TBranch *)next_branch()))
    {
        // Grab the branch type name
        string type_name(branch->GetLeaf(branch->GetName())->GetTypeName());

        // Check the type kind
        bool is_atomic = root_type_name_to_hdf5_type.count(type_name) == 1;
        
        // If this is not a handleable type, just continue
        if(is_atomic)
        {
            // Grab the type

        }
        else
        {
            // Unhandled type
            if(verbose)
            {
                cerr << "WARNING: Unhandled branch type \""
                     << type_name
                     << "\" - skipping."
                     << endl;
            }

            continue;
        }
    }


}


void walk_and_convert(TDirectory *directory,
                      CommonFG *group,
                      unsigned depth = 0)
{
    // Generate a list of keys in the directory
    TIter next_key(directory->GetListOfKeys());

    // Go through the keys, handling them by their type
    TKey *key = NULL;
    while((key = (TKey *)next_key()))
    {
        // Grab the object and its type
        TObject *object = key->ReadObj();
        TClass *object_type = object->IsA();

        // Print information if requested
        if(verbose)
        {
            for(unsigned i = 0; i < depth; i++)
            {
                cout << '\t';
            }
            cout << "Processing " << key->GetName() << endl;
        }

        // Switch based on type
        if(object_type->InheritsFrom(TDirectory::Class()))
        {
            // This is a ROOT directory, so first create a corresponding HDF5
            // group and then recurse into it
            Group new_group = group->createGroup(key->GetName());
            walk_and_convert((TDirectory *)key->ReadObj(),
                             &new_group,
                             depth + 1);
        }
        else if(object_type->InheritsFrom(TTree::Class()))
        {
            // This is a ROOT tree, so we need to create a new HDF5 dataset with
            // custom type matching the TTree branches, and then copy all the
            // data into it
            create_dataset_from_tree_in_group((TTree *)object, group);
        }
        else
        {
            // This type is currently unhandled
            if(verbose)
            {
                cerr << "WARNING: Unhandled object type \""
                     << object_type->GetName() 
                     << "\" - skipping."
                     << endl;
            }
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
