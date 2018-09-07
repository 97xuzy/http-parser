cmake_minimum_required(VERSION 3.8)

if (MSVC)
	add_compile_options(/W4)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}" /O2)
else()
	add_compile_options(-Wall -Wextra -Wpedantic)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}" -ggdb )
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}" -O3 -DNDEBUG -pipe)
endif()

add_library (libhttp_parser http_parser.c)

target_include_directories (libhttp_parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
