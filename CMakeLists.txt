cmake_minimum_required(VERSION 3.16)
project(Messenger VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-g -std=c++23)

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

# ==========================================
# ТЕСТЫ
# ==========================================

# Включаем GoogleTest
enable_testing()
include(FetchContent)
FetchContent_Declare(
	googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG release-1.12.1
)

# Для Windows: отключаем установку, чтобы не попасть в PATH
if(WIN32)
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

FetchContent_MakeAvailable(googletest)

add_executable(run_tests 
	tests/tests.cpp
    ${HEADERS}
)

target_include_directories(run_tests PRIVATE src)
target_link_libraries(run_tests PRIVATE 
    GTest::gtest 
    GTest::gtest_main 
    OpenSSL::SSL 
    OpenSSL::Crypto 
    pthread
)
# Регистрируем тесты
add_test(NAME all_tests COMMAND run_tests)

# Опционально: добавить запуск тестов в CI/CD
add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS run_tests
    COMMENT "Running tests..."
)

# Для отладки тестов (опционально)
add_executable(run_tests_debug
    tests/tests.cpp
    ${HEADERS}
)

target_include_directories(run_tests_debug PRIVATE src)
target_link_libraries(run_tests_debug PRIVATE 
    GTest::gtest 
    GTest::gtest_main 
    OpenSSL::SSL 
    OpenSSL::Crypto 
    pthread
)
set_target_properties(run_tests_debug PROPERTIES
    COMPILE_FLAGS "-g -O0"
)

if(WIN32)
    target_link_libraries(messenger_server ws2_32)
    target_link_libraries(messenger_client ws2_32)
endif()
