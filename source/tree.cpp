#include "tree.h"

// Standard includes
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <sstream>
#include <utility>
#include <functional>
#include <vector>
#include <tuple>

// Boost includes
// HACK: Need to define this macro to tell Boost not to use deprecated
// Boost.System constructs which result in unused-variable errors.
#ifndef BOOST_SYSTEM_NO_DEPRECATED
#define BOOST_SYSTEM_NO_DEPRECATED 1
#endif
#include <boost/filesystem.hpp>

// ROOT includes
#include <TROOT.h>
#include <TBranch.h>
#include <TLeaf.h>

// root2hdf5 includes
#include "options.h"
#include "type.h"


// Standard namespaces
using namespace std;

// Boost namespace aliases
namespace fs = boost::filesystem;

// root2hdf5 namespaces
using namespace root2hdf5::tree;
using namespace root2hdf5::options;
using namespace root2hdf5::type;


// Private function and mapping declarations
namespace root2hdf5
{
    namespace tree
    {
        // This method provides a convenient interface for executing code via
        // CINT which might be too long for the TROOT::ProcessLine method (which
        // has a limit of 2043 characters).  The method creates a temporary file
        // in which to write the code, has CINT load the file from disk, and
        // then cleans up the temporary file.  This method returns true on
        // success and false on failure.
        bool process_long_line(const string & long_line);

        // This method returns the offset of the member in a struct using CINT.
        // E.g. with the struct
        //
        //      struct TestStruct
        //      {
        //          int a;
        //          int b;
        //          struct {
        //              float d;
        //              double e;
        //          } c;
        //      };
        //
        // One could do:
        //
        //      offsetof_member_in_type_by_name("TestStruct", "c")
        //
        // and receive 8 (or whatever the platform-dependent value would be).
        size_t offsetof_member_in_type_by_name(string type_name,
                                               string member_name);

        // This method returns the size of the member of a struct using CINT.
        // E.g. with the struct
        //
        //      struct TestStruct
        //      {
        //          int a;
        //          int b;
        //          struct {
        //              float d;
        //              double e;
        //          } c;
        //      };
        //
        // One could do:
        //
        //      offsetof_member_in_type_by_name("TestStruct", "c");
        //
        // and receive 16 (or whatever the platform/padding-dependent value
        // would be).
        size_t sizeof_member_in_type_by_name(string type_name,
                                             string member_name);

        // Converts a TLeaf into a string representation of a member suitable
        // for inclusion in a C struct representing an HDF5 compound data type
        // and inserts it into the stringstream used to build up the code for
        // the C struct.  The resulting type may not match the original branch
        // type (e.g. in the case of non-scalar types such as vector<...>), but
        // the map_branch_and_build_converter function will allocate an 
        // intermediate buffer for the TTree to load its data into, a converter
        // function which will convert the data from the intermediate buffer
        // into the HDF5 struct, and a cleanup function to remove the
        // intermediate buffer.
        bool insert_hdf5_struct_member_for_leaf(stringstream & container,
                                                TLeaf *leaf);

        // Converts a TBranch into a string representation of a member suitable
        // for inclusion in a C struct representing an HDF5 compound data type
        // and inserts it into the stringstream used to build up the code for
        // the C struct.  This function is recursive and will call itself for
        // any subbranches, otherwise calling insert_hdf5_struct_member_for_leaf
        // to handle individual leaves.
        bool insert_hdf5_struct_member_for_branch(stringstream & container,
                                                  TBranch *branch);

        // Converts a TTree (and its branches) to a string representing a struct
        // suitable for use as a buffer for an HDF5 compound type.  The function
        // returns a pair of the form:
        //      (success, struct_type_name, code_for_struct)
        tuple<bool, string, string> hdf5_struct_for_tree(TTree *tree);
    }
}


bool root2hdf5::tree::process_long_line(const string & long_line)
{
    // TODO: It might be better to add some more stringent error checking in
    // here, but most of these things are such robust, low-level OS things that
    // if they fail, the user's environment is probably already melting around
    // them, so just assume the temporary file creation/writing/deletion will
    // work.  However, still check LoadMacro's return code, because it is easy
    // and because we can't trust ROOT.

    // Generate a temporary path to write the line to
    fs::path temp_path = fs::temp_directory_path() 
                         / fs::unique_path("%%%%-%%%%-%%%%-%%%%.cpp");

    // Write the line to the temporary file.  It is very important that we
    // include a newline at the end or ROOT's crappy interpreter will give an
    // "unexpected EOF" error sporadically.
    ofstream output(temp_path.c_str(), ofstream::out);
    output << long_line << endl;
    output.close();

    // Process the line
    if(gROOT->LoadMacro((temp_path.native()).c_str()) < 0)
    {
        if(verbose)
        {
            cerr << "ERROR: Unable to load temporary macro" << endl;
        }

        return false;
    }

    // Remove the file
    fs::remove(temp_path);
    
    return true;
}


size_t root2hdf5::tree::offsetof_member_in_type_by_name(string type_name,
                                                        string member_name)
{
    return (size_t)gROOT->ProcessLine(
        (string("offsetof(") + type_name + "," + member_name + ");").c_str()
    );
}

size_t root2hdf5::tree::sizeof_member_in_type_by_name(string type_name,
                                                      string member_name)
{
    return (size_t)gROOT->ProcessLine(
        (string("sizeof(((") 
         + type_name 
         + "*)0)->" 
         + member_name 
         + ");").c_str()
    );
}


bool root2hdf5::tree::insert_hdf5_struct_member_for_leaf(
    stringstream & container,
    TLeaf *leaf
)
{
    // Grab the type name
    string type_name(leaf->GetTypeName());

    // Check if the leaf is a scalar type
    hid_t scalar_hdf5_type 
        = root_type_name_to_scalar_hdf5_type(type_name);
    if(scalar_hdf5_type != -1)
    {
        // This is a scalar type, so just insert the ROOT type
        container << type_name << " " << leaf->GetName() << ";";
        return true;
    }

    // Check if the leaf is a vector type
    root_vector_conversion vector_conversion
        = root_type_name_to_vector_hdf5_type(type_name);
    if(vector_conversion.valid)
    {
        // TODO: Insert the vector type
        return true;
    }

    // Otherwise this is an unsupported-but-ignorable type, so don't error, but
    // print a warning if we are in verbose mode
    if(verbose)
    {
        cerr << "WARNING: Leaf \"" << leaf->GetName() << "\" has an "
             << "unknown type \"" << type_name << "\" - skipping";
    }

    return true;
}


bool root2hdf5::tree::insert_hdf5_struct_member_for_branch(
    stringstream & container,
    TBranch *branch
)
{
    // Create a stringstream to contain the struct for the branch
    stringstream result;

    // First, check if this is just a scalar branch (i.e. single-leaf with no
    // subbranches).  If that is the case, then we just return a single member
    // for the parent struct.
    if(branch->GetNleaves() == 1 
       && branch->GetListOfBranches()->GetEntries() == 0)
    {
        // This is a single scalar branch, so insert it directly into the root
        // of the parent struct.
        return insert_hdf5_struct_member_for_leaf(
            container,
            branch->GetLeaf(branch->GetName())
        );
    }
    else
    {
        // Otherwise, this is a more complex branch, possibly containing
        // multiple leaves and/or subbranches.  We generate a struct member to
        // insert into the parent struct.
        container << "struct{";
        
        // First add our leaves
        TIter next_leaf(branch->GetListOfLeaves());
        TLeaf *leaf = NULL;
        while((leaf = (TLeaf *)next_leaf()))
        {
            if(!insert_hdf5_struct_member_for_leaf(container, leaf))
            {
                // The function failed, it should have printed an error message
                // if necessary, so just return
                return false;
            }
        }

        // Then recurse into subbranches
        TIter next_subbranch(branch->GetListOfBranches());
        TBranch *subbranch = NULL;
        while((subbranch = (TBranch *)next_subbranch()))
        {
            if(!insert_hdf5_struct_member_for_branch(container, subbranch))
            {
                // The function failed, it should have printed an error message
                // if necessary, so just return
                return false;
            }
        }

        // And close off the struct
        container << "}" << branch->GetName() << ";";

        return true;
    }
}


tuple<bool, string, string> root2hdf5::tree::hdf5_struct_for_tree(TTree *tree)
{
    // Create a stringstream to contain the outer struct
    stringstream struct_name;
    stringstream struct_code;

    // Set up the outer struct stuff
    struct_name << "tree_" << (long int)tree;
    struct_code << "struct " << struct_name.str() << "{";

    // Loop over the branches of the tree and add their members
    TIter next_branch(tree->GetListOfBranches());
    TBranch *branch = NULL;
    while((branch = (TBranch *)next_branch()))
    {
        // Add the code for the branch
        if(!insert_hdf5_struct_member_for_branch(struct_code, branch))
        {
            return tuple<bool, string, string>(false, "", "");
        }
    }

    // Close up the struct
    struct_code << "};";

    return tuple<bool, string, string>(true,
                                       struct_name.str(),
                                       struct_code.str());
}


bool root2hdf5::tree::convert(TTree *tree,
                              hid_t parent_destination)
{
    // Create a string which represents a struct that we can use to construct
    // the HDF5 composite data type
    tuple<bool, string, string> hdf5_struct_success_name_code
        = hdf5_struct_for_tree(tree);
    if(!get<0>(hdf5_struct_success_name_code))
    {
        // Conversion method should have printed an ERROR, just return
        return false;
    }
    string hdf5_struct_name = get<1>(hdf5_struct_success_name_code);
    string hdf5_struct_code = get<2>(hdf5_struct_success_name_code);
    cout << hdf5_struct_code << endl;

    // Tell CINT about the structure
    if(!process_long_line(hdf5_struct_code))
    {
        if(verbose)
        {
            cerr << "ERROR: Unable to generate temporary struct for converting "
                 << "tree \"" << tree->GetName() << "\"";
        }

        return false;
    }

    // TODO: Perhaps find some way to check the code being executed here,
    // although it is probably not essential since ROOT will probably crash
    // internally before returning anything.

    // Create a stringstream to build up our commands
    stringstream command;

    // Calculate the size of the structure
    command.str("");
    command << "sizeof(" << hdf5_struct_name << ");";
    size_t hdf5_struct_size = (size_t)gROOT->ProcessLine(
        command.str().c_str()
    );
    
    // Tell CINT to allocate an instance of the structure
    command.str("");
    command << "(void *)(new " << hdf5_struct_name << ");";
    void *hdf5_struct = (void *)gROOT->ProcessLine(command.str().c_str());

    // Create the (initially-empty) HDF5 compound datatype to use for the tree
    hid_t hdf5_type = H5Tcreate(H5T_COMPOUND, hdf5_struct_size);

    // TODO: Build the datatype

    // Create the dataspace with the same dimensions as the tree
    hsize_t n_entries = tree->GetEntries();
    hid_t hdf5_space = H5Screate_simple(1, &n_entries, NULL);

    /*
    // Create the dataset
    hid_t hdf5_dataset = H5Dcreate2(parent_destination,
                                    tree->GetName(),
                                    hdf5_type,
                                    hdf5_space,
                                    H5P_DEFAULT,
                                    H5P_DEFAULT,
                                    H5P_DEFAULT);

    // Loop through the tree, getting every entry, calling the converter, and
    // then writing it to the HDF5 dataset
    // TODO: Implement
    //

    // Close the data set
    if(H5Dclose(hdf5_dataset) < 0)
    {
        if(verbose)
        {
            cerr << "ERROR: Couldn't close HDF5 dataset for tree \""
                 << tree->GetName() << "\"" << endl;
        }

        return false;
    }*/
    
    // Close out the HDF5 data space
    if(H5Sclose(hdf5_space) < 0)
    {
        if(verbose)
        {
            cerr << "ERROR: Couldn't close HDF5 data space for tree \""
                 << tree->GetName() << "\"" << endl;
        }

        return false;
    }

    // Close out the HDF5 data type
    if(H5Tclose(hdf5_type) < 0)
    {
        if(verbose)
        {
            cerr << "ERROR: Couldn't close HDF5 data type for tree \""
                 << tree->GetName() << "\"" << endl;
        }

        return false;
    }

    // Tell CINT to deallocate the instance of the structure
    command.str("");
    command << "delete ((" << hdf5_struct_name << " *)" << hdf5_struct << ");";
    gROOT->ProcessLine(command.str().c_str());
    hdf5_struct = NULL;

    // All done
    return true;
}
