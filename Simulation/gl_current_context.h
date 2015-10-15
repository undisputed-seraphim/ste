// StE
// � Shlomi Steinberg, 2015

#pragma once

#include "stdafx.h"
#include "gl_context.h"
#include "StEngineControl.h"

namespace StE {
namespace LLR {

class gl_current_context {
private:
	friend class gl_context;

private:
	static thread_local gl_context *current;

public:
	static gl_context *get() { return current; }
};

}
}
