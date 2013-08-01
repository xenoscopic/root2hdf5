#pragma once

// Standard includes
#include <string>

// HDF5 includes
#include <hdf5.h>


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

            private:


        };
    }
}
