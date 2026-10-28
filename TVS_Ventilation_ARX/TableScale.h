#pragma once
#include <cfloat>

namespace tvsTable {
inline bool positive(double value) { return value > 0.0 && value <= DBL_MAX; }
struct Sizing {
    double denominator;
    double mmPerUnit;
    double paperTextHeight;
    Sizing(double scale = 50.0, double units = 1.0, double height = 2.5)
        : denominator(scale), mmPerUnit(units), paperTextHeight(height) {}
    double textHeight() const { return paperTextHeight * denominator / mmPerUnit; }
    // Original table templates use height 250 and dimensions in matching units.
    double templateFactor() const { return textHeight() / 250.0; }
    bool valid() const {
        return positive(denominator) && positive(mmPerUnit)
            && positive(paperTextHeight) && positive(textHeight());
    }
};
}
