#pragma once
// TVS elements only move, rotate and mirror. A scaling (SCALE, a scaled block exploded, ...) would
// move the points while the sizes, radii and lengths of prelazi / T komadi stay: the network
// falls apart, so it is refused (TVS_SETTINGS changes the drawing units, a detail is a viewport).
#include "gemat3d.h"
#include "acutads.h"
#include <chrono>
#include <cmath>
#include <windows.h>
// A uniform scaling is taken (points AND sizes, radii, lengths scaled) only while the ARX module
// allows it: pasting / inserting / exploding between drawings with other units, TVS_PRERACUNAJ
// (process environment TVS_SCALE_OK=1, set by ARX ScaleWindow.h; the two modules share nothing else).
namespace tvsTransform {
inline bool scalingAllowed(){wchar_t v[4]={0};return GetEnvironmentVariableW(L"TVS_SCALE_OK",v,4)>0&&v[0]==L'1';}
// Uniform scale factor s of m (the three axes equally long); false when not uniform.
inline bool uniform(const AcGeMatrix3d& m,double& s){
    const AcGeVector3d axes[3]={AcGeVector3d::kXAxis,AcGeVector3d::kYAxis,AcGeVector3d::kZAxis};double l[3];
    for(int i=0;i<3;++i){AcGeVector3d v=axes[i];v.transformBy(m);l[i]=v.length();}
    s=l[0];return s>1e-12&&std::fabs(l[1]-s)<=1e-9*s&&std::fabs(l[2]-s)<=1e-9*s;
}
// Sizes of every TVS element (TVS_Entity members, drawing units).
template<class T> inline void scaleCommon(T* e,double s){e->SizeA*=s;e->SizeB*=s;e->Elev*=s;e->ElevUp*=s;e->ElevDown*=s;e->WipeoutLength*=s;}
inline bool rigid(const AcGeMatrix3d& m){
    const AcGeVector3d axes[3]={AcGeVector3d::kXAxis,AcGeVector3d::kYAxis,AcGeVector3d::kZAxis};
    for(int i=0;i<3;++i){AcGeVector3d v=axes[i];v.transformBy(m);if(std::fabs(v.length()-1)>1e-6)return false;}
    return true;
}
// subTransformBy: 1 = rigid (move / rotate / mirror), s > 0 = allowed uniform scaling, 0 = refuse.
inline double accepted(const AcGeMatrix3d& m){
    if(rigid(m))return 1;double s=0;
    return uniform(m,s)&&scalingAllowed()?s:0;
}
// Message once per command (every selected element is asked), eNotApplicable.
inline Acad::ErrorStatus refuse(){
    static long long last=0;
    const long long now=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    if(now-last>3000)acutPrintf(L"\nTVS elementi se ne skaliraju (dimenzije i dužine fitinga bi ostale iste i mreža bi se raspala). Jedinice crteža: TVS_SETTINGS.");
    last=now;return Acad::eNotApplicable;
}
}
