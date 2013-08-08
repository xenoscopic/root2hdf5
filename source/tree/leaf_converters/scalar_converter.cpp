#include "tree/leaf_converters/scalar_converter.h"

// root2hdf5 includes
#include "type.h"


// Standard namespaces
using namespace std;

// root2hdf5 namespaces
using namespace root2hdf5::tree::leaf_converters;
using namespace root2hdf5::tree::map_hdf5;
using namespace root2hdf5::type;


bool scalar_converter::can_handle(TLeaf *leaf)
{
    // Handle anything that is convertible to an HDF5 scalar type
    return root_type_name_to_scalar_hdf5_type(leaf->GetTypeName()) != -1;
}

string scalar_converter::member_for_conversion_struct(TLeaf *leaf)
{
    // Just return a single scalar member of the same type as the ROOT type.
    // Specifically:
    //      typename leaf_name;
    return string(leaf->GetTypeName()) + " " + leaf->GetName() + ";";
}

hid_t scalar_converter::hdf5_type(TLeaf * leaf, 
                                  vector<hdf5_type_deallocator> & deallocators)
{
    // Silence unused variable warnings
    (void)deallocators;

    // Just return the scalar HDF5 type
    return root_type_name_to_scalar_hdf5_type(leaf->GetTypeName());
}
