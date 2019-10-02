#pragma once

#define MSEC_PER_SEC    1000L
#define USEC_PER_SEC    1000000L
#define USEC_PER_MSEC   1000L
#define NSEC_PER_SEC    1000000000L
#define NSEC_PER_MSEC   1000000L
#define NSEC_PER_USEC   1000L

#define SEC_TO_MS(_s) ((_s) * MSEC_PER_SEC)
#define SEC_TO_US(_s) ((_s) * USEC_PER_SEC)
#define SEC_TO_NS(_s) ((_s) * NSEC_PER_SEC)

#define MS_TO_SEC(_ms) ((_ms) / MSEC_PER_SEC)
#define MS_TO_US(_ms) ((_ms) * USEC_PER_MSEC)
#define MS_TO_NS(_ms) ((_ms) * NSEC_PER_MSEC)

#define US_TO_MS(_us) ((_us) / USEC_PER_MSEC)
#define US_TO_NS(_us) ((_us) * NSEC_PER_USEC)

#define NS_TO_SEC(_ns) ((_ns) / NSEC_PER_SEC)
#define NS_TO_MS(_ns) ((_ns) / NSEC_PER_MSEC)
#define NS_TO_US(_ns) ((_ns) / NSEC_PER_USEC)
