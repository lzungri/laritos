#pragma once

/**
 * Based on byteswap header from linux source
 */

#include <stdint.h>
#include <generated/autoconf.h>

#define bswap16(x) \
    ({ \
        uint16_t __x = (uint16_t) (x); \
        ((((__x) & 0xff00) >> 8) | \
         (((__x) & 0x00ff) <<  8)); \
    })

#define bswap32(x) \
    ({ \
        uint32_t __x = (uint32_t) (x); \
         ((((__x) & 0xff000000) >> 24) | \
          (((__x) & 0x00ff0000) >>  8) | \
          (((__x) & 0x0000ff00) <<  8) | \
          (((__x) & 0x000000ff) << 24)); \
    })

#define bswap64(x) \
    ({ \
        uint32_t __x = (uint32_t) (x); \
        ((((__x) & 0xff00000000000000ull) >> 56) | \
         (((__x) & 0x00ff000000000000ull) >> 40) | \
         (((__x) & 0x0000ff0000000000ull) >> 24) | \
         (((__x) & 0x000000ff00000000ull) >> 8)  | \
         (((__x) & 0x00000000ff000000ull) << 8)  | \
         (((__x) & 0x0000000000ff0000ull) << 24) | \
         (((__x) & 0x000000000000ff00ull) << 40) | \
         (((__x) & 0x00000000000000ffull) << 56)); \
    })


#ifdef CONFIG_CPU_LITTLE_ENDIAN
#define le16_to_cpu(val) (val)
#define le32_to_cpu(val) (val)
#define le64_to_cpu(val) (val)
#define be16_to_cpu(val) bswap16(val)
#define be32_to_cpu(val) bswap32(val)
#define be64_to_cpu(val) bswap64(val)

#define cpu_to_le16(val) (val)
#define cpu_to_le32(val) (val)
#define cpu_to_le64(val) (val)
#define cpu_to_be16(val) bswap16(val)
#define cpu_to_be32(val) bswap32(val)
#define cpu_to_be64(val) bswap64(val)

#else

#define le16_to_cpu(val) bswap16(val)
#define le32_to_cpu(val) bswap32(val)
#define le64_to_cpu(val) bswap64(val)
#define be16_to_cpu(val) (val)
#define be32_to_cpu(val) (val)
#define be64_to_cpu(val) (val)

#define cpu_to_le16(val) bswap16(val)
#define cpu_to_le32(val) bswap32(val)
#define cpu_to_le64(val) bswap64(val)
#define cpu_to_be16(val) (val)
#define cpu_to_be32(val) (val)
#define cpu_to_be64(val) (val)
#endif
