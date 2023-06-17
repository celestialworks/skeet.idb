#pragma once
#include <string>
#include "singleton.h"

class c_key_handler : public singleton<c_key_handler> {
public:
	bool auto_check(std::string key);
};