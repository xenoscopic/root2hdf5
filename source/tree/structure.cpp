#include "tree/structure.h"

// Standard includes
#include <sstream>

// ROOT includes
#include <TBranch.h>
#include <TLeaf.h>

// root2hdf5 includes
#include "options.h"
#include "tree/type.h"
#include "tree/walk.h"


// Standard namespaces
using namespace std;

// root2hdf5 namespaces
using namespace root2hdf5::tree::structure;
using namespace root2hdf5::options;
using namespace root2hdf5::tree::type;
using namespace root2hdf5::tree::walk;


// Private namespace members
namespace root2hdf5
{
    namespace tree
    {
        namespace structure
        {
            // This private function is the real meat-and-bones of the struct
            // building process.  It could conceivably be done as part of a
            // lambda as is the case for the branch opener/closer, but it made
            // sense to keep it as a separate function since it was a bit on
            // the long side and will only grow as more datatypes become
            // supported.
            bool insert_struct_member_for_leaf(TLeaf *leaf,
                                               stringstream &container);
        }
    }
}


bool root2hdf5::tree::structure::insert_struct_member_for_leaf(
    TLeaf *leaf,
    stringstream &container
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
            return insert_struct_member_for_leaf(leaf, result);
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
