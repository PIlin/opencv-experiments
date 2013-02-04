#ifndef IO_SERVICE_HPP__
#define IO_SERVICE_HPP__


namespace boost { namespace asio {
	class io_service;
}}

boost::asio::io_service& get_io_service();


void io_service_poll();

#endif