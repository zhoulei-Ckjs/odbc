cmake_minimum_required(VERSION 3.16)
project(main)

aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/src/ SRC)

add_executable(${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include/)

target_link_libraries(${PROJECT_NAME} odbc)