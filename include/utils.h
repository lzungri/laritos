#pragma once

#define STRINGIFY(_x) #_x
#define TOSTRING(x) STRINGIFY(x)

#define ARRAYSIZE(_a) (sizeof(a) / sizeof(a[0]))
