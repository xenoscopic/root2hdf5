#include "tree.h"

// Standard includes
#include <iostream>
#include <string>
#include <utility>

// HACK: Use Boost.Tuple instead of std::tuple because at the moment, the LLVM-
// provided libc++ doesn't support the std::tuple, and Boost.Tuple is
// effectively the same thing.
// Boost includes
#include <boost/tuple/tuple.hpp>

// ROOT includes
#include <TROOT.h>
#include <TBranch.h>
#include <TLeaf.h>

// root2hdf5 includes
#include "options.h"
#include "tree/structure.h"
#include "tree/map_hdf5.h"
#include "tree/map_root.h"


// Standard namespaces
using namespace std;

// root2hdf5 namespaces
using namespace root2hdf5::tree;
using namespace root2hdf5::options;
using namespace root2hdf5::tree::structure;
using namespace root2hdf5::tree::map_hdf5;
using namespace root2hdf5::tree::map_root;


bool root2hdf5::tree::convert(TTree *tree,
                              hid_t parent_destination)
{
    // Create a string which represents a struct that we can use to construct
    // the HDF5 composite data type
    string hdf5_struct_name = struct_type_name_for_tree(tree);
    if(!create_struct_code_for_tree(tree))
    {
        // The struct code generation failed, and it should have printed an
        // error if necessary, so just bail
        return false;
    }

    // Create an HDF5 type which can be used to create our dataset
    hid_t hdf5_type = -1;
    hdf5_type_deallocator hdf5_deallocator;
    boost::tie(hdf5_type, hdf5_deallocator)
        = hdf5_type_for_tree(tree);
    if(hdf5_type == -1)
    {
        // HDF5 type generation has failed, and it should have already printed a
        // message if necessary, so just bail
        return false;
    }
    
    // Tell CINT to allocate an instance of the structure
    void *hdf5_struct = allocate_instance_by_name(hdf5_struct_name);

    // Map the ROOT tree into the hdf5_struct instance
    bool root_map_success = false;
    root_converter converter;
    root_resource_deallocator root_deallocator;
    boost::tie(root_map_success, converter, root_deallocator)
        = map_root_tree_into_struct_and_build_converter(tree, hdf5_struct);
    if(!root_map_success)
    {
        // ROOT mapping has failed, and it should have already printed a message
        // if necessary, so just bail
        return false;
    }

    // Create the dataspace with the same dimensions as the tree and a
    // single-element dataset to represent the in-memory space
    const hsize_t n_entries = tree->GetEntries();
    const hsize_t n_memory_entries = 1; // We only have a single instance of the
                                        // struct in-memory
    hid_t hdf5_file_space = H5Screate_simple(1, &n_entries, NULL);
    hid_t hdf5_memory_space = H5Screate_simple(1, &n_memory_entries, NULL);
    if(hdf5_file_space < 0 || hdf5_memory_space < 0)
    {
        // Data space creation failed
        if(verbose)
        {
            cerr << "ERROR: Unable to create HDF5 data spaces for tree \""
                 << tree->GetName() << "\"" << endl;
        }

        return false;
    }

    // Create the dataset
    hid_t hdf5_dataset = H5Dcreate2(parent_destination,
                                    tree->GetName(),
                                    hdf5_type,
                                    hdf5_file_space,
                                    H5P_DEFAULT,
                                    H5P_DEFAULT,
                                    H5P_DEFAULT);
    if(hdf5_dataset < 0)
    {
        // Dataset creation failed
        if(verbose)
        {
            cerr << "ERROR: Unable to create HDF5 dataset for tree \""
                 << tree->GetName() << "\"" << endl;
        }

        return false;
    }

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

    // Call the root mapping deallocator
    if(!root_deallocator())
    {
        // The deallocator should have already printed an error if necessary, so
        // just bail
        return false;
    }

    // Tell CINT to deallocate the instance of the structure
    deallocate_instance_by_name_and_location(hdf5_struct_name, hdf5_struct);
    hdf5_struct = NULL;

    // Call the HDF5 type deallocator
    if(!hdf5_deallocator())
    {
        // The deallocator should have already printed an error if necessary, so
        // just bail
        return false;
    }

    // All done
    return true;
}
