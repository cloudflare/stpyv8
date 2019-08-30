#pragma once

#undef COMPILER
#undef TRUE
#undef FALSE

#include "src/v8.h"

#include "src/bootstrapper.h"
#include "src/natives.h"
#include "src/platform.h"
#include "src/scopes.h"

#include "src/debug.h"

#include "src/serialize.h"
#include "src/stub-cache.h"
#include "src/heap.h"

#include "src/parser.h"
#include "src/compiler.h"
#include "src/scanner.h"

#include "src/api.h"

namespace v8i = v8::internal;
