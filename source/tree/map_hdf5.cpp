#include "tree/map_hdf5.h"

// ROOT includes
#include <TBranch.h>
#include <TLeaf.h>

// root2hdf5 includes
#include "tree/structure.h"
#include "tree/leaf_converters.h"


// Standard namespaces
using namespace std;

// root2hdf5 namespaces
using namespace root2hdf5::tree::map_hdf5;
using namespace root2hdf5::tree::leaf_converters;


pair<hid_t, hdf5_type_deallocator> 
root2hdf5::tree::map_hdf5::hdf5_type_for_tree(TTree *tree)
{
    // Compute the structure type name for the tree
    // TODO: Implement
    return make_pair(-1, []()->bool{return true;});
}
