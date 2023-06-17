#pragma once
#include "singleton.h"
#include "scripting/duktape-cpp/DuktapeCpp.h"

class c_scripting : public singleton<c_scripting>
{
public:
	void init();

protected:
	duk::Context cxt;
};

