#include <vector>
#include <iostream>

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/euml/euml.hpp>


using namespace std;
using namespace boost::msm::front::euml;
namespace msm = boost::msm;


#define STATE_ACTION(NAME) \
	BOOST_MSM_EUML_ACTION(NAME) { \
		template <class Evt,class Fsm,class State> \
		void operator()(Evt const& ,Fsm& ,State& )

#define ACTION(NAME) \
	BOOST_MSM_EUML_ACTION(NAME) { \
		template <class Evt,class Fsm,class SourceState, class TargetState> \
		void operator()(Evt const& ,Fsm& ,SourceState&, TargetState&)


namespace {

	// events
	BOOST_MSM_EUML_EVENT(dummy_event)
	BOOST_MSM_EUML_EVENT(wait_continue)
	BOOST_MSM_EUML_EVENT(wait_done)
	BOOST_MSM_EUML_EVENT(check_found)
	BOOST_MSM_EUML_EVENT(check_timeout)
	BOOST_MSM_EUML_EVENT(check_again)


	// actions
	STATE_ACTION(Dark_Entry)
	{

	}};

	STATE_ACTION(Wait_Entry)
	{

	}};

	STATE_ACTION(CheckLight_Entry)
	{

	}};

	STATE_ACTION(Found_Entry)
	{

	}};

	ACTION(LightOn)
	{

	}};



	// states
	BOOST_MSM_EUML_STATE((),InitLight)
	BOOST_MSM_EUML_STATE((Dark_Entry),Dark)
	BOOST_MSM_EUML_STATE((Wait_Entry),Wait)
	BOOST_MSM_EUML_STATE((CheckLight_Entry),CheckLight)
	BOOST_MSM_EUML_STATE((Found_Entry),Found)
	BOOST_MSM_EUML_STATE((),Error)

	BOOST_MSM_EUML_TRANSITION_TABLE((
		InitLight + dummy_event / LightOn == Dark,
		Dark + dummy_event == Wait,
		Wait + wait_continue == Wait,
		Wait + wait_done / LightOn == CheckLight,
		CheckLight + check_found == Found,
		CheckLight + check_again == Dark,
		CheckLight + check_timeout == Error
		), transition_table)

	BOOST_MSM_EUML_DECLARE_STATE_MACHINE(( transition_table, //STT
	                            init_ << InitLight, // Init State
	                            no_action, // Entry
	                            no_action, // Exit
	                            attributes_ << no_attributes_//, // Attributes
	                            //configure_ << no_configure_//, // configuration
	                            //transition << no_transition_ // no_transition handler
	                            ),
	                          calibrator_fsm_) //fsm name

	typedef msm::back::state_machine<calibrator_fsm_> calibrator_fsm;
}

void pstate(calibrator_fsm const& p)
{
	cout << " -> " << p.current_state() << endl;
}

void fsm_test()
{
	calibrator_fsm fsm;

	fsm.start();
	fsm.process_event(dummy_event); pstate(fsm);
}