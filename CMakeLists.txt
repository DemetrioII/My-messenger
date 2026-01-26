cmake_minimum_required(VERSION 3.16)
project(Messenger VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-g)

# OpenSSL + Qt
find_package(OpenSSL REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Пути к заголовкам
include_directories(include)

# Собираем cpp из src/
file(GLOB_RECURSE SRC_FILES
    src/*.cpp
)

# Заголовки (чисто чтобы IDE знали про них)
file(GLOB_RECURSE HEADER_FILES
    include/*.hpp
    include/*.h
)

# Сервер
add_executable(messenger_server
    main.cpp         # точка входа сервера
    ${SRC_FILES}
    ${HEADER_FILES}
)

target_link_libraries(messenger_server PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    Qt6::Widgets
    pthread
)

# Клиент
add_executable(messenger_client
    client.cpp       # точка входа клиента
    ${SRC_FILES}
    ${HEADER_FILES}
)

target_link_libraries(messenger_client PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    Qt6::Widgets
    pthread
)

if(WIN32)
    target_link_libraries(messenger_server ws2_32)
    target_link_libraries(messenger_client ws2_32)
endif()
