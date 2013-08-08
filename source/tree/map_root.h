#pragma once

// Standard includes
#include <functional>

// HACK: Use Boost.Tuple instead of std::tuple because at the moment, the LLVM-
// provided libc++ doesn't support the std::tuple, and Boost.Tuple is
// effectively the same thing.
// Boost includes
#include <boost/tuple/tuple.hpp>

// ROOT includes
#include <TTree.h>


namespace root2hdf5
{
    namespace tree
    {
        namespace map_root
        {
            // Callback type for executing ROOT converters
            typedef std::function<bool()> root_converter;

            // Callback type for deallocating ROOT conversion resources
            typedef std::function<bool()> root_resource_deallocator;

            // NOTE: Before calling this method, the
            // root2hdf5::structure::struct_code_for_tree method must have been
            // called to generate a CINT-known structure for mapping data into.

            // This function walks the leaves of a TTree, calling SetAddress on
            // each, mapping it either directly into the struct instance or
            // mapping it to some intermediate buffer and creating a converter
            // and deallocator callback.  This method returns a tuple of the
            // form:
            //      (success, combined_converter, combined_deallocator)
            boost::tuple<bool, root_converter, root_resource_deallocator>
            map_root_tree_into_struct_and_build_converter(
                TTree *tree,
                void *struct_instance
            );
        }
    }
}
