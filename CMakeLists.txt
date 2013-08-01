# Set the minmum CMake version
cmake_minimum_required(VERSION 2.6.4)

# Set the project name
project(root2hdf5)

# Use C++11 with strict compilation flags
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -pedantic -Werror")

# Add extra CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

# Find ROOT
FIND_PACKAGE(ROOT REQUIRED)
include_directories(${ROOT_INCLUDE_DIR})

# Find HDF5
find_package(HDF5 COMPONENTS CXX REQUIRED)
include_directories(${HDF5_INCLUDE_DIRS})
add_definitions(${HDF5_DEFINITIONS})

# Find Boost
# HACK: In some older versions of Boost, the filesystem library was not divided
# into system/filesystem libraries, so system may not exist on older Boost
# installs.  Here we make it optional and only link against it if it exists.
find_package(Boost COMPONENTS program_options filesystem REQUIRED)
find_package(Boost COMPONENTS system)
include_directories(${Boost_INCLUDE_DIR})
set(BOOST_LINK_TARGETS
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_FILESYSTEM_LIBRARY})
if(Boost_SYSTEM_FOUND)
    set(BOOST_LINK_TARGETS
        ${BOOST_LINK_TARGETS}
        ${Boost_SYSTEM_LIBRARY})
endif()

# Add local include directories
include_directories(${PROJECT_SOURCE_DIR}/source)

# Create the target
add_executable(root2hdf5
               source/options.cpp
               source/convert.cpp
               source/tree.cpp
               source/root2hdf5.cpp)
target_link_libraries(root2hdf5
                      ${ROOT_LIBRARIES}
                      ${HDF5_LIBRARIES}
                      ${BOOST_LINK_TARGETS})
