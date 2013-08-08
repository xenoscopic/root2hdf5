#pragma once

// Standard includes
#include <string>

// HDF5 includes
#include <hdf5.h>


namespace root2hdf5
{
    namespace type
    {
        // Converts a ROOT type name to an HDF5 scalar (atomic) type.  If no
        // conversion exists, this function will return -1.
        hid_t root_type_name_to_scalar_hdf5_type(std::string type_name);
    }
}
