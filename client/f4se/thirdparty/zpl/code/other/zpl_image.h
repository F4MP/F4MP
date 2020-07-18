/*

ZPL - Image module

Usage:
  #define ZPLI_IMPLEMENTATION exactly in ONE source file right BEFORE including the library, like:

  #define ZPLI_IMPLEMENTATION
  #include"zpl_image.h"

Dependencies:
  zpl.h
  stb_image.h

  Make sure you properly include them!

Optional switches:
  ZPLI_NO_GIF
  ZPLI_NO_IMAGE_OPS

Credits:
  Read AUTHORS.md

GitHub:
  https://github.com/zpl-c/zpl

Version History:
  2.0.0 - New syntax

  1.0.2 - Switch fixes
  1.0.1 - Got rid of unused switches and fixes
  1.0.0 - Initial version



  This Software is dual licensed under the following licenses:

  Unlicense
  This is free and unencumbered software released into the public domain.

  Anyone is free to copy, modify, publish, use, compile, sell, or
  distribute this software, either in source code form or as a compiled
  binary, for any purpose, commercial or non-commercial, and by any
  means.

  In jurisdictions that recognize copyright laws, the author or authors
  of this software dedicate any and all copyright interest in the
  software to the public domain. We make this dedication for the benefit
  of the public at large and to the detriment of our heirs and
  successors. We intend this dedication to be an overt act of
  relinquishment in perpetuity of all present and future rights to this
  software under copyright law.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
  OTHER DEALINGS IN THE SOFTWARE.

  For more information, please refer to <http://unlicense.org/>

  Apache 2.0
  Copyright 2017-2018 Dominik Madarász <zaklaus@outlook.com>
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

*/

#ifndef ZPL_INCLUDE_ZPL_IMAGE_H
#define ZPL_INCLUDE_ZPL_IMAGE_H

#if defined(__cplusplus)
extern "C" {
#endif

  typedef union zpli_rgb_colour {
      u32 colour;
      struct { u8 r, g, b;};
  } zpli_rgb_colour;

  typedef struct zpli_hsv_colour {
      u32 colour;
      struct { u8 h, s, v; };
  } zpli_hsv_colour;

  ZPL_DEF zpli_rgb_colour zpli_rgb_lerp(zpli_rgb_colour a, zpli_rgb_colour b, f32 t);

  ////////////////////////////////////////////////////////////////
  //
  // GIF Loader
  //
  // Uses stb_image.h for loading gif frames.
  //

#ifndef ZPLI_NO_GIF

  typedef struct zpli_gif_result {
      i32 delay;
      u8 *data;
      struct zpli_gif_result *next;
  } zpli_gif_result;

  ZPL_DEF zpli_gif_result *zpli_gif_load(char const *filename, i32 *x, i32 *y, i32 *frames);
  ZPL_DEF void             zpli_gif_free(zpli_gif_result *gif, b32 aligned);
#endif

  ////////////////////////////////////////////////////////////////
  //
  // Image Operations
  //

#ifndef ZPLI_NO_IMAGE_OPS
  // NOTE(ZaKlaus): This is not sRGB aware!
  ZPL_DEF void zpli_rgb_resize(u32 *source, i32 source_w, i32 source_h,
                                    u32 *dest, i32 dest_w, i32 dest_h,
                                    i32 blur_iter, u32 *blur_mem);

  ZPL_DEF void zpli_rgb_filter(u32 *source, i32 source_w, i32 source_h,
                                    u32 *dest,
                                    f64 *filter, i32 filter_w, i32 filter_h,
                                    f64 factor, f64 bias);

  // TODO(ZaKlaus): Implement this
  /*
  ZPL_DEF void         zpli_init_srgb_table(u8 **table);
  ZPL_DEF zpli_rgb_colour zpli_lin_to_srgb    (u8 *table, f64 vals[3]);
  */

  ZPL_DEF zpli_hsv_colour zpli_rgb_to_hsv (zpli_rgb_colour colour);
  ZPL_DEF zpli_rgb_colour zpli_hsv_to_rgb (zpli_hsv_colour colour);
#endif

#if defined(__cplusplus)
}
#endif

#if defined(ZPLI_IMPLEMENTATION) && !defined(ZPLI_IMPLEMENTATION_DONE)
#define ZPLI_IMPLEMENTATION_DONE

#if defined(__cplusplus)
extern "C" {
#endif

  zpli_rgb_colour zpli_rgb_lerp(zpli_rgb_colour a, zpli_rgb_colour b, f32 t) {
#define LERP(c1, c2, c3) c1*(1.0-c3) + c2*c3
      zpli_rgb_colour result = {0};

      result.r = LERP(a.r, b.r, t);
      result.g = LERP(a.g, b.g, t);
      result.b = LERP(a.b, b.b, t);

      return result;
#undef LERP
  }

  // NOTE(ZaKlaus): Gif

#ifndef ZPL_NO_GIF
  zpli_gif_result *zpli_gif_load(char const *filename, i32 *x, i32 *y, i32 *frames) {
      FILE *file;
      stbi__context s;
      zpli_gif_result *result;

      if (!(file = stbi__fopen(filename, "rb"))) {
          stbi__errpuc("can't open file", "Unable to open file"); return 0;
      }

      stbi__start_file(&s, file);

      if (stbi__gif_test(&s)) {
          i32 c;
          stbi__gif g;
          zpli_gif_result *head = stbi__malloc(zpl_size_of(zpli_gif_result_t));
          zpli_gif_result *prev = 0, *gr = head;

          zpl_zero_item(&g);
          zpl_zero_item(head);

          *frames = 0;

          while(gr->data = stbi__gif_load_next(&s, &g, &c, 4)) {
              if (gr->data == cast(u8 *)&s) {
                  gr->data = 0;
                  break;
              }

              if (prev) prev->next = gr;
              gr->delay = g.delay;
              prev = gr;
              gr = cast(zpli_gif_result *)stbi__malloc(zpl_size_of(zpli_gif_result_t));
              zpl_zero_item(gr);
              ++(*frames);
          }

          STBI_FREE(g.out);

          if (gr != head) {
              //STBI_FREE(gr);
          }

          if (*frames > 0) {
              *x = g.w;
              *y = g.h;
          }

          result = head;
      }
      else {
          // TODO(ZaKlaus): Add support.
          result = 0; //stbi__load_main(&s, x, y, frames, &bpp, 4, &ri, 8);
          *frames = !!result;
      }

      fclose(file);
      return result;
  }

  void zpli_gif_free(zpli_gif_result *gif, b32 aligned) {
      zpli_gif_result *elem, *prev = 0;
      for (elem = gif; elem; elem = elem->next) {
          if (aligned) {
              zpl_mfree(elem->data);
          }
          else {
              STBI_FREE(elem->data);
          }
          STBI_FREE(prev);
          prev = elem;
      }

      STBI_FREE(prev);
  }
#endif

  zpli_hsv_colour zpli_rgb_to_hsv(zpli_rgb_colour colour) {
      zpli_hsv_colour result = {0};
      u8 rgb_min, rgb_max;

      rgb_min = colour.r < colour.g ? (colour.r < colour.b ? colour.r : colour.b) : (colour.g < colour.b ? colour.g : colour.b);
      rgb_max = colour.r > colour.g ? (colour.r > colour.b ? colour.r : colour.b) : (colour.g > colour.b ? colour.g : colour.b);

      result.v = rgb_max;
      if (result.v == 0) {
          result.h = result.s = 0;
          return result;
      }

      result.s = 255 * cast(i64)(rgb_max - rgb_min) / result.v;
      if (result.s == 0) {
          result.h = 0;
          return result;
      }

      /**/ if (rgb_max == colour.r) {
          result.h = 0   + 43 * (colour.g - colour.b) / (rgb_max - rgb_min);
      }
      else if (rgb_max == colour.g) {
          result.h = 85  + 43 * (colour.b - colour.r) / (rgb_max - rgb_min);
      }
      else {
          result.h = 171 + 43 * (colour.r - colour.g) / (rgb_max - rgb_min);
      }

      return result;
  }

  zpli_rgb_colour zpli_hsv_to_rgb(zpli_hsv_colour colour) {
      zpli_rgb_colour result = {0};
      u8 region, rem, p, q, t;

      if (colour.s == 0) {
          result.r = result.g = result.b = colour.v;
          return result;
      }

      region =  colour.h / 43;
      rem    = (colour.h - (region * 43)) * 6;

      p = (colour.v * (255 - colour.s)) >> 8;
      q = (colour.v * (255 - ((colour.s * rem) >> 8))) >> 8;
      t = (colour.v * (255 - ((colour.s * (255 - rem)) >> 8))) >> 8;

      switch (region)
      {
          case 0: result.r = colour.v; result.g = t; result.b = p; break;
          case 1: result.r = q; result.g = colour.v; result.b = p; break;
          case 2: result.r = p; result.g = colour.v; result.b = t; break;
          case 3: result.r = p; result.g = q; result.b = colour.v; break;
          case 4: result.r = t; result.g = p; result.b = colour.v; break;
          default:result.r = colour.v; result.g = p; result.b = q; break;
      }

      return result;
  }

#ifndef ZPL_NO_IMAGE_OPS
  void zpli_rgb_resize(u32 *source, i32 source_w, i32 source_h,
                                       u32 *dest, i32 dest_w, i32 dest_h,
                                       i32 blur_iter, u32 *blur_mem) {

      zpli_rgb_colour *src = cast(zpli_rgb_colour *)&(source);
      zpli_rgb_colour *dst = cast(zpli_rgb_colour *)&(dest);

      b32 x_down = dest_w < source_w;
      b32 y_down = dest_h < source_h;

      i32 step_x;
      i32 step_y;

      if(x_down) {
          step_x = cast(i32)(source_w / cast(f32)dest_w);
      }
      else {
          step_x = cast(i32)(dest_w / cast(f32)source_w);
      }

      if(y_down) {
          step_y = cast(i32)(source_h / cast(f32)dest_h);
      }
      else {
          step_y = cast(i32)(dest_h / cast(f32)source_h);
      }



      for (i32 y = 0; y < dest_h; ++y) {
          for(i32 x = 0; x < dest_w; ++x) {
              zpli_rgb_colour colour = {0};

              i32 o_x = x/step_x;
              if (x_down) o_x = x*step_x;
              i32 o_y = y/step_y;
              if (y_down) o_y = y*step_y;

              colour = src[o_y*source_w + o_x];

              dst[y*dest_w + x] = colour;
          }
      }

      if (blur_iter > 0) {
          ZPL_ASSERT_NOT_NULL(blur_mem);
          zpl_local_persist f64 filter[5][5] = {
              0, 0, 1, 0, 0,
              0, 1, 1, 1, 0,
              1, 1, 1, 1, 1,
              0, 1, 1, 1, 0,
              0, 0, 1, 0, 0,
          };

          f64 factor = 1.0 / 13.0;
          f64 bias = 0.0;

          zpl_memcopy(blur_mem, dest, dest_w*dest_h*4);

          zpli_rgb_filter(blur_mem, dest_w, dest_h,
                               cast(u32 *)dest,
                               &filter[0][0], 5, 5,
                               factor, bias);


      }
  }

  void zpli_rgb_filter(u32 *source, i32 source_w, i32 source_h,
                                       u32 *dest,
                                       f64 *filter, i32 filter_w, i32 filter_h,
                                       f64 factor, f64 bias) {

      zpli_rgb_colour *src = cast(zpli_rgb_colour *)(source);
      zpli_rgb_colour *dst = cast(zpli_rgb_colour *)(dest);

      for (i32 y = 0; y < source_h; ++y) {
          for(i32 x = 0; x < source_w; ++x) {
              i32 r = 0, g = 0, b = 0;

              for (i32 fy = 0; fy < filter_h; ++fy) {
                  for (i32 fx = 0; fx < filter_w; ++fx) {
                      i32 img_x = (x - filter_w / 2 + fx + source_w) % source_w;
                      i32 img_y = (y - filter_h / 2 + fy + source_h) % source_h;

                      r += src[img_y * source_w + img_x].r * filter[fy*filter_w + fx];
                      g += src[img_y * source_w + img_x].g * filter[fy*filter_w + fx];
                      b += src[img_y * source_w + img_x].b * filter[fy*filter_w + fx];
                  }
              }

              dst[y * source_w + x].r = zpl_min(zpl_max(cast(i32)(factor * r + bias), 0), 255);
              dst[y * source_w + x].g = zpl_min(zpl_max(cast(i32)(factor * g + bias), 0), 255);
              dst[y * source_w + x].b = zpl_min(zpl_max(cast(i32)(factor * b + bias), 0), 255);
          }
      }
  }

#endif

#if defined(__cplusplus)
}
#endif

#endif

#endif // ZPL_INCLUDE_ZPL_IMAGE_H
