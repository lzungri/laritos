#pragma once

#define min(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#define max(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define abs(_n) ((_n) < 0 ? -(_n) : (_n))
#define sign(_n) ((_n) < 0 ? -1 : 1)
