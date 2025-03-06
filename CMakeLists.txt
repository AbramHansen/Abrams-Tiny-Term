cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_NAME abrams_tiny_term)

project(${PROJECT_NAME})

include(GNUInstallDirs)

find_package(SDL3 REQUIRED CONFIG)

set(HEADERS
    include/ascii_font.h
    include/terminal.h
)

set(SOURCES
    src/main.cpp
    src/ascii_font.cpp
    src/terminal.cpp
)

# Configure paths.h.in with different paths based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(MEDIA_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/")
    set(BINARY_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/${PROJECT_NAME}")
    set(INSTALLED true)
else()
    set(MEDIA_DIR "${CMAKE_CURRENT_BINARY_DIR}/../media/")
    set(INSTALLED false)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/paths.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/paths.h)

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY ${CMAKE_SOURCE_DIR}/media/ DESTINATION ${MEDIA_DIR})
