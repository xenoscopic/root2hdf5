#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_tree_walk
#include <boost/test/unit_test.hpp>


// Standard includes
#include <vector>
#include <string>

// Boost includes
#include <boost/bind.hpp>

// ROOT includes
#include <TTree.h>

// root2hdf5 includes
#include "tree/walk.h"


// Standard namespaces
using namespace std;

// Boost namespaces
using namespace boost;

// root2hdf5 namespaces
using namespace root2hdf5::tree::walk;


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

    // Create a container for mapping out the tree
    vector<string> map;

    // Walk
    walk_tree(
        tree,
        [&map](TBranch *branch) -> bool {
            map.push_back(string("open ") + branch->GetName());
            return true;
        },
        [&map](TBranch *branch) -> bool {
            map.push_back(string("close ") + branch->GetName());
            return true;
        },
        [&map](TLeaf *leaf) -> bool {
            map.push_back(string("process ") + leaf->GetName());
            return true;
        }
    );

    // Compare with the expected result
    const vector<string> expected = {
        "process branch_leaf_1",
        "process branch_leaf_2",
        "open branch_3",
        "process leaf_1",
        "process leaf_2",
        "close branch_3"
    };
    // HACK: For some reason we can't use BOOST_CHECK_EQUAL with the std::vector
    // type, so just do this instead.
    BOOST_REQUIRE(map == expected);

    // Clean up the tree
    delete tree;
}
