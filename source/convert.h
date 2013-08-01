#pragma once

// ROOT includes
#include <TDirectory.h>

// HDF5 includes
#include <hdf5.h>


namespace root2hdf5
{
    namespace convert
    {
        // Primary conversion method
        bool convert(TDirectory *directory,
                     hid_t parent_destination);
    }
}
