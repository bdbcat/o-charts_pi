/******************************************************************************
 * Conservative grid-centre triangle rasterisation for chart-safety areas.
 ******************************************************************************/

#ifndef O_CHARTS_CHART_SAFETY_RASTER_H
#define O_CHARTS_CHART_SAFETY_RASTER_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ocharts {
namespace chart_safety {

struct Point {
    double x;
    double y;
};

struct GeographicBounds {
    double min_lat;
    double min_lon;
    double max_lat;
    double max_lon;

    bool Contains(double lat, double lon) const {
        if( lat < min_lat || lat > max_lat ) return false;
        if( max_lon > 180.0 ) {
            if( lon < max_lon - 360.0 ) lon += 360.0;
        }
        else if( min_lon < -180.0 ) {
            if( lon > min_lon + 360.0 ) lon -= 360.0;
        }
        return lon >= min_lon && lon <= max_lon;
    }
};

inline int LegacyCounterClockwise(const Point &a, const Point &b,
                                  const Point &point) {
    const double dx1 = b.x - a.x;
    const double dx2 = point.x - a.x;
    const double dy1 = b.y - a.y;
    const double dy2 = point.y - a.y;
    return dx1 * dy2 > dy1 * dx2 ? 1 : -1;
}

inline bool LegacySegmentsIntersect(const Point &a, const Point &b,
                                    const Point &c, const Point &d) {
    return LegacyCounterClockwise(a, b, c) *
                   LegacyCounterClockwise(a, b, d) <=
               0 &&
           LegacyCounterClockwise(c, d, a) *
                   LegacyCounterClockwise(c, d, b) <=
               0;
}

// Match G_PtInPolygon(), including its float query-coordinate conversion and
// edge convention.  Safety-grid verification compares against that existing
// provider contract, so a faster raster must not silently change which side
// of a triangle boundary owns a grid point.
inline bool LegacyPointInTriangle(const Point &a, const Point &b,
                                  const Point &c, const Point &point) {
    Point ray_start = {static_cast<float>(point.x),
                       static_cast<float>(point.y)};
    Point ray_end = ray_start;
    ray_end.x = 1.e8;
    int intersections = 0;
    intersections += LegacySegmentsIntersect(ray_start, ray_end, a, b) ? 1 : 0;
    intersections += LegacySegmentsIntersect(ray_start, ray_end, b, c) ? 1 : 0;
    intersections += LegacySegmentsIntersect(ray_start, ray_end, c, a) ? 1 : 0;
    return (intersections & 1) != 0;
}

inline bool RasterizeTriangle(const Point &a, const Point &b, const Point &c,
                              const std::vector<double> &column_x,
                              const std::vector<double> &row_y,
                              const std::vector<double> &column_lon,
                              const std::vector<double> &row_lat,
                              const GeographicBounds &object_bounds,
                              const GeographicBounds &primitive_bounds,
                              const uint8_t *active_cells,
                              std::vector<uint64_t> *hit_cells) {
    if( !hit_cells || column_x.empty() || row_y.empty() ||
        column_lon.size() != column_x.size() ||
        row_lat.size() != row_y.size() )
        return false;
    if( !std::isfinite(a.x) || !std::isfinite(a.y) ||
        !std::isfinite(b.x) || !std::isfinite(b.y) ||
        !std::isfinite(c.x) || !std::isfinite(c.y) )
        return false;

    const double min_x = std::min(a.x, std::min(b.x, c.x));
    const double max_x = std::max(a.x, std::max(b.x, c.x));
    const double min_y = std::min(a.y, std::min(b.y, c.y));
    const double max_y = std::max(a.y, std::max(b.y, c.y));
    const std::vector<double>::const_iterator col_begin_it =
        std::lower_bound(column_x.begin(), column_x.end(), min_x);
    const std::vector<double>::const_iterator col_end_it =
        std::upper_bound(column_x.begin(), column_x.end(), max_x);
    const std::vector<double>::const_iterator row_begin_it =
        std::lower_bound(row_y.begin(), row_y.end(), min_y);
    const std::vector<double>::const_iterator row_end_it =
        std::upper_bound(row_y.begin(), row_y.end(), max_y);
    if( col_begin_it == column_x.end() || row_begin_it == row_y.end() ||
        col_begin_it == col_end_it || row_begin_it == row_end_it )
        return false;

    const std::size_t col_begin = col_begin_it - column_x.begin();
    const std::size_t col_end = col_end_it - column_x.begin();
    const std::size_t row_begin = row_begin_it - row_y.begin();
    const std::size_t row_end = row_end_it - row_y.begin();
    bool any_hit = false;
    for( std::size_t row = row_begin; row < row_end; ++row ) {
        const Point point_base = {0.0, row_y[row]};
        for( std::size_t col = col_begin; col < col_end; ++col ) {
            const std::size_t index = row * column_x.size() + col;
            if( active_cells && !active_cells[index] ) continue;
            // IsPointInObjArea() first applies both the object's selection
            // box and the individual triangle primitive's geographic box.
            // These checks determine ownership of points lying exactly on a
            // shared chart-cell/depth-area boundary and therefore form part
            // of the existing semantic contract, not merely an optimisation.
            if( !object_bounds.Contains(row_lat[row], column_lon[col]) ||
                !primitive_bounds.Contains(row_lat[row], column_lon[col]) )
                continue;
            Point point = point_base;
            point.x = column_x[col];
            if( !LegacyPointInTriangle(a, b, c, point) ) continue;
            (*hit_cells)[index / 64] |= uint64_t(1) << (index % 64);
            any_hit = true;
        }
    }
    return any_hit;
}

}  // namespace chart_safety
}  // namespace ocharts

#endif  // O_CHARTS_CHART_SAFETY_RASTER_H
