#pragma once

// root2hdf5 includes
#include "tree/leaf_converters.h"


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
                hid_t hdf5_type_for_leaf(
                    TLeaf * leaf, 
                    std::vector<
                        root2hdf5::tree::map_hdf5::hdf5_type_deallocator
                    > & deallocators
                );
                bool map_leaf_and_build_converter(
                    TLeaf *leaf,
                    void *address,
                    std::vector<
                        root2hdf5::tree::map_root::root_converter
                    > & converters,
                    std::vector<
                        root2hdf5::tree::map_root::root_resource_deallocator
                    > & deallocators
                );
            }
        }
    }
}
