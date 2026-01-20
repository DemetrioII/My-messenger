cmake_minimum_required(VERSION 3.16)
project(Messenger VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS -g)

# Находим OpenSSL
find_package(OpenSSL REQUIRED)

# Все заголовки
set(HEADERS
    include/models/chat.hpp
    include/models/message.hpp
    include/models/user.hpp
    include/encryption/AESGCMEncryption.hpp
    include/encryption/identity_key.hpp
    include/network/transport/interface.hpp
    include/network/transport/raw_socket.hpp
    include/network/transport/transport.hpp
    include/network/transport/connection.hpp
    include/network/transport/event_loop.hpp
    include/network/transport/acceptor.hpp
    include/network/transport/server.hpp
    include/network/transport/client.hpp
    include/network/transport/handling.hpp
    include/network/protocol/framing.hpp
    include/network/protocol/parser.hpp
    include/services/messageing_service.hpp
    include/services/messaging_server.hpp
    include/services/messaging_client.hpp
    common.hpp
)

# Сервер
add_executable(messenger_server
    main.cpp
    ${HEADERS}
)

target_include_directories(messenger_server PRIVATE src)
target_link_libraries(messenger_server PRIVATE OpenSSL::SSL OpenSSL::Crypto pthread)

# Клиент
add_executable(messenger_client
    client.cpp
    ${HEADERS}
)

target_include_directories(messenger_client PRIVATE src)
target_link_libraries(messenger_client PRIVATE OpenSSL::SSL OpenSSL::Crypto pthread)

# Для Windows
if(WIN32)
    target_link_libraries(messenger_server ws2_32)
    target_link_libraries(messenger_client ws2_32)
endif()
