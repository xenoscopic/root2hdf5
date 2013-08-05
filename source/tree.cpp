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

// Boost includes
// HACK: Need to define this macro to tell Boost not to use deprecated
// Boost.System constructs which result in unused-variable errors.
#ifndef BOOST_SYSTEM_NO_DEPRECATED
#define BOOST_SYSTEM_NO_DEPRECATED 1
#endif
#include <boost/assign.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

// ROOT includes
#include <TROOT.h>
#include <TBranch.h>
#include <TLeaf.h>

// root2hdf5 includes
#include "options.h"


// Standard namespaces
using namespace std;

// Boost namespaces
using namespace boost;
using namespace boost::assign;

// Boost namespace aliases
namespace fs = boost::filesystem;

// root2hdf5 namespaces
using namespace root2hdf5::tree;
using namespace root2hdf5::options;


// Private function and mapping declarations
namespace root2hdf5
{
    namespace tree
    {
        // Map of scalar type conversions from ROOT to HDF5
        // Normally we would just use the _t type names, but if ROOT has vectors
        // included as branches, then it gives the base type name as a standard
        // C/C++ type name, so we need to be able to convert those too!
        map<string, hid_t> root_type_name_to_hdf5_type = 
        map_list_of
            // Boolean types
            // HACK: HDF5 expects it's booleans to be > 1 byte, so mapping
            // these to the native HDF5 bools and then giving the offset of a
            // bool type in the generated structure makes HDF5 suspect there
            // are overlapping members in the data structure, so we hack the
            // bools into signed chars.
            ("bool", H5T_NATIVE_SCHAR)
            ("Bool_t", H5T_NATIVE_SCHAR)

            // Signed character types
            ("char", H5T_NATIVE_SCHAR)
            ("Char_t", H5T_NATIVE_SCHAR)

            // Unsigned character types
            ("unsigned char", H5T_NATIVE_UCHAR)
            ("UChar_t", H5T_NATIVE_UCHAR)

            // Signed short types
            ("short", H5T_NATIVE_SHORT)
            ("Short_t", H5T_NATIVE_SHORT)

            // Unsigned short types
            ("unsigned short", H5T_NATIVE_USHORT)
            ("UShort_t", H5T_NATIVE_USHORT)

            // Signed int types
            ("int", H5T_NATIVE_INT)
            ("Int_t", H5T_NATIVE_INT)

            // Unsigned int types
            ("unsigned int", H5T_NATIVE_UINT)
            ("unsigned", H5T_NATIVE_UINT)
            ("UInt_t", H5T_NATIVE_UINT)

            // Signed long types
            ("long", H5T_NATIVE_LONG)
            ("Long_t", H5T_NATIVE_LONG)

            // Unsigned long types
            ("unsigned long", H5T_NATIVE_ULONG)
            ("ULong_t", H5T_NATIVE_ULONG)

            // Signed long long types
            ("long long", H5T_NATIVE_LLONG)
            ("Long64_t", H5T_NATIVE_LLONG)

            // Unsigned long long types
            ("ULong_t", H5T_NATIVE_ULLONG)
            ("ULong64_t", H5T_NATIVE_ULLONG)

            // Float types
            ("float", H5T_NATIVE_FLOAT)
            ("Float_t", H5T_NATIVE_FLOAT)

            // Double types
            ("double", H5T_NATIVE_DOUBLE)
            ("Double_t", H5T_NATIVE_DOUBLE)
        ;

        // Convenience wrapper for loading potentially long code
        bool process_long_line(const string & long_line);

        // Converts a TBranch to a string representing a member suitable for
        // inclusion in an HDF5 struct
        string hdf5_struct_member_for_branch(TBranch *branch);

        // Converts a TTree (and its branches) to a string representing a struct
        // suitable for use as an HDF5 struct.  Returns a pair of
        //  (type_name, code_for_type)
        pair<string, string> hdf5_struct_for_tree(TTree *tree);

        // Type for providing callbacks to data conversion functions which need
        // to be called every time TTree::GetEntry is called to convert
        // non-scalar members
        typedef std::function<void()> converter;

        converter map_branch_and_build_converter(
            TBranch *branch,
            string branch_prefix,
            string hdf5_struct_name,
            hid_t hdf5_type,
            void *hdf5_struct
        );

        converter map_tree_and_build_composite_type_and_converter(
            TTree *tree,
            string hdf5_struct_name,
            hid_t hdf5_type,
            void *hdf5_struct
        );
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


string root2hdf5::tree::hdf5_struct_member_for_branch(TBranch *branch)
{
    // Create a stringstream to contain the struct for the branch
    stringstream result;

    // First, check if this is just a scalar branch (i.e. single-leaf with no
    // subbranches).  If that is the case, then we just return a single member
    // for the parent struct.
    if(branch->GetNleaves() == 1 
       && branch->GetListOfBranches()->GetEntries() == 0)
    {
        // This is a single scalar branch.  Grab the type name
        string type_name = branch->GetLeaf(branch->GetName())->GetTypeName();

        // Check that we can support this type
        if(root_type_name_to_hdf5_type.count(type_name) != 1)
        {
            // Unsupported type
            if(verbose)
            {
                cerr << "WARNING: Unsupported type \"" << type_name << "\" for "
                     << "branch \"" << branch->GetName() << "\" - skipping"
                     << endl;
            }

            return "";
        }

        // We can support this just fine
        result << type_name << " " << branch->GetName() << ";";
    }
    else
    {
        // This is a more complex branch
        // TODO: Implement
        return "";
    }

    return result.str();
}


pair<string, string> root2hdf5::tree::hdf5_struct_for_tree(TTree *tree)
{
    // Create a stringstream to contain the outer struct
    stringstream struct_name;
    stringstream result;

    // Set up the outer struct stuff
    struct_name << "tree_" << (long int)tree;
    result << "struct " << struct_name.str() << "{";

    // Loop over the branches of the tree and add their structs
    TIter next_branch(tree->GetListOfBranches());
    TBranch *branch = NULL;
    while((branch = (TBranch *)next_branch()))
    {
        // Calculate the member
        string branch_struct_member = 
            hdf5_struct_member_for_branch(branch);

        // Make sure it was supported
        if(branch_struct_member == "")
        {
            continue;
        }

        result << branch_struct_member;
    }

    // Close up the struct
    result << "};";

    return make_pair(struct_name.str(), result.str());
}


converter root2hdf5::tree::map_branch_and_build_converter(
    TBranch *branch,
    string branch_prefix,
    string hdf5_struct_name,
    hid_t hdf5_type,
    void *hdf5_struct
)
{
    // TODO: When we support other branches, we need to redo this function and
    // remove this conditional, just handling the general case

    // First, check if this is just a scalar branch (i.e. single-leaf with no
    // subbranches).  If that is the case, then we just return a single member
    // for the parent struct.
    if(branch->GetNleaves() == 1 
       && branch->GetListOfBranches()->GetEntries() == 0)
    {
        // This is a single scalar branch.  Grab the type name
        string type_name = branch->GetLeaf(branch->GetName())->GetTypeName();

        // Check that we can support this type
        if(root_type_name_to_hdf5_type.count(type_name) != 1)
        {
            // No need to provide a warning here, it will have already been
            // provided in hdf5_struct_member_for_branch
            return [](){};
        }

        // Check the offset of the field in our struct
        size_t leaf_offset = gROOT->ProcessLine(
            (string("offsetof(")
             + hdf5_struct_name
             + string(",")
             + string(branch->GetName())
             + string(");")).c_str()
        );

        // Grab the HDF5 type
        hid_t hdf5_scalar_type = root_type_name_to_hdf5_type[type_name];
        if(H5Tinsert(hdf5_type,
                     branch->GetName(), // TODO: Fix this for leaves
                     leaf_offset,
                     hdf5_scalar_type) < 0)
        {
            if(verbose)
            {
                // TODO: Fix this for leaves
                cerr << "ERROR: Could not insert scalar type for leaf \""
                     << branch->GetName() << "\"" << endl;
            }

            // TODO: How can we return an error here?
            return [](){};
        }

        // No complex converter necessary
        return [](){};
    }
    else
    {
        // This is a more complex branch
        return [](){};
    }

    // Return an empty converter
    return [](){};   
}

converter root2hdf5::tree::map_tree_and_build_composite_type_and_converter(
    TTree *tree,
    string hdf5_struct_name,
    hid_t hdf5_type,
    void *hdf5_struct
)
{
    // Create a vector of converters that we can use in our own lambda
    vector<converter> converters;

    // Loop over the branches of the tree and add their structs
    TIter next_branch(tree->GetListOfBranches());
    TBranch *branch = NULL;
    while((branch = (TBranch *)next_branch()))
    {
        // Map and calculate the converter for the branch
        converters.push_back(
            map_branch_and_build_converter(
                branch,
                "",
                hdf5_struct_name,
                hdf5_type,
                hdf5_struct
            )
        );
    }

    return [=]()
    {
        // Loop over all branch converters and call them
        for(auto it = converters.begin();
            it != converters.end();
            it++)
        {
            (*it)();
        }
    };
}


bool root2hdf5::tree::convert(TTree *tree,
                              hid_t parent_destination)
{
    // Create a string which represents a struct that we can use to construct
    // the HDF5 composite data type
    auto hdf5_struct_name_code = hdf5_struct_for_tree(tree);
    string hdf5_struct_name = hdf5_struct_name_code.first;
    string hdf5_struct_code = hdf5_struct_name_code.second;

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

    // Create the HDF5 compound datatype to use for the tree
    hid_t hdf5_type = H5Tcreate(H5T_COMPOUND, hdf5_struct_size);

    // Map the tree branches into the HDF5 structure and fill in the composite
    // HDF5 type
    converter convert = map_tree_and_build_composite_type_and_converter(
        tree,
        hdf5_struct_name,
        hdf5_type,
        hdf5_struct
    );

    // Create the dataspace with the same dimensions as the tree
    hsize_t n_entries = tree->GetEntries();
    hid_t hdf5_space = H5Screate_simple(1, &n_entries, NULL);

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
    }
    
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
