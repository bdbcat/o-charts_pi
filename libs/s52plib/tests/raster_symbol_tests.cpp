#include "../src/raster_symbol_utils.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
unsigned checks = 0;
void Check(bool passed, const char *description) {
  ++checks;
  if (!passed) throw std::runtime_error(description);
}

void Transparency() {
  wxImage image(8, 4);
  Check(!raster_symbol::HasPartialAlpha(image), "image without alpha");
  image.InitAlpha();
  std::fill(image.GetAlpha(), image.GetAlpha() + 32, 255);
  Check(!raster_symbol::HasPartialAlpha(image), "fully opaque image");
  image.GetAlpha()[0] = 0;
  Check(!raster_symbol::HasPartialAlpha(image), "binary transparency");
  // The old row loop only inspected the first 'height' bytes.
  image.GetAlpha()[31] = 128;
  Check(raster_symbol::HasPartialAlpha(image), "partial alpha in final pixel");
  image.GetAlpha()[31] = 255;
  image.GetAlpha()[7] = 1;
  Check(raster_symbol::HasPartialAlpha(image), "partial alpha after first height bytes");
  image.GetAlpha()[7] = 255;
  image.GetAlpha()[16] = 254;
  Check(raster_symbol::HasPartialAlpha(image), "nearly opaque pixel");
}

void Blending() {
  for (int symbol = 0; symbol < 256; ++symbol) {
    for (int background = 0; background < 256; ++background) {
      Check(raster_symbol::Blend(symbol, background, 0) == background,
            "transparent pixel must preserve background");
      Check(raster_symbol::Blend(symbol, background, 255) == symbol,
            "opaque pixel must preserve symbol");
    }
  }
  const unsigned char channels[] = {0, 1, 63, 127, 128, 254, 255};
  for (unsigned char symbol : channels)
    for (unsigned char background : channels)
      for (int alpha = 0; alpha < 256; ++alpha) {
        const double opacity = alpha / 255.0;
        const auto expected = std::lround(symbol * opacity + background * (1 - opacity));
        Check(raster_symbol::Blend(symbol, background, alpha) == expected,
              "partial transparency must match reference composition");
      }
}

void Dimensions() {
  wxSize size;
  Check(raster_symbol::ScaledSize(21, 32, 1, size) && size == wxSize(21, 32),
        "native dimensions");
  Check(raster_symbol::ScaledSize(21, 32, 2.5, size) && size == wxSize(52, 80),
        "fractional display scale preserves truncation");
  Check(raster_symbol::ScaledSize(21, 32, .001, size) && size == wxSize(1, 1),
        "tiny symbols retain one pixel");
  Check(raster_symbol::ScaledSize(256, 256, 8, size) && size == wxSize(2048, 2048),
        "high DPI within allocation budget");
  Check(!raster_symbol::ScaledSize(0, 32, 1, size), "invalid source size");
  Check(!raster_symbol::ScaledSize(21, 32, 0, size), "zero scale");
  Check(!raster_symbol::ScaledSize(21, 32, -1, size), "negative scale");
  Check(!raster_symbol::ScaledSize(21, 32, std::numeric_limits<double>::quiet_NaN(), size),
        "NaN scale");
  Check(!raster_symbol::ScaledSize(21, 32, std::numeric_limits<double>::infinity(), size),
        "infinite scale");
  Check(!raster_symbol::ScaledSize(21, 32, 1e100, size), "overflowing scale");
  Check(!raster_symbol::ScaledSize(4096, 4096, 1, size), "pixel budget");
  Check(!raster_symbol::ScaledSize(1, 1, 4097, size), "dimension budget");
}

void Visibility() {
  Check(raster_symbol::IntersectsViewport(0, 0, 20, 20, 100, 100), "visible symbol");
  Check(raster_symbol::IntersectsViewport(-19, -19, 20, 20, 100, 100), "clipped top left");
  Check(raster_symbol::IntersectsViewport(99, 99, 20, 20, 100, 100), "clipped bottom right");
  Check(!raster_symbol::IntersectsViewport(-20, 0, 20, 20, 100, 100), "fully left");
  Check(!raster_symbol::IntersectsViewport(0, -20, 20, 20, 100, 100), "fully above");
  Check(!raster_symbol::IntersectsViewport(100, 0, 20, 20, 100, 100), "fully right");
  Check(!raster_symbol::IntersectsViewport(0, 100, 20, 20, 100, 100), "fully below");
}
}  // namespace

int main() {
  try {
    Transparency();
    Blending();
    Dimensions();
    Visibility();
    std::cout << "Passed " << checks << " raster symbol regression checks\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
