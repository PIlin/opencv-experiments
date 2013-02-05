#ifndef LIGHT_CONTROLLER_HPP__
#define LIGHT_CONTROLLER_HPP__

#include <functional>

#include "StateController.hpp"
#include "IDetection.hpp"

struct LightID
{
	uint32_t msb;
	uint32_t lsb;

	LightID(uint32_t msb, uint32_t lsb) : msb(msb), lsb(lsb) {}

	bool operator<(LightID const& o) const { return std::make_pair(msb, lsb) < std::make_pair(o.msb, o.lsb); }
	bool operator==(LightID const& o) const { return std::make_pair(msb, lsb) == std::make_pair(o.msb, o.lsb); }
	bool operator!=(LightID const& o) const { return !(*this == o); }

	template <typename ostream_T>
	friend ostream_T& operator<<(ostream_T& os, LightID const& lid)
	{
		os << std::hex << lid.msb << " " << lid.lsb << std::dec;
		return os;
	}
};

class Light;

class LightController : public LightControl, public IDetectionConsumer
{
public:

	virtual void Enable(LightID const& id, std::function<void()> on_error);
	virtual void Disable(LightID const& id, std::function<void()> on_error);

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

	bool have_undetected() const;
	LightID get_undetected() const;

	void do_discovery();

private:

	std::map<LightID, std::unique_ptr<Light>> lightmap;

	class Impl;
	friend class Impl;
	std::unique_ptr<Impl> impl;

	std::weak_ptr<IDetector> detector;

	void resetLightInfo(Light & l);

};


#endif  // LIGHT_CONTROLLER_HPP__