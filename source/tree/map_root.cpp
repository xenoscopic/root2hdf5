#include "tree/map_root.h"

// Standard includes
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
using namespace root2hdf5::tree::map_root;
using namespace root2hdf5::options;
using namespace root2hdf5::tree::walk;
using namespace root2hdf5::tree::structure;
using namespace root2hdf5::tree::leaf_converters;


boost::tuple<bool, root_converter, root_resource_deallocator>
root2hdf5::tree::map_root::map_root_tree_into_struct_and_build_converter(
    TTree *tree,
    void *struct_instance
)
{
    // Create the deallocator and converter lists
    vector<root_converter> converters;
    vector<root_resource_deallocator> deallocators;

    // Compute the structure type name for the tree
    string hdf5_struct_name = struct_type_name_for_tree(tree);

    // Create data structure to track the current stack of branches
    vector<string> path_stack;

    // Walk the tree and do mappings
    bool success = walk_tree(
        tree,

        // Branch open
        [&path_stack](TBranch *branch) -> bool {
            // Push the branch onto the path stack
            path_stack.push_back(branch->GetName());

            return true;
        },

        // Leaf process
        [&path_stack,
         &converters,
         &deallocators,
         hdf5_struct_name,
         struct_instance]
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

            // Calculate the position in memory
            void *leaf_location 
                = (void *)(((char *)struct_instance) + leaf_offset_in_struct);

            // Set the address
            return converter->map_leaf_and_build_converter(
                leaf,
                leaf_location,
                converters,
                deallocators
            );
        },

        // Branch close
        [&path_stack](TBranch *branch) -> bool {
            // Silence unused variable warnings
            (void)branch;

            // Clear this branch off the path stack
            path_stack.pop_back();            

            return true;
        }
    );

    // Check that the walking went correctly
    if(!success)
    {
        if(verbose)
        {
            cerr << "ERROR: Unable to map ROOT tree \"" << tree->GetName() 
            << "\" into corresponding structure" << endl;
        }
    }

    // All done
    return boost::make_tuple(
        success,
        [=]() -> bool {
            for(auto it = converters.begin();
                it != converters.end();
                it++)
            {
                if(!(*it)())
                {
                    return false;
                }
            }

            return true;
        },
        [=]() -> bool {
            // Iterate over deallocators in reverse so we close resources in the
            // opposite order of how we open them
            for(auto it = deallocators.rbegin();
                it != deallocators.rend();
                it++)
            {
                if(!(*it)())
                {
                    return false;
                }
            }

            return true;
        }
    );
}
