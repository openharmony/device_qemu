#include "hdf_log.h"
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "device_resource_if.h"
#include "los_vm_map.h"
#include "los_mmu_descriptor_v6.h"
#include "cfiflash.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

static struct block_operations g_cfiBlkops = {
    NULL,           /* int     (*open)(FAR struct inode *inode); */
    NULL,           /* int     (*close)(FAR struct inode *inode); */
    CfiBlkRead,
    CfiBlkWrite,
    CfiBlkGeometry,
    NULL,           /* int     (*ioctl)(FAR struct inode *inode, int cmd, unsigned long arg); */
    NULL,           /* int     (*unlink)(FAR struct inode *inode); */
};

struct block_operations *GetCfiBlkOps()
{
    return &g_cfiBlkops;
}

static LosVmMapRegion *HdfCfiMapInit(const struct DeviceResourceNode *node)
{
    int ret, count;
    uint32_t pbase, len;
    struct DeviceResourceIface *p = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);

    if ((ret = p->GetUint32(node, "pbase", &pbase, 0))) {
        HDF_LOGE("[%s]GetUint32 error:%d", __func__, ret);
        return NULL;
    }
    if ((ret = p->GetUint32(node, "vbase", (uint32_t *)&g_cfiFlashBase, 0))) {
        HDF_LOGE("[%s]GetUint32 error:%d", __func__, ret);
        return NULL;
    }
    if ((ret = p->GetUint32(node, "size", &len, 0))) {
        HDF_LOGE("[%s]GetUint32 error:%d", __func__, ret);
        return NULL;
    }

    count = (len + MMU_DESCRIPTOR_L2_SMALL_SIZE - 1) >> MMU_DESCRIPTOR_L2_SMALL_SHIFT;
    ret = LOS_ArchMmuMap(&LOS_GetKVmSpace()->archMmu, (VADDR_T)g_cfiFlashBase, pbase, count,
                        VM_MAP_REGION_FLAG_PERM_READ | VM_MAP_REGION_FLAG_PERM_WRITE);
    if (ret != count) {
        HDF_LOGE("[%s]LOS_ArchMmuMap error:%d", __func__, ret);
        return NULL;
    }
    LosVmMapRegion *r;
    r = LOS_RegionAlloc(LOS_GetKVmSpace(), (VADDR_T)g_cfiFlashBase, len, VM_MAP_REGION_FLAG_PERM_READ |
                        VM_MAP_REGION_FLAG_PERM_WRITE | VM_MAP_REGION_TYPE_DEV, 0);
    if (r == NULL) {
        HDF_LOGE("[%s]LOS_RegionAlloc error", __func__);
        LOS_ArchMmuUnmap(&LOS_GetKVmSpace()->archMmu, (VADDR_T)g_cfiFlashBase, count);
        return NULL;
    }

    return r;
}

int HdfCfiDriverInit(struct HdfDeviceObject *deviceObject)
{
    LosVmMapRegion *r;

    if (deviceObject == NULL ||  deviceObject->property == NULL) {
        HDF_LOGE("[%s]deviceObject or property is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    r = HdfCfiMapInit(deviceObject->property);
    if (r == NULL) {
        return HDF_FAILURE;
    }

    if(CfiFlashInit()) {
        LOS_RegionFree(LOS_GetKVmSpace(), r);
        return HDF_ERR_NOT_SUPPORT;
    }

    return HDF_SUCCESS;
}

struct HdfDriverEntry g_cfiDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "cfi_flash_driver",
    .Bind = NULL,
    .Init = HdfCfiDriverInit,
    .Release = NULL,
};

HDF_INIT(g_cfiDriverEntry);

static struct MtdDev g_cfiMtdDev = {
    .priv = NULL,
    .type = 3,  // MTD_NORFLASH
    .size = CFIFLASH_CAPACITY,
    .eraseSize = CFIFLASH_ERASEBLK_SIZE,
    .erase = CfiMtdErase,
    .read = CfiMtdRead,
    .write = CfiMtdWrite,
};

struct MtdDev *GetCfiMtdDev()
{
    return &g_cfiMtdDev;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
