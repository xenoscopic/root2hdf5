#include "tree/leaf_converters/vector_converter.h"

// Boost includes
#include <boost/algorithm/string.hpp>

// root2hdf5 includes
#include "type.h"


// Standard namespaces
using namespace std;

// Boost namespaces
using namespace boost;

// root2hdf5 namespaces
using namespace root2hdf5::tree::leaf_converters::vector_converter;
using namespace root2hdf5::tree::leaf_converters;
using namespace root2hdf5::tree::map_hdf5;
using namespace root2hdf5::type;


root_vector_conversion 
vector_converter::root_type_name_to_vector_hdf5_type(string type_name)
{
    // Create the (initially-invalid) return object
    root_vector_conversion result = {false, 0, -1};

    // Do some (nasty) parsing of the type-name
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
        result.depth++;

        // Parse off outer vector
        replace_first(type_name, "vector<", "");
        replace_last(type_name, ">", "");
        trim(type_name);
    }

    // If the vector depth is 0, then there were no vectors, and this is a case
    // that we were not designed to handle, so return an invalid conversion
    if(result.depth == 0)
    {
        return result;
    }

    // Otherwise, we are now at the core type of the vector structure, so check
    // if it is a supportable HDF5 scalar type
    result.scalar_hdf5_type = root_type_name_to_scalar_hdf5_type(type_name);

    // If we have successfully converted the scalar type, then the result is
    // good to go.  If not, leave it as invalid.
    if(result.scalar_hdf5_type != -1)
    {
        result.valid = true;
    }

    return result;
}


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


hid_t vector_converter::hdf5_type(TLeaf * leaf, 
                                  vector<hdf5_type_deallocator> & deallocators)
{
    // TODO: Implement
    return -1;
}
