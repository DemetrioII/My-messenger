# My Messenger

Educational messenger written in C++ with a client-server architecture, Qt Widgets UI, encrypted message transport, and a growing application/transport/protocol split.

## Features

- Client and server modes
- Qt Widgets GUI
- Encrypted messages
- Chat rooms
- File transfer
- Command-based UI
- Logging through `log4cplus`

## Project Layout

- `app/` - runtime and configuration
- `application/` - commands, dispatching, messaging, session, and use-case services
- `transport/` - sockets, event loop, connection handling, server/client runtime
- `protocol/` - message parsing and serialization
- `domain/` - core data models
- `infrastructure/` - logging, crypto, and backend helpers
- `ui/` - Qt windows and widgets

## Build

### Requirements

- CMake 3.20+
- C++23 compiler
- Qt6 Widgets
- Optional: `log4cplus`

### Configure and build

```bash
cmake -S . -B build
cmake --build build -j
```

If you want to disable optional logging:

```bash
cmake -S . -B build -DBUILD_LOGGING=OFF
```

## Run

The project builds separate client and server executables.

```bash
./build/messenger_server
./build/messenger_client
```

If your build directory uses another name, replace `build` accordingly.

## Commands

The client uses command-style input.

- `/login <username>`
- `/room <name>`
- `/join <name>`
- `/send <chat> <message...>`
- `/pmess <recipient> <message...>`
- `/getpub <username>`
- `/sendfile <recipient> <filename>`
- `/connect <ip> <port> <dh> <identity> <sig>`
- `/disconnect <username>`
- `/exit`
- `/help`

## Security Notes

- Message payloads are encrypted with AES-GCM.
- The protocol includes replay-protection state.
- Key exchange and trust establishment are still evolving.
- This is an educational project, not a security-audited production system.

## Tests

If tests are enabled in your build:

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build
```

## Status

The project is actively being refactored toward a cleaner separation of responsibilities:

- `application` contains orchestration and commands
- `transport` contains raw networking
- `protocol` contains serialization/parsing
- `ui` contains presentation

Some parts are still transitionary while the architecture is being simplified.

## License

Copyright (C) 2025 DemetrioII

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
