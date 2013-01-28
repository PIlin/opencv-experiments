#include <iostream>

#define PPF() do { std::cout << __PRETTY_FUNCTION__ << std::endl; } while(0)
#define PPFX(x) do { std::cout << __PRETTY_FUNCTION__ << x << std::endl; } while(0)

#define STATE_ACTION(NAME) \
	BOOST_MSM_EUML_ACTION(NAME) { \
		template <class Evt,class Fsm,class State> \
		void operator()(Evt const& e, Fsm& fsm, State& )

#define ACTION(NAME) \
	BOOST_MSM_EUML_ACTION(NAME) { \
		template <class Evt,class Fsm,class SourceState, class TargetState> \
		void operator()(Evt const& e, Fsm& fsm, SourceState&, TargetState&)

ACTION(start_next_song)
{
PPF();
}};

ACTION(start_prev_song)
{
PPF();
}};


ACTION(start_playback)
{
PPF();
}};

ACTION(resume_playback)
{
PPF();
}};

ACTION(pause_playback)
{
PPF();
}};

ACTION(stop_playback)
{
PPF();
}};

ACTION(stop_and_open)
{
PPF();
}};


ACTION(open_drawer)
{
PPF();
}};


ACTION(close_drawer)
{
PPF();
}};


ACTION(store_cd_info)
{
PPF();
}};
