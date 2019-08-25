#pragma once

#define STRINGIFY(_x) #_x
#define TOSTRING(x) STRINGIFY(x)

#define ARRAYSIZE(_a) (sizeof(_a) / sizeof(_a[0]))
