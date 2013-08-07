#include "tree/leaf_converters/vector_converter.h"

// root2hdf5 includes
#include "type.h"


// Standard namespaces
using namespace std;

// root2hdf5 namespaces
using namespace root2hdf5::tree::leaf_converters;
using namespace root2hdf5::type;


bool vector_converter::can_handle(TLeaf *leaf)
{
    // TODO: Implement
    return false;
    // return root_type_name_to_vector_hdf5_type(leaf->GetTypeName()).valid;
}

string vector_converter::member_for_conversion_struct(TLeaf *leaf)
{
    // TODO: Implement
    return "";
}
