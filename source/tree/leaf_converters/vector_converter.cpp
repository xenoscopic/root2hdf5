#include "tree/leaf_converters/vector_converter.h"

// Standard includes
#include <iostream>

// Boost includes
#include <boost/algorithm/string.hpp>

// ROOT includes
#include <TROOT.h>

// root2hdf5 includes
#include "tree/structure.h"
#include "options.h"
#include "type.h"
#include "cint.h"


// Standard namespaces
using namespace std;

// Boost namespaces
using namespace boost;

// root2hdf5 namespaces
using namespace root2hdf5::tree::leaf_converters::vector_converter;
using namespace root2hdf5::tree::leaf_converters;
using namespace root2hdf5::tree::map_hdf5;
using namespace root2hdf5::tree::map_root;
using namespace root2hdf5::tree::structure;
using namespace root2hdf5::options;
using namespace root2hdf5::type;
using namespace root2hdf5::cint;


root_vector_conversion 
vector_converter::root_type_name_to_vector_hdf5_type(string type_name)
{
    // Create the (initially-invalid) return object
    root_vector_conversion result = {false, 0, -1};

    // Do some (nasty) parsing of the type-name
    while(true)
    {
        // Check the outermost type to make sure it is a vector
        bool is_vector = starts_with(type_name, "vector<")
                         && ends_with(type_name, ">");

        // This is not a vector
        if(!is_vector)
        {
            break;
        }

        // Increment rank
        result.depth++;

        // Parse off outer vector
        replace_first(type_name, "vector<", "");
        replace_last(type_name, ">", "");
        trim(type_name);
    }

    // If the vector depth is 0, then there were no vectors, and this is a case
    // that we were not designed to handle, so return an invalid conversion
    if(result.depth == 0)
    {
        return result;
    }

    // Otherwise, we are now at the core type of the vector structure, so check
    // if it is a supportable HDF5 scalar type
    result.scalar_hdf5_type = root_type_name_to_scalar_hdf5_type(type_name);

    // If we have successfully converted the scalar type, then the result is
    // good to go.  If not, leave it as invalid.
    if(result.scalar_hdf5_type != -1)
    {
        result.valid = true;
    }

    return result;
}


bool vector_converter::can_handle(TLeaf *leaf)
{
    return root_type_name_to_vector_hdf5_type(leaf->GetTypeName()).valid;
}


string vector_converter::member_for_conversion_struct(TLeaf *leaf)
{
    // HACK: ROOT cannot easily include the HDF5 headers because CINT is a piece
    // of junk.  Anyway, it seems the easiest way to add the data structures we
    // need to ROOT is just to typedef them ourselves.  Of course, if HDF5
    // changes the implementation of this struct, we are borked hard, but we can
    // do some version checking in that case.  It isn't elegant, but it'll work.
    static bool root_informed_of_hvl_t = false;
    if(!root_informed_of_hvl_t)
    {
        gROOT->ProcessLine("typedef struct{size_t len;void *p;}hvl_t;");
        root_informed_of_hvl_t = true;
    }

    return string("hvl_t ") + leaf->GetName() + ";";
}


hid_t vector_converter::hdf5_type_for_leaf(
    TLeaf * leaf, 
    vector<hdf5_type_deallocator> & deallocators
)
{
    // Generate a conversion
    root_vector_conversion conversion
        = root_type_name_to_vector_hdf5_type(leaf->GetTypeName());

    // Loop over the nested vectors, creating an HDF5 type
    hid_t inner_type = conversion.scalar_hdf5_type;
    hid_t outer_type = -1;
    for(unsigned i = 0; i < conversion.depth; i++)
    {
        // Wrap the inner type
        outer_type = H5Tvlen_create(inner_type);

        // Generate a deallocator for the outer type
        deallocators.push_back([=]() -> bool {
            if(H5Tclose(outer_type) < 0)
            {
                if(verbose)
                {
                    cerr << "ERROR: Unable to close variable length type for "
                         << "leaf \"" << leaf->GetName() << "\" at depth " << i
                         << endl;
                }

                return false;
            }

            return true;
        });

        // Set the new inner_type
        inner_type = outer_type;
    }

    return outer_type;
}


bool vector_converter::map_leaf_and_build_converter(
    TLeaf *leaf,
    void *address,
    vector<root_converter> & converters,
    vector<root_resource_deallocator> & deallocators
)
{
    // Generate a conversion
    root_vector_conversion conversion
        = root_type_name_to_vector_hdf5_type(leaf->GetTypeName());

    // Have ROOT generate a dictionary for the branch type
    // gInterpreter->GenerateDictionary(leaf->GetTypeName(), "vector");
    process_long_line(
        string("#include <vector>\n")
        + "#ifdef __CINT__\n"
        + "#pragma link C++ class " + leaf->GetTypeName()+ "+;\n"
        + "#endif",
        true
    );

    // Allocate a buffer that we can use to load the vector into and add a
    // deallocator for it
    void *buffer = allocate_instance_by_name(leaf->GetTypeName());
    deallocators.push_back([=]() -> bool {
        deallocate_instance_by_name_and_location(leaf->GetTypeName(),
                                                 buffer);
        return true;
    });

    // Set the leaf address to the intermediate buffer
    leaf->SetAddress(buffer);

    // Build a converter
    converters.push_back([=]() -> bool {
        // Convert the struct address to a hvl_t
        hvl_t *vl_member = (hvl_t *)address;

        // TODO: Implement
        vl_member->len = 0;

        return true;
    });

    return true;
}
