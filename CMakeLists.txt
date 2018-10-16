cmake_minimum_required(VERSION 3.8)

project(http-parser)

if (MSVC)
	add_compile_options(/W4)
	#set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}" /O2)
	#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}" /O2 /std=c++17)
else()
	#add_compile_options(-Wall -Wextra -Wpedantic)
	add_compile_options(-Wall -Wextra)
	#set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}" -ggdb )
	#set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}" -O3 -DNDEBUG -pipe)
	#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}" -g -std=c++17)
	#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}" -O3 -DNDEBUG -pipe -std=c++17)
endif()

add_library (http_parser http_parser.cpp http_parser.c)

target_include_directories (http_parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# c++17
target_compile_features(http_parser PUBLIC cxx_std_17)

#
#
# Test
#
#
enable_testing()
add_executable(test1 test.cpp)
target_link_libraries (test1 LINK_PUBLIC http_parser)

add_test(Test1 test1)
