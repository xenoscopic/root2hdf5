#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_tree_structure
#include <boost/test/unit_test.hpp>


// Standard includes
#include <vector>
#include <string>

// ROOT includes
#include <TTree.h>

// root2hdf5 includes
#include "tree/structure.h"


// Standard namespaces
using namespace std;

// root2hdf5 namespaces
using namespace root2hdf5::tree::structure;


struct ComplexBranch
{
    int leaf_1;
    bool leaf_2;
};


BOOST_AUTO_TEST_CASE(test_walk_tree)
{
    // TODO: Should add some subbranches in here

    // First, create a TTree to experiment with
    TTree *tree = new TTree("TestTree", "Testing Tree");

    // Create some branches
    int branch_1;
    tree->Branch("branch_1", &branch_1, "branch_leaf_1/I");
    double branch_2;
    tree->Branch("branch_2", &branch_2, "branch_leaf_2/D");
    ComplexBranch branch_3;
    tree->Branch("branch_3", &branch_3, "leaf_1/I:leaf_2/O");

    // Test code generation
    string struct_name = struct_type_name_for_tree(tree);
    BOOST_CHECK_EQUAL(
        struct_code_for_tree(tree),
        string("struct ") + struct_name + 
        "{Int_t branch_leaf_1;Double_t branch_leaf_2;"
        "struct{Int_t leaf_1;Bool_t leaf_2;}branch_3;};"
    );

    // Clean up the tree
    delete tree;
}
