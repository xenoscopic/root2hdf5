#pragma once

// ROOT includes
#include <TDirectory.h>

// HDF5 includes
#include <H5Cpp.h>


// Definitions to work around H5 namespaces
#ifdef H5_NO_NAMESPACE
#define HDF5_GROUP_TYPE CommonFG
#else
#define HDF5_GROUP_TYPE H5::CommonFG
#endif


namespace root2hdf5
{
    namespace convert
    {
        // Primary conversion method
        bool convert(TDirectory *directory,
                     HDF5_GROUP_TYPE *group);
    }
}
