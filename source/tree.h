#pragma once

// ROOT includes
#include <TTree.h>

// HDF5 includes
#include <hdf5.h>


namespace root2hdf5
{
    namespace tree
    {
        // This method converts a TTree object into an HDF5 dataset with a
        // compound datatype modeled after the TTree branches.  The dataset is
        // created in the HDF5 file or group pointed to by parent_destination.
        // This method returns false if the conversion fails, true if it
        // succeeds.
        bool convert(TTree *tree,
                     hid_t parent_destination);
    }
}
