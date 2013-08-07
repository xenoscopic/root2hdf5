#include "tree/walk.h"

// ROOT includes
#include <TObjArray.h>


// Standard namespaces
using namespace std;

// root2hdf5 namespaces
using namespace root2hdf5::tree::walk;


// Private namespace members
namespace root2hdf5
{
    namespace tree
    {
        namespace walk
        {
            // This method is used internall to implement the recursive walking
            bool walk_branch(TBranch *branch,
                             branch_processor branch_opener,
                             branch_processor branch_closer,
                             leaf_processor leaf_handler);
        }
    }
}


bool root2hdf5::tree::walk::walk_branch(TBranch *branch,
                                        branch_processor branch_opener,
                                        branch_processor branch_closer,
                                        leaf_processor leaf_handler)
{
    // Grab the list of leaves and subbranches for the branch
    TObjArray *leaves = branch->GetListOfLeaves();
    TObjArray *subbranches = branch->GetListOfBranches();

    // Check if this is a single-leaf/no-subbranch branch, and if so, just call
    // the leaf handler on it
    if(leaves->GetEntries() == 1 
       && subbranches->GetEntries() == 0)
    {
        return leaf_handler((TLeaf *)(leaves->At(0)));
    }

    // Otherwise, we need to call the branch opener and iterate over all branch
    // members
    if(!branch_opener(branch))
    {
        return false;
    }

    // Iterate over leaves
    TIter next_leaf(leaves);
    TLeaf *leaf = NULL;
    while((leaf = (TLeaf *)next_leaf()))
    {
        if(!leaf_handler(leaf))
        {
            return false;
        }
    }

    // Iterate over subbranches, recursing
    TIter next_subbranch(subbranches);
    TBranch *subbranch = NULL;
    while((subbranch = (TBranch *)next_subbranch()))
    {
        if(!walk_branch(subbranch, branch_opener, branch_closer, leaf_handler))
        {
            return false;
        }
    }

    // Call the branch closer
    return branch_closer(branch);
}


bool root2hdf5::tree::walk::walk_tree(TTree *tree,
                                      branch_processor branch_opener,
                                      branch_processor branch_closer,
                                      leaf_processor leaf_handler)
{
    // Loop over each branch at the root level of the tree and do its walking
    TIter next_branch(tree->GetListOfBranches());
    TBranch *branch = NULL;
    while((branch = (TBranch *)next_branch()))
    {
        if(!walk_branch(branch, branch_opener, branch_closer, leaf_handler))
        {
            return false;
        }
    }

    return true;
}
