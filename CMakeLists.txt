# Set the minmum CMake version
cmake_minimum_required(VERSION 2.8.3)

# Set the project name
project(root2hdf5)

# Use C++11 with strict compilation flags
# HACK: We use the old c++0x flag to support pre-4.7 versions of GCC which do
# not know that it ended up being C++11.  This is also supported by Clang.  If
# this starts to become a bit of a pain, we can maybe do some smarter compiler
# detection.
# TODO: Add -Werror back into here after development is more finished.
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -Wextra -pedantic")

# Add extra CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

# Enable testing with CTest/Boost Unit Test Framework
enable_testing()

# Find ROOT
FIND_PACKAGE(ROOT REQUIRED)
include_directories(${ROOT_INCLUDE_DIR})

# Find HDF5
find_package(HDF5 COMPONENTS C REQUIRED)
include_directories(${HDF5_INCLUDE_DIRS})
add_definitions(${HDF5_DEFINITIONS})

# Find Boost
# HACK: In some older versions of Boost, the filesystem library was not divided
# into system/filesystem libraries, so system may not exist on older Boost
# installs.  Here we make it optional and only link against it if it exists.
find_package(Boost
             COMPONENTS
             program_options
             filesystem
             unit_test_framework
             REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
set(BOOST_LINK_TARGETS
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_FILESYSTEM_LIBRARY})

# Add local include directories
include_directories(${PROJECT_SOURCE_DIR}/source)

# Create the library target
set(library_sources
    source/options.cpp
    source/convert.cpp
    source/type.cpp
    source/tree.cpp
    source/tree/walk.cpp
    source/tree/structure.cpp
    source/tree/convert.cpp
    source/tree/leaf_converters.cpp
    source/tree/leaf_converters/scalar_converter.cpp
    source/tree/leaf_converters/vector_converter.cpp)
add_library(root2hdf5
            STATIC
            ${library_sources})
target_link_libraries(root2hdf5
                      ${ROOT_LIBRARIES}
                      ${HDF5_LIBRARIES}
                      ${BOOST_LINK_TARGETS})

# Create the main target
add_executable(root2hdf5-bin
               source/root2hdf5.cpp)
set_target_properties(root2hdf5-bin
                      PROPERTIES
                      OUTPUT_NAME
                      root2hdf5)
target_link_libraries(root2hdf5-bin
                      root2hdf5
                      ${ROOT_LIBRARIES}
                      ${HDF5_LIBRARIES}
                      ${BOOST_LINK_TARGETS})

# Create test targets
add_executable(test_tree_type
               test/test_tree_type.cpp)
target_link_libraries(test_tree_type
                      root2hdf5
                      ${ROOT_LIBRARIES}
                      ${HDF5_LIBRARIES}
                      ${BOOST_LINK_TARGETS}
                      ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
add_test(tree_type test_tree_type)

add_executable(test_tree_walk
               test/test_tree_walk.cpp)
target_link_libraries(test_tree_walk
                      root2hdf5
                      ${ROOT_LIBRARIES}
                      ${HDF5_LIBRARIES}
                      ${BOOST_LINK_TARGETS}
                      ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
add_test(tree_walk test_tree_walk)

add_executable(test_tree_structure
               test/test_tree_structure.cpp)
target_link_libraries(test_tree_structure
                      root2hdf5
                      ${ROOT_LIBRARIES}
                      ${HDF5_LIBRARIES}
                      ${BOOST_LINK_TARGETS}
                      ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
add_test(tree_structure test_tree_structure)
