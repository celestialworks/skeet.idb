#pragma once
#include "singleton.h"

class c_bones : public singleton<c_bones> {

public:
	bool m_running;

public:
	bool setup( Player* player, BoneArray* out, LagRecord* record );
	bool BuildBones( Player* target, int mask, BoneArray* out, LagRecord* record );
};