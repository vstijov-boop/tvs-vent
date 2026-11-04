#pragma once

// Database geometry stays in WCS. AutoCAD interactive input uses the current UCS.
// Include after the ObjectARX headers; no global UCS changes or cached matrices.
namespace tvsCoordinates {
inline int transformPoint(const ads_point source, int from, int to, ads_point result)
{
    resbuf fromSystem = {}, toSystem = {};
    fromSystem.restype = toSystem.restype = RTSHORT;
    fromSystem.resval.rint = static_cast<short>(from);
    toSystem.resval.rint = static_cast<short>(to);
    return acedTrans(source, &fromSystem, &toSystem, 0, result);
}

inline int getPoint(const ads_point baseWcs, const ACHAR *prompt, ads_point resultWcs)
{
    ads_point baseUcs, pickedUcs, pickedWcs;
    if (baseWcs && transformPoint(baseWcs, 0, 1, baseUcs) != RTNORM)
        return RTCAN;
    const int status = acedGetPoint(baseWcs ? baseUcs : NULL, prompt, pickedUcs);
    if (status != RTNORM) return status; // Preserve keywords, Enter and cancellation.
    if (transformPoint(pickedUcs, 1, 0, pickedWcs) != RTNORM) return RTCAN;
    for (int i = 0; i < 3; ++i) resultWcs[i] = pickedWcs[i];
    return RTNORM;
}

inline int selectEntity(const ACHAR *prompt, ads_name entity, ads_point resultWcs)
{
    ads_point pickedUcs, pickedWcs;
    const int status = acedEntSel(prompt, entity, pickedUcs);
    if (status != RTNORM) return status;
    if (transformPoint(pickedUcs, 1, 0, pickedWcs) != RTNORM) return RTCAN;
    for (int i = 0; i < 3; ++i) resultWcs[i] = pickedWcs[i];
    return RTNORM;
}
}
