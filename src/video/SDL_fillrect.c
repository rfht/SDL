/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "SDL_internal.h"

#include "SDL_blit.h"
#include "../SDL_intrin.h"

#ifdef __SSE__
/* *INDENT-OFF* */ /* clang-format off */

#if defined(_MSC_VER) && !defined(__clang__)
#define SSE_BEGIN \
    __m128 c128; \
    c128.m128_u32[0] = color; \
    c128.m128_u32[1] = color; \
    c128.m128_u32[2] = color; \
    c128.m128_u32[3] = color;
#else
#define SSE_BEGIN \
    __m128 c128; \
    DECLARE_ALIGNED(Uint32, cccc[4], 16); \
    cccc[0] = color; \
    cccc[1] = color; \
    cccc[2] = color; \
    cccc[3] = color; \
    c128 = *(__m128 *)cccc;
#endif

#define SSE_WORK \
    for (i = n / 64; i--;) { \
        _mm_stream_ps((float *)(p+0), c128); \
        _mm_stream_ps((float *)(p+16), c128); \
        _mm_stream_ps((float *)(p+32), c128); \
        _mm_stream_ps((float *)(p+48), c128); \
        p += 64; \
    }

#define SSE_END

#define DEFINE_SSE_FILLRECT(bpp, type) \
static void SDL_FillSurfaceRect##bpp##SSE(Uint8 *pixels, int pitch, Uint32 color, int w, int h) \
{ \
    int i, n; \
    Uint8 *p = NULL; \
 \
    SSE_BEGIN; \
 \
    while (h--) { \
        n = (w) * (bpp); \
        p = pixels; \
 \
        if (n > 63) { \
            int adjust = 16 - ((uintptr_t)p & 15); \
            if (adjust < 16) { \
                n -= adjust; \
                adjust /= (bpp); \
                while (adjust--) { \
                    *((type *)p) = (type)color; \
                    p += (bpp); \
                } \
            } \
            SSE_WORK; \
        } \
        if (n & 63) { \
            int remainder = (n & 63); \
            remainder /= (bpp); \
            while (remainder--) { \
                *((type *)p) = (type)color; \
                p += (bpp); \
            } \
        } \
        pixels += pitch; \
    } \
 \
    SSE_END; \
}

static void SDL_FillSurfaceRect1SSE(Uint8 *pixels, int pitch, Uint32 color, int w, int h)
{
    int i, n;

    SSE_BEGIN;
    while (h--) {
        Uint8 *p = pixels;
        n = w;

        if (n > 63) {
            int adjust = 16 - ((uintptr_t)p & 15);
            if (adjust) {
                n -= adjust;
                SDL_memset(p, color, adjust);
                p += adjust;
            }
            SSE_WORK;
        }
        if (n & 63) {
            int remainder = (n & 63);
            SDL_memset(p, color, remainder);
        }
        pixels += pitch;
    }

    SSE_END;
}
/* DEFINE_SSE_FILLRECT(1, Uint8) */
DEFINE_SSE_FILLRECT(2, Uint16)
DEFINE_SSE_FILLRECT(4, Uint32)

/* *INDENT-ON* */ /* clang-format on */
#endif            /* __SSE__ */

static void SDL_FillSurfaceRect1(Uint8 *pixels, int pitch, Uint32 color, int w, int h)
{
    int n;
    Uint8 *p = NULL;

    while (h--) {
        n = w;
        p = pixels;

        if (n > 3) {
            switch ((uintptr_t)p & 3) {
            case 1:
                *p++ = (Uint8)color;
                --n;
                SDL_FALLTHROUGH;
            case 2:
                *p++ = (Uint8)color;
                --n;
                SDL_FALLTHROUGH;
            case 3:
                *p++ = (Uint8)color;
                --n;
            }
            SDL_memset4(p, color, (n >> 2));
        }
        if (n & 3) {
            p += (n & ~3);
            switch (n & 3) {
            case 3:
                *p++ = (Uint8)color;
                SDL_FALLTHROUGH;
            case 2:
                *p++ = (Uint8)color;
                SDL_FALLTHROUGH;
            case 1:
                *p++ = (Uint8)color;
            }
        }
        pixels += pitch;
    }
}

static void SDL_FillSurfaceRect2(Uint8 *pixels, int pitch, Uint32 color, int w, int h)
{
    int n;
    Uint16 *p = NULL;

    while (h--) {
        n = w;
        p = (Uint16 *)pixels;

        if (n > 1) {
            if ((uintptr_t)p & 2) {
                *p++ = (Uint16)color;
                --n;
            }
            SDL_memset4(p, color, (n >> 1));
        }
        if (n & 1) {
            p[n - 1] = (Uint16)color;
        }
        pixels += pitch;
    }
}

static void SDL_FillSurfaceRect3(Uint8 *pixels, int pitch, Uint32 color, int w, int h)
{
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    Uint8 b1 = (Uint8)(color & 0xFF);
    Uint8 b2 = (Uint8)((color >> 8) & 0xFF);
    Uint8 b3 = (Uint8)((color >> 16) & 0xFF);
#elif SDL_BYTEORDER == SDL_BIG_ENDIAN
    Uint8 b1 = (Uint8)((color >> 16) & 0xFF);
    Uint8 b2 = (Uint8)((color >> 8) & 0xFF);
    Uint8 b3 = (Uint8)(color & 0xFF);
#endif
    int n;
    Uint8 *p = NULL;

    while (h--) {
        n = w;
        p = pixels;

        while (n--) {
            *p++ = b1;
            *p++ = b2;
            *p++ = b3;
        }
        pixels += pitch;
    }
}

static void SDL_FillSurfaceRect4(Uint8 *pixels, int pitch, Uint32 color, int w, int h)
{
    while (h--) {
        SDL_memset4(pixels, color, w);
        pixels += pitch;
    }
}

/*
 * This function performs a fast fill of the given rectangle with 'color'
 */
int SDL_FillSurfaceRect(SDL_Surface *dst, const SDL_Rect *rect, Uint32 color)
{
    if (dst == NULL) {
        return SDL_InvalidParamError("SDL_FillSurfaceRect(): dst");
    }

    /* If 'rect' == NULL, then fill the whole surface */
    if (rect == NULL) {
        rect = &dst->clip_rect;
        /* Don't attempt to fill if the surface's clip_rect is empty */
        if (SDL_RectEmpty(rect)) {
            return 0;
        }
    }

    return SDL_FillSurfaceRects(dst, rect, 1, color);
}

#if SDL_ARM_NEON_BLITTERS
void FillSurfaceRect8ARMNEONAsm(int32_t w, int32_t h, uint8_t *dst, int32_t dst_stride, uint8_t src);
void FillSurfaceRect16ARMNEONAsm(int32_t w, int32_t h, uint16_t *dst, int32_t dst_stride, uint16_t src);
void FillSurfaceRect32ARMNEONAsm(int32_t w, int32_t h, uint32_t *dst, int32_t dst_stride, uint32_t src);

static void fill_8_neon(Uint8 *pixels, int pitch, Uint32 color, int w, int h)
{
    FillSurfaceRect8ARMNEONAsm(w, h, (uint8_t *)pixels, pitch >> 0, color);
    return;
}

static void fill_16_neon(Uint8 *pixels, int pitch, Uint32 color, int w, int h)
{
    FillSurfaceRect16ARMNEONAsm(w, h, (uint16_t *)pixels, pitch >> 1, color);
    return;
}

static void fill_32_neon(Uint8 *pixels, int pitch, Uint32 color, int w, int h)
{
    FillSurfaceRect32ARMNEONAsm(w, h, (uint32_t *)pixels, pitch >> 2, color);
    return;
}
#endif

#if SDL_ARM_SIMD_BLITTERS
void FillSurfaceRect8ARMSIMDAsm(int32_t w, int32_t h, uint8_t *dst, int32_t dst_stride, uint8_t src);
void FillSurfaceRect16ARMSIMDAsm(int32_t w, int32_t h, uint16_t *dst, int32_t dst_stride, uint16_t src);
void FillSurfaceRect32ARMSIMDAsm(int32_t w, int32_t h, uint32_t *dst, int32_t dst_stride, uint32_t src);

static void fill_8_simd(Uint8 *pixels, int pitch, Uint32 color, int w, int h)
{
    FillSurfaceRect8ARMSIMDAsm(w, h, (uint8_t *)pixels, pitch >> 0, color);
    return;
}

static void fill_16_simd(Uint8 *pixels, int pitch, Uint32 color, int w, int h)
{
    FillSurfaceRect16ARMSIMDAsm(w, h, (uint16_t *)pixels, pitch >> 1, color);
    return;
}

static void fill_32_simd(Uint8 *pixels, int pitch, Uint32 color, int w, int h)
{
    FillSurfaceRect32ARMSIMDAsm(w, h, (uint32_t *)pixels, pitch >> 2, color);
    return;
}
#endif

int SDL_FillSurfaceRects(SDL_Surface *dst, const SDL_Rect *rects, int count,
                  Uint32 color)
{
    SDL_Rect clipped;
    Uint8 *pixels;
    const SDL_Rect *rect;
    void (*fill_function)(Uint8 * pixels, int pitch, Uint32 color, int w, int h) = NULL;
    int i;

    if (dst == NULL) {
        return SDL_InvalidParamError("SDL_FillSurfaceRects(): dst");
    }

    /* Nothing to do */
    if (dst->w == 0 || dst->h == 0) {
        return 0;
    }

    /* Perform software fill */
    if (!dst->pixels) {
        return SDL_SetError("SDL_FillSurfaceRects(): You must lock the surface");
    }

    if (rects == NULL) {
        return SDL_InvalidParamError("SDL_FillSurfaceRects(): rects");
    }

    /* This function doesn't usually work on surfaces < 8 bpp
     * Except: support for 4bits, when filling full size.
     */
    if (dst->format->BitsPerPixel < 8) {
        if (count == 1) {
            const SDL_Rect *r = &rects[0];
            if (r->x == 0 && r->y == 0 && r->w == dst->w && r->h == dst->h) {
                if (dst->format->BitsPerPixel == 4) {
                    Uint8 b = (((Uint8)color << 4) | (Uint8)color);
                    SDL_memset(dst->pixels, b, (size_t)dst->h * dst->pitch);
                    return 1;
                }
            }
        }
        return SDL_SetError("SDL_FillSurfaceRects(): Unsupported surface format");
    }

#if SDL_ARM_NEON_BLITTERS
    if (SDL_HasNEON() && dst->format->BytesPerPixel != 3 && fill_function == NULL) {
        switch (dst->format->BytesPerPixel) {
        case 1:
            fill_function = fill_8_neon;
            break;
        case 2:
            fill_function = fill_16_neon;
            break;
        case 4:
            fill_function = fill_32_neon;
            break;
        }
    }
#endif
#if SDL_ARM_SIMD_BLITTERS
    if (SDL_HasARMSIMD() && dst->format->BytesPerPixel != 3 && fill_function == NULL) {
        switch (dst->format->BytesPerPixel) {
        case 1:
            fill_function = fill_8_simd;
            break;
        case 2:
            fill_function = fill_16_simd;
            break;
        case 4:
            fill_function = fill_32_simd;
            break;
        }
    }
#endif

    if (fill_function == NULL) {
        switch (dst->format->BytesPerPixel) {
        case 1:
        {
            color |= (color << 8);
            color |= (color << 16);
#ifdef __SSE__
            if (SDL_HasSSE()) {
                fill_function = SDL_FillSurfaceRect1SSE;
                break;
            }
#endif
            fill_function = SDL_FillSurfaceRect1;
            break;
        }

        case 2:
        {
            color |= (color << 16);
#ifdef __SSE__
            if (SDL_HasSSE()) {
                fill_function = SDL_FillSurfaceRect2SSE;
                break;
            }
#endif
            fill_function = SDL_FillSurfaceRect2;
            break;
        }

        case 3:
            /* 24-bit RGB is a slow path, at least for now. */
            {
                fill_function = SDL_FillSurfaceRect3;
                break;
            }

        case 4:
        {
#ifdef __SSE__
            if (SDL_HasSSE()) {
                fill_function = SDL_FillSurfaceRect4SSE;
                break;
            }
#endif
            fill_function = SDL_FillSurfaceRect4;
            break;
        }

        default:
            return SDL_SetError("Unsupported pixel format");
        }
    }

    for (i = 0; i < count; ++i) {
        rect = &rects[i];
        /* Perform clipping */
        if (!SDL_GetRectIntersection(rect, &dst->clip_rect, &clipped)) {
            continue;
        }
        rect = &clipped;

        pixels = (Uint8 *)dst->pixels + rect->y * dst->pitch +
                 rect->x * dst->format->BytesPerPixel;

        fill_function(pixels, dst->pitch, color, rect->w, rect->h);
    }

    /* We're done! */
    return 0;
}
