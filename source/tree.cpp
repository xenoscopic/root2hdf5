#include "tree.h"

// Standard includes
#include <iostream>
#include <map>
#include <string>
#include <limits>

// Boost includes
#include <boost/assign.hpp>

// ROOT includes
#include <TBranch.h>
#include <TLeaf.h>

// root2hdf5 includes
#include "options.h"


// Standard namespaces
using namespace std;

// Boost namespaces
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

        size_t BRANCH_NOT_SUPPORTED = numeric_limits<size_t>::max();
        size_t size_of_branch(TBranch *branch);
    }
}


size_t root2hdf5::tree::size_of_branch(TBranch *branch)
{
    // Grab the branch name
    string branch_name(branch->GetName());

    // Grab the leaf representing the branch's data
    TLeaf *leaf = branch->GetLeaf(branch_name.c_str());

    // Grab the branch type name
    string type_name = leaf->GetTypeName();

    // If this a scalar type, we can just return the leaf's size as reported by
    // ROOT
    if(root_type_name_to_hdf5_type.count(type_name) == 1)
    {
        return (size_t)leaf->GetLenType();
    }

    if(verbose)
    {
        cerr << "WARNING: Unsupported branch \""
             << branch_name
             << "\" of type \""
             << type_name
             << "\" - skipping"
             << endl;
    }

    return BRANCH_NOT_SUPPORTED;
}


bool root2hdf5::tree::convert(TTree *tree,
                              hid_t parent_destination)
{
    // Loop through the branches of the tree
    TIter next_branch(tree->GetListOfBranches());
    TBranch *branch = NULL;
    while((branch = (TBranch *)next_branch()))
    {
        // Get the branch size
        size_t branch_size = size_of_branch(branch);

        // If it is not supported, move on
        if(branch_size == BRANCH_NOT_SUPPORTED)
        {
            continue;
        }
    }

    // TODO: Remove
    parent_destination = 1;

    // All done
    return true;
}
