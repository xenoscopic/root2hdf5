#pragma once

// root2hdf5 includes
#include "tree/leaf_converters.h"


namespace root2hdf5
{
    namespace tree
    {
        namespace leaf_converters
        {
            namespace vector_converter
            {
                // Structure for representing metadata about conversion of some
                // nested STL vector type into a multi-dimensional array of an
                // HDF5 scalar type.  For example, this conversion would be
                // valid for types such as:
                //
                //  vector<float>
                //  vector< vector<int> >
                //  vector< vector< vector<double> > >
                //  ...
                struct root_vector_conversion
                {
                    bool valid; // Whether or not the conversion is valid
                    unsigned depth; // The number of nested vectors
                    hid_t scalar_hdf5_type; // The HDF5 scalar type equivalent
                                            // to the type at the core of the
                                            // vectors
                };

                // Converts a ROOT type name representing nested STL vector
                // types into a root_vector_conversion containing metadata about
                // the type conversion.  If no conversion exists, the is_valid
                // member of the root_vector_conversion will be set to false.
                root_vector_conversion root_type_name_to_vector_hdf5_type(
                    std::string type_name
                );

                bool can_handle(TLeaf *leaf);
                std::string member_for_conversion_struct(TLeaf *leaf);
                hid_t hdf5_type(
                    TLeaf * leaf, 
                    std::vector<
                        root2hdf5::tree::map_hdf5::hdf5_type_deallocator
                    > & deallocators
                );
            }
        }
    }
}
