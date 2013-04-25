#include "LightController.hpp"
#include "utils.hpp"
#include "io_service.hpp"

#include <array>
#include <list>

#include <boost/asio.hpp>

#include <opencv2/opencv.hpp>

#include <simple_command.pb.h>

LightID::LightID(::NodeAddress const& na) :
	id(na.addr())
{

}

void LightID::set_node_address(::NodeAddress* na) const
{
	if (!na)
		throw std::runtime_error("no address field to initialize");

	na->set_addr(id);
}


namespace ba = boost::asio;


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

	bool detected;

	TrackID trid;
};


struct Command
{
	ECommand com;
	LightID id;
	std::function<void(Command const& com, uint32_t answer)> on_answer;
	std::function<void(Command const& com)> on_timeout;
	uint32_t number;
};

struct Notification
{
	LightID id;
	uint32_t number;

	uint8_t x;
	uint8_t y;
};

class LightController::Impl
{
public:

	struct TimeredCommand
	{
		Command com;
		std::shared_ptr<ba::deadline_timer> timer;
	};

	Impl(LightController& owner) try:
		owner(owner),
		port(make_unique<SerialPort>(
			"/dev/tty.usbmodemfa1311",
			57600, get_io_service(),
			[this](std::vector<uint8_t>& data) { on_serial_data_receive(data); })),
		discovery_timer(get_io_service(), discovery_timeout),
		is_discovery_going(false)
	{

	}
	catch(std::exception& e)
	{
		std::cerr << "LightController::Impl Got exception\n" << e.what() << std::endl;
		throw;
	}
	catch(...)
	{
		fputs("LightController::Impl Got unknow exception.", stderr);
		throw;
	}

	void on_serial_data_receive(std::vector<uint8_t>& data)
	{
		PPFX(data.size());

		assert(!data.empty());

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

					std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< size = " << size << std::endl;
					std::cout << package.DebugString() << std::endl;
					std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<"  << std::endl;

					process_incoming_message_package(package);
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


		data.clear();
	}

	void process_incoming_message_package(MessagePackage const& mp)
	{
		if (mp.has_simple_command())
			process_incoming_simple_command(mp.simple_command());

		if (mp.has_simple_answer())
			process_incoming_simple_answer(mp.simple_answer());

		if (mp.has_debug_print())
			process_incoming_debug_print(mp.debug_print());
	}

	void process_incoming_simple_command(SimpleCommand const& sc)
	{

	}

	void process_incoming_simple_answer(SimpleAnswer const& sa)
	{
		if (BEACON == sa.command())
		{
			process_incoming_beacon(sa);
		}
		else
		{
			LightID lid = LightID(sa.node_id());

			auto it = std::find_if(commands.begin(), commands.end(), [&](TimeredCommand& com){
				return sa.command() == com.com.com
					&& lid == com.com.id
					&& sa.number() == com.com.number;
			});

			if (it != commands.end())
			{
				Command & com = it->com;
				PPFX("found command " << com.com << "num " << sa.number() << " for " << com.id);


				auto lit = owner.lightmap.find(com.id);
				if (lit != owner.lightmap.end())
				{
					PPFX("found command answer");

					if (com.on_answer)
						com.on_answer(com, sa.answer());

					it->timer->cancel();
					commands.erase(it);
				}
			}
			else
			{
				PPFX("no command " << sa.command() << "num " << sa.number() << " for " << lid);
			}
		}
	}

	void process_incoming_debug_print(DebugPrint const& dp)
	{

	}

	void process_incoming_beacon(SimpleAnswer const& be)
	{
		auto& lm = owner.lightmap;
		LightID lid = LightID(be.node_id());
		auto it = lm.find(lid);
		if (it == lm.end())
		{
			PPFX("beacon from new node " << lid);

			lm.insert(std::make_pair(lid, make_unique<Light>(Light{false, false})));
		}
		else
		{
			PPFX("beacon from existing node " << lid);
		}
	}


	void on_timer_timeout(std::shared_ptr<ba::deadline_timer> timer, boost::system::error_code const& error)
	{
		if (error == ba::error::operation_aborted)
			return;


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

			if (it->com.on_timeout)
				it->com.on_timeout(it->com);

			commands.erase(it);
		}
	}

	void enqueue_command(Command com)
	{
		static uint32_t command_counter = 0;

		com.number = command_counter++;

		auto timer = std::make_shared<ba::deadline_timer>(get_io_service(), command_timeout);

		timer->async_wait([this, timer](boost::system::error_code const& error) {
			this->on_timer_timeout(timer, error);
		});

		commands.push_back({com, timer});

		send_command(com);
	}

	void send_command(Command const& com)
	{
		MessagePackage package;
		{
			SimpleCommand* pcom = package.mutable_simple_command();
			SimpleCommand& c = *pcom;

			com.id.set_node_address(c.mutable_node_id());
			c.set_command(com.com);
			c.set_number(com.number);
		}

		send_message_package(package);
	}

	void send_notification(Notification const& no)
	{
		MessagePackage package;
		{
			PositionNotify* ppno = package.mutable_position_notify();
			PositionNotify& pno = *ppno;

			no.id.set_node_address(pno.mutable_node_id());
			pno.set_number(no.number);
			pno.set_x(no.x);
			pno.set_y(no.y);
		}

		send_message_package(package);
	}

	void send_message_package(MessagePackage& package) try
	{
		if (!package.IsInitialized())
			throw std::runtime_error("not initialized");

		std::array<uint8_t, 256> a;
		auto succ = package.SerializeToArray(a.data(), a.size());
		if (!succ)
			throw std::runtime_error("cannot serialize command to array");

		auto s = package.ByteSize();

		{
			std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> size = " << (int)s << std::endl;
			std::cout << package.DebugString() << std::endl;
			std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> size = " << std::endl;
		}

		port->writebyte(static_cast<uint8_t>(s));
		port->write(a.data(), s);
	}
	catch(std::exception& ex)
	{
		std::cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
		std::cerr << ex.what() << std::endl;
		std::cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
	}


	void do_discovery()
	{
		if (is_discovery_going)
			return;

		PPF();

		discovery_timer.expires_from_now(discovery_timeout);
		discovery_timer.async_wait([this](boost::system::error_code const&)
			{
				is_discovery_going = false;
			});

		Command c = {BEACON, LightID()};
		send_command(c);

		is_discovery_going = true;
	}


	std::list<TimeredCommand> commands;

	friend class LightController;
	LightController& owner;
	std::unique_ptr<SerialPort> port;


	const boost::posix_time::seconds discovery_timeout = boost::posix_time::seconds(20);
	const boost::posix_time::seconds command_timeout = boost::posix_time::seconds(1);
	ba::deadline_timer discovery_timer;
	bool is_discovery_going;
};




void LightController::Enable(LightID const& id, std::function<void()> on_error)
{
	PPFX(id);

	auto it = lightmap.find(id);
	if (it == lightmap.end())
		throw std::invalid_argument("id is not found");

	impl->enqueue_command(Command{LIGHT_ON, id,
		[this](Command const& com, uint32_t answer)
		{
			if (answer == 0)
			{
				PPFX("some error in the node");
				return;
			}

			auto it = lightmap.find(com.id);
			if (it != lightmap.end())
				it->second->enabled = true;
			else
				PPFX("id is not found");
		},
		[on_error](Command const& com)
		{
			if (on_error)
				on_error();
		}
	});
}

void LightController::Disable(LightID const& id, std::function<void()> on_error)
{
	PPFX(id);

	auto it = lightmap.find(id);
	if (it == lightmap.end())
		throw std::invalid_argument("id is not found");

	impl->enqueue_command(Command{LIGHT_OFF, id,
		[this](Command const& com, uint32_t answer)
		{
			if (answer == 0)
			{
				PPFX("some error in the node");
				return;
			}

			auto it = lightmap.find(com.id);
			if (it != lightmap.end())
				it->second->enabled = false;
			else
				PPFX("id is not found");
		},
		[on_error](Command const& com)
		{
			if (on_error)
				on_error();
		}
	});
}

bool LightController::IsEnabled(LightID const& id)
{
	// PPFX(id);

	auto it = lightmap.find(id);
	if (it == lightmap.end())
		throw std::invalid_argument("id is not found");

	return it->second->enabled;
}

bool LightController::IsDisabled(LightID const& id)
{
	// PPFX(id);

	return !IsEnabled(id);
}

void LightController::SetDetectedID(LightID const& id, uint32_t track_id)
{


	auto it = lightmap.find(id);

	if (it != lightmap.end())
	{
		PPFX("found light " << id << " new trid " << track_id);

		Light& l = *it->second;
		l.detected = true;
		l.trid = track_id;

		if (auto de = detector.lock())
		{
			auto p = de->getTrackPos(track_id);
			send_position_notification(id, p.x, p.y);
		}
	}
}

void LightController::resetLightInfo(Light & l)
{
	l.detected = false;
	l.trid = 0;
	l.enabled = false;
}

void LightController::trackMoved(TrackID id, double x, double y)
{
	auto it = std::find_if(lightmap.begin(), lightmap.end(),
		[&](decltype(lightmap)::value_type const& it) -> bool
		{
			Light const & l = *it.second;
			return l.detected && l.trid == id;
		});

	if (it != lightmap.end())
	{
		PPFX("light found id" << it->first);


		send_position_notification(it->first, x, y);
	}
	else
	{
		PPFX("ligth is not found");
	}
}

void LightController::send_position_notification(LightID const& id, double x, double y)
{
	static uint32_t number = 0;
	number++;

	static auto clamp = [](double v) -> uint8_t {
		if (v < 0.) return 0;
		if (v > 255.) return 255;
		return static_cast<uint8_t>(v);
	};

	impl->send_notification(Notification{id,
		number,
		clamp(x * 255.), clamp(y * 255.)
	});
}

void LightController::trackLost(TrackID id)
{
	auto it = std::find_if(lightmap.begin(), lightmap.end(),
		[&](decltype(lightmap)::value_type const& it) -> bool
		{
			Light const & l = *it.second;
			return l.detected && l.trid == id;
		});

	if (it != lightmap.end())
	{
		PPFX("light found id" << it->first);

		Light& l = *it->second;
		resetLightInfo(l);
	}
	else
	{
		PPFX("ligth is not found");
	}
}

void LightController::DetectionFailed(LightID const& id)
{
	PPFX(id);

	auto it = lightmap.find(id);
	if (it != lightmap.end())
	{
		PPFX("light found id" << it->first);

		Light& l = *it->second;
		resetLightInfo(l);
	}
	else
	{
		PPFX("ligth is not found");
	}
}


bool LightController::have_undetected() const
{
	auto it = std::find_if(lightmap.begin(), lightmap.end(),
		[&](decltype(lightmap)::value_type const& it) -> bool
		{
			Light const & l = *it.second;
			return !l.detected;
		});

	return it != lightmap.end();
}

LightID LightController::get_undetected() const
{
	static LightID last_undetected;

	auto cmp = [&](decltype(lightmap)::value_type const& it) -> bool
		{
			Light const & l = *it.second;
			return !l.detected;
		};

	auto ub = lightmap.upper_bound(last_undetected);

	auto it = std::find_if(ub, lightmap.end(), cmp);
	if (it == lightmap.end())
		it = std::find_if(lightmap.begin(), ub, cmp);

	if (it != lightmap.end())
	{
		last_undetected = it->first;
		return it->first;
	}
	else
	{
		last_undetected = LightID();
	}

	throw std::logic_error("there is no undetected lights");
}

void LightController::do_discovery()
{
	impl->do_discovery();
}


LightController::LightController() :
	impl(make_unique<Impl>(std::ref(*this)))
{
	//lightmap.insert(std::make_pair(LightID{0}, make_unique<Light>(Light{false, false})));
}

LightController::~LightController()
{

}




