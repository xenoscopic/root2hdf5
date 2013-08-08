#pragma once

// Standard includes
#include <functional>
#include <string>

// ROOT includes
#include <TLeaf.h>

// HDF5 includes
#include <hdf5.h>

// root2hdf5 includes
#include "tree/map_hdf5.h"


namespace root2hdf5
{
    namespace tree
    {
        namespace leaf_converters
        {
            // This struct outlines the functionality needed in a leaf
            // converter, without requiring that the converter inherit from some
            // base class.  This might change in the future if the functionality
            // of a leaf converter becomes complex, but I don't really see a
            // need for strict OO at the moment.
            struct leaf_converter
            {
                // This function should return true if this leaf converter can
                // handle the leaf, false if not.
                std::function<bool(TLeaf *)> can_handle;

                // This function should return a string which can be included in
                // the code of a parent struct and that HDF5 data can be written
                // from and ROOT data written to.
                std::function<std::string(TLeaf *)> 
                    member_for_conversion_struct;

                // This function should return an HDF5 type which can be
                // included in the compound type of the parent branch in the
                // tree.  If the converter requires a deallocator for the type,
                // it can register it with the hdf5_type_deallocator vectors.
                std::function<hid_t(
                    TLeaf *, 
                    std::vector<
                        root2hdf5::tree::map_hdf5::hdf5_type_deallocator
                    > &
                )> hdf5_type;


            };

            // Finds a leaf converter suitable for doing the leaf conversion.
            // If no conversion is found, this function returns NULL.  If a
            // non-null leaf-converter is returned, it will be a global
            // instance, and therefore you should not deallocate it.
            leaf_converter * find_converter(TLeaf *leaf);
        }
    }
}
