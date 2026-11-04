#include "../geometry/bypass/Generator.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#if defined(__SSE2__) || defined(_M_X64)
#include <xmmintrin.h>
#endif
#ifdef _MSC_VER
#include <float.h>
#endif
using namespace tvsBypass;
static int tests=0;
static void expect(bool value,const char* text){++tests;if(!value){std::cerr<<"FAIL FP "<<text<<std::endl;std::exit(1);}}
struct State {
 int rounding,flags;unsigned csr,control;
 State():rounding(std::fegetround()),flags(std::fetestexcept(FE_ALL_EXCEPT)),csr(0),control(0){
#if defined(__SSE2__) || defined(_M_X64)
 csr=_mm_getcsr();
#endif
#ifdef _MSC_VER
 _controlfp_s(&control,0,0);
#endif
 }
 bool same(const State& b)const{return rounding==b.rounding&&flags==b.flags&&csr==b.csr&&control==b.control;}
};
int main(){
 std::cout<<"FP default supported="<<bounded::supported()<<" rounding="<<std::fegetround()
 <<" FLT_EVAL_METHOD="<<FLT_EVAL_METHOD<<std::endl;
 expect(bounded::supported(),"default strict environment must be supported");
 Input p(100,100,.1);p.D=400;p.C=400;p.s0=-765.685424949238;
 {
  bounded::Environment test;
  expect(test.active,"can hold environment");
  std::feclearexcept(FE_ALL_EXCEPT);std::feraiseexcept(FE_INEXACT|FE_DIVBYZERO);
  State before;Result r=check(p);State after;
  expect(r.status==Feasible,"normal numerical execution");
  expect(before.same(after),"successful check restores all flags and controls");
  before=State();tvsBypassSearch::Result generated=tvsBypassSearch::generate(p,tvsBypassSearch::Policy(5,10,25));after=State();
  expect(generated.hasCandidate&&before.same(after),"generator success restores all flags and controls");
  Input invalid=p;invalid.D=-1;before=State();r=check(invalid);after=State();
  expect(r.status==InvalidInput&&before.same(after),"invalid return restores state");
  Input boundary=p;boundary.policy.maxCells=1;before=State();r=check(boundary);after=State();
  expect(before.same(after),"unresolved return restores state");
  before=State();
  try {bounded::Environment local;bounded::I a(1),b(0);(void)(a/b);}
  catch(const std::exception&){}
  after=State();expect(before.same(after),"exception restores state");
  const int modes[]={FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
  for(int j=0;j<3;++j){
   expect(std::fesetround(modes[j])==0,"set alternate rounding");
   before=State();bool ok=bounded::supported();r=check(p);after=State();
   expect(!ok&&r.status==Unsupported,"alternate rounding rejected");
   expect(before.same(after),"rejected rounding not silently normalized");
   before=State();generated=tvsBypassSearch::generate(p,tvsBypassSearch::Policy(5,10,25));after=State();
   expect(generated.status==tvsBypassSearch::Unsupported&&before.same(after),"generator rejects rounding with exact state preservation");
  }
  expect(std::fesetround(FE_TONEAREST)==0,"restore nearest");
#if defined(__SSE2__) || defined(_M_X64)
  unsigned csr=_mm_getcsr();const unsigned bits[]={0x8000,0x0040,0x8040};
  for(int j=0;j<3;++j){
   _mm_setcsr(csr|bits[j]);before=State();bool ok=bounded::supported();r=check(p);after=State();
   std::cout<<"FP unsupported CSR="<<std::hex<<before.csr<<" after="<<after.csr<<std::dec<<" supported="<<ok<<std::endl;
   expect(!ok&&r.status==Unsupported,"FTZ/DAZ individually and together rejected");
   expect(before.same(after),"rejected FTZ/DAZ exact state restored");
   before=State();generated=tvsBypassSearch::generate(p,tvsBypassSearch::Policy(5,10,25));after=State();
   expect(generated.status==tvsBypassSearch::Unsupported&&before.same(after),"generator rejects FTZ/DAZ with exact state preservation");
   _mm_setcsr(csr);
  }
#endif
 }
 std::cout<<"PASS FP "<<tests<<" environment checks"<<std::endl;
}
