#include "tree/map_hdf5.h"

// Standard includes
#include <string>
#include <vector>

// Boost includes
#include <boost/algorithm/string.hpp>

// ROOT includes
#include <TBranch.h>
#include <TLeaf.h>

// root2hdf5 includes
#include "options.h"
#include "tree/walk.h"
#include "tree/structure.h"
#include "tree/leaf_converters.h"


// Standard namespaces
using namespace std;

// Boost namespaces
using namespace boost;

// root2hdf5 namespaces
using namespace root2hdf5::tree::map_hdf5;
using namespace root2hdf5::options;
using namespace root2hdf5::tree::walk;
using namespace root2hdf5::tree::structure;
using namespace root2hdf5::tree::leaf_converters;


boost::tuple<hid_t, hdf5_type_deallocator>
root2hdf5::tree::map_hdf5::hdf5_type_for_tree(TTree *tree)
{
    // Create the deallocator list
    vector<hdf5_type_deallocator> deallocators;

    // Compute the structure type name for the tree
    string hdf5_struct_name = struct_type_name_for_tree(tree);

    // Grab the size for the struct
    size_t hdf5_struct_size = sizeof_type_by_name(hdf5_struct_name);

    // Create the HDF5 data type for the tree and create a deallocator for it
    hid_t result = H5Tcreate(H5T_COMPOUND, hdf5_struct_size);
    if(result < 0)
    {
        if(verbose)
        {
            cerr << "ERROR: Couldn't create compound data type for tree \""
                 << tree->GetName() << "\"" << endl;
        }

        return boost::make_tuple(-1, []()->bool{return true;});
    }
    deallocators.push_back([=]() -> bool {
        // Close out the type
        if(H5Tclose(result) < 0)
        {
            if(verbose)
            {
                cerr << "ERROR: Couldn't close HDF5 data type for tree \""
                     << tree->GetName() << "\"" << endl;
            }

            return false;
        }

        return true;
    });

    // Create data structures to track the current stack of branches, HDF5
    // types, and offsets
    vector<string> path_stack; // Stack of parent paths
    vector<size_t> path_offset_stack = {0}; // Stack of parent offsets in the
                                            // top-level HDF5 struct
    vector<hid_t> hdf5_type_stack = {result}; // Stack of HDF5 compound types

    // Walk the tree and add subtypes
    bool success = walk_tree(
        tree,

        // Branch open
        [&path_stack,
         &path_offset_stack,
         &hdf5_type_stack,
         &deallocators,
         hdf5_struct_name]
        (TBranch *branch) -> bool {
            // Push the branch onto the path stack
            path_stack.push_back(branch->GetName());

            // Calculate the complete path to this branch in the master struct
            string branch_path_in_struct = join(path_stack, ".");

            // Calculate the offset of this branch in the top-level HDF5 struct
            path_offset_stack.push_back(offsetof_member_in_type_by_name(
                hdf5_struct_name,
                branch_path_in_struct
            ));

            // Calculate the size of this branch in the struct
            size_t branch_size_in_struct = sizeof_member_in_type_by_name(
                hdf5_struct_name,
                branch_path_in_struct
            );

            // Create the new compound data type and push it onto the stack
            hid_t branch_type = H5Tcreate(
                H5T_COMPOUND,
                branch_size_in_struct
            );
            if(branch_type < 0)
            {
                if(verbose)
                {
                    cerr << "ERROR: Unable to create compound type for branch "
                         << "at path \"" << branch_path_in_struct << "\""
                         << endl;
                }

                return false;
            }

            // Add the type to the stack
            hdf5_type_stack.push_back(branch_type);

            // Create a deallocator
            deallocators.push_back([=]() -> bool {
                if(H5Tclose(branch_type) < 0)
                {
                    if(verbose)
                    {
                        cerr << "ERROR: Couldn't close HDF5 data type for "
                             << "branch at path \"" << branch_path_in_struct
                             << "\"" << endl;
                    }

                    return false;
                }

                return true;
            });

            return true;
        },

        // Leaf process
        [&path_stack,
         &path_offset_stack,
         &hdf5_type_stack,
         &deallocators,
         hdf5_struct_name]
        (TLeaf *leaf) -> bool {
            // First, find a leaf converter, and if we can't find one, just
            // ignore the leaf (a warning will have already been generated)
            leaf_converter *converter = find_converter(leaf);
            if(converter == NULL)
            {
                return true;
            }

            // Temporarily push the leaf onto the path stack just to calculate
            // its path in the master struct
            path_stack.push_back(leaf->GetName());
            string leaf_path_in_struct = join(path_stack, ".");
            path_stack.pop_back();

            // Calculate the offset of this leaf in the master struct
            size_t leaf_offset_in_struct = offsetof_member_in_type_by_name(
                hdf5_struct_name,
                leaf_path_in_struct
            );

            // Calculate the offset of this branch relative to its parent
            size_t leaf_offset_in_parent
                = leaf_offset_in_struct - path_offset_stack.back();

            // Calculate the HDF5 type for the leaf
            hid_t leaf_type = converter->hdf5_type_for_leaf(leaf, deallocators);

            // Insert the HDF5 type into the parent compound type
            if(H5Tinsert(hdf5_type_stack.back(),
                         leaf->GetName(),
                         leaf_offset_in_parent,
                         leaf_type) < 0)
            {
                if(verbose)
                {
                    cerr << "ERROR: Unable to insert compound type for leaf \""
                     << leaf->GetName() << "\" into parent compound type"
                     << endl;
                }

                return false;
            }

            return true;
        },

        // Branch close
        [&path_stack,
         &path_offset_stack,
         &hdf5_type_stack,
         &deallocators,
         hdf5_struct_name]
        (TBranch *branch) -> bool {
            // Clear this branch off the path stack
            path_stack.pop_back();

            // Grab the offset of this branch in the master struct and pop it
            // off the stack
            size_t branch_offset_in_struct = path_offset_stack.back();
            path_offset_stack.pop_back();
            
            // Calculate the offset of this branch relative to its parent
            size_t branch_offset_in_parent
                = branch_offset_in_struct - path_offset_stack.back();

            // Pop the type off the stack
            hid_t branch_type = hdf5_type_stack.back();
            hdf5_type_stack.pop_back();

            // Insert the HDF5 type into the parent compound type
            if(H5Tinsert(hdf5_type_stack.back(),
                         branch->GetName(),
                         branch_offset_in_parent,
                         branch_type) < 0)
            {
                if(verbose)
                {
                    cerr << "ERROR: Unable to insert compound type for branch "
                     << "\"" << branch->GetName() << "\" into parent compound "
                     << "type" << endl;
                }

                return false;
            }

            return true;
        }
    );

    // Check that the walking went correctly
    if(!success)
    {
        if(verbose)
        {
            cerr << "ERROR: Unable to generate HDF5 compound type for tree \""
             << tree->GetName() << "\"" << endl;
        }
        
        result = -1;
    }
    
    // All done
    return boost::make_tuple(result, [=]() -> bool {
        for(auto it = deallocators.rbegin();
            it != deallocators.rend();
            it++)
        {
            // Iterate over deallocators in reverse so we close types in the
            // opposite order of how we open them
            if(!(*it)())
            {
                return false;
            }
        }

        return true;
    });
}
