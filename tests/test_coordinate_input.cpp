#include <cassert>
#include <cmath>
#include <cstddef>
typedef double ads_point[3];
typedef long ads_name[2];
typedef wchar_t ACHAR;
struct resbuf { int restype; union { short rint; } resval; };
const int RTSHORT=5003, RTNORM=5100, RTCAN=-5002, RTKWORD=-5005, RTNONE=5000;
static double angle=0, origin[3]={0,0,0};
static bool failTransform=false, hasBase=false;
static int inputStatus=RTNORM;
static ads_point input={12,34,5}, seenBase;
int acedTrans(const ads_point p,const resbuf* from,const resbuf* to,int displacement,ads_point out) {
 assert(!displacement); assert(from->restype==RTSHORT && to->restype==RTSHORT);
 if(failTransform)return -1;
 const double c=std::cos(angle),s=std::sin(angle);
 if(from->resval.rint==1 && to->resval.rint==0){
 out[0]=origin[0]+c*p[0]-s*p[1];out[1]=origin[1]+s*p[0]+c*p[1];out[2]=origin[2]+p[2];
 }else{assert(from->resval.rint==0 && to->resval.rint==1);
 double x=p[0]-origin[0],y=p[1]-origin[1];out[0]=c*x+s*y;out[1]=-s*x+c*y;out[2]=p[2]-origin[2];}
 return RTNORM;
}
int acedGetPoint(const ads_point base,const ACHAR*,ads_point out){
 hasBase=base!=NULL;for(int i=0;i<3;++i){if(base)seenBase[i]=base[i];out[i]=input[i];}return inputStatus;
}
int acedEntSel(const ACHAR*,ads_name entity,ads_point out){entity[0]=42;for(int i=0;i<3;++i)out[i]=input[i];return inputStatus;}
#include "../TVS_Ventilation_ARX/CoordinateInput.h"
static void equal(const ads_point a,const ads_point b){for(int i=0;i<3;++i)assert(std::fabs(a[i]-b[i])<1e-8);}
int main(){
 const double rotations[]={0,0.523598775598299,1.570796326794897,-0.785398163397448};
 const double offsets[]={0,10000};
 const double units[]={1,10,1000};
 for(int r=0;r<4;++r)for(int o=0;o<2;++o)for(int u=0;u<3;++u){
 angle=rotations[r];origin[0]=offsets[o]/units[u];origin[1]=-2000/units[u];origin[2]=300/units[u];
 input[0]=1200/units[u];input[1]=3400/units[u];input[2]=50/units[u];
 ads_point expected={origin[0]+std::cos(angle)*input[0]-std::sin(angle)*input[1],origin[1]+std::sin(angle)*input[0]+std::cos(angle)*input[1],origin[2]+input[2]},out;
 assert(tvsCoordinates::getPoint(NULL,L"",out)==RTNORM);assert(!hasBase);equal(out,expected);
 // Base and output can alias, as in command loops. Base must go back to UCS.
 assert(tvsCoordinates::getPoint(out,L"",out)==RTNORM);assert(hasBase);equal(seenBase,input);equal(out,expected);
 ads_name entity={0,0};assert(tvsCoordinates::selectEntity(L"",entity,out)==RTNORM);assert(entity[0]==42);equal(out,expected);
 }
 const int statuses[]={RTCAN,RTKWORD,RTNONE,-1};
 for(int k=0;k<4;++k){inputStatus=statuses[k];ads_point out={9,8,7},saved={9,8,7};ads_name entity={0,0};
 assert(tvsCoordinates::getPoint(NULL,L"",out)==inputStatus);equal(out,saved);
 assert(tvsCoordinates::selectEntity(L"",entity,out)==inputStatus);equal(out,saved);}
 inputStatus=RTNORM;failTransform=true;ads_point out={9,8,7},saved={9,8,7};ads_name entity={0,0};
 assert(tvsCoordinates::getPoint(NULL,L"",out)==RTCAN);equal(out,saved);
 assert(tvsCoordinates::getPoint(out,L"",out)==RTCAN);equal(out,saved);
 assert(tvsCoordinates::selectEntity(L"",entity,out)==RTCAN);equal(out,saved);
}
