#define main originalMoveNetworkTests
#include "test_move_network.cpp"
#undef main
int main(){
    originalMoveNetworkTests();
    const double scales[]={1,10,1000},angles[]={0,.523598775598299,1.570796326794897,-.785398163397448};
    for(int u=0;u<3;++u)for(int r=0;r<4;++r){
        std::vector<Body> n=fixture();double f=scales[u],a=angles[r];
        for(size_t i=0;i<n.size();++i){
            for(size_t p=0;p<n[i].ports.size();++p){n[i].ports[p]=turn(n[i].ports[p],a,f,false);n[i].axes[p]=turn(n[i].axes[p],a,1,true);n[i].sections[p].a/=f;n[i].sections[p].b/=f;}
            for(size_t p=0;p<n[i].junctions.size();++p)n[i].junctions[p]=turn(n[i].junctions[p],a,f,false);
        }
        int chosen[]={3,3,7,1,3,3,3,3,1,1};
        std::vector<int> masks(chosen,chosen+10);Result out;
        Point delta=turn(Point(0,300),a,f,true);
        assert(planSelection(n,masks,delta,1/f,.1/f,out));assert(out.changed==10);assert(out.problem==-1);
        for(size_t i=0;i<n.size();++i)for(size_t p=0;p<n[i].ports.size();++p)
            assert(tvsMoveGeometry::pointsNear(out.ports[i][p],n[i].ports[p]+((masks[i]&(1<<p))?delta:Point()),1e-7/f));
        masks[9]=0;assert(!planSelection(n,masks,delta,1/f,.1/f,out)); // must not pull an unselected branch
        masks[9]=1;masks[2]=3;assert(!planSelection(n,masks,delta,1/f,.1/f,out));assert(out.problem==2); // partial wye
        masks[2]=7;assert(!planSelection(n,masks,turn(Point(10,300),a,f,true),1/f,.1/f,out)); // lateral stretch forbidden
        assert(!planSelection(n,masks,turn(Point(0,1000),a,f,true),1/f,.1/f,out)); // inverted branch
    }
    // Two collinear ducts: stretch one selected end, leave the connected duct wholly fixed.
    std::vector<Body> n;n.push_back(body(Pipe,Point(0,0),Point(1000,0),250));
    n.push_back(body(Pipe,Point(1000,0),Point(2000,0),250));
    std::vector<int> masks(2,0);masks[0]=1;Result out;
    assert(planSelection(n,masks,Point(-200,0),1,.1,out));assert(out.changed==1);
    assert(tvsMoveGeometry::pointsNear(out.ports[1][0],n[1].ports[0],1e-9));
    assert(tvsMoveGeometry::pointsNear(out.ports[1][1],n[1].ports[1],1e-9));
    masks[0]=0;assert(!planSelection(n,masks,Point(10,0),1,.1,out));
    return 0;
}
