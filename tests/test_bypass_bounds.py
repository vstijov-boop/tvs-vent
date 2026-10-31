"""Independent exact-Fraction checks of the numerical enclosure mechanism."""
import fractions
import math
import pathlib
import random
import subprocess
import tempfile
import unittest
F=fractions.Fraction
class BypassBounds(unittest.TestCase):
    def test_outward_arithmetic_and_trig(self):
        root=pathlib.Path(__file__).resolve().parents[1]
        header=(root/"geometry/bypass/OutwardInterval.h").as_posix()
        code='#include "'+header+'"\n'+r'''
#include <iostream>
#include <iomanip>
#include <string>
using namespace tvsBypass::bounded;
int main(){std::string op;double a,b;std::cout<<std::setprecision(17);
while(std::cin>>op>>a>>b){I x(a),y(b),z;
if(op=="+")z=x+y;else if(op=="-")z=x-y;else if(op=="*")z=x*y;else if(op=="/")z=x/y;
else if(op=="sqrt")z=root(x);else if(op=="sin")z=sinePoint(a);else z=cosinePoint(a);
std::cout<<z.lo<<" "<<z.hi<<"\n";}}
'''
        rng=random.Random(3102026)
        cases=[]
        for _ in range(100):
            a=math.ldexp(rng.uniform(-1,1),rng.randint(-500,500))
            b=math.ldexp(rng.uniform(.5,1),rng.randint(-500,500))
            for op in ["+","-","*","/"]:cases.append((op,a,b))
        cases.extend([("+",.1,.2),("-",1.,1.),("*",1e-250,1e-250),("/",1.,10.)])
        cases.extend(("sqrt",x,0.) for x in [0.,.1,2.,1e-300,1e-200,1e200])
        cases.extend((op,x,0.) for op in ["sin","cos"] for x in [0.,.1,math.pi/6,math.pi/4,math.pi/3,1.1]+[rng.uniform(0,1.1) for _ in range(15)])
        with tempfile.TemporaryDirectory() as directory:
            path=pathlib.Path(directory)/"bounds.cpp";exe=pathlib.Path(directory)/"bounds"
            path.write_text(code)
            subprocess.run(["g++","-std=c++11","-O2","-fno-fast-math","-ffp-contract=off","-Wall","-Wextra","-Werror","-pedantic",str(path),"-o",str(exe)],check=True)
            bad=subprocess.run(["g++","-std=c++11","-ffast-math","-fsyntax-only",str(path)],capture_output=True,text=True)
            self.assertNotEqual(bad.returncode,0);self.assertIn("strict IEEE",bad.stderr)
            data="".join(f"{op} {a:.17g} {b:.17g}\n" for op,a,b in cases)
            output=subprocess.run([str(exe)],input=data,capture_output=True,text=True,check=True).stdout.splitlines()
        self.assertEqual(len(output),len(cases))
        for (op,a,b),line in zip(cases,output):
            lo,hi=map(float,line.split());left,right=F(lo),F(hi)
            self.assertLessEqual(left,right)
            if op in ["+","-","*","/"]:
                exact={"+":lambda:F(a)+F(b),"-":lambda:F(a)-F(b),"*":lambda:F(a)*F(b),"/":lambda:F(a)/F(b)}[op]()
                self.assertLessEqual(left,exact,(op,a,b,line));self.assertGreaterEqual(right,exact,(op,a,b,line))
            elif op=="sqrt":
                self.assertGreaterEqual(left,0);self.assertLessEqual(left*left,F(a));self.assertGreaterEqual(right*right,F(a))
            else:
                # Exact rational alternating-series enclosure: no double/libm oracle.
                x=F(a);term=x if op=="sin" else F(1);total=term
                for n in range(1,41):
                    denominator=(2*n)*(2*n+1) if op=="sin" else (2*n-1)*(2*n)
                    term=-term*x*x/denominator;total+=term
                denominator=82*83 if op=="sin" else 81*82
                next_term=-term*x*x/denominator
                low=min(total,total+next_term);high=max(total,total+next_term)
                self.assertLessEqual(left,low,(op,a,line));self.assertGreaterEqual(right,high,(op,a,line))
        print(f"PASS {len(cases)} independent exact-Fraction enclosure cases; fast-math rejected")
if __name__=="__main__":unittest.main(verbosity=2)
