static int checks=0,cases=0;
static void require(bool b){++checks;if(!b){std::cerr<<"FAILED check "<<checks<<" case "<<cases<<"\n";std::exit(1);}}
static void equal(double a,double b,double eps=1e-8){require(std::fabs(a-b)<=eps);}
static void pointEqual(const AcGePoint3d& a,const tvsOffset::WorldPoint& b,double f){equal(a.x,b.x,1e-7/f);equal(a.y,b.y,1e-7/f);equal(a.z,b.z,1e-7/f);}
int main(){
 using namespace tvsOffset;namespace adapter=tvsOffsetAdapter;
 const double units[]={1,10,1000},headings[]={0,.6457718232379019,pi()/2,pi(),-2.2};
 const double angles[]={30,45,60},dims[][2]={{400,200},{200,600},{600,200}};
 for(int u=0;u<3;++u)for(int h=0;h<5;++h)for(int a=0;a<3;++a)
 for(int sign=-1;sign<=1;sign+=2)for(int d=0;d<3;++d)for(int mode=0;mode<2;++mode){
  ++cases;double f=units[u],ri=mode==0?dims[d][1]*.5:150;
  Input input(sign*2000,dims[d][0],dims[d][1],ri,angles[a]*pi()/180,0);
  Result geometry=solve(input);require(geometry.status==Ok);
  adapter::Placement placement(WorldPoint(12000/f,-5000/f,37/f),headings[h],f,-400);
  adapter::Result dto=adapter::adapt(input,geometry,placement);require(dto.ready());
  equal(dto.elbows[0].middle.x,placement.firstMiddle.x,1e-8/f);
  equal(dto.elbows[0].middle.y,placement.firstMiddle.y,1e-8/f);
  equal(dto.elbows[0].elevation*f,-400);equal(dto.elbows[1].elevation*f,-400+sign*2000);
  equal(dto.middle.physicalLengthMm,geometry.straightLength);
  equal(dto.middle.projectedLengthMm,geometry.projectedLength);
  equal(dto.middle.firstElevationMm,-400+geometry.axis[1].z);
  equal(dto.middle.lastElevationMm,-400+geometry.axis[2].z);
  equal(dto.middle.lastElevationMm-dto.middle.firstElevationMm,geometry.axis[2].z-geometry.axis[1].z);
  for(int j=0;j<2;++j){
   const adapter::Elbow& e=dto.elbows[j];TVS_TAP tap;
   equal(e.sizeA*f,dims[d][0]);equal(e.sizeB*f,dims[d][1]);
   tap.units=f;tap.SizeA=e.sizeA;tap.SizeB=e.sizeB;tap.RadiusConst=e.innerRadius;
   tap.RadiusVariableParameter=.5;
   tap.RadiusTypeRect=mode==0?RadiusTypeRect_RadiusVariable:RadiusTypeRect_RadiusConstant;
   tap.Swectangle=e.angle;tap.MiddlePoint=AcGePoint3d(e.middle.x,e.middle.y,e.middle.z);
   tap.Startvector=AcGeVector3d(e.startVector.x,e.startVector.y,e.startVector.z);
   require(e.form==(((sign>0)==(j==0))?adapter::Up:adapter::Down));
   tap.put_Form(e.form==adapter::Up?Form_Up:Form_Down);
   equal(tap.get_radius(),e.innerRadius);
   // Poison the stored redraw state AFTER form setup. project executes real expressions afresh.
   tap.Radius=-987654;tap.MA=AcGePoint3d(-999,-888,-777);tap.MC=AcGePoint3d(999,888,777);
   tap.project();pointEqual(tap.MA,e.expectedMA,f);pointEqual(tap.MC,e.expectedMC,f);
   equal(tap.MA.z,37/f);equal(tap.MC.z,37/f);
   equal(e.normal.x,0);equal(e.normal.y,0);equal(e.normal.z,1);
   if(j==0){pointEqual(tap.MA,dto.incomingLast,f);pointEqual(tap.MC,dto.middle.first,f);}
   else {pointEqual(tap.MA,dto.outgoingFirst,f);pointEqual(tap.MC,dto.middle.last,f);}
  }
 }
 std::cout<<"PASS "<<cases<<" mappings through extracted real TAP radius/form/projection expressions (30/45/60, rise/fall, 5 headings, mm/cm/m, A!=B, 2 radius modes)\n";
 Input input(400,400,200,100,pi()/4,0);Result geometry=solve(input);
 adapter::Placement placement(WorldPoint(1000,2000,37),0,1,-400);
 adapter::Result dto=adapter::adapt(input,geometry,placement);require(dto.ready());
 equal(dto.incomingLast.x,858.5786437626905);equal(dto.middle.first.x,1000);
 equal(dto.middle.last.x,1282.842712474619);equal(dto.outgoingFirst.x,1424.264068711929);
 equal(dto.middle.firstElevationMm,-341.4213562373095);equal(dto.middle.lastElevationMm,-58.5786437626905);
 equal(dto.elbows[0].elevation,-400);equal(dto.elbows[1].elevation,0);
 placement.startElevationMm=1e9;placement.firstMiddle.z=-73;
 adapter::Result other=adapter::adapt(input,geometry,placement);require(other.ready());
 equal(other.middle.first.x,dto.middle.first.x);equal(other.middle.last.x,dto.middle.last.x);
 equal(other.middle.first.z,-73);equal(other.middle.last.z,-73);
 equal(other.middle.physicalLengthMm,dto.middle.physicalLengthMm);
 std::cout<<"PASS fixed placement oracle and independent drawing Z / physical reference elevation\n";
 Result none=solve(Input(0,400,200,100,pi()/4,0));
 require(adapter::adapt(input,none,placement).status==adapter::NoGeometry);
 placement.mmPerUnit=0;require(adapter::adapt(input,geometry,placement).status==adapter::InvalidParameters);placement.mmPerUnit=1;
 Result mismatched=geometry;mismatched.axis[3].z=401;
 require(adapter::adapt(input,mismatched,placement).status==adapter::InconsistentResult);
 placement.firstMiddle=WorldPoint(1e30,1e30,37);
 require(adapter::adapt(input,geometry,placement).status==adapter::UnrepresentablePlacement);
 // Exact zero-length middle is legal in the pure adapter, no production minimum is introduced.
 Input zero(117.1572875253809902396622551580603843,400,200,100,pi()/4,0);
 Result degenerate=solve(zero);require(degenerate.status==Ok);
 placement.firstMiddle=WorldPoint();require(adapter::adapt(zero,degenerate,placement).ready());
 std::cout<<"PASS invalid/absent geometry, mismatched pair, lost coordinate precision, Lmin=0\n";
 std::cout<<"STUB LIMITATION: no ObjectARX runtime, DB, drawing pipeline or real AcGe rotation executed\n";
 std::cout<<"ALL PASSED: "<<checks<<" checks\n";return 0;
}
