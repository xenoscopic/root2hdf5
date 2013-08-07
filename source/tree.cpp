#include "tree.h"

// Standard includes
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <sstream>
#include <utility>
#include <functional>
#include <vector>
#include <tuple>

// Boost includes
// HACK: Need to define this macro to tell Boost not to use deprecated
// Boost.System constructs which result in unused-variable errors.
#ifndef BOOST_SYSTEM_NO_DEPRECATED
#define BOOST_SYSTEM_NO_DEPRECATED 1
#endif
#include <boost/filesystem.hpp>

// ROOT includes
#include <TROOT.h>
#include <TBranch.h>
#include <TLeaf.h>

// root2hdf5 includes
#include "options.h"
#include "type.h"
#include "tree/structure.h"


// Standard namespaces
using namespace std;

// Boost namespace aliases
namespace fs = boost::filesystem;

// root2hdf5 namespaces
using namespace root2hdf5::tree;
using namespace root2hdf5::options;
using namespace root2hdf5::type;
using namespace root2hdf5::tree::structure;


// Private function and mapping declarations
namespace root2hdf5
{
    namespace tree
    {
        // This method provides a convenient interface for executing code via
        // CINT which might be too long for the TROOT::ProcessLine method (which
        // has a limit of 2043 characters).  The method creates a temporary file
        // in which to write the code, has CINT load the file from disk, and
        // then cleans up the temporary file.  This method returns true on
        // success and false on failure.
        bool process_long_line(const string & long_line);

        // This method returns the offset of the member in a struct using CINT.
        // E.g. with the struct
        //
        //      struct TestStruct
        //      {
        //          int a;
        //          int b;
        //          struct {
        //              float d;
        //              double e;
        //          } c;
        //      };
        //
        // One could do:
        //
        //      offsetof_member_in_type_by_name("TestStruct", "c")
        //
        // and receive 8 (or whatever the platform-dependent value would be).
        size_t offsetof_member_in_type_by_name(string type_name,
                                               string member_name);

        // This method returns the size of the member of a struct using CINT.
        // E.g. with the struct
        //
        //      struct TestStruct
        //      {
        //          int a;
        //          int b;
        //          struct {
        //              float d;
        //              double e;
        //          } c;
        //      };
        //
        // One could do:
        //
        //      offsetof_member_in_type_by_name("TestStruct", "c");
        //
        // and receive 16 (or whatever the platform/padding-dependent value
        // would be).
        size_t sizeof_member_in_type_by_name(string type_name,
                                             string member_name);

        // Type for handling conversion/resource-deallocation callbacks
        typedef std::function<bool()> callback;

        // This method calls TLeaf::SetAddress for the specified leaf, either
        // mapping the leaf directly into the HDF5 struct, or allocating an
        // intermediate conversion buffer/function for more complex types.  It
        // also builds up the HF5 compound type which will be used to create the
        // HDF5 dataset.  Any conversion or deallocation callbacks will be
        // registered in the provided containers.  Returns true on success,
        // false on failure.
        bool set_leaf_buffer_build_converter_and_build_hdf5_type(
            TLeaf *leaf, // The leaf to map
            string prefix, // The prefix which must be included in front of the
                           // leaf name so that the resulting name is relative
                           // to the root of the HDF5 struct.  E.g. if this leaf
                           // is located as follows:
                           //
                           // (root)/
                           //       branch_1/
                           //           subbranch_1/
                           //               leaf_1
                           // 
                           // The value of prefix would be
                           // "branch1.subbranch_1" and the function will
                           // include that followed by a period and the leaf
                           // name for doing any offsetof or sizeof calls
                           // relative to the root HDF5 struct type.  This value
                           // is also used to calculate offsets relative to the
                           // parent compound type in the master HDF5 struct.
            string hdf5_struct_name, // The name of the root HDF5 struct type
            hid_t parent_compound_type, // The compound type for the parent
                                        // branch
            void *hdf5_struct, // The allocated instance of the root HDF5 struct
            vector<callback> & conversions, // Conversion callback registration
                                            // container for conversion
                                            // functions which need to be called
                                            // whenever a new TTree entry is
                                            // loaded
            vector<callback> & deallocators // Deallocator callback registration
                                            // container for deallocator
                                            // functions which need to be called
                                            // once after the TTree has been
                                            // completely converted
        );

        // This method is the TBranch equivalent of the
        // set_leaf_buffer_build_converter_and_build_hdf5_type method.  If the
        // branch has a complex type (e.g. multiple leaves and/or subbranches),
        // it will create a sub-compound HDF5 type.  It calls itself and
        // set_leaf_buffer_build_converter_and_build_hdf5_type recursively for
        // the provided branch.  Arguments are as for
        // set_leaf_buffer_build_converter_and_build_hdf5_type.
        bool set_branch_buffer_build_converter_and_build_hdf5_type(
            TBranch *branch,
            string prefix,
            string hdf5_struct_name,
            hid_t parent_compound_type,
            void *hdf5_struct,
            vector<callback> & conversions,
            vector<callback> & deallocators
        );

        // This method is the TTree equivalent of the
        // set_leaf_buffer_build_converter_and_build_hdf5_type and
        // set_branch_buffer_build_converter_and_build_hdf5_type methods.  It
        // calls these methods recursively to map and build converters for the
        // entire tree.  It returns a tuple of the form:
        //      (success, conversion_callback, deallocation_callback)
        tuple<bool, callback, callback>
        set_tree_buffers_build_converters_and_build_hdf5_type(
            TTree *tree,
            string hdf5_struct_name,
            hid_t hdf5_compound_type,
            void *hdf5_struct
        );
    }
}


bool root2hdf5::tree::process_long_line(const string & long_line)
{
    // TODO: It might be better to add some more stringent error checking in
    // here, but most of these things are such robust, low-level OS things that
    // if they fail, the user's environment is probably already melting around
    // them, so just assume the temporary file creation/writing/deletion will
    // work.  However, still check LoadMacro's return code, because it is easy
    // and because we can't trust ROOT.

    // Generate a temporary path to write the line to
    fs::path temp_path = fs::temp_directory_path() 
                         / fs::unique_path("%%%%-%%%%-%%%%-%%%%.cpp");

    // Write the line to the temporary file.  It is very important that we
    // include a newline at the end or ROOT's crappy interpreter will give an
    // "unexpected EOF" error sporadically.
    ofstream output(temp_path.c_str(), ofstream::out);
    output << long_line << endl;
    output.close();

    // Process the line
    if(gROOT->LoadMacro((temp_path.native()).c_str()) < 0)
    {
        if(verbose)
        {
            cerr << "ERROR: Unable to load temporary macro" << endl;
        }

        return false;
    }

    // Remove the file
    fs::remove(temp_path);
    
    return true;
}


size_t root2hdf5::tree::offsetof_member_in_type_by_name(string type_name,
                                                        string member_name)
{
    return (size_t)gROOT->ProcessLine(
        (string("offsetof(") + type_name + "," + member_name + ");").c_str()
    );
}

size_t root2hdf5::tree::sizeof_member_in_type_by_name(string type_name,
                                                      string member_name)
{
    return (size_t)gROOT->ProcessLine(
        (string("sizeof(((") 
         + type_name 
         + "*)0)->" 
         + member_name 
         + ");").c_str()
    );
}


bool root2hdf5::tree::set_leaf_buffer_build_converter_and_build_hdf5_type(
    TLeaf *leaf,
    string prefix,
    string hdf5_struct_name,
    hid_t parent_compound_type,
    void *hdf5_struct,
    vector<callback> & conversions,
    vector<callback> & deallocators
)
{
    // TODO: Remove
    if(root_type_name_to_scalar_hdf5_type(leaf->GetTypeName()) == -1)
    {
        // Ignore non-scalar types for now
        return true;
    }

    // Calculate the name of the member representing the leaf relative to the
    // the main struct root
    string leaf_path(leaf->GetName());
    if(prefix != "")
    {
        leaf_path = prefix + "." + leaf_path;
    }

    // Calculate the offset of the parent compound type and our own type, and
    // use the relative difference as the offset for insertion in the parent
    // compound type.
    size_t parent_offset = prefix == "" ? 0
                                        : offsetof_member_in_type_by_name(
                                              hdf5_struct_name,
                                              prefix
                                          );
    size_t self_offset = offsetof_member_in_type_by_name(hdf5_struct_name,
                                                         leaf_path);
    size_t relative_offset = self_offset - parent_offset;

    // Grab the type name
    string type_name(leaf->GetTypeName());

    // Check if the leaf is a scalar type
    hid_t scalar_hdf5_type 
        = root_type_name_to_scalar_hdf5_type(type_name);
    if(scalar_hdf5_type != -1)
    {
        // This is a scalar type, so just insert the standard HDF5 type
        if(H5Tinsert(parent_compound_type,
                     leaf->GetName(),
                     relative_offset,
                     scalar_hdf5_type) < 0)
        {
            if(verbose)
            {
                cerr << "ERROR: Unable to insert scalar type for leaf \""
                     << leaf->GetName() << "\" into parent compound type"
                     << endl;
            }

            return false;
        }

        // Set the leaf mapping
        leaf->SetAddress((void *)(((char *)hdf5_struct) + self_offset));
        
        return true;
    }

    // Check if the leaf is a vector type
    root_vector_conversion vector_conversion
        = root_type_name_to_vector_hdf5_type(type_name);
    if(vector_conversion.valid)
    {
        // TODO: Insert the vector type and build converter
        return true;
    }

    // Otherwise this is an unsupported-but-ignorable type, so don't error, and
    // assume the struct-generation method already printed a warning about it
    // being skipped
    return true;
}


bool root2hdf5::tree::set_branch_buffer_build_converter_and_build_hdf5_type(
    TBranch *branch,
    string prefix,
    string hdf5_struct_name,
    hid_t parent_compound_type,
    void *hdf5_struct,
    vector<callback> & conversions,
    vector<callback> & deallocators
)
{
    // First, check if this is just a scalar branch (i.e. single-leaf with no
    // subbranches).  If that is the case, then we just return a single member
    // for the parent struct.
    if(branch->GetListOfLeaves()->GetEntries() == 1 
       && branch->GetListOfBranches()->GetEntries() == 0)
    {
        // This is a single scalar branch, so insert it directly into the parent
        // compound type
        return set_leaf_buffer_build_converter_and_build_hdf5_type(
            branch->GetLeaf(branch->GetName()),
            prefix,
            hdf5_struct_name,
            parent_compound_type,
            hdf5_struct,
            conversions,
            deallocators
        );
    }
    else
    {
        // Otherwise, this is a more complex branch, possibly containing
        // multiple leaves and/or subbranches.

        // Calculate the name of the struct representing the branch relative to
        // the main struct root
        string branch_path(branch->GetName());
        if(prefix != "")
        {
            branch_path = prefix + "." + branch_path;
        }

        // Calculate the size of the member and generate a new HDF5 compound
        // datatype representing the branch
        size_t branch_type_size = sizeof_member_in_type_by_name(
            hdf5_struct_name,
            branch_path
        );
        hid_t compound_type = H5Tcreate(H5T_COMPOUND, branch_type_size);
        
        // First add our leaves
        TIter next_leaf(branch->GetListOfLeaves());
        TLeaf *leaf = NULL;
        while((leaf = (TLeaf *)next_leaf()))
        {
            if(!set_leaf_buffer_build_converter_and_build_hdf5_type(
                leaf,
                branch_path,
                hdf5_struct_name,
                compound_type,
                hdf5_struct,
                conversions,
                deallocators
            ))
            {
                // The function failed, it should have printed an error message
                // if necessary, so just return
                return false;
            }
        }

        // Then recurse into subbranches
        TIter next_subbranch(branch->GetListOfBranches());
        TBranch *subbranch = NULL;
        while((subbranch = (TBranch *)next_subbranch()))
        {
            if(!set_branch_buffer_build_converter_and_build_hdf5_type(
                subbranch,
                branch_path,
                hdf5_struct_name,
                compound_type,
                hdf5_struct,
                conversions,
                deallocators
            ))
            {
                // The function failed, it should have printed an error message
                // if necessary, so just return
                return false;
            }
        }

        // Insert the compount type into the parent type
        if(H5Tinsert(parent_compound_type,
                     branch->GetName(),
                     offsetof_member_in_type_by_name(hdf5_struct_name,
                                                     branch_path),
                     compound_type) < 0)
        {
            if(verbose)
            {
                cerr << "ERROR: Unable to insert compound type for branch \""
                     << branch->GetName() << "\" into parent compound type"
                     << endl;
            }

            return false;
        }

        // Create a cleanup callback to deallocate our compound type
        deallocators.push_back([=]()->bool{
            if(H5Tclose(compound_type) < 0)
            {
                if(verbose)
                {
                    cerr << "ERROR: Unable to close compound type for branch \""
                     << branch->GetName() << "\"" << endl;
                }
                
                return false;
            }

            return true;
        });

        return true;
    }
}


tuple<bool, callback, callback>
root2hdf5::tree::set_tree_buffers_build_converters_and_build_hdf5_type(
    TTree *tree,
    string hdf5_struct_name,
    hid_t hdf5_compound_type,
    void *hdf5_struct
)
{
    // Create callback containers
    vector<callback> converters;
    vector<callback> deallocators;

    // Loop over the branches of the tree and do their mapping
    TIter next_branch(tree->GetListOfBranches());
    TBranch *branch = NULL;
    while((branch = (TBranch *)next_branch()))
    {
        // Do mappings
        if(!set_branch_buffer_build_converter_and_build_hdf5_type(
            branch,
            "",
            hdf5_struct_name,
            hdf5_compound_type,
            hdf5_struct,
            converters,
            deallocators
        ))
        {
            // If things fail, the insertion method should have printed an error
            // if necessary, so just return
            // TODO: Should we actually return the existing deallocators at this
            // point in case the code wants to try to continue on with
            // conversion?
            return make_tuple(false,
                              []()->bool{return false;},
                              []()->bool{return false;});
        }
    }

    // All done, create combination callbacks and return
    return make_tuple(
        true,
        [=]()->bool
        {
            // Loop over all branch converters and call them
            for(auto it = converters.begin();
                it != converters.end();
                it++)
            {
                if(!(*it)())
                {
                    // If a convert fails, return false
                    return false;
                }
            }

            // If every converter succeeded, return true
            return true;
        },
        [=]()->bool
        {
            // Loop over all branch converters and call them
            for(auto it = deallocators.begin();
                it != deallocators.end();
                it++)
            {
                if(!(*it)())
                {
                    // If a deallocator fails, return false
                    return false;
                }
            }

            // If every deallocator succeeded, return true
            return true;
        }
    );
}


bool root2hdf5::tree::convert(TTree *tree,
                              hid_t parent_destination)
{
    // Create a string which represents a struct that we can use to construct
    // the HDF5 composite data type
    string hdf5_struct_name = struct_type_name_for_tree(tree);
    string hdf5_struct_code = struct_code_for_tree(tree);
    if(hdf5_struct_code == "")
    {
        // The struct code generation failed (and it should have printed an
        // error), so just return
        return false;
    }
    cout << hdf5_struct_code << endl;

    // Tell CINT about the structure
    if(!process_long_line(hdf5_struct_code))
    {
        if(verbose)
        {
            cerr << "ERROR: Unable to generate temporary struct for converting "
                 << "tree \"" << tree->GetName() << "\"";
        }

        return false;
    }

    // TODO: Perhaps find some way to check the code being executed here,
    // although it is probably not essential since ROOT will probably crash
    // internally before returning anything.

    // Create a stringstream to build up our commands
    stringstream command;

    // Calculate the size of the structure
    command.str("");
    command << "sizeof(" << hdf5_struct_name << ");";
    size_t hdf5_struct_size = (size_t)gROOT->ProcessLine(
        command.str().c_str()
    );
    
    // Tell CINT to allocate an instance of the structure
    command.str("");
    command << "(void *)(new " << hdf5_struct_name << ");";
    void *hdf5_struct = (void *)gROOT->ProcessLine(command.str().c_str());

    // Create the (initially-empty) HDF5 compound datatype to use for the tree
    hid_t hdf5_type = H5Tcreate(H5T_COMPOUND, hdf5_struct_size);

    // Build the datatype/converter/deallocator
    auto branch_map_success_converter_deallocator
        = set_tree_buffers_build_converters_and_build_hdf5_type(
              tree,
              hdf5_struct_name,
              hdf5_type,
              hdf5_struct
          );
    if(!get<0>(branch_map_success_converter_deallocator))
    {
        // Branch mapping failed and should have printed an error, just return
        return false;
    }
    callback converter = get<1>(branch_map_success_converter_deallocator);
    callback deallocator = get<2>(branch_map_success_converter_deallocator);

    // Create the dataspace with the same dimensions as the tree and a
    // single-element dataset to represent the in-memory space
    const hsize_t n_entries = tree->GetEntries();
    const hsize_t n_memory_entries = 1; // We only have a single instance of the
                                        // struct in-memory
    hid_t hdf5_file_space = H5Screate_simple(1, &n_entries, NULL);
    hid_t hdf5_memory_space = H5Screate_simple(1, &n_memory_entries, NULL);

    // Create the dataset
    hid_t hdf5_dataset = H5Dcreate2(parent_destination,
                                    tree->GetName(),
                                    hdf5_type,
                                    hdf5_file_space,
                                    H5P_DEFAULT,
                                    H5P_DEFAULT,
                                    H5P_DEFAULT);

    // Loop through the tree, getting every entry, calling the converter, and
    // then writing it to the HDF5 dataset
    const hsize_t slab_stride = 1;
    const hsize_t slab_count = 1;
    for(hsize_t i = 0; i < n_entries; i++)
    {
        // Load the entry
        if(tree->GetEntry((Long64_t)i) < 1)
        {
            if(verbose)
            {
                cerr << "ERROR: Unable to read entry " << i << " from tree"
                     << endl;
            }

            return false;
        }

        // Call the converter
        if(!converter())
        {
            // If the converter failed, it should have printed a message if
            // necessary, so just return
            return false;
        }

        // Do the HDF5 fill.  First, select the single event hyperslab to fill,
        // and then write!
        if(H5Sselect_hyperslab(hdf5_file_space,
                               H5S_SELECT_SET,
                               &i,
                               &slab_stride,
                               &slab_count,
                               NULL) < 0)
        {
            if(verbose)
            {
                cerr << "ERROR: Unable to select hyperslab for entry " << i
                     << endl;
            }

            return false;
        }
        if(H5Dwrite(hdf5_dataset,
                    hdf5_type,
                    hdf5_memory_space,
                    hdf5_file_space,
                    H5P_DEFAULT,
                    hdf5_struct) < 0)
        {
            if(verbose)
            {
                cerr << "ERROR: Unable to write hyperslab fro entry " << i
                     << endl;
            }

            return false;
        }

    }

    // Close the data set
    if(H5Dclose(hdf5_dataset) < 0)
    {
        if(verbose)
        {
            cerr << "ERROR: Couldn't close HDF5 dataset for tree \""
                 << tree->GetName() << "\"" << endl;
        }

        return false;
    }

    // Call the deallocator
    if(!deallocator())
    {
        // The deallocator should have already printed an error if necessary, so
        // just return
        return false;
    }

    // Close out the HDF5 memory data space
    if(H5Sclose(hdf5_memory_space) < 0)
    {
        if(verbose)
        {
            cerr << "ERROR: Couldn't close HDF5 memory data space for tree \""
                 << tree->GetName() << "\"" << endl;
        }

        return false;
    }
    
    // Close out the HDF5 file data space
    if(H5Sclose(hdf5_file_space) < 0)
    {
        if(verbose)
        {
            cerr << "ERROR: Couldn't close HDF5 file data space for tree \""
                 << tree->GetName() << "\"" << endl;
        }

        return false;
    }

    // Close out the HDF5 data type
    if(H5Tclose(hdf5_type) < 0)
    {
        if(verbose)
        {
            cerr << "ERROR: Couldn't close HDF5 data type for tree \""
                 << tree->GetName() << "\"" << endl;
        }

        return false;
    }

    // Tell CINT to deallocate the instance of the structure
    command.str("");
    command << "delete ((" << hdf5_struct_name << " *)" << hdf5_struct << ");";
    gROOT->ProcessLine(command.str().c_str());
    hdf5_struct = NULL;

    // All done
    return true;
}
