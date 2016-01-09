#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <iostream>
#include <utility>

using namespace boost::asio;
using namespace boost::system;

io_service service;
ip::tcp::acceptor acceptor(service,
                           ip::tcp::endpoint(ip::tcp::v4(), 7777));

void do_accept(yield_context yield) {
  while (true) {
    auto socket = std::make_shared<ip::tcp::socket>(service);
    acceptor.async_accept(*socket, yield);
    spawn(service, [=](yield_context yield) {
      async_write(*socket, buffer("Hello", 5), yield);
      socket->close();
    });
    std::cerr << "Accepted!" << std::endl;
  }
}

int main() {
  spawn(service, do_accept);
  service.run();
}
