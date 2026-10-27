#pragma once

namespace tvsUnits {
inline bool validFactor(double value) { return value == 1.0 || value == 10.0 || value == 1000.0; }
inline double toMillimetres(double value, double factor) { return value * factor; }
inline double fromMillimetres(double value, double factor) { return value / factor; }
inline double toMetres(double value, double factor) { return value * factor / 1000.0; }
inline double squareMetres(double area, double factor) { return area * factor * factor / 1000000.0; }
}
