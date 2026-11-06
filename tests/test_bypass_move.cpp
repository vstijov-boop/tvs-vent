// SDK-independent regression: an obilazak (TVS_BYPASS) is one rigid body between its inlet and
// outlet ducts; its six other members are empty bodies (see tvsMove::fuseBypasses).
#define main originalMoveNetworkTests
#include "test_move_network.cpp"
#undef main
static std::vector<Body> bypassLine(bool fused){
    std::vector<Body> n;
    n.push_back(body(Pipe,Point(-100,-1000),Point(-100,-100),250));            // 0 vertical duct
    n.push_back(elbow(Point(-100,-100),Point(0,0),250));n.back().axes[0]=Point(0,1);n.back().axes[1]=Point(1,0); // 1
    n.push_back(body(Pipe,Point(0,0),Point(1000,0),250));                      // 2 inlet
    if(fused){n.push_back(body(Rigid,Point(1000,0),Point(2500,0),250));        // 3 obilazak
        for(int m=0;m<6;++m)n.push_back(Body());}                              // 4..9 other members
    else{n.push_back(Body());n.back().ports.push_back(Point(1000,0));n.back().ports.push_back(Point(1100,0));} // vertical elbow, kind 0
    n.push_back(body(Pipe,Point(2500,0),Point(4000,0),250));                   // outlet
    n.push_back(elbow(Point(4000,0),Point(4100,-100),250));
    n.push_back(body(Pipe,Point(4100,-100),Point(4100,-1000),250));
    return n;
}
int main(){
    originalMoveNetworkTests();
    Result out;
    // TVS_STRETCH: window cuts inlet and outlet and holds the whole obilazak.
    {std::vector<Body> n=bypassLine(true);std::vector<int> masks(n.size(),0);
     masks[2]=2;masks[3]=3;masks[10]=1;
     assert(planSelection(n,masks,Point(300,0),1,.1,out));assert(out.problem==-1);assert(out.changed==3);
     assert(tvsMoveGeometry::pointsNear(out.ports[2][1],Point(1300,0),1e-9));   // inlet longer
     assert(tvsMoveGeometry::pointsNear(out.ports[3][0],Point(1300,0),1e-9));
     assert(tvsMoveGeometry::pointsNear(out.ports[3][1],Point(2800,0),1e-9));
     assert(tvsMoveGeometry::pointsNear(out.ports[10][0],Point(2800,0),1e-9));  // outlet shorter
     assert(!planSelection(n,masks,Point(0,300),1,.1,out));                     // lateral stretch forbidden
     masks[3]=1;assert(!planSelection(n,masks,Point(300,0),1,.1,out));assert(out.problem==3); // window cuts the obilazak
    }
    // Regression (881a629..691bc75): unfused vertical elbow is Blocked, so the stretch is refused.
    {std::vector<Body> n=bypassLine(false);std::vector<int> masks(n.size(),0);masks[2]=2;masks[3]=3;masks[4]=1;
     assert(!planSelection(n,masks,Point(300,0),1,.1,out));assert(out.error==UnsupportedBody);}
    // TVS_MOVE: moving the inlet sideways carries the obilazak and the outlet; vertical ducts stretch.
    {std::vector<Body> n=bypassLine(true);
     assert(plan(n,2,Point(0,300),1,.1,out));assert(out.problem==-1);
     assert(out.masks[3]==3&&out.masks[10]==3);
     assert(tvsMoveGeometry::pointsNear(out.ports[3][1],Point(2500,300),1e-9));
     assert(tvsMoveGeometry::pointsNear(out.ports[0][0],Point(-100,-1000),1e-9));
     assert(tvsMoveGeometry::pointsNear(out.ports[12][1],Point(4100,-1000),1e-9));
     for(int m=4;m<10;++m)assert(!out.masks[m]);
    }
    return 0;
}
