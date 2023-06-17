#pragma once
#include "singleton.h"
#include "includes.h"

class c_legitbot : public singleton<c_legitbot>
{
	Player* calcTarget();
	void Aim();
};