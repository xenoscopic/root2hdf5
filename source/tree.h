#pragma once

// Standard includes
#include <string>

// HDF5 includes
#include <H5Cpp.h>


// Definitions to work around H5 namespaces
#ifdef H5_NO_NAMESPACE
#define HDF5_TYPE_CLASS DataType
#else
#define HDF5_TYPE_CLASS H5::DataType
#endif


namespace root2hdf5
{
    namespace tree
    {
        class branch_converter
        {
            public:
                // Constructors
                branch_converter(std::string root_type_name);

                // Data type properties
                size_t size();
                const HDF5_TYPE_CLASS & hdf5_type();

            private:
                

        };
    }
}
