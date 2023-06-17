#pragma once
#include "singleton.h"

class c_movement : public singleton<c_movement> {
public:
	void JumpRelated();
	void Strafe();
	void FixMove(CUserCmd* cmd, ang_t old_angles);
	void FastStop();
	void AutoStop();
	void ClampMovementSpeed(float speed);
	void SlowMotion();
};