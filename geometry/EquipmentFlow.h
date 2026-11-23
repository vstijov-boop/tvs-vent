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
//   Terminal at the vertical outlet of a koljeno gore / dolje (TVS_TAP Form_Up / Form_Down): the
//   elbow's port 1 is the outlet in plan; its level is the elbow axis kota -/+ r (1 - cos sweep)
//   (outletLevel), the level the port is matched with (EquipmentNodes.h).
//   Resetka na dnu (tvsEquipment::Bottom): port[0] is the tap point on a duct axis (pickDuct: in
//   plan on the axis, inside the duct, nearest kota). The readers split the duct there (not in the
//   drawing): two pieces and a tap junction (tapKind) with the grille as a zero-length branch
//   (FlowCommand.h splitBottom; losses geometry/PressureDrop.h BottomTap).
#include <cmath>
#include <vector>
namespace tvsEquipmentFlow {
// tvsMove::Node::kind of a linked equipment block (1 duct, 2 elbow, 3 prelaz, 4 T komad, 0 other)
inline int nodeKind(){return 5;}
// tvsMove::Node::kind of the tap junction of a resetka na dnu (made by the readers, not in the drawing)
inline int tapKind(){return 6;}
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
// level of the vertical outlet of a koljeno gore / dolje: axis kota `elevation`, up / down, axis
// radius r (inner radius + side in the bend plane / 2), sweep in radians (as OffsetService.h)
inline double outletLevel(double elevation,bool up,double r,double sweep){
    return elevation+(up?1:-1)*r*(1-std::cos(sweep));
}
// resetka na dnu: t (0..1) of the plan point p on the duct axis a -> b when p is within tol of the
// axis and more than tol inside both ends; -1 otherwise (also at an end: that is a terminal AtEnd)
inline double onAxis(double ax,double ay,double bx,double by,double px,double py,double tol){
    const double ux=bx-ax,uy=by-ay,l2=ux*ux+uy*uy;if(!(l2>0))return -1;
    const double t=((px-ax)*ux+(py-ay)*uy)/l2,l=std::sqrt(l2);
    if(t*l<=tol||(1-t)*l<=tol)return -1;
    return std::hypot(ax+ux*t-px,ay+uy*t-py)<=tol?t:-1;
}
struct Axis {double ax,ay,bx,by,level;};   // a duct in plan and its kota
// the duct a resetka na dnu at (x, y) with level `level` sits on: on its axis (tol), nearest kota
// (stacked ducts); -1 none. t: the tap point on that duct (0..1 from a)
inline int pickDuct(const std::vector<Axis>& d,double x,double y,double level,double tol,double& t){
    int best=-1;double off=0;t=-1;
    for(size_t k=0;k<d.size();++k){const double s=onAxis(d[k].ax,d[k].ay,d[k].bx,d[k].by,x,y,tol);if(s<0)continue;
        const double o=std::fabs(d[k].level-level);if(best<0||o<off){best=static_cast<int>(k);off=o;t=s;}}
    return best;
}
// the three ports of the tap junction lie this far from the tap point (along the duct both ways,
// and down for the branch), so that the flow network joins each pair only (it joins within tol)
inline double tapOffset(double tol){return 2*tol;}
// terminal flow [m3/h]: record first, then the duct's; conflict = both set and different (> 0.5 m3/h)
inline double terminalFlow(double record,double duct,bool& conflict){
    conflict=record>0&&duct>0&&std::fabs(record-duct)>0.5;
    return record>0?record:(duct>0?duct:0);
}
}
