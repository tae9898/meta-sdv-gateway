cmake_minimum_required(VERSION 3.10)
project(sdv-gateway C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 경고 플래그
add_compile_options(-Wall -Wextra -Werror)

# 소스 파일
set(SOURCES
    src/main.c
    src/gateway.c
    src/can_handler.c
    src/rs485_handler.c
    src/doip_handler.c
)

# 실행 파일
add_executable(sdv-gateway ${SOURCES})

target_include_directories(sdv-gateway PRIVATE ${CMAKE_SOURCE_DIR}/include)

# pthread 링크
target_link_libraries(sdv-gateway PRIVATE pthread)

# 설치
install(TARGETS sdv-gateway DESTINATION ${CMAKE_INSTALL_BINDIR})
