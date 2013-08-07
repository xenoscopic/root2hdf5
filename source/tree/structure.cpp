#include "tree/structure.h"

// Standard includes
#include <sstream>

// ROOT includes
#include <TBranch.h>
#include <TLeaf.h>

// root2hdf5 includes
#include "options.h"
#include "tree/walk.h"
#include "tree/leaf_converters.h"


// Standard namespaces
using namespace std;

// root2hdf5 namespaces
using namespace root2hdf5::tree::structure;
using namespace root2hdf5::options;
using namespace root2hdf5::tree::walk;
using namespace root2hdf5::tree::leaf_converters;


string root2hdf5::tree::structure::struct_type_name_for_tree(TTree *tree)
{
    // Create a stringstream to build the result
    stringstream result;

    // Create a unique name
    result << "tree_" << (long int)tree;

    return result.str();
}


string
root2hdf5::tree::structure::struct_code_for_tree(TTree *tree)
{
    // Create a stringstream to build the result
    stringstream result;

    // Create the outer structure
    result << "struct " << struct_type_name_for_tree(tree) << "{";

    // Walk the tree and add submembers
    bool success = walk_tree(
        tree,
        [&result](TBranch *branch) -> bool {
            // Hide unused variable warning
            (void)branch;
            result << "struct{";
            return true;
        },
        [&result](TBranch *branch) -> bool {
            result << "}" << branch->GetName() << ";";
            return true;
        },
        [&result](TLeaf *leaf) -> bool {
            // Find a leaf converter
            leaf_converter *converter = find_converter(leaf);

            // If we couldn't find one, just ignore the leaf, but warn about
            // skipping it if necessary.
            // TODO: We warn about skipping leaves here because struct
            // generation happens to be the first step we do, and consequently
            // we don't warn in the HDF5 type generation and TTree address
            // mapping steps, but I wonder if we should do a new 0th pass where
            // we just go through and look for unsupported leaves.  This will
            // also be necessary in the case of TBranches which are non-scalar
            // but have no supported submembers, which will, at the moment,
            // fail due to empty generated structs.
            if(converter == NULL)
            {
                if(verbose)
                {
                    cerr << "WARNING: Leaf \"" << leaf->GetName() << "\" has "
                         << "an unknown type \"" << leaf->GetTypeName()
                         << "\" - skipping" << endl;
                }

                return true;
            }

            // Add it's structure member
            result << converter->member_for_conversion_struct(leaf);

            return true;
        }
    );

    // If things failed during walking, return a bogus result
    if(!success)
    {
        return "";
    }

    // Close the outer structure
    result << "};";

    // All done
    return result.str();
}
