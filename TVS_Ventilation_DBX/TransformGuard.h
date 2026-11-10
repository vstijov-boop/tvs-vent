#pragma once
// TVS elements only move, rotate and mirror. A scaling (SCALE, a scaled block exploded, ...) would
// move the points while the sizes, radii and lengths of prelazi / T komadi stay: the network
// falls apart, so it is refused (TVS_SETTINGS changes the drawing units, a detail is a viewport).
#include "gemat3d.h"
#include "acutads.h"
#include <chrono>
#include <cmath>
namespace tvsTransform {
inline bool rigid(const AcGeMatrix3d& m){
    const AcGeVector3d axes[3]={AcGeVector3d::kXAxis,AcGeVector3d::kYAxis,AcGeVector3d::kZAxis};
    for(int i=0;i<3;++i){AcGeVector3d v=axes[i];v.transformBy(m);if(std::fabs(v.length()-1)>1e-6)return false;}
    return true;
}
// Message once per command (every selected element is asked), eNotApplicable.
inline Acad::ErrorStatus refuse(){
    static long long last=0;
    const long long now=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    if(now-last>3000)acutPrintf(L"\nTVS elementi se ne skaliraju (dimenzije i dužine fitinga bi ostale iste i mreža bi se raspala). Jedinice crteža: TVS_SETTINGS.");
    last=now;return Acad::eNotApplicable;
}
}
