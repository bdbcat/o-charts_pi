/***************************************************************************
 * Copyright (C) 2026 OpenCPN contributors
 * This program is free software under the GNU General Public License v2+.
 ***************************************************************************/
#ifndef RASTER_SYMBOL_UTILS_H
#define RASTER_SYMBOL_UTILS_H

#include <cmath>
#include <cstddef>
#include <wx/image.h>

namespace raster_symbol {

// S-52 symbols are small. These generous limits also allow high-DPI displays,
// while preventing a corrupt scale from allocating unbounded drawing buffers.
inline bool ScaledSize(int width, int height, double scale, wxSize &size) {
  constexpr int max_dimension = 4096;
  constexpr size_t max_pixels = 4 * 1024 * 1024;
  if (width <= 0 || height <= 0 || !std::isfinite(scale) || scale <= 0)
    return false;
  const double w = width * scale;
  const double h = height * scale;
  if (!std::isfinite(w) || !std::isfinite(h) || w > max_dimension ||
      h > max_dimension)
    return false;
  const int scaled_width = w < 1 ? 1 : static_cast<int>(w);
  const int scaled_height = h < 1 ? 1 : static_cast<int>(h);
  if (static_cast<size_t>(scaled_width) * scaled_height > max_pixels)
    return false;
  size = wxSize(scaled_width, scaled_height);
  return true;
}

inline bool HasPartialAlpha(const wxImage &image) {
  const unsigned char *alpha = image.GetAlpha();
  if (!alpha) return false;
  const size_t pixels = static_cast<size_t>(image.GetWidth()) * image.GetHeight();
  for (size_t i = 0; i < pixels; ++i) {
    if (alpha[i] != wxIMAGE_ALPHA_TRANSPARENT &&
        alpha[i] != wxIMAGE_ALPHA_OPAQUE)
      return true;
  }
  return false;
}

inline unsigned char Blend(unsigned char symbol, unsigned char background,
                           unsigned char alpha) {
  // Integer arithmetic gives exact transparent/opaque endpoints and rounds
  // intermediate values to the nearest channel value.
  return static_cast<unsigned char>(
      (symbol * alpha + background * (255 - alpha) + 127) / 255);
}

inline bool IntersectsViewport(double x, double y, int width, int height,
                               int viewport_width, int viewport_height) {
  return width > 0 && height > 0 && x < viewport_width && y < viewport_height &&
         x + width > 0 && y + height > 0;
}

}  // namespace raster_symbol
#endif
