#pragma once // clang-format off

// all modules that inherit from IModule
// used like in the shutdown phase where we have to
// call a method on every single dependency type

#include "LogHandler.h"
#include "SafeLogger.h"

#include "EngineLocator.h"
#include "ObjectProvider.h"

#include "AIM.h"

#include "AsyncGate.h"
#include "Dispatch.h"
#include "PatchManager.h"
#include "TaskQueue.h"

#include "MutexGuard.h"
#include "CallFunction.h"
#include "ProcessEvent.h"
#include "ProcessInternal.h"

// clang-format on