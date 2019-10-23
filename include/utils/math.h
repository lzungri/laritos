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
#define sign_extend_32(_n, _bits) (int32_t) \
    (((_n) & (1 << ((_bits) - 1))) ? ((int32_t) (_n) - (1 << (_bits))) : (_n))

