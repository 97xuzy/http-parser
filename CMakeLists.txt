cmake_minimum_required(VERSION 3.8)

project(http-parser)

if (MSVC)
	add_compile_options(/W4)
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}" /O2)
else()
	add_compile_options(-Wall -Wextra -Wpedantic)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}" -ggdb )
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}" -O3 -DNDEBUG -pipe)
endif()

add_library (http_parser http_parser.c)

target_include_directories (http_parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
