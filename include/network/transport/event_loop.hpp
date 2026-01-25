// event_loop.hpp
#pragma once
#include "interface.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

class EventLoop : public IEventLoop {
  Fd epollfd;
  Fd wakeup_fd;
  std::unordered_map<int, std::weak_ptr<IEventHandler>> fd_to_handler;

public:
  EventLoop();

  void remove_fd(int fd);

  ~EventLoop();

  void add_fd(int fd_to_add, std::weak_ptr<IEventHandler> handler,
              uint32_t events) override;

  void stop() override;

  void run_once(int timeout_ms = -1) override;
};
