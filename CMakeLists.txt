cmake_minimum_required (VERSION 2.6)
project (boost-lockfree)

enable_testing()

set(Boost_USE_MULTITHREADED ON)
find_package(Boost REQUIRED COMPONENTS thread)
add_definitions(-DBOOST_ALL_NO_LIB)
link_directories(${Boost_LIBRARY_DIRS})
include_directories(AFTER SYSTEM ${Boost_INCLUDE_DIRS})

include_directories(.)

add_subdirectory (libs/lockfree/doc)
add_subdirectory (libs/lockfree/examples)
add_subdirectory (libs/lockfree/test)
