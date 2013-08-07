#include "leaf_converters.h"

// Standard includes
#include <vector>

// root2hdf5 includes
#include "tree/leaf_converters/scalar_converter.h"
#include "tree/leaf_converters/vector_converter.h"


// Standard namespaces
using namespace std;

// root2hdf5 namespaces
using namespace root2hdf5::tree::leaf_converters;


// Private namespace members
namespace root2hdf5
{
    namespace tree
    {
        namespace leaf_converters
        {
            vector<leaf_converter> _leaf_converters =
            {
                // Scalar converter
                {
                    scalar_converter::can_handle,
                    scalar_converter::member_for_conversion_struct
                },

                // Vector converter
                {
                    vector_converter::can_handle,
                    vector_converter::member_for_conversion_struct
                }
            };
        }
    }
}


leaf_converter * root2hdf5::tree::leaf_converters::find_converter(TLeaf *leaf)
{
    // Try to find a registered converter
    for(auto it = _leaf_converters.begin();
        it != _leaf_converters.end();
        it++)
    {
        if(it->can_handle(leaf))
        {
            return &(*it);
        }
    }

    // No viable converter found
    return NULL;
}
