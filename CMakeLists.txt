cmake_minimum_required(VERSION 3.10)
project(learnv8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -g -O0 -Wall -Wpedantic")

add_executable( port
    src/Wrapper.cpp
    src/Exception.cpp
    src/Utils.cpp
    src/Engine.cpp
    src/Context.cpp
    src/SoirV8.cpp
)

include_directories(
    src
    ../v8/include
    /usr/include/python2.7
)

find_library(
    V8_MONOLITH v8_monolith HINTS ../v8/out.gn/x64.release.sample/obj
)

target_link_libraries( port
    ${V8_MONOLITH}
    boost_python
    boost_system
    python2.7
    pthread
)

#compile v8 sample:
#g++ -I. -Iinclude samples/hello-world.cc -o hello_world -lv8_monolith -Lout.gn/x64.release.sample/obj/ -pthread -std=c++0x

