#pragma once
// TVS_TK_TIP dialog sketch (plan, schematic, in the style of TVS_TK_TIP_mockup.html): the T komad of
// the chosen type and, on the selected port, what follows it (elbow, prelaz, duct). Pure geometry,
// no Windows / AutoCAD: the dialog draws the primitives with GDI and tests/tee_sketch_svg.cpp writes
// the same primitives as SVG to check every case as a picture. Coordinates: screen-like (y down),
// mockup viewBox units; the dialog fits the bounding box of the primitives into its frame.
#include <cmath>
#include <string>
#include <vector>
namespace tvsTeeSketch {
enum Type {Branch=0,Pants=1,Tap=2,Plain=3};
enum Turn {Right=0,Left=1,Up=2,Down=3};
struct Pt {double x,y;Pt(double px=0,double py=0):x(px),y(py){}};
enum Kind {Polygon,Polyline,Circle,Text};
struct Prim {
    Kind kind;std::vector<Pt> pts;unsigned fill,stroke;double width,r;bool dash,bold;std::wstring text;int align;   // align: 0 left, 1 centre, 2 right
    Prim():kind(Polyline),fill(0xFFFFFFFFu),stroke(0xFFFFFFFFu),width(1),r(0),dash(false),bold(false),align(0){}
};
const unsigned none=0xFFFFFFFFu;
struct Side {bool used,hasDuct,prelaz;int elbow,turn,order;Side():used(false),hasDuct(false),prelaz(false),elbow(0),turn(Right),order(0){}};
struct Input {int type;bool flipped;int selected;Side side[2];Input():type(Branch),flipped(false),selected(0){}};

inline double pi(){return 3.14159265358979323846;}
inline void quad(std::vector<Pt>& out,Pt a,Pt c,Pt b,int n=10){
    for(int i=1;i<=n;++i){const double t=double(i)/n,u=1-t;out.push_back(Pt(u*u*a.x+2*u*t*c.x+t*t*b.x,u*u*a.y+2*u*t*c.y+t*t*b.y));}
}
struct Port {Pt at,dir;};
// T komad body and its ports B (0) and C (1); the inlet is always on the left.
inline void body(const Input& in,std::vector<Prim>& out,Port ports[2],Pt& inletLabel){
    const unsigned fill=0xE7ECF2,stroke=0xA5B4C6,axis=0x849BB3;
    Prim b;b.kind=Polygon;b.fill=fill;b.stroke=stroke;b.width=1;
    Prim ax;ax.kind=Polyline;ax.stroke=axis;ax.dash=true;ax.width=1;
    std::vector<Prim> axes;
    if(in.type==Pants){   // inlet from the left, branches up (B) and down (C), as TVS TK-G: outlets opposite
        b.pts.push_back(Pt(96,96));b.pts.push_back(Pt(124,96));quad(b.pts,Pt(124,96),Pt(140,96),Pt(140,80));
        b.pts.push_back(Pt(140,70));b.pts.push_back(Pt(160,70));b.pts.push_back(Pt(160,146));b.pts.push_back(Pt(140,146));
        b.pts.push_back(Pt(140,136));quad(b.pts,Pt(140,136),Pt(140,120),Pt(124,120));b.pts.push_back(Pt(96,120));
        ax.pts.push_back(Pt(96,108));ax.pts.push_back(Pt(150,108));axes.push_back(ax);
        ax.pts.clear();ax.pts.push_back(Pt(150,70));ax.pts.push_back(Pt(150,146));axes.push_back(ax);
        ports[0].at=Pt(150,70);ports[0].dir=Pt(0,-1);ports[1].at=Pt(150,146);ports[1].dir=Pt(0,1);
    }else{
        b.pts.push_back(Pt(96,96));
        if(in.type==Branch){b.pts.push_back(Pt(126,96));quad(b.pts,Pt(126,96),Pt(140,96),Pt(140,79));b.pts.push_back(Pt(160,79));b.pts.push_back(Pt(160,86));quad(b.pts,Pt(160,86),Pt(160,96),Pt(178,96));}
        else if(in.type==Tap){b.pts.push_back(Pt(128,96));b.pts.push_back(Pt(142,82));b.pts.push_back(Pt(160,82));b.pts.push_back(Pt(160,96));b.pts.push_back(Pt(178,96));}
        else{b.pts.push_back(Pt(140,96));b.pts.push_back(Pt(140,79));b.pts.push_back(Pt(160,79));b.pts.push_back(Pt(160,96));b.pts.push_back(Pt(178,96));}
        b.pts.push_back(Pt(178,120));b.pts.push_back(Pt(96,120));
        const double top=in.type==Tap?82:79;
        ax.pts.push_back(Pt(96,108));ax.pts.push_back(Pt(178,108));axes.push_back(ax);
        ax.pts.clear();ax.pts.push_back(Pt(150,108));ax.pts.push_back(Pt(150,top));axes.push_back(ax);
        ports[0].at=Pt(150,top);ports[0].dir=Pt(0,-1);ports[1].at=Pt(178,108);ports[1].dir=Pt(1,0);
    }
    out.push_back(b);
    if(in.type==Branch){Prim seam;seam.kind=Polyline;seam.stroke=0xB6C4D2;seam.pts.push_back(Pt(121,96));seam.pts.push_back(Pt(121,120));out.push_back(seam);}
    for(size_t i=0;i<axes.size();++i)out.push_back(axes[i]);
    inletLabel=Pt(99,92);
}
// What follows a port: optional prelaz before the elbow, elbow (plan arc), optional prelaz after it,
// the duct; Gore / Dolje: a riser symbol. Widths: port 10, after a prelaz 6 (schematic, as the mockup).
inline void chain(const Side& s,const Port& p,bool selected,bool flipped,std::vector<Prim>& out){
    const bool vertical=s.elbow&&(s.turn==Up||s.turn==Down);
    if(!selected&&!s.hasDuct)return;
    const double angle=vertical?0:s.elbow*pi()/180;
    double sign=s.turn==Left?-1:1;if(flipped)sign=-sign;   // the sketch is mirrored, the turn is not
    const Pt v=p.dir,n(-v.y,v.x);
    std::vector<Pt> pts;std::vector<double> w;pts.push_back(Pt(0,0));
    const bool tr=s.prelaz;const double big=10,small=6,radius=25;
    double width=big;w.push_back(width);
    if(selected){
        if(tr&&s.order==1){pts.push_back(Pt(30,0));width=small;w.push_back(width);}
        if(angle>0){const Pt o=pts.back();
            for(int i=1;i<=20;++i){const double t=angle*i/20;pts.push_back(Pt(o.x+radius*std::sin(t),o.y+sign*radius*(1-std::cos(t))));w.push_back(width);}}
        const Pt d(std::cos(angle),sign*std::sin(angle));
        if(tr&&s.order==0){const Pt o=pts.back();pts.push_back(Pt(o.x+d.x*30,o.y+d.y*30));width=small;w.push_back(width);}
        {const Pt o=pts.back();const double L=s.hasDuct?34:vertical?16:12;pts.push_back(Pt(o.x+d.x*L,o.y+d.y*L));w.push_back(width);}
    }else{pts.push_back(Pt(26,0));w.push_back(tr?small:big);}
    auto map=[&](const Pt& q){return Pt(p.at.x+v.x*q.x+n.x*q.y,p.at.y+v.y*q.x+n.y*q.y);};
    if(pts.size()>1){
        // prelaz: widths change linearly over the cone (from the point before the narrower width)
        std::vector<Pt> left,right,axis;
        for(size_t i=0;i<pts.size();++i){
            const Pt a=pts[i>0?i-1:0],b=pts[i+1<pts.size()?i+1:i];double dx=b.x-a.x,dy=b.y-a.y,l=std::sqrt(dx*dx+dy*dy);if(l<=0)l=1;
            const double h=w[i];
            left.push_back(map(Pt(pts[i].x-dy/l*h,pts[i].y+dx/l*h)));right.push_back(map(Pt(pts[i].x+dy/l*h,pts[i].y-dx/l*h)));axis.push_back(map(pts[i]));
        }
        Prim poly;poly.kind=Polygon;poly.pts=left;for(size_t i=right.size();i-->0;)poly.pts.push_back(right[i]);
        poly.fill=selected?0xE3F0FC:0xF1F4F8;poly.stroke=selected?0x0863A9:0xB6C4D2;poly.width=selected?1.3:1;out.push_back(poly);
        Prim ax;ax.kind=Polyline;ax.pts=axis;ax.stroke=selected?0x0863A9:0xB6C4D2;ax.dash=true;out.push_back(ax);
        if(selected)for(size_t i=1;i+1<pts.size();++i)if(std::fabs(w[i]-w[i+1])>1e-9||(angle>0&&(i==1||i+2==pts.size()))){   // joints
            Prim j;j.kind=Polyline;j.stroke=0x4788B9;j.pts.push_back(left[i]);j.pts.push_back(right[i]);out.push_back(j);}
        if(vertical&&selected){Prim c;c.kind=Circle;c.pts.push_back(axis.back());c.r=9;c.fill=0xE3F0FC;c.stroke=0x0863A9;c.width=1.3;out.push_back(c);
            if(s.turn==Up){Prim dot;dot.kind=Circle;dot.pts.push_back(axis.back());dot.r=3;dot.fill=0x0863A9;dot.stroke=0x0863A9;out.push_back(dot);}
            else{const Pt e=axis.back();Prim x1;x1.kind=Polyline;x1.stroke=0x0863A9;x1.width=1.3;x1.pts.push_back(Pt(e.x-6,e.y-6));x1.pts.push_back(Pt(e.x+6,e.y+6));out.push_back(x1);
                Prim x2=x1;x2.pts.clear();x2.pts.push_back(Pt(e.x-6,e.y+6));x2.pts.push_back(Pt(e.x+6,e.y-6));out.push_back(x2);}}
    }
}
// All primitives, in viewBox units, mirrored when flipped (texts stay readable).
inline void build(const Input& in,std::vector<Prim>& out){
    out.clear();Port ports[2];Pt inlet;body(in,out,ports,inlet);
    for(int k=0;k<2;++k)if(in.side[k].used&&k!=in.selected)chain(in.side[k],ports[k],false,in.flipped,out);
    if(in.side[in.selected].used)chain(in.side[in.selected],ports[in.selected],true,in.flipped,out);
    for(int k=0;k<2;++k)if(in.side[k].used){Prim c;c.kind=Circle;c.pts.push_back(ports[k].at);c.r=k==in.selected?4:3;
        c.fill=k==in.selected?0x0068AE:0xE7ECF2;c.stroke=0x8FA7BD;out.push_back(c);}
    Prim t;t.kind=Text;t.fill=0x275476;
    t.pts.assign(1,inlet);t.text=L"Ulaz";t.align=0;out.push_back(t);
    const wchar_t* names[2]={L"B",L"C"};
    for(int k=0;k<2;++k)if(in.side[k].used){t.pts.assign(1,Pt(ports[k].at.x+(ports[k].dir.x!=0?2:9),ports[k].at.y+(ports[k].dir.y<0?-4:ports[k].dir.y>0?12:15)));
        t.text=names[k];t.align=0;t.bold=k==in.selected;out.push_back(t);}
    if(in.flipped)for(size_t i=0;i<out.size();++i){for(size_t j=0;j<out[i].pts.size();++j)out[i].pts[j].x=300-out[i].pts[j].x;
        if(out[i].kind==Text)out[i].align=2-out[i].align;}
}
// Bounding box of the drawing (texts as small boxes).
inline void bounds(const std::vector<Prim>& p,double& x0,double& y0,double& x1,double& y1){
    x0=y0=1e9;x1=y1=-1e9;
    for(size_t i=0;i<p.size();++i)for(size_t j=0;j<p[i].pts.size();++j){
        const double r=p[i].kind==Circle?p[i].r:p[i].kind==Text?10:0,x=p[i].pts[j].x,y=p[i].pts[j].y;
        const double tw=p[i].kind==Text?(p[i].align==2?-22.0:p[i].align==1?-11.0:0.0):0;
        if(x-r+tw<x0)x0=x-r+tw;if(x+r+(p[i].kind==Text?22+tw:0)>x1)x1=x+r+(p[i].kind==Text?22+tw:0);
        if(y-r<y0)y0=y-r;if(y+(p[i].kind==Text?2:r)>y1)y1=y+(p[i].kind==Text?2:r);}
}
}
