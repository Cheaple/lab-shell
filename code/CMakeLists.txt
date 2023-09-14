cmake_minimum_required(VERSION 3.18)

project(EDA093-lab1 LANGUAGES C)

find_package(Curses REQUIRED)
find_library(TERMCAP_LIBRARY termcap)

add_executable(lsh parse.c lsh.c)

if(CURSES_FOUND)
    target_link_libraries(lsh PRIVATE readline ncurses)
elseif(TERMCAP_LIBRARY)
    target_link_libraries(lsh PRIVATE readline termcap)
else()
    message(FATAL_ERROR "Neither ncurses nor termcap found.")
endif()

target_compile_options(lsh PRIVATE "-ggdb3" "-O0" "-Wall" "-Wextra")
