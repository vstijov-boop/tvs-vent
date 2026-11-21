#pragma once
// Oprema na mrezi in the network readers (TVS_PROTOK, TVS_PAD, TVS_DPD_IZVOZ, TVS_DIM_PRATI,
// TVS_PROVJERA): the pure rules (geometry/Equipment.h records, no AutoCAD).
// TVS_Ventilation_ARX/EquipmentNodes.h reads the linked blocks; tests/test_equipment_flow.cpp.
//   Ports: stored in drawing units with the record's mmPerUnit; read in the current drawing units
//   (factor = mm per unit of the drawing): x * mmPerUnit / factor.
//   Level: TVS ducts carry the kota in Elev, their end points often have z = 0; a port takes the
//   z of the duct / fitting end it lies on in plan (several stacked: the one whose kota is nearest
//   to the record's level), so the 3D joints of TVS_PROTOK see it.
//   Terminal flow: the record's qTerminal when set; else the flow written on the duct it sits on
//   (TVS_PROTOK wrote the terminal flow there before the block was linked). Both set and
//   different: the record wins, with a warning.
#include <cmath>
#include <vector>
namespace tvsEquipmentFlow {
// tvsMove::Node::kind of a linked equipment block (1 duct, 2 elbow, 3 prelaz, 4 T komad, 0 other)
inline int nodeKind(){return 5;}
inline double unitScale(double mmPerUnit,double factor){
    if(!(mmPerUnit>0)||!(factor>0))return 1.0;
    const double s=mmPerUnit/factor;return std::fabs(s-1)<1e-12?1.0:s;
}
struct Candidate {double x,y,z,level;};   // an end of another element: plan point, its z, the element's kota
// index of the candidate a port at (x, y) with level `level` joins, -1 none (plan distance <= tol)
inline int pickLevel(const std::vector<Candidate>& c,double x,double y,double level,double tol){
    int best=-1;double off=0;
    for(size_t k=0;k<c.size();++k){if(std::hypot(c[k].x-x,c[k].y-y)>tol)continue;
        const double d=std::fabs(c[k].level-level);if(best<0||d<off){best=static_cast<int>(k);off=d;}}
    return best;
}
// terminal flow [m3/h]: record first, then the duct's; conflict = both set and different (> 0.5 m3/h)
inline double terminalFlow(double record,double duct,bool& conflict){
    conflict=record>0&&duct>0&&std::fabs(record-duct)>0.5;
    return record>0?record:(duct>0?duct:0);
}
}
