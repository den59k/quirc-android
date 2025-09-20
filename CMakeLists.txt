cmake_minimum_required(VERSION 3.10)
project(quirc_jni)

add_library(quirc STATIC
  quirc/lib/quirc.c quirc/lib/decode.c quirc/lib/identify.c quirc/lib/version_db.c
)

target_include_directories(quirc PUBLIC
    quirc/lib
)

# Нативная библиотека с JNI-обёрткой
add_library(quirc_jni SHARED
    src/main/cpp/quirc_jni.cpp
)

find_library(log-lib log)

target_include_directories(quirc_jni PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/quirc/lib
)

target_link_libraries(quirc_jni
    quirc
    ${log-lib}
)

# Компиляция на C++11
set_target_properties(quirc_jni PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)