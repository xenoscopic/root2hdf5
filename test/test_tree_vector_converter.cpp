#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_tree_vector_converter
#include <boost/test/unit_test.hpp>


// root2hdf5 includes
#include "tree/leaf_converters/vector_converter.h"


// root2hdf5 namespaces
using namespace root2hdf5::tree::leaf_converters::vector_converter;


BOOST_AUTO_TEST_CASE(test_root_type_name_to_vector_hdf5_type)
{
    // 1-D
    root_vector_conversion conversion = root_type_name_to_vector_hdf5_type(
        "vector<int>"
    );
    BOOST_REQUIRE(conversion.valid);
    BOOST_CHECK_EQUAL(conversion.depth, 1U);
    BOOST_CHECK_EQUAL(conversion.scalar_hdf5_type, H5T_NATIVE_INT);

    // 2-D
    conversion = root_type_name_to_vector_hdf5_type(
        "vector<vector<Float_t> >"
    );
    BOOST_REQUIRE(conversion.valid);
    BOOST_CHECK_EQUAL(conversion.depth, 2U);
    BOOST_CHECK_EQUAL(conversion.scalar_hdf5_type, H5T_NATIVE_FLOAT);

    // 3-D
    conversion = root_type_name_to_vector_hdf5_type(
        "vector< vector< vector< double > > >"
    );
    BOOST_REQUIRE(conversion.valid);
    BOOST_CHECK_EQUAL(conversion.depth, 3U);
    BOOST_CHECK_EQUAL(conversion.scalar_hdf5_type, H5T_NATIVE_DOUBLE);

    // Invalid
    BOOST_REQUIRE(!root_type_name_to_vector_hdf5_type("unknown").valid);
}
