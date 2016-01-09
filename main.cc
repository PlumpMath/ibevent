#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>
#include <iostream>

void foo(boost::asio::yield_context yield) {
  std::cout << "It works!" << std::endl;
}

int main() {
  try {
    boost::asio::io_service io_service;
    boost::asio::spawn(io_service, foo);
    boost::asio::spawn(io_service, foo);
    boost::asio::spawn(io_service, foo);
    boost::asio::spawn(io_service, foo);
    io_service.run();
  } catch (std::exception &e) {
    std::cerr << "FATAL: " << e.what() << std::endl;
  }
}
