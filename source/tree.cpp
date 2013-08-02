#include "tree.h"

// Standard includes
#include <iostream>
#include <map>
#include <string>

// Boost includes
#include <boost/assign.hpp>
#include <boost/algorithm/string.hpp>

// ROOT includes
#include <TBranch.h>
#include <TLeaf.h>

// root2hdf5 includes
#include "options.h"


// Standard namespaces
using namespace std;

// Boost namespaces
using namespace boost;
using namespace boost::assign;

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
            ("bool", H5T_NATIVE_HBOOL)
            ("Bool_t", H5T_NATIVE_HBOOL)

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

        string type_name_for_branch(TBranch *branch);
        bool is_scalar_branch(TBranch *branch);
        bool convert_scalar_branch(TBranch *branch,
                                   hid_t tree_group);
        bool is_convertible_vector_branch(TBranch *branch);
        bool convert_vector_branch(TBranch *branch,
                                   hid_t tree_group);
        bool convert_branch(TBranch *branch,
                            hid_t tree_group);

        class branch_converter
        {
            public:
                branch_converter(TBranch *branch);

                // Return the size of the buffer necessary for loading the
                // branch

                size_t buffer_size();

                // Return a datatype for the branch which can be included in the
                // compound data type generated for the tree.  The returned type
                // has the same lifetime as the class instance.
                hid_t hdf5_data_type();

                // Set the buffer for loading data into
                void set_buffer_address(void *buffer);

                // Convert the data in the buffer
                void convert_data();

            private:
                TBranch *_branch;
        };
    }
}


branch_converter::branch_converter(TBranch *branch) :
_branch(branch)
{

}

string root2hdf5::tree::type_name_for_branch(TBranch *branch)
{
    return branch->GetLeaf(branch->GetName())->GetTypeName();
}

bool root2hdf5::tree::is_scalar_branch(TBranch *branch)
{
    return root_type_name_to_hdf5_type.count(type_name_for_branch(branch)) == 1;
}

bool root2hdf5::tree::convert_scalar_branch(TBranch *branch,
                                            hid_t tree_group)
{
    // Grab the tree and entry count
    TTree *tree = branch->GetTree();
    hsize_t n_entries = tree->GetEntries();

    // Grab the leaf representing the branch
    // TLeaf *leaf = branch->GetLeaf(branch->GetName());

    // Create a dataset to represent the branch
    hid_t dataspace = H5Screate_simple(1, &n_entries, NULL);

    // Create the dataset
    // TODO: Should we check our map lookup here?
    hid_t dataset = H5Dcreate2(
        tree_group,
        branch->GetName(),
        root_type_name_to_hdf5_type[type_name_for_branch(branch)],
        dataspace,
        H5P_DEFAULT,
        H5P_DEFAULT,
        H5P_DEFAULT
    );

    // Disable all branches in the tree except the selected one
    tree->SetBranchStatus("*", 0);
    tree->SetBranchStatus(branch->GetName(), 1);




    // Cleanup
    if(H5Dclose(dataset) < 0)
    {
        // Closing the dataset failed
        if(verbose)
        {
            cerr << "ERROR: Closing dataset failed for branch \""
                 << branch->GetName() << "\"" << endl;
        }

        return false;
    }

    if(H5Sclose(dataspace) < 0)
    {
        // Closing the dataspace failed
        if(verbose)
        {
            cerr << "ERROR: Closing dataspace failed for branch \""
                 << branch->GetName() << "\"" << endl;
        }

        return false;
    }

    return true;
}

bool root2hdf5::tree::is_convertible_vector_branch(TBranch *branch)
{
    // Grab the type name
    string type_name = type_name_for_branch(branch);

    // Do some nasty parsing of the type name.  Maybe we can replace this with a
    // regex in the future, but for now we can do it manually
    unsigned vector_rank = 0;
    while(true)
    {
        // Check the outermost type to make sure it is a vector
        bool is_vector = starts_with(type_name, "vector<")
                         && ends_with(type_name, ">");

        // This is not a vector
        if(!is_vector)
        {
            break;
        }

        // Increment rank
        vector_rank++;

        // Parse off outer vector
        replace_first(type_name, "vector<", "");
        replace_last(type_name, ">", "");
        trim(type_name);
    }

    // If the vector rank is 0, it means there were no vectors, which we aren't
    // designed to handle
    if(vector_rank == 0)
    {
        return false;
    }

    // Okay, we have the core type now.  Check if it is a scalar type we can
    // support.
    if(root_type_name_to_hdf5_type.count(type_name) == 0)
    {
        return false;
    }

    // If we can support the scalar type, make sure the user hasn't nested so
    // many vectors 
    if(vector_rank > H5S_MAX_RANK)
    {
        return false;
    }

    return true;
}

bool root2hdf5::tree::convert_vector_branch(TBranch *branch,
                                            hid_t tree_group)
{
    // TODO: Implement
    branch = NULL;
    tree_group = 0;
    return true;
}


bool root2hdf5::tree::convert_branch(TBranch *branch,
                                     hid_t tree_group)
{
    // Make sure this is a single-leaf branch for now.  Multileaf branches are
    // currently unsupported.
    if(branch->GetNleaves() != 1)
    {
        if(verbose)
        {
            cout << "WARNING: Branch \"" << branch->GetName() 
                 << "\" has an unsupported number of leaves ("
                 << branch->GetNleaves() << ") - skipping" << endl;
        }
        
        return true;
    }

    // Otherwise see if it is some type of branch we can handle
    if(is_scalar_branch(branch))
    {
        return convert_scalar_branch(branch, tree_group);
    }
    else if(is_convertible_vector_branch(branch))
    {
        return convert_vector_branch(branch, tree_group);
    }
    
    // Unsupported type.  Warn, but return success.
    if(verbose)
    {
        cerr << "Branch \"" << branch->GetName() << "\" has unsupported type \""
             << type_name_for_branch(branch) << "\" - skipping" << endl;
    }

    return true;
}


bool root2hdf5::tree::convert(TTree *tree,
                              hid_t parent_destination)
{
    // Create a new group to represent the tree
    hid_t tree_group = H5Gcreate2(parent_destination,
                                  tree->GetName(),
                                  H5P_DEFAULT,
                                  H5P_DEFAULT,
                                  H5P_DEFAULT);

    // Loop through the branches of the tree
    TIter next_branch(tree->GetListOfBranches());
    TBranch *branch = NULL;
    while((branch = (TBranch *)next_branch()))
    {
        // Convert the branch
        if(!convert_branch(branch, tree_group))
        {
            return false;
        }
    }

    // Close up the tree group
    if(H5Gclose(tree_group) < 0)
    {
        if(verbose)
        {
            cerr << "ERROR: Closing tree group \"" 
                 << tree->GetName() 
                 << "\" failed";
        }
        return false;
    }

    // All done
    return true;
}
