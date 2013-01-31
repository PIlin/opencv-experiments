#include <array>
#include <list>
#include <functional>

#include <boost/asio.hpp>

#include <simple_command.pb.h>

using namespace std;

namespace ba = boost::asio;

ba::io_service iosrv;

class SerialPort : private boost::noncopyable
{
public:
	typedef std::function<void(std::vector<uint8_t>&)> on_receive_t;

	SerialPort(
		std::string name, uint32_t baud, ba::io_service& service,
		on_receive_t on_receive
		) :
		port(service, name),
		on_receive_clb(on_receive)
	{
		port.set_option(ba::serial_port_base::baud_rate(baud));

		begin_async_receive();
	}

	void write(std::string s)
	{
		ba::write(port, ba::buffer(s.c_str(), s.size()));
	}

	void writebyte(char c)
	{
		ba::write(port, ba::buffer(&c, 1));
	}

	void writebytes(void* c, size_t size)
	{
		ba::write(port, ba::buffer(c, size));
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

	void begin_async_receive()
	{
		port.async_read_some(
			ba::buffer(recv_buffer.data(), recv_buffer.size()),
			[this](boost::system::error_code const& error, size_t received) {
				this->on_receive(error, received);
			}
			);
	}

	void on_receive(boost::system::error_code const& err, size_t received)
	{
		if (err)
		{
			std::cerr << err << std::endl;
			return;
		}

		recv_data.insert(recv_data.end(), recv_buffer.begin(), recv_buffer.begin() + received);

		on_receive_clb(recv_data);

		begin_async_receive();
	}

private:

	ba::serial_port port;

	std::array<uint8_t, 1024> recv_buffer;
	std::vector<uint8_t> recv_data;
	on_receive_t on_receive_clb;
};

void on_serial_data_receive(std::vector<uint8_t>& data)
{
	for (auto c : data)
	{
		putchar(c);
	}
	putchar('\n');

	data.clear();
}



boost::posix_time::seconds period(1);

void on_timer(SerialPort& port)
{
	static bool on = true;

	simple_command com;
	com.set_node_id(123);
	com.set_command(on ? LIGHT_ON : LIGHT_OFF);
	on = !on;

	std::vector<uint8_t> data(com.ByteSize(), 0);

	assert(com.SerializeToArray(data.data(), data.size()));


	port.writebytes(data.data(), data.size());
}


int main()
{
	SerialPort port("/dev/tty.usbmodemfd141", 9600, iosrv,
			[](std::vector<uint8_t>& data) { on_serial_data_receive(data); });

	ba::deadline_timer timer(iosrv, period);

	std::function<void(boost::system::error_code const&)> on_timeout;
	auto relauch_timer = [&]() { timer.async_wait(on_timeout); };
	on_timeout = [&](boost::system::error_code const&)
		{
			on_timer(port);
			timer.expires_from_now(period);
			relauch_timer();
		};

	relauch_timer();

	while(true)
	{
		iosrv.poll();
	}

	return 0;
}