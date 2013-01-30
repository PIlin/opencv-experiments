#include "LightController.hpp"
#include "utils.hpp"

#include <array>
#include <list>

#include <boost/asio.hpp>

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

class Light
{
public:
	bool enabled;
};


struct Command
{
	uint8_t com;
	LightID id;
	std::function<void(LightID)> on_answer;
};

class LightController::Impl
{
public:

	struct TimeredCommand
	{
		Command com;
		std::shared_ptr<ba::deadline_timer> timer;
	};

	Impl(LightController& owner) :
		owner(owner),
		port(make_unique<SerialPort>("/dev/tty.usbmodemfa141", 9600, iosrv,
			[this](std::vector<uint8_t>& data) { on_serial_data_receive(data); }))
	{}

	void on_serial_data_receive(std::vector<uint8_t>& data)
	{
		PPFX(data.size());

		for (auto c : data)
		{
			auto it = std::find_if(commands.begin(), commands.end(), [&](TimeredCommand& com){
				return com.com.com == c;
			});

			if (it != commands.end())
			{
				Command & com = it->com;
				auto lit = owner.lightmap.find(com.id);
				if (lit == owner.lightmap.end())
				{
					std::cout << "Found command answer";

					if (com.on_answer)
						com.on_answer(com.id);

					it->timer->cancel();
					commands.erase(it);
				}
				else
				{
					std::cout << "No command for answer " << c << std::endl;
				}
			}
		}

		data.clear();
	}

	void on_timer_timeout(std::shared_ptr<ba::deadline_timer> timer, boost::system::error_code const& error)
	{
		PPFX(error);

		auto it = std::find_if(commands.begin(), commands.end(), [&](TimeredCommand& c) {
			return c.timer == timer;
		});

		if (it == commands.end())
		{
			std::cout << "Timer not found" << std::endl;
		}
		else
		{
			std::cout << "Timer found for command " << it->com.com << std::endl;
			commands.erase(it);
		}
	}

	void enqueue_command(Command com)
	{
		port->writebyte(com.com);

		auto timer = std::make_shared<ba::deadline_timer>(iosrv, boost::posix_time::seconds(5));

		commands.back().timer->async_wait([this, timer](boost::system::error_code const& error) {
			this->on_timer_timeout(timer, error);
		});

		commands.push_back({com, timer});
	}


	std::list<TimeredCommand> commands;

	friend class LightController;
	LightController& owner;
	std::unique_ptr<SerialPort> port;
};




void LightController::Enable(LightID const& id)
{
	PPFX(id);

	auto it = lightmap.find(id);
	if (it == lightmap.end())
		throw std::invalid_argument("id is not found");

	impl->enqueue_command(Command{'H', id,
		[this](LightID id)
		{
			auto it = lightmap.find(id);
			if (it != lightmap.end())
				it->second->enabled = true;
			else
				PPFX("id is not found");
		}
	});
}

void LightController::Disable(LightID const& id)
{
	PPFX(id);

	auto it = lightmap.find(id);
	if (it == lightmap.end())
		throw std::invalid_argument("id is not found");

	impl->enqueue_command(Command{'L', id,
		[this](LightID id)
		{
			auto it = lightmap.find(id);
			if (it != lightmap.end())
				it->second->enabled = false;
			else
				PPFX("id is not found");
		}
	});
}

bool LightController::IsEnabled(LightID const& id)
{
	PPFX(id);

	auto it = lightmap.find(id);
	if (it == lightmap.end())
		throw std::invalid_argument("id is not found");

	return it->second->enabled;
}

bool LightController::IsDisabled(LightID const& id)
{
	PPFX(id);

	return !IsEnabled(id);
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


	// static char c = 'H';
	// c = (c == 'L' ? 'H' : 'L');

	// port->writebyte(c);

}

LightController::LightController() :
	impl(make_unique<Impl>(std::ref(*this)))
{
	lightmap.insert(std::make_pair(LightID{0}, make_unique<Light>(Light{false})));
}

LightController::~LightController()
{

}