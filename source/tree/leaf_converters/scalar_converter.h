#pragma once

// Standard includes
#include <string>

// ROOT includes
#include <TLeaf.h>


namespace root2hdf5
{
    namespace tree
    {
        namespace leaf_converters
        {
            namespace scalar_converter
            {
                bool can_handle(TLeaf *leaf);
                std::string member_for_conversion_struct(TLeaf *leaf);
            }
        }
    }
}
