#include "../TVS_Ventilation_DBX/UnitConversion.h"
#include <cassert>
#include <cmath>
int main() {
    const double factors[] = {1.0, 10.0, 1000.0};
    for (unsigned i = 0; i < 3; ++i) {
        const double f = factors[i];
        const double a = tvsUnits::fromMillimetres(400.0, f);
        const double b = tvsUnits::fromMillimetres(200.0, f);
        const double length = tvsUnits::fromMillimetres(3000.0, f);
        assert(std::fabs(tvsUnits::toMetres(length, f) - 3.0) < 1e-12);
        const double area = tvsUnits::squareMetres(2*(a+b)*length, f);
        assert(std::fabs(area - 3.6) < 1e-12);
        assert(std::fabs(area*0.0006*7850.0 - 16.956) < 1e-10);
        assert(std::fabs(tvsUnits::toMillimetres(a, f) - 400.0) < 1e-12);
        // Same flow and physical cross-section must produce the same velocity.
        const double speed = 720.0/3600.0 / tvsUnits::squareMetres(a*b, f);
        assert(std::fabs(speed - 2.5) < 1e-12);
        // Annular elbow developed surface must be invariant under drawing units.
        const double ri = tvsUnits::fromMillimetres(150.0, f);
        const double arcLength = (ri+a/2)*3.14159265358979323846/2;
        const double elbowArea = tvsUnits::squareMetres(2*(a+b)*arcLength, f);
        assert(std::fabs(elbowArea - 0.6597344572538566) < 1e-12);
    }
    assert(!tvsUnits::validFactor(0.0));
    assert(!tvsUnits::validFactor(100.0));
}
