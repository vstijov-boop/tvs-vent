"""Compile actual topology helpers, without AutoCAD, against small geometry fixtures."""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


def function(text, signature):
    start = text.index(signature)
    brace = text.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        if text[end] == '{':
            depth += 1
        elif text[end] == '}':
            depth -= 1
        end += 1
    return text[start:end]


class MoveTopology(unittest.TestCase):
    def test_actual_helpers(self):
        header = (ROOT / 'TVS_Ventilation_ARX/TVSMoveCommand.h').read_text(encoding='utf-8-sig')
        extracted = '\n'.join(function(header, signature) for signature in (
            'inline bool uniqueContact(', 'inline bool section(', 'inline bool sideBranches('))
        source = r'''
#include <vector>
#include <cassert>
#include <cstddef>
#include "MoveGeometry.h"
using tvsMoveGeometry::Point;
struct Node {std::vector<Point> ports;double a,b,elevation;bool round;
 Node():a(250),b(250),elevation(0),round(false){}};
struct Contact {int node,port;Contact():node(-1),port(-1){}};
''' + extracted + r'''
static Node pipe(Point a,Point b){Node n;n.ports.push_back(a);n.ports.push_back(b);return n;}
int main(){
 for(int scale=1;scale<=1000;scale*=10){
  const double tol=.1/scale;
  std::vector<Node> n;
  n.push_back(pipe(Point(0,0),Point(2000.0/scale,0)));
  n.push_back(pipe(Point(0,0),Point(-300.0/scale,-300.0/scale)));
  n.push_back(pipe(Point(2000.0/scale,0),Point(2300.0/scale,-300.0/scale)));
  n.push_back(pipe(Point(-300.0/scale,-300.0/scale),Point(-300.0/scale,-2000.0/scale)));
  n.push_back(pipe(Point(2300.0/scale,-300.0/scale),Point(2300.0/scale,-2000.0/scale)));
  Contact c;assert(uniqueContact(n,0,0,tol,c));assert(c.node==1&&c.port==0);
  assert(uniqueContact(n,0,1,tol,c));assert(c.node==2&&c.port==0);
  const int pipes[3]={0,3,4};assert(!sideBranches(n,pipes,tol));
  n.push_back(pipe(Point(1000.0/scale,0),Point(1000.0/scale,300.0/scale)));
  assert(sideBranches(n,pipes,tol));n.pop_back();
  n.push_back(n[1]);assert(!uniqueContact(n,0,0,tol,c));n.pop_back();
  n[1].ports[0].x=.09/scale;assert(uniqueContact(n,0,0,tol,c));
  n[1].ports[0].x=.11/scale;assert(!uniqueContact(n,0,0,tol,c));
  Node a,b;a.a=b.a=250.0/scale;a.b=b.b=250.0/scale;
  assert(section(a,b,tol));b.elevation=1.0/scale;assert(!section(a,b,tol));b.elevation=0;
  b.b=400.0/scale;assert(!section(a,b,tol));a.round=b.round=true;assert(section(a,b,tol));
  b.round=false;assert(!section(a,b,tol));
 }
}
'''
        with tempfile.TemporaryDirectory() as folder:
            cpp = pathlib.Path(folder) / 'topology.cpp'
            binary = pathlib.Path(folder) / 'topology'
            cpp.write_text(source)
            subprocess.run(['g++', '-std=c++98', '-Wall', '-Wextra', '-Werror', '-pedantic',
                            '-I', str(ROOT / 'TVS_Ventilation_ARX'), str(cpp), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    unittest.main()
