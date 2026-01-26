#include "../../../include/network/transport/event_loop.hpp"

EventLoop::EventLoop() {
  epollfd.reset_fd(epoll_create1(0));
  if (epollfd.get_fd() == -1) {
    throw std::runtime_error("epoll_create1 failed");
  }

  wakeup_fd.reset_fd(eventfd(0, EFD_NONBLOCK));
  if (wakeup_fd.get_fd() == -1)
    throw std::runtime_error("eventfd failed");

  epoll_event ev{};
  ev.events = EPOLLIN;
  ev.data.fd = wakeup_fd.get_fd();

  epoll_ctl(epollfd.get_fd(), EPOLL_CTL_ADD, wakeup_fd.get_fd(), &ev);
}

void EventLoop::remove_fd(int fd) {
  epoll_ctl(epollfd.get_fd(), EPOLL_CTL_DEL, fd, nullptr);
  fd_to_handler.erase(fd);
}

EventLoop::~EventLoop() { stop(); }

void EventLoop::add_fd(int fd_to_add, std::weak_ptr<IEventHandler> handler,
                       uint32_t events) {
  struct epoll_event ev;
  ev.events = events;
  ev.data.fd = fd_to_add;

  epoll_ctl(epollfd.get_fd(), EPOLL_CTL_ADD, fd_to_add, &ev);

  fd_to_handler[fd_to_add] = handler;
}

void EventLoop::stop() {
  uint64_t one = 1;
  write(wakeup_fd.get_fd(), &one, sizeof(one));
}

void EventLoop::run_once(int timeout_ms) {
  struct epoll_event events[MAX_EVENTS] = {};
  int nfds = epoll_wait(epollfd.get_fd(), events, MAX_EVENTS, timeout_ms);
  if (nfds == -1) {
    if (errno == EINTR)
      return;
    throw std::runtime_error("ошибка epoll_wait" +
                             std::string(strerror(errno)));
  }

  for (int i = 0; i < nfds; ++i) {
    int temp_fd = events[i].data.fd;
    uint32_t event_mask = events[i].events;

    if (temp_fd == wakeup_fd.get_fd()) {
      uint64_t dummy;
      read(wakeup_fd.get_fd(), &dummy, sizeof(dummy));
      continue; // просто пробуждение
    }

    auto it = fd_to_handler.find(temp_fd);
    if (it != fd_to_handler.end() && it->second.lock()) {
      it->second.lock()->handle_event(temp_fd, event_mask);
    }
  }
}
