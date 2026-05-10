cmake_minimum_required(VERSION 3.20)
set(CMAKE_C_COMPILER /usr/bin/cc CACHE FILEPATH "C compiler" FORCE)
set(CMAKE_CXX_COMPILER /usr/bin/c++ CACHE FILEPATH "C++ compiler" FORCE)
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

option(BUILD_TESTS "Build tests" ON)
option(BUILD_LOGGING "Build with log4cplus" ON)

if(BUILD_LOGGING)
  include(FetchContent)
  FetchContent_Declare(
    log4cplus
    GIT_REPOSITORY https://github.com/log4cplus/log4cplus.git
    GIT_TAG REL_2_1_2
  )
  set(LOG4CPLUS_BUILD_TESTING OFF CACHE BOOL "" FORCE)
  set(LOG4CPLUS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(log4cplus)
endif()

file(GLOB_RECURSE MESSENGER_DOMAIN_HEADERS CONFIGURE_DEPENDS
  include/models/*.hpp
  include/models/*.h
)

file(GLOB_RECURSE MESSENGER_DOMAIN_SOURCES CONFIGURE_DEPENDS
  src/models/*.cpp
)

file(GLOB_RECURSE MESSENGER_PROTOCOL_HEADERS CONFIGURE_DEPENDS
  include/protocol/*.hpp
  include/protocol/*.h
)

file(GLOB_RECURSE MESSENGER_PROTOCOL_SOURCES CONFIGURE_DEPENDS
  src/protocol/*.cpp
)

file(GLOB_RECURSE MESSENGER_TRANSPORT_HEADERS CONFIGURE_DEPENDS
  include/transport/*.hpp
  include/transport/*.h
)

file(GLOB_RECURSE MESSENGER_TRANSPORT_SOURCES CONFIGURE_DEPENDS
  src/transport/*.cpp
)

file(GLOB_RECURSE MESSENGER_INFRASTRUCTURE_HEADERS CONFIGURE_DEPENDS
  include/infrastructure/*.hpp
  include/infrastructure/*.h
)

file(GLOB_RECURSE MESSENGER_INFRASTRUCTURE_SOURCES CONFIGURE_DEPENDS
  src/infrastructure/*.cpp
)

file(GLOB_RECURSE MESSENGER_APPLICATION_HEADERS CONFIGURE_DEPENDS
  include/app/*.hpp
  include/app/*.h
  include/application/*.hpp
  include/application/*.h
)

file(GLOB_RECURSE MESSENGER_APPLICATION_SOURCES CONFIGURE_DEPENDS
  src/application/*.cpp
)

file(GLOB_RECURSE MESSENGER_UI_HEADERS CONFIGURE_DEPENDS
  include/ui/*.hpp
  include/ui/*.h
)

file(GLOB_RECURSE MESSENGER_UI_SOURCES CONFIGURE_DEPENDS
  src/ui/*.cpp
)

add_library(messenger_core STATIC
  ${MESSENGER_DOMAIN_HEADERS}
  ${MESSENGER_DOMAIN_SOURCES}
  ${MESSENGER_PROTOCOL_HEADERS}
  ${MESSENGER_PROTOCOL_SOURCES}
  ${MESSENGER_TRANSPORT_HEADERS}
  ${MESSENGER_TRANSPORT_SOURCES}
  ${MESSENGER_INFRASTRUCTURE_HEADERS}
  ${MESSENGER_INFRASTRUCTURE_SOURCES}
  ${MESSENGER_APPLICATION_HEADERS}
  ${MESSENGER_APPLICATION_SOURCES}
  ${MESSENGER_UI_HEADERS}
  ${MESSENGER_UI_SOURCES}
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

if(BUILD_LOGGING)
  target_link_libraries(messenger_core
    PUBLIC
      log4cplus::log4cplus
  )
  target_compile_definitions(messenger_core PUBLIC MESSENGER_WITH_LOG4CPLUS=1)
endif()

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
