#include "tree/structure.h"

// Standard includes
#include <sstream>

// ROOT includes
#include <TBranch.h>
#include <TLeaf.h>
#include <TROOT.h>

// root2hdf5 includes
#include "options.h"
#include "cint.h"
#include "tree/walk.h"
#include "tree/leaf_converters.h"


// Standard namespaces
using namespace std;

// root2hdf5 namespaces
using namespace root2hdf5::tree::structure;
using namespace root2hdf5::options;
using namespace root2hdf5::cint;
using namespace root2hdf5::tree::walk;
using namespace root2hdf5::tree::leaf_converters;


string root2hdf5::tree::structure::struct_type_name_for_tree(TTree *tree)
{
    // Create a stringstream to build the result
    stringstream result;

    // Create a unique name
    result << "tree_" << (long int)tree;

    return result.str();
}


bool root2hdf5::tree::structure::create_struct_code_for_tree(TTree *tree,
                                                             string *code)
{
    // Create a stringstream to build the structure
    stringstream structure;

    // Create the outer structure
    structure << "struct " << struct_type_name_for_tree(tree) << "{";

    // Walk the tree and add submembers
    bool success = walk_tree(
        tree,
        // Branch open
        [&structure](TBranch *branch) -> bool {
            // Hide unused variable warning
            (void)branch;
            structure << "struct{";
            return true;
        },
        // Leaf process
        [&structure](TLeaf *leaf) -> bool {
            // Find a leaf converter
            leaf_converter *converter = find_converter(leaf);

            // If we couldn't find one, just ignore the leaf, but warn about
            // skipping it if necessary.
            // TODO: We warn about skipping leaves here because struct
            // generation happens to be the first step we do, and consequently
            // we don't warn in the HDF5 type generation and TTree address
            // mapping steps, but I wonder if we should do a new 0th pass where
            // we just go through and look for unsupported leaves.  This will
            // also be necessary in the case of TBranches which are non-scalar
            // but have no supported submembers, which will, at the moment,
            // fail due to empty generated structs.
            if(converter == NULL)
            {
                if(verbose)
                {
                    cerr << "WARNING: Leaf \"" << leaf->GetName() << "\" has "
                         << "an unknown type \"" << leaf->GetTypeName()
                         << "\" - skipping" << endl;
                }

                return true;
            }

            // Add it's structure member
            structure << converter->member_for_conversion_struct(leaf);

            return true;
        },
        // Branch close
        [&structure](TBranch *branch) -> bool {
            structure << "}" << branch->GetName() << ";";
            return true;
        }
    );

    // If things failed during walking, return a bogus structure
    if(!success)
    {
        if(verbose)
        {
            cerr << "ERROR: Unable to generate temporary struct for converting "
             << "tree \"" << tree->GetName() << "\"" << endl;
        }

        return false;
    }

    // Close the outer structure
    structure << "};";

    // If the user wants a copy of the code, give it to them, regardless of
    // whether or not it compiles
    if(code != NULL)
    {
        *code = structure.str();
    }

    // Inform CINT about the structure
    bool result = process_long_line(structure.str());
    if(!result && verbose)
    {
        cerr << "ERROR: Unable to compile temporary struct for converting tree "
             << "\"" << tree->GetName() << "\"";
    }

    return result;
}


void * root2hdf5::tree::structure::allocate_instance_by_name(string type_name)
{
    return (void *)gROOT->ProcessLine(
        (string("(void *)(new ") + type_name + ");").c_str()
    );
}


void root2hdf5::tree::structure::deallocate_instance_by_name_and_location(
    string type_name,
    void *location
)
{
    // Create a stringstream to build up the command
    stringstream command;
    command << "delete ((" << type_name << " *)" << location << ");";

    // Execute it
    gROOT->ProcessLine(
        command.str().c_str()
    );
}


size_t root2hdf5::tree::structure::offsetof_member_in_type_by_name(
    string type_name,
    string member_name
)
{
    return (size_t)gROOT->ProcessLine(
        (string("offsetof(") + type_name + "," + member_name + ");").c_str()
    );
}


size_t root2hdf5::tree::structure::sizeof_member_in_type_by_name(
    string type_name,
    string member_name
)
{
    // HACK: This may look like a null-pointer deference, but no dereference
    // ever actually happens, and this is the recommended way of doing this.
    return (size_t)gROOT->ProcessLine(
        (string("sizeof(((") 
         + type_name 
         + "*)0)->" 
         + member_name 
         + ");").c_str()
    );
}


size_t root2hdf5::tree::structure::sizeof_type_by_name(string type_name)
{
    return (size_t)gROOT->ProcessLine(
        (string("sizeof(") + type_name + ");").c_str()
    );
}

