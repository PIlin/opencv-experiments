#ifndef LIGHT_CONTROLLER_HPP__
#define LIGHT_CONTROLLER_HPP__

#include "StateController.hpp"

struct LightID
{
	uint32_t id;

	bool operator<(LightID const& o) const { return id < o.id; }


	template <typename ostream_T>
	friend ostream_T& operator<<(ostream_T& os, LightID const& lid)
	{
		os << lid.id;
		return os;
	}
};

class LightController : public LightControl
{
public:

	virtual void Enable(LightID const& id);
	virtual void Disable(LightID const& id);

	virtual bool IsEnabled(LightID const& id);
	virtual bool IsDisabled(LightID const& id);

	virtual void SetDetectedID(LightID const& id, uint32_t track_id);
	virtual void DetectionFailed(LightID const& id);

public:

	// LightController();

};


#endif  // LIGHT_CONTROLLER_HPP__