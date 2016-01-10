#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <utility>

using namespace boost::asio;
using namespace boost::system;

io_service service;
ip::tcp::acceptor acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), 7777));

void do_handle(std::shared_ptr<ip::tcp::socket> socket, yield_context yield) {
  try {
    while (true) {
      char buf[4096];
      size_t size = socket->async_read_some(buffer(buf), yield);
      async_write(*socket, buffer(buf, size), yield);
    }
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}

void do_accept(yield_context yield) {
  using namespace std::placeholders;

  while (true) {
    auto socket = std::make_shared<ip::tcp::socket>(service);
    acceptor.async_accept(*socket, yield);
    std::cerr << socket->remote_endpoint() << std::endl;
    spawn(service, std::bind(do_handle, socket, _1));
  }
}

int main() {
  spawn(service, do_accept);
  service.run();
}
