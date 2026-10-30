#ifndef TVS_OFFSET_RECORD_RULES_H
#define TVS_OFFSET_RECORD_RULES_H
#include <cmath>
#include <cfloat>
namespace tvsOffsetRecordRules {
enum ScalarStatus { ScalarValid,ScalarCorrupt,ScalarUnsupported };
inline ScalarStatus validate(int version,int role,bool validGuid,double rise){
    if(version!=1)return ScalarUnsupported;
    if(!validGuid||role<0||role>2||!(rise>=-DBL_MAX&&rise<=DBL_MAX)||(role!=1&&rise!=0))return ScalarCorrupt;
    return ScalarValid;
}
inline bool physical(double projection,double rise,double& length){
    if(!(projection>=0&&projection<=DBL_MAX&&rise>=-DBL_MAX&&rise<=DBL_MAX))return false;
    double a=projection,b=std::fabs(rise),largest=a>b?a:b;
    if(largest==0){length=0;return true;}
    a/=largest;b/=largest;double value=largest*std::sqrt(a*a+b*b);
    if(!(value>=0&&value<=DBL_MAX))return false;length=value;return true;
}
}
#endif
