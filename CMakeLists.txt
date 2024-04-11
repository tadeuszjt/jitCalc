# To build project:
# mkdir build
# cd build
# cmake ..
# make
# ./jitCalc
cmake_minimum_required(VERSION 3.10)

project(jitCalc)

# make sure to use c++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# include directories to find header files
include_directories(lexer)
include_directories(parser)
include_directories(codegen)
include_directories(.)

# make sure bison is installed and provide grammar file
find_package(BISON REQUIRED)
set(BISON_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/parser/grammar.y)
set(BISON_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/parser/grammar.tab.cc)
add_custom_command(
    OUTPUT ${BISON_OUTPUT}
    COMMAND ${BISON_EXECUTABLE} --language=c++ -d ${BISON_INPUT} -o ${BISON_OUTPUT}
    DEPENDS ${BISON_INPUT}
    COMMENT "Running Bison"
)

# create executable from sources
add_executable(jitCalc main.cpp parse.cpp lexer/lexer.cpp parser/ast.cpp codegen/emit.cpp codegen/moduleBuilder.cpp codegen/symbols.cpp ${BISON_OUTPUT})

# make sure llvm is installed
find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(llvm_libs support core executionengine mcjit native)
target_link_libraries(jitCalc ${llvm_libs})



