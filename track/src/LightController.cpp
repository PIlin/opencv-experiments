#include "LightController.hpp"
#include "utils.hpp"

#include <array>

#include <boost/asio.hpp>

namespace ba = boost::asio;

ba::io_service iosrv;


class SerialPort : private boost::noncopyable
{
public:
	SerialPort(std::string name, uint32_t baud, ba::io_service& service) :
		port(service, name)
	{
		port.set_option(ba::serial_port_base::baud_rate(baud));

		// port.async_read_some(const MutableBufferSequence &buffers, const ReadHandler &handler)
	}

	void write(std::string s)
	{
		ba::write(port, ba::buffer(s.c_str(), s.size()));
	}

	void writebyte(char c)
	{
		ba::write(port, ba::buffer(&c, 1));
	}

	char readbyte()
	{
		char c;
		ba::read(port, ba::buffer(&c, 1));
		return c;
	}

	std::string readline()
	{
		std::string result;
		char c;
		while (true)
		{
			c = readbyte();

			switch (c)
			{
			case '\r':
			case '\n':
				return result;
			default:
				result += c;
				break;
			}
		}
	}

private:

	ba::serial_port port;

	std::array<uint8_t, 1024> recv_buffer;
};


void LightController::Enable(LightID const& id)
{
	PPFX(id);
}

void LightController::Disable(LightID const& id)
{
	PPFX(id);
}

bool LightController::IsEnabled(LightID const& id)
{
	PPFX(id);

	return false;
}

bool LightController::IsDisabled(LightID const& id)
{
	PPFX(id);

	return true;
}

void LightController::SetDetectedID(LightID const& id, uint32_t track_id)
{
	PPFX(id << " " << track_id);
}

void LightController::DetectionFailed(LightID const& id)
{
	PPFX(id);
}



void LightController::poll()
{
	// PPF();

	boost::system::error_code error;
	iosrv.poll(error);
	if (error)
	{
		std::cerr << "LightController::poll() error: " << error.message() << std::endl;
	}


	static char c = 'H';
	c = (c == 'L' ? 'H' : 'L');

	port->writebyte(c);

}

LightController::LightController() :
	port(make_unique<SerialPort>("/dev/tty.usbmodemfd141", 9600, iosrv))
{

}

LightController::~LightController()
{

}