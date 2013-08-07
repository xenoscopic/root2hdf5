#pragma once

// Standard includes
#include <functional>

// ROOT includes
#include <TTree.h>
#include <TBranch.h>
#include <TLeaf.h>


namespace root2hdf5
{
    namespace tree
    {
        namespace walk
        {
            // Types for defining callbacks to be used in TTree walking
            typedef std::function<bool(TBranch *)> branch_processor;
            typedef std::function<bool(TLeaf *)> leaf_processor;

            // This function implements the primary walking logic for TTree
            // conversion.  It does depth-first iteration of a TTree and its
            // branches and leaves.  If it hits a branch with only a single leaf
            // and no subbranches (which is the case for any scalar branch with
            // no subbranches), it will not call the branch opener/closer, but
            // rather just call the leaf handler.  If it hits a complex branch
            // with multiple leaves and/or at least one subbranch, it will first
            // call the branch opener, then call the leaf handler for each leaf,
            // and then recurse into each subbranch, finally calling the branch
            // closer on the branch after all leaves and subbranches have been
            // processed.
            bool walk_tree(TTree *tree,
                           branch_processor branch_opener,
                           branch_processor branch_closer,
                           leaf_processor leaf_handler);
        }
    }
}
