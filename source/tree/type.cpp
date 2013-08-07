#include "type.h"

// Standard includes
#include <map>

// Boost includes
#include <boost/assign.hpp>
#include <boost/algorithm/string.hpp>


// Standard namespaces
using namespace std;

// Boost namespaces
using namespace boost;
using namespace boost::assign;

// root2hdf5 namespaces
using namespace root2hdf5::tree::type;


// Private declarations
namespace root2hdf5
{
    namespace tree
    {
        namespace type
        {
            // Map of scalar type conversions from ROOT to HDF5
            // Normally we would just use the _t type names, but if ROOT has
            // vectors included as branches, then it gives the base type name as
            // a standard C/C++ type name, so we need to be able to convert
            // those too!
            map<string, hid_t> _root_type_name_to_scalar_hdf5_type = 
            map_list_of
                // Boolean types
                // HACK: HDF5 expects it's booleans to be > 1 byte, so mapping
                // these to the native HDF5 bools and then giving the offset of
                // a bool type in the generated structure makes HDF5 suspect
                // there are overlapping members in the data structure, so we
                // hack the bools into signed chars.
                ("bool", H5T_NATIVE_SCHAR)
                ("Bool_t", H5T_NATIVE_SCHAR)

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
        }
    }
}


hid_t
root2hdf5::tree::type::root_type_name_to_scalar_hdf5_type(string type_name)
{
    // First check that a viable conversion actually exists
    if(_root_type_name_to_scalar_hdf5_type.count(type_name) == 0)
    {
        return -1;
    }

    // If it does, give it to the user
    return _root_type_name_to_scalar_hdf5_type[type_name];
}


root_vector_conversion 
root2hdf5::tree::type::root_type_name_to_vector_hdf5_type(string type_name)
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
