#pragma once

// Standard includes
#include <functional>
#include <string>

// ROOT includes
#include <TLeaf.h>


namespace root2hdf5
{
    namespace tree
    {
        namespace leaf_converters
        {
            struct leaf_converter
            {
                std::function<bool(TLeaf *)> can_handle;
                std::function<std::string(TLeaf *)> 
                    member_for_conversion_struct;
                
            };

            // Finds a leaf converter suitable for doing the leaf conversion.
            // If no conversion is found, this function returns NULL.  If a
            // non-null leaf-converter is returned, it will be a global
            // instance, and therefore you should not deallocate it.
            leaf_converter * find_converter(TLeaf *leaf);
        }
    }
}
