cmake_minimum_required(VERSION 3.10)
project(pm-receiver C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

add_executable(pm-receiver
    src/pm_receiver.c
    src/frame_decoder.c
    src/sensor_db.c
)

target_include_directories(pm-receiver PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(pm-receiver PRIVATE sqlite3 pthread dl)

install(TARGETS pm-receiver DESTINATION ${CMAKE_INSTALL_BINDIR})
