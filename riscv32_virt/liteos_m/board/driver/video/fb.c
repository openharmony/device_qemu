/****************************************************************************
 * video/fb.c
 * Framebuffer character driver
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "fb.h"

#include "errno.h"
#include "string.h"

#define gerr        PRINT_ERR
#define DEBUGASSERT LOS_ASSERT

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* This structure defines one framebuffer device.  Note that which is
 * everything in this structure is constant data set up and initialization
 * time.  Therefore, no there is requirement for serialized access to this
 * structure.
 */

struct fb_chardev_s {
    struct fb_vtable_s *vtable; /* Framebuffer interface */
    void *fbmem;                /* Start of frame buffer memory */
    size_t fblen;               /* Size of the framebuffer */
    uint8_t plane;              /* Video plan number */
    uint8_t bpp;                /* Bits per pixel */
};

#define FB_DEV_MAXNUM 32
static struct fb_chardev_s *g_fb_dev[FB_DEV_MAXNUM] = {NULL};

/****************************************************************************
 * Public Functions
 ****************************************************************************/
int fb_open(const char *key, struct fb_mem **result)
{
    struct fb_mem *fbmem = NULL;
    struct fb_chardev_s *fb;
    struct fb_vtable_s *vtable;
    int ret = -EINVAL;

    if (key == NULL || strlen(key) >= PATH_MAX) {
        return -EINVAL;
    }
    FbMemHold();
    ret = FbMemLookup(key, &fbmem, 0);
    FbMemDrop();
    if (ret == 0) {
        fb = (struct fb_chardev_s *)fbmem->data;
        if (fb == NULL) {
            return -ENODEV;
        }

        vtable = fb->vtable;
        if (vtable == NULL) {
            return -EINVAL;
        }

        if (vtable->fb_open) {
            ret = vtable->fb_open(vtable);
            if (ret == 0) {
                *result = fbmem;
            }
        }
    }
    return ret;
}

int fb_close(struct fb_mem *fbmem)
{
    struct fb_chardev_s *fb;
    struct fb_vtable_s *vtable;
    int ret = -EINVAL;

    fb = (struct fb_chardev_s *)fbmem->data;
    if (fb == NULL) {
        return -ENODEV;
    }

    vtable = fb->vtable;
    if (vtable == NULL) {
        return -EINVAL;
    }

    if (vtable->fb_release) {
        ret = vtable->fb_release(vtable);
    }
    return ret;
}

int fb_ioctl(struct fb_mem *fbMem, int cmd, unsigned long arg)
{
    struct fb_chardev_s *fb = NULL;
    int ret;

    /* Get the framebuffer instance */
    fb = (struct fb_chardev_s *)fbMem->data;
    /* Process the IOCTL command */

    switch (cmd) {
        case FIOC_MMAP:
            { /* Get color plane info */
                void **ppv = (void **)((uintptr_t)arg);
                /* Return the address corresponding to the start of frame buffer. */
                DEBUGASSERT(ppv != NULL);
                *ppv = fb->fbmem;
                ret = OK;
            }
            break;

        case FBIOGET_VIDEOINFO:
            { /* Get color plane info */
                struct fb_videoinfo_s *vinfo = (struct fb_videoinfo_s *)((uintptr_t)arg);
                DEBUGASSERT(fb->vtable != NULL && fb->vtable->getvideoinfo != NULL);
                ret = fb->vtable->getvideoinfo(fb->vtable, &vinfo);
            }
            break;

        case FBIOGET_PLANEINFO:
            { /* Get video plane info */
                struct fb_planeinfo_s *pinfo = (struct fb_planeinfo_s *)((uintptr_t)arg);
                DEBUGASSERT(fb->vtable != NULL && fb->vtable->getplaneinfo != NULL);
                ret = fb->vtable->getplaneinfo(fb->vtable, fb->plane, &pinfo);
            }
            break;

#ifdef CONFIG_FB_CMAP
        case FBIOGET_CMAP:
            { /* Get RGB color mapping */
                struct fb_cmap_s *cmap = (struct fb_cmap_s *)((uintptr_t)arg);
                DEBUGASSERT(fb->vtable != NULL && fb->vtable->getcmap != NULL);
                ret = fb->vtable->getcmap(fb->vtable, &cmap);
            }
            break;

        case FBIOPUT_CMAP:
            { /* Put RGB color mapping */
                struct fb_cmap_s *cmap = (struct fb_cmap_s *)((uintptr_t)arg);
                DEBUGASSERT(fb->vtable != NULL && fb->vtable->putcmap != NULL);
                ret = fb->vtable->putcmap(fb->vtable, &cmap);
            }
            break;
#endif
#ifdef CONFIG_FB_HWCURSOR
        case FBIOGET_CURSOR:
            { /* Get cursor attributes */
                struct fb_cursorattrib_s *attrib = (struct fb_cursorattrib_s *)((uintptr_t)arg);
                DEBUGASSERT(fb->vtable != NULL && fb->vtable->getcursor != NULL);
                ret = fb->vtable->getcursor(fb->vtable, &attrib);
            }
            break;

        case FBIOPUT_CURSOR:
            { /* Set cursor attibutes */
                struct fb_setcursor_s *cursor = (struct fb_setcursor_s *)((uintptr_t)arg);
                DEBUGASSERT(fb->vtable != NULL && fb->vtable->setcursor != NULL);
                ret = fb->vtable->setcursor(fb->vtable, &cursor);
            }
            break;
#endif

#ifdef CONFIG_FB_UPDATE
        case FBIO_UPDATE:
            { /* Update the modified framebuffer data  */
                struct fb_area_s *area = (struct fb_area_s *)((uintptr_t)arg);
                DEBUGASSERT(fb->vtable != NULL && fb->vtable->updatearea != NULL);
                ret = fb->vtable->updatearea(fb->vtable, area);
            }
            break;
#endif

#ifdef CONFIG_FB_SYNC
        case FBIO_WAITFORVSYNC:
            { /* Wait upon vertical sync */
                ret = fb->vtable->waitforvsync(fb->vtable);
            }
            break;
#endif

#ifdef CONFIG_FB_OVERLAY
        case FBIO_SELECT_OVERLAY:
            { /* Select video overlay */
                struct fb_overlayinfo_s oinfo;
                DEBUGASSERT(fb->vtable != NULL && fb->vtable->getoverlayinfo != NULL);
                ret = fb->vtable->getoverlayinfo(fb->vtable, arg, &oinfo);
                if (ret == OK) {
                    fb->fbmem = oinfo.fbmem;
                    fb->fblen = oinfo.fblen;
                    fb->bpp = oinfo.bpp;
                }
            }
            break;

        case FBIOGET_OVERLAYINFO:
            { /* Get video overlay info */
                struct fb_overlayinfo_s *oinfo = (struct fb_overlayinfo_s *)((uintptr_t)arg);
                DEBUGASSERT(fb->vtable != NULL && fb->vtable->getoverlayinfo != NULL);
                ret = fb->vtable->getoverlayinfo(fb->vtable, oinfo->overlay, &oinfo);
            }
            break;

        case FBIOSET_TRANSP:
            { /* Set video overlay transparency */
                struct fb_overlayinfo_s *oinfo = (struct fb_overlayinfo_s *)((uintptr_t)arg);
                DEBUGASSERT(fb->vtable != NULL && fb->vtable->settransp != NULL);
                ret = fb->vtable->settransp(fb->vtable, &oinfo);
            }
            break;

        case FBIOSET_CHROMAKEY:
            { /* Set video overlay chroma key */
                struct fb_overlayinfo_s *oinfo = (struct fb_overlayinfo_s *)((uintptr_t)arg);
                DEBUGASSERT(fb->vtable != NULL && fb->vtable->setchromakey != NULL);
                ret = fb->vtable->setchromakey(fb->vtable, &oinfo);
            }
            break;

        case FBIOSET_COLOR:
            { /* Set video overlay color */
                struct fb_overlayinfo_s *oinfo = (struct fb_overlayinfo_s *)((uintptr_t)arg);
                DEBUGASSERT(fb->vtable != NULL && fb->vtable->setcolor != NULL);
                ret = fb->vtable->setcolor(fb->vtable, &oinfo);
            }
            break;

        case FBIOSET_BLANK:
            { /* Blank or unblank video overlay */
                struct fb_overlayinfo_s *oinfo = (struct fb_overlayinfo_s *)((uintptr_t)arg);
                DEBUGASSERT(fb->vtable != NULL && fb->vtable->setblank != NULL);
                ret = fb->vtable->setblank(fb->vtable, &oinfo);
            }
            break;

        case FBIOSET_AREA:
            { /* Set active video overlay area */
                struct fb_overlayinfo_s *oinfo = (struct fb_overlayinfo_s *)((uintptr_t)arg);
                DEBUGASSERT(fb->vtable != NULL && fb->vtable->setarea != NULL);
                ret = fb->vtable->setarea(fb->vtable, &oinfo);
            }
            break;

#ifdef CONFIG_FB_OVERLAY_BLIT
        case FBIOSET_BLIT:
            { /* Blit operation between video overlays */
                struct fb_overlayblit_s *blit = (struct fb_overlayblit_s *)((uintptr_t)arg);
                DEBUGASSERT(fb->vtable != NULL && fb->vtable->blit != NULL);
                ret = fb->vtable->blit(fb->vtable, &blit);
            }
            break;

        case FBIOSET_BLEND:
            { /* Blend operation between video overlays */
                struct fb_overlayblend_s *blend = (struct fb_overlayblend_s *)((uintptr_t)arg);
                DEBUGASSERT(fb->vtable != NULL && fb->vtable->blend != NULL);
                ret = fb->vtable->blend(fb->vtable, &blend);
            }
            break;
#endif
#endif /* CONFIG_FB_OVERLAY */

        default:
            DEBUGASSERT(fb->vtable != NULL && fb->vtable->fb_ioctl != NULL);
            ret = fb->vtable->fb_ioctl(fb->vtable, cmd, arg);
            break;
    }

    return ret;
}

int getplaneinfo(struct fb_mem *fbmem, struct fb_planeinfo_s **result)
{
    int ret;
    struct fb_chardev_s *fb;

    fb = (struct fb_chardev_s *)fbmem->data;

    struct fb_planeinfo_s pinfo;

    ret = fb->vtable->getplaneinfo(fb->vtable, fb->plane, &pinfo);
    if (ret == 0) {
        *result = &pinfo;
    }

    return 0;
}

/****************************************************************************
 * Name: fb_register
 *
 * Description:
 *   Register the framebuffer character device at /dev/fbN where N is the
 *   display number if the devices supports only a single plane.  If the
 *   hardware supports multiple color planes, then the device will be
 *   registered at /dev/fbN.M where N is the again display number but M
 *   is the display plane.
 *
 * Input Parameters:
 *   display - The display number for the case of boards supporting multiple
 *             displays or for hardware that supports multiple
 *             layers (each layer is consider a display).  Typically zero.
 *   plane   - Identifies the color plane on hardware that supports separate
 *             framebuffer "planes" for each color component.
 *
 * Returned Value:
 *   Zero (OK) is returned success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

int fb_register(int display, int plane)
{
    struct fb_chardev_s *fb = NULL;
    struct fb_videoinfo_s vinfo;
    struct fb_planeinfo_s pinfo;
#ifdef CONFIG_FB_OVERLAY
    struct fb_overlayinfo_s oinfo;
#endif
    char devname[16];
    int nplanes;
    int ret;

    if (display < 0 || display >= FB_DEV_MAXNUM) return -EINVAL;

    /* Allocate a framebuffer state instance */
    fb = (struct fb_chardev_s *)malloc(sizeof(struct fb_chardev_s));
    if (fb == NULL) {
        return -ENOMEM;
    }

    /* Initialize the frame buffer device. */
    ret = up_fbinitialize(display);
    if (ret < 0) {
        gerr("ERROR: up_fbinitialize() failed for display %d: %d\n", display, ret);
        goto errout_with_fb;
    }

    DEBUGASSERT((unsigned)plane <= UINT8_MAX);
    fb->plane = plane;

    fb->vtable = up_fbgetvplane(display, plane);
    if (fb->vtable == NULL) {
        gerr("ERROR: up_fbgetvplane() failed, vplane=%d\n", plane);
        goto errout_with_fb;
    }

    /* Initialize the frame buffer instance. */
    DEBUGASSERT(fb->vtable->getvideoinfo != NULL);
    ret = fb->vtable->getvideoinfo(fb->vtable, &vinfo);
    if (ret < 0) {
        gerr("ERROR: getvideoinfo() failed: %d\n", ret);
        goto errout_with_fb;
    }

    nplanes = vinfo.nplanes;
    DEBUGASSERT(vinfo.nplanes > 0 && (unsigned)plane < vinfo.nplanes);

    DEBUGASSERT(fb->vtable->getplaneinfo != NULL);
    ret = fb->vtable->getplaneinfo(fb->vtable, plane, &pinfo);
    if (ret < 0) {
        gerr("ERROR: getplaneinfo() failed: %d\n", ret);
        goto errout_with_fb;
    }

    fb->fbmem = pinfo.fbmem;
    fb->fblen = pinfo.fblen;
    fb->bpp = pinfo.bpp;

    /* Clear the framebuffer memory */
    memset(pinfo.fbmem, 0, pinfo.fblen);

#ifdef CONFIG_FB_OVERLAY
    /* Initialize first overlay but do not select */
    DEBUGASSERT(fb->vtable->getoverlayinfo != NULL);
    ret = fb->vtable->getoverlayinfo(fb->vtable, 0, &oinfo);
    if (ret < 0) {
        gerr("ERROR: getoverlayinfo() failed: %d\n", ret);
        goto errout_with_fb;
    }

    /* Clear the overlay memory. Necessary when plane 0 and overlay 0
     * different.
     */

    memset(oinfo.fbmem, 0, oinfo.fblen);
#endif

    /* Register the framebuffer device */
    if (nplanes < 2) {
        (void)sprintf_s(devname, 16, "/dev/fb%d", display);
    } else {
        (void)sprintf_s(devname, 16, "/dev/fb%d.%d", display, plane);
    }

    ret = register_driver(devname, (void *)fb);
    if (ret < 0) {
        gerr("ERROR: register_driver() failed: %d\n", ret);
        goto errout_with_fb;
    }

    g_fb_dev[display] = fb;

    return OK;

errout_with_fb:
    free(fb);
    return ret;
}

int fb_unregister(int display)
{
    struct fb_chardev_s *fb = NULL;
    char devname[16];

    if (display < 0 || display >= FB_DEV_MAXNUM) return -EINVAL;

    (void)sprintf_s(devname, 16, "/dev/fb%d", display);
    unregister_driver(devname);

    up_fbuninitialize(display);

    fb = g_fb_dev[display];
    free(fb);
    g_fb_dev[display] = NULL;

    return 0;
}