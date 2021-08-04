#ifndef __MMZ_H__
#define __MMZ_H__

#include "los_typedef.h"
#include "sys/ioctl.h"

typedef enum {
    MMZ_CACHE = 1,                  /* allocate mmz with cache attribute */
    MMZ_NOCACHE,                    /* allocate mmz with nocache attribute */
    MMZ_FREE,                       /* free mmz */
    MAP_CACHE,
    MAP_NOCACHE,
    UNMAP,
    FLUSH_CACHE,
    FLUSH_NOCACHE,
    INVALIDATE,
    MMZ_MAX
} MMZ_TYPE;

typedef struct {
    void *vaddr;
    uint64_t paddr;
    int32_t size;
} MmzMemory;

#define MMZ_IOC_MAGIC               'M'
#define MMZ_CACHE_TYPE              _IOR(MMZ_IOC_MAGIC, MMZ_CACHE, MmzMemory)
#define MMZ_NOCACHE_TYPE            _IOR(MMZ_IOC_MAGIC, MMZ_NOCACHE, MmzMemory)
#define MMZ_FREE_TYPE               _IOR(MMZ_IOC_MAGIC, MMZ_FREE, MmzMemory)
#define MMZ_MAP_CACHE_TYPE          _IOR(MMZ_IOC_MAGIC, MAP_CACHE, MmzMemory)
#define MMZ_MAP_NOCACHE_TYPE        _IOR(MMZ_IOC_MAGIC, MAP_NOCACHE, MmzMemory)
#define MMZ_UNMAP_TYPE              _IOR(MMZ_IOC_MAGIC, UNMAP, MmzMemory)
#define MMZ_FLUSH_CACHE_TYPE        _IOR(MMZ_IOC_MAGIC, FLUSH_CACHE, MmzMemory)
#define MMZ_FLUSH_NOCACHE_TYPE      _IOR(MMZ_IOC_MAGIC, FLUSH_NOCACHE, MmzMemory)
#define MMZ_INVALIDATE_TYPE         _IOR(MMZ_IOC_MAGIC, INVALIDATE, MmzMemory)

#define MMZ_NODE                    "/dev/mmz"

int DevMmzRegister(void);

#endif
