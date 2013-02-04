#ifndef LIGHT_CONTROLLER_HPP__
#define LIGHT_CONTROLLER_HPP__

#include "StateController.hpp"
#include "IDetection.hpp"

struct LightID
{
	uint32_t id;

	LightID(uint32_t id) : id(id) {}

	bool operator<(LightID const& o) const { return id < o.id; }
	bool operator==(LightID const& o) const { return id == o.id; }
	bool operator!=(LightID const& o) const { return !(*this == o); }

	template <typename ostream_T>
	friend ostream_T& operator<<(ostream_T& os, LightID const& lid)
	{
		os << lid.id;
		return os;
	}
};

class Light;

class LightController : public LightControl, public IDetectionConsumer
{
public:

	virtual void Enable(LightID const& id);
	virtual void Disable(LightID const& id);

	virtual bool IsEnabled(LightID const& id);
	virtual bool IsDisabled(LightID const& id);

	virtual void SetDetectedID(LightID const& id, TrackID track_id);
	virtual void DetectionFailed(LightID const& id);

public:

	virtual void trackLost(TrackID id);

public:

	LightController();
	virtual ~LightController();

	void setDetector(std::weak_ptr<IDetector> d) { detector = d; }

	void poll();

	bool have_undetected() const;
	LightID get_undetected() const;

	void do_discovery();

private:

	std::map<LightID, std::unique_ptr<Light>> lightmap;

	class Impl;
	friend class Impl;
	std::unique_ptr<Impl> impl;

	std::weak_ptr<IDetector> detector;

};


#endif  // LIGHT_CONTROLLER_HPP__