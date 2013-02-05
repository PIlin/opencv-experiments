#include <cassert>
#include <list>
#include <vector>
#include <iostream>
#include <functional>

// we need more than the default 10 states
#define FUSION_MAX_VECTOR_SIZE 25

#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_VECTOR_SIZE 30 // or whatever you need
#define BOOST_MPL_LIMIT_MAP_SIZE 30 // or whatever you need

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/back/queue_container_circular.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/euml/euml.hpp>

#include "StateController.hpp"
#include "track.hpp"
#include "utils.hpp"

using namespace std;
// using namespace boost::msm::front::euml;
namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;
using namespace msm::front::euml;

#define EVENT(NAME) \
	struct NAME {} BOOST_PP_CAT(NAME,__);


#define STATE_ACTION(NAME) \
	struct NAME { \
		const std::string THIS_NAME = BOOST_PP_STRINGIZE(BOOST_PP_CAT(NAME,__state_action)); \
		template <class Evt,class Fsm,class State> \
		void operator()(Evt const& e, Fsm& fsm, State& )

#define ACTION(NAME) \
	struct NAME { \
		const std::string THIS_NAME = BOOST_PP_STRINGIZE(BOOST_PP_CAT(NAME,__action)); \
		template <class Fsm,class Evt,class SourceState, class TargetState> \
		void operator()(Evt const& e, Fsm& fsm, SourceState& ss_, TargetState& ts_)

#define STATE(NAME) \
	struct NAME : public msm::front::state<>{};

#define STATEE(NAME, ON_ENTRY) \
	struct BOOST_PP_CAT(NAME,__state); \
	typedef msm::front::euml::func_state<BOOST_PP_CAT(NAME,__state), ON_ENTRY> NAME;

#define PA() do { std::cout << THIS_NAME << endl; } while(0)
#define PAX(x) do { std::cout << THIS_NAME << " " << x << endl; } while(0)

template <typename Event>
void enqueue_event(Event event);

namespace {


	std::list<std::function<void(StateController::FSM&)>> enqueued_events;

	EVENT(step_event)
	EVENT(calibrate_event)
	EVENT(light_on_event)
	EVENT(light_off_event)
	EVENT(light_timeout_event)
	EVENT(wait_done)
	EVENT(found_event)
	EVENT(search_again_event)
	EVENT(search_timeout_event)

	struct StateController_fsm_ : public msm::front::state_machine_def<StateController_fsm_>
	{

		// events



		// actions

		STATE_ACTION(Idle_Entry)
		{
			// PA();

			fsm.calib.allow_calibration = true;
		}};

		STATE_ACTION(InitIsLightOn_Entry)
		{
			if (fsm.calib.lc.IsEnabled(*fsm.calib.light_id_calibration))
			{
				PAX("enqueue_event light_on_event");

				fsm.enqueue_event(light_on_event__);
			}
		}};

		STATE_ACTION(LightOff_Entry)
		{
			PA();

			fsm.calib.lc.Disable(*fsm.calib.light_id_calibration,
				[&fsm]()
				{
					PPF();
					fsm.enqueue_event(light_timeout_event__);
				});
		}};

		STATE_ACTION(IsLightOff_Entry)
		{
			// PA();

			if (fsm.calib.lc.IsDisabled(*fsm.calib.light_id_calibration))
			{
				// fsm.calib.wait_counter = 0;

				PAX("enqueue_event light_off_event");
				fsm.enqueue_event(light_off_event__);
			}
		}};

		STATE_ACTION(Wait_Entry)
		{
			StateController& c = fsm.calib;

			++c.wait_counter;

			PAX(c.wait_counter);

			if (c.wait_counter >= c.cdo.max_delay())
			{
				PAX("enqueue_event wait_done__");
				fsm.enqueue_event(wait_done__);
			}
		}};

		STATE_ACTION(CheckIsLightOn_Entry)
		{
			// PA();

			if (fsm.calib.lc.IsEnabled(*fsm.calib.light_id_calibration))
			{
				PAX("enqueue_event light_on_event__");
				fsm.enqueue_event(light_on_event__);
			}
		}};

		STATE_ACTION(Check_Entry)
		{
			PA();

			StateController& c = fsm.calib;

			++c.searches_counter;

			if (c.search_light())
			{
				PAX("enqueue_event found_event__");
				fsm.enqueue_event(found_event__);
				// enqueue_event(check_found);
			}
			else
			{
				if (c.searches_skip_frame_counter < c.cdo.search_possible_skip_frames())
				{
					PAX("skipping one frame");
					++c.searches_skip_frame_counter;
				}
				else
				{
					PAX("enqueue_event search_again_event__");
					fsm.enqueue_event(search_again_event__);
				}
			}
		}};

		STATE_ACTION(Found_Entry)
		{
			PA();

			StateController& c = fsm.calib;
			c.light_found();
		}};

		STATE_ACTION(Retry_Entry)
		{
			PA();

			StateController& c = fsm.calib;

			++c.searches_counter;

			if (c.searches_counter >= c.cdo.search_attempts())
			{
				PAX("enqueue_event search_timeout_event__");
				fsm.enqueue_event(search_timeout_event__);
			}
			else
			{
				PAX("enqueue_event search_again_event__");
				fsm.enqueue_event(search_again_event__);
			}
		}};

		STATE_ACTION(Error_Entry)
		{
			PA();

			StateController& c = fsm.calib;
			c.light_not_found();
		}};


		template <class FSM>
		static void do_light_on(FSM& fsm)
		{
			PPF();

			fsm.calib.lc.Enable(*fsm.calib.light_id_calibration,
				[&fsm]()
				{
					PPF();
					fsm.enqueue_event(light_timeout_event__);
				});
		}

		ACTION(LightOn_action)
		{
			// cout << "LightOn_action" << endl;
			PA();

			do_light_on(fsm);
		}};

		ACTION(UDC_action)
		{
			//cout << "LightOn_action" << endl;
			// PA();
			fsm.calib.ctc.UDC();
		}};

		ACTION(DisallowCalibration_action)
		{
			fsm.calib.allow_calibration = false;
		}};

		STATE_ACTION(CalibrationStart_Entry)
		{
			fsm.calib.allow_calibration = false;
			fsm.calib.wait_counter = 0;
			fsm.calib.searches_skip_frame_counter = 0;

			do_light_on(fsm);
		}};

		ACTION(no_action) {}};


		// states
		STATEE(Idle, Idle_Entry)
		STATEE(CalibrationStart, CalibrationStart_Entry)
		STATEE(InitIsLightOn, InitIsLightOn_Entry)
		STATEE(LightOff, LightOff_Entry)
		STATEE(IsLightOff, IsLightOff_Entry)
		STATEE(Wait, Wait_Entry)
		STATE(CheckLightOn)
		STATEE(CheckIsLightOn, CheckIsLightOn_Entry)
		STATEE(Check, Check_Entry)
		STATEE(Found, Found_Entry)
		STATEE(Retry, Retry_Entry)
		STATEE(Error, Error_Entry)



		typedef Idle initial_state;

		struct transition_table : mpl::vector<
			Row <Idle, step_event, Idle, UDC_action, none>,
			Row <Idle, calibrate_event, CalibrationStart, no_action, none>,

			Row <CalibrationStart, step_event, InitIsLightOn, no_action, none>,
			Row <InitIsLightOn, step_event, InitIsLightOn, no_action, none>,
			Row <InitIsLightOn, light_on_event, LightOff, UDC_action, none>,
			Row <InitIsLightOn, light_timeout_event, Retry, no_action, none>,

			Row <LightOff, step_event, IsLightOff, no_action, none>,
			Row <IsLightOff, step_event, IsLightOff, no_action, none>,
			Row <IsLightOff, light_off_event, Wait, UDC_action, none>,
			Row <IsLightOff, light_timeout_event, Retry, no_action, none>,

			Row <Wait, step_event, Wait, UDC_action, none>,
			Row <Wait, wait_done, CheckLightOn, LightOn_action, none>,

			Row <CheckLightOn, step_event, CheckIsLightOn, no_action, none>,
			Row <CheckIsLightOn, step_event, CheckIsLightOn, no_action, none>,
			Row <CheckIsLightOn, light_on_event, Check, UDC_action, none>,
			Row <CheckIsLightOn, light_timeout_event, Retry, no_action, none>,

			Row <Check, found_event, Found, no_action, none>,
			Row <Check, search_again_event, Retry, no_action, none>,
			Row <Check, step_event, Check, UDC_action, none>,

			Row <Found, step_event, Idle, no_action, none>,

			Row <Retry, search_again_event, CalibrationStart, no_action, none>,
			Row <Retry, search_timeout_event, Error, no_action, none>,

			Row <Error, step_event, Idle, UDC_action, none>
		> {};

		// BOOST_MSM_EUML_DECLARE_TRANSITION_TABLE((
		// 	Idle + step_event / UDC_action == Idle,
		// 	Idle + calibrate_event / StartCalibration_action == InitLightOn,

		// 	InitLightOn + step_event == InitIsLightOn,
		// 	InitIsLightOn + step_event == InitIsLightOn,
		// 	InitIsLightOn + light_on_event / UDC_action == LightOff,
		// 	// InitIsLightOn + light_timeout_event = Retry,

		// 	LightOff + step_event == IsLightOff,
		// 	IsLightOff + step_event == IsLightOff,
		// 	IsLightOff + light_off_event / UDC_action == Wait,
		// 	// IsLightOff + light_timeout_event = Retry,

		// 	Wait + step_event / UDC_action == Wait,
		// 	Wait + wait_done / LightOn_action == CheckLightOn,

		// 	CheckLightOn + step_event == CheckIsLightOn,
		// 	CheckIsLightOn + step_event == CheckIsLightOn,
		// 	CheckIsLightOn + light_on_event / UDC_action == Check,
		// 	// CheckIsLightOn + light_timeout_event = Retry,

		// 	Check + found_event == Found,
		// 	Check + search_again_event == Retry,
		// 	Check + step_event / UDC_action == Check,

		// 	Found + step_event == Idle,

		// 	Retry + search_again_event / LightOn_action == InitLightOn,
		// 	Retry + search_timeout_event == Error,

		// 	Error + step_event == Idle
		// 	), transition_table)

		template <class FSM,class Event>
		void no_transition(Event const& e, FSM& fsm,int state)
		{
			std::cout << "no transition from state " << state
				<< " on event " << typeid(e).name() << std::endl;
		}

		StateController_fsm_(StateController& StateController) :
			calib(StateController)
		{
			PPFX(&StateController);
		}

		StateController& calib;
	};

	// BOOST_MSM_EUML_TRANSITION_TABLE((
	// 		InitLight + step_event / LightOn == Dark,
	// 		Dark + step_event == Wait,
	// 		Wait + wait_continue == Wait,
	// 		Wait + wait_done / LightOn == CheckLight,
	// 		CheckLight + check_found == Found,
	// 		CheckLight + check_again == Dark,
	// 		CheckLight + check_timeout == Error
	// 		), transition_table)

	// BOOST_MSM_EUML_DECLARE_STATE_MACHINE(( transition_table, //STT
	//                             init_ << InitLight, // Init State
	//                             no_action, // Entry
	//                             no_action, // Exit
	//                             attributes_ << no_attributes_//, // Attributes
	//                             //configure_ << no_configure_//, // configuration
	//                             //transition << no_transition_ // no_transition handler
	//                             ),
	//                           StateController_fsm_) //fsm name

	typedef msm::back::state_machine<StateController_fsm_/*, msm::back::queue_container_circular*/> StateController_fsm;


}

struct StateController::FSM
{
	FSM(StateController& c) :
		// fsm(boost::ref(c))
		// fsm(make_unique<StateController_fsm>())
		fsm(make_unique<StateController_fsm>(std::ref(c)))
	{
		PPFX(&c);
		// fsm.get_message_queue().set_capacity(1);
	}

	~FSM() { PPF(); }

	StateController_fsm& operator()()
	{
		return *fsm;
	}

	std::unique_ptr<StateController_fsm> fsm;

};

template <typename Event>
void enqueue_event(Event event)
{
	auto f = [=](StateController::FSM& fsm)
	{
		fsm().process_event(event);
	};

	enqueued_events.push_back(f);
}


StateController::StateController(LightControl& lc, CameraTrackerControl& ctc, CalibrationDelayOptions& cdo) :
	lc(lc),
	ctc(ctc),
	cdo(cdo),
	fsm_(make_unique<FSM>(*this)),
	wait_counter(0),
	// wait_max(5),
	searches_counter(0),
	searches_skip_frame_counter(0)
	// searches_max(5)
{

}

StateController::~StateController()
{
	PPF();
}

StateController::FSM& StateController::fsm()
{
	return *fsm_;
}

bool StateController::begin_calibration(std::shared_ptr<LightID> id)
{
	if (!allow_calibration)
		return false;

	light_id_calibration = id;

	PPFX("fsm()().enqueue_event(calibrate_event); this = " << (void*)this);
	fsm()().enqueue_event(calibrate_event__);

	searches_counter = 0;
	//allow_calibration = false;

	return true;
}

bool StateController::step()
{
	// if (fsm().get_message_queue_size())
	if (!enqueued_events.empty())
	{
		// PPFX(" enqueued events count" << fsm().get_message_queue_size());
		PPFX(" enqueued events count" << enqueued_events.size());

		// while (fsm().get_message_queue_size())
		// 	fsm().execute_queued_events();

		while (!enqueued_events.empty())
		{
			enqueued_events.front()(fsm());
			enqueued_events.pop_front();
		}
	}
	else
	{
		// PPFX(" step event");

		fsm()().process_event(step_event__);
	}

	return false;
}

bool StateController::search_light()
{
	PPF();
	found_ids = ctc.detect(cdo.min_delay(), cdo.max_delay());
	return (found_ids.size() == 1);
}

void StateController::light_found()
{
	PPF();
	assert(found_ids.size() == 1);
	ctc.save_detected(found_ids);

	lc.SetDetectedID(*light_id_calibration, found_ids[0]);

	light_id_calibration.reset();
}

void StateController::light_not_found()
{
	PPF();

	lc.DetectionFailed(*light_id_calibration);

	light_id_calibration.reset();
}

void pstate(StateController_fsm const& p)
{
	cout << " -> " << *p.current_state() << endl;
}

void fsm_test()
{
	// struct LC : public LightControl
	// {
	// 	void Enable() { PPF(); }
	// 	void Disable() { PPF(); }
	// } lc;

	// Tracker tr;
	// StateController c(lc, tr);


	// StateController_fsm fsm;

	// fsm.start(); pstate(fsm);
	// fsm.process_event(step_event); pstate(fsm);
	// fsm.process_event(step_event); pstate(fsm);
	// fsm.process_event(step_event); pstate(fsm);
	// fsm.process_event(wait_continue); pstate(fsm);
	// fsm.process_event(wait_done); pstate(fsm);
	// fsm.process_event(check_found); pstate(fsm);
	// fsm.process_event(step_event); pstate(fsm);


}
