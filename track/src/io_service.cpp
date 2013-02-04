#include "io_service.hpp"

#include <boost/asio.hpp>

#include <iostream>

boost::asio::io_service& get_io_service()
{
	static boost::asio::io_service iosrv;
	return iosrv;
}

void io_service_poll()
{
	boost::system::error_code error;
	get_io_service().poll(error);

	if (error)
	{
		std::cerr << "io_service_poll() error: " << error.message() << std::endl;
	}
}