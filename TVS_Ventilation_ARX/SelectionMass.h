#pragma once
#include <algorithm>
#include <cfloat>
#include <vector>

namespace tvsSelection {
template<class Key> struct Total {
    std::vector<Key> objects;
    double mass;
    double correction;
    unsigned long count;
    Total(): mass(0.0), correction(0.0), count(0) {}
    void add(const Key &key, bool supported, double value) {
        if (std::find(objects.begin(),objects.end(),key) != objects.end()) return;
        objects.push_back(key);
        if (!supported || !(value >= 0.0 && value <= DBL_MAX)) return;
        // Compensated summation; round only when presenting the result.
        const double part = value-correction;
        const double next = mass+part;
        correction = (next-mass)-part;
        mass = next;
        ++count;
    }
    bool contains(const Key &key) const {
        return std::find(objects.begin(),objects.end(),key) != objects.end();
    }
};
template<class Identity> struct CacheKey {
    std::vector<Identity> objects;
    unsigned long revision;
    bool valid;
    CacheKey(): revision(0), valid(false) {}
    bool matches(const Identity *data, size_t count, unsigned long value) const {
        return valid && revision == value && count == objects.size()
            && std::equal(objects.begin(),objects.end(),data);
    }
    void set(const Identity *data, size_t count, unsigned long value) {
        if (count) objects.assign(data,data+count); else objects.clear();
        revision=value; valid=true;
    }
    void clear() { objects.clear(); valid=false; }
};
}
