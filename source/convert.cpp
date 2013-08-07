#include "convert.h"

// C Standard includes
#include <cstring>

// Standard includes
#include <iostream>

// ROOT includes
#include <TClass.h>
#include <TKey.h>
#include <TTree.h>

// root2hdf5 includes
#include "options.h"
#include "tree.h"


// Standard namespaces
using namespace std;

// root2hdf5 namespaces
using namespace root2hdf5::options;
using namespace root2hdf5::tree;


bool root2hdf5::convert::convert(TDirectory *directory,
                                 hid_t parent_destination)
{
    // Generate a list of keys in the directory
    TIter next_key(directory->GetListOfKeys());

    // Go through the keys, handling them by their type
    TKey *key = NULL;
    TKey *previous = NULL;
    while((key = (TKey *)next_key()))
    {
        // ROOT uses this stupid concept called a "cycle number" to save
        // multiple header-like objects for the same object.  For example, a
        // TTree named "bob" might appear as "bob;1" and "bob;2".  Both point to
        // the same object, but "bob;2" is the later, complete revision.  This
        // happens in the TTree case because TTree's auto-save to their file
        // every 100 MB.  Everytime an autosave happens, the previous autosave
        // cycle numbers are removed, but when TFile::Write is called, the last
        // autosave cycle number remains (who knows why...) and a new biggest
        // cycle number is created.  Thus, we only convert the highest cycle
        // number for each key (TIter will go from highest cycle number to
        // lowest number, so if we've already processed a key with this name, we
        // can skip any duplicately named ones).
        // TODO: This may not be the correct procedure for every data-type, so
        // we may need to reevaluate this filtering procedure in the future.
        if(previous != NULL && strcmp(key->GetName(), previous->GetName()) == 0)
        {
            continue;
        }

        // Record the previously processed key
        previous = key;

        // Grab the object and its type
        TObject *object = key->ReadObj();
        TClass *object_type = object->IsA();

        // Print information if requested
        if(verbose)
        {
            cout << "Processing "
                 << key->GetName()
                 << ";"
                 << key->GetCycle()
                 << endl;
        }

        // Switch based on type
        if(object_type->InheritsFrom(TDirectory::Class()))
        {
            // This is a ROOT directory, so first create a corresponding HDF5
            // group and then recurse into it
            hid_t new_group = H5Gcreate2(parent_destination,
                                         key->GetName(),
                                         H5P_DEFAULT,
                                         H5P_DEFAULT,
                                         H5P_DEFAULT);
            if(!convert((TDirectory *)key->ReadObj(), new_group))
            {
                return false;
            }
            
            if(H5Gclose(new_group) < 0)
            {
                if(verbose)
                {
                    cerr << "ERROR: Closing group \"" 
                         << key->GetName() 
                         << "\" failed";
                }
                return false;
            }
        }
        else if(object_type->InheritsFrom(TTree::Class()))
        {
            // This is a ROOT tree, so we need to create a new HDF5 dataset with
            // custom type matching the TTree branches, and then copy all the
            // data into it
            root2hdf5::tree::convert((TTree *)object, parent_destination);
        }
        else
        {
            // This type is currently unhandled
            if(verbose)
            {
                cerr << "WARNING: Unhandled object type \""
                     << object_type->GetName() 
                     << "\" - skipping"
                     << endl;
            }
        }
    }

    // All done
    return true;
}
