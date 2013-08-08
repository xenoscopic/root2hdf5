#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_type
#include <boost/test/unit_test.hpp>


// root2hdf5 includes
#include "type.h"


// root2hdf5 namespaces
using namespace root2hdf5::type;


BOOST_AUTO_TEST_CASE(test_root_type_name_to_scalar_hdf5_type)
{
    BOOST_CHECK_EQUAL(root_type_name_to_scalar_hdf5_type("char"),
                      H5T_NATIVE_SCHAR);
    BOOST_CHECK_EQUAL(root_type_name_to_scalar_hdf5_type("unknown"),
                      -1);
}
