#pragma once

// ROOT includes
#include <TTree.h>

// HDF5 includes
#include <hdf5.h>


namespace root2hdf5
{
    namespace tree
    {
        bool convert(TTree *tree,
                     hid_t parent_destination);
    }
}
