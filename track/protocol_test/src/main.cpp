#include <array>
#include <list>
#include <functional>

#include <boost/asio.hpp>

#include <simple_command.pb.h>

#include "utils.hpp"

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
		PPF();
		ba::write(port, ba::buffer(s.c_str(), s.size()));
	}

	template <typename T>
	void write(T const& t)
	{
		PPF();
		ba::write(port, ba::buffer(t.data(), t.size()));
	}

	void write(void* c, size_t size)
	{
		PPF();
		ba::write(port, ba::buffer(c, size));
	}

	void writebyte(char c)
	{
		PPF();
		ba::write(port, ba::buffer(&c, 1));
	}

	char readbyte()
	{
		PPF();
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
	// PPFX( "size = " << data.size() );

	assert(!data.empty());

	// for (auto c : data)
	// 	printf("0x%02x ", c);
	// cout << endl;
	// for (auto c : data)
	// 	putchar(c);
	// cout << endl;



	uint8_t const* new_b = &*data.begin();
	uint8_t const* e = new_b + data.size();

	{
		uint8_t const* b = new_b;

		while (e - b)
		{
			uint8_t size = b[0];
			// PPFX("message size have to be " << (int)size);
			++b;

			if (e - b < size)
				break;


			MessagePackage package;
			if (package.ParseFromArray(b, size))
			{
				b += size;
				new_b = b;

				cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< size = " << size << endl;
				cout << package.DebugString() << endl;
				cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<"  << endl;
			}

		}

	}

	if (e - new_b)
	{
		std::copy(new_b, e, data.begin());
		data.resize(e - new_b);
	}
	else
	{
		data.clear();
	}
	// PPFX( "new size = " << data.size() );

/*
	for (auto c : data)
		printf("0x%02x ", c);
	cout << endl;

	data.clear();
*/
}



boost::posix_time::seconds period(2);

void on_timer(SerialPort& port) try
{
	// PPF();


	static bool on = true;

	MessagePackage package;
	SimpleCommand* pcom = package.mutable_simple_command();
	SimpleCommand& com = *pcom;

	// SimpleCommand com;
	com.set_node_id(3665386816);
	com.set_command(on ? LIGHT_ON : LIGHT_OFF);
	on = !on;

	array<uint8_t, 256> a;
	auto succ = package.SerializeToArray(a.data(), a.size());
	assert(succ);

	auto s = package.ByteSize();

	cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> size = " << s << endl;
	cout << com.DebugString() << endl;

	// for (size_t i = 0; i<s; ++i)
	// {
	// 	printf("0x%02x ", a[i]);
	// }
	// cout << endl;


	cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;



	port.writebyte(static_cast<uint8_t>(s));
	port.write(a.data(), s);

	// exit(0);
}
catch(std::exception& ex)
{
	std::cerr << ex.what() << std::endl;
}


int main()
{
	SerialPort port(
		"/dev/tty.usbmodemfd141",
		// "/dev/tty.usbmodemfa141",
		57600, iosrv,
			[](std::vector<uint8_t>& data) { on_serial_data_receive(data); });

	ba::deadline_timer timer(iosrv, boost::posix_time::seconds(1));

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