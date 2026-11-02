#ifndef TVS_SLOPE_PROJECTION_H
#define TVS_SLOPE_PROJECTION_H
#include "OffsetGeometry.h"
#include "OffsetRecordRules.h"
// Generic projection of two parallel rectangular joint planes, not another offset solver.
namespace tvsSlopeProjection {
inline bool joints(const tvsOffset::WorldPoint& first,const tvsOffset::WorldPoint& last,
                   double width,double depth,double rise,tvsOffset::WorldPoint points[2][4]){
    double dx=last.x-first.x,dy=last.y-first.y,projection=0,length=0;
    if(!tvsOffsetRecordRules::physical(std::fabs(dx),dy,projection)||projection<=0
        ||!tvsOffsetRecordRules::physical(projection,rise,length)||length<=0||!(width>0&&width<=DBL_MAX&&depth>0&&depth<=DBL_MAX))return false;
    double ux=dx/projection,uy=dy/projection,ns=-rise/length;
    for(int j=0;j<2;++j){const tvsOffset::WorldPoint& base=j==0?first:last;
        for(int k=0;k<4;++k){double transverse=(k&1?1:-1)*width/2,along=(k&2?1:-1)*depth/2*ns;
            points[j][k]=tvsOffset::WorldPoint(base.x+ux*along-uy*transverse,base.y+uy*along+ux*transverse,base.z);
            if(!tvsOffset::finite(points[j][k].x)||!tvsOffset::finite(points[j][k].y)||!tvsOffset::finite(points[j][k].z))return false;}}
    return true;
}
// Round sloped straight (V1 member of a round bypass): plan silhouette of a tube of diameter
// D is two side lines at +-D/2 between the end points (side points of a tilted end circle
// stay on the axis station); the tilted end faces are drawn by the adjacent round elbows.
inline bool roundSides(const tvsOffset::WorldPoint& first,const tvsOffset::WorldPoint& last,double diameter,
                       tvsOffset::WorldPoint sides[2][2]){
    const double dx=last.x-first.x,dy=last.y-first.y,len=std::sqrt(dx*dx+dy*dy); // C++98: DBX geometry header
    if(!(len>0)||!(diameter>0&&diameter<=DBL_MAX))return false;
    const double nx=-dy/len*diameter/2,ny=dx/len*diameter/2;
    for(int k=0;k<2;++k){const double s=k?1:-1;
        sides[k][0]=tvsOffset::WorldPoint(first.x+s*nx,first.y+s*ny,first.z);
        sides[k][1]=tvsOffset::WorldPoint(last.x+s*nx,last.y+s*ny,first.z);
        if(!tvsOffset::finite(sides[k][0].x)||!tvsOffset::finite(sides[k][0].y)||!tvsOffset::finite(sides[k][1].x)||!tvsOffset::finite(sides[k][1].y))return false;}
    return true;
}
}
#endif
