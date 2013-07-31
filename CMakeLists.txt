# Set the minmum CMake version
cmake_minimum_required(VERSION 2.6.4)

# Set the project name
project(root2hdf5)

# Use C++11 with strict compilation flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -pedantic -Werror")

# Add extra CMake modules
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

# Find ROOT
FIND_PACKAGE(ROOT REQUIRED)
include_directories(${ROOT_INCLUDE_DIR})

# Find HDF5
find_package(HDF5 REQUIRED)
include_directories(${HDF5_INCLUDE_DIRS})
add_definitions(${HDF5_DEFINITIONS})

# Find Boost
find_package(Boost COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

# Create the target
add_executable(root2hdf5 root2hdf5.cpp)
target_link_libraries(root2hdf5
                      ${ROOT_LIBRARIES}
                      ${HDF5_CXX_LIBRARIES}
                      ${Boost_PROGRAM_OPTIONS_LIBRARY})
