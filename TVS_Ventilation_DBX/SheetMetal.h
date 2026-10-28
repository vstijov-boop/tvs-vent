#pragma once
#include <cmath>
#include <cfloat>

// Physical millimetres in, square metres and kilograms out.
// Rectangular formulas and thickness bands follow KGTEZ.lsp.
// No seams, flanges, connectors or purchasing reserve are included.
namespace tvsSheetMetal {
const double pi = 3.14159265358979323846;
inline bool nonnegative(double x) { return x >= 0.0 && x <= DBL_MAX; }
inline double larger(double a, double b) { return a > b ? a : b; }
inline double thickness(double largestSideMm)
{
    if (!(largestSideMm > 0.0) || !nonnegative(largestSideMm)) return 0.0;
    if (largestSideMm <= 500.0) return 0.6;
    if (largestSideMm <= 1000.0) return 0.8;
    if (largestSideMm <= 1500.0) return 1.0;
    return 1.2;
}
inline double perimeter(double a, double b)
{
    return b == 0.0 ? pi * a : 2.0 * (a + b);
}
inline bool section(double a, double b)
{
    return a > 0.0 && nonnegative(a) && nonnegative(b);
}
inline double pipeArea(double a, double b, double length)
{
    return perimeter(a, b) * length / 1.0e6;
}
inline double elbowArea(double a, double b, double innerRadius, double angleRadians)
{
    return perimeter(a, b) * (innerRadius + a / 2.0)
        * std::fabs(angleRadians) / 1.0e6;
}
inline double transitionArea(double a, double b, double a2, double b2, double length)
{
    // KGTEZ uses average perimeter * axial length for rectangular transitions.
    if (b > 0.0 && b2 > 0.0)
        return (a + b + a2 + b2) * length / 1.0e6;
    // Existing TVS equivalent-cone approximation for round/mixed transitions.
    const double r1 = perimeter(a, b) / (2.0 * pi);
    const double r2 = perimeter(a2, b2) / (2.0 * pi);
    return pi * (r1 + r2) * std::sqrt(length * length + (r1-r2)*(r1-r2)) / 1.0e6;
}
inline double wyeArea(double a, double b, double a2, double b2, double branchLength)
{
    const double opening = b2 == 0.0 ? pi*a2*a2/4.0 : a2*b2;
    return (perimeter(a,b)*(a2 + 2.0*branchLength)
        + perimeter(a2,b2)*branchLength - opening) / 1.0e6;
}
inline double mass(double areaM2, double thicknessMm)
{
    return areaM2 * (thicknessMm / 1000.0) * 7850.0;
}
}
