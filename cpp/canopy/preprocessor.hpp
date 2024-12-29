#pragma once

namespace silva {
#define SILVA_USED __attribute__((__used__))

#define STRINGIFY(x)            #x
#define TOSTRING(x)             STRINGIFY(x)
#define SILVA_CPP_LOCATION      __FILE_NAME__ ":" TOSTRING(__LINE__) ":" TOSTRING(__COUNTER__)
#define SILVA_CPP_LOCATION_LONG __FILE__ ":" TOSTRING(__LINE__) ":" TOSTRING(__COUNTER__)
}
