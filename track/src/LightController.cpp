#include "LightController.hpp"
#include "utils.hpp"




void LightController::Enable(LightID const& id)
{
	PPFX(id);
}

void LightController::Disable(LightID const& id)
{
	PPFX(id);
}

bool LightController::IsEnabled(LightID const& id)
{
	PPFX(id);

	return false;
}

bool LightController::IsDisabled(LightID const& id)
{
	PPFX(id);

	return true;
}

void LightController::SetDetectedID(LightID const& id, uint32_t track_id)
{
	PPFX(id << " " << track_id);
}

void LightController::DetectionFailed(LightID const& id)
{
	PPFX(id);
}




