cmake_minimum_required(VERSION 3.20)
project(Messenger VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenSSL REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Threads REQUIRED)

file(GLOB_RECURSE CORE_SRC CONFIGURE_DEPENDS
    src/*.cpp
)

file(GLOB_RECURSE CORE_HEADERS CONFIGURE_DEPENDS
    include/*.hpp
    include/*.h
)

list(REMOVE_ITEM CORE_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp
)

add_library(messenger_core STATIC
    ${CORE_SRC}
    ${CORE_HEADERS}
)

target_include_directories(messenger_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(messenger_core PRIVATE
    -Wall
    -Wextra
)

target_link_libraries(messenger_core
    PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
        Qt6::Widgets
        Threads::Threads
)

add_executable(messenger_server
    main.cpp
)

target_link_libraries(messenger_server
    PRIVATE
        messenger_core
)

add_executable(messenger_client
    client.cpp
)

target_link_libraries(messenger_client
    PRIVATE
        messenger_core
)

include(CTest)
enable_testing()

option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
  include(FetchContent)
  FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG release-1.12.1
  )

  if(WIN32)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  endif()

  set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  add_executable(run_tests
      tests/tests.cpp
  )

  target_link_libraries(run_tests
      PRIVATE
          messenger_core
          GTest::gtest
          GTest::gtest_main
  )

  add_test(NAME all_tests COMMAND run_tests)

  add_custom_target(check
      COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
      DEPENDS run_tests
      COMMENT "Running tests..."
  )
endif()

if(WIN32)
  target_link_libraries(messenger_server PRIVATE ws2_32)
  target_link_libraries(messenger_client PRIVATE ws2_32)
endif()
