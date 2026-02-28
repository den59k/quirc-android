cmake_minimum_required(VERSION 3.10)
project(quirc_host)

add_library(quirc STATIC
  ../../quirc/lib/quirc.c
  ../../quirc/lib/decode.c
  ../../quirc/lib/identify.c
  ../../quirc/lib/version_db.c
)

target_include_directories(quirc PUBLIC
  ../../quirc/lib
)

add_executable(quirc_test
    test.cpp
)

target_link_libraries(quirc_test
    quirc
)

set_target_properties(quirc_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)