#include "../geometry/SizeFollowRules.h"
#include <cassert>
#include <cstdio>
using namespace tvsFollowRules;
int main(){
    assert(height(400,200,false)==200);assert(height(250,0,true)==250);assert(height(250,0,false)==250);
    // same level
    assert(spansMeet(3000,200,3000,300,1));
    // prelaz flush with the top: axes differ by 50, spans still overlap
    assert(spansMeet(3000,200,2950,300,1));
    // stacked: 200 high at 3000 and 200 high at 2700 (100 clear)
    assert(!spansMeet(3000,200,2700,200,1));
    // touching (top of one = bottom of the other) is not a joint
    assert(!spansMeet(3000,200,2800,200,1));
    // no heights: axes
    assert(spansMeet(0,0,0,0,1));assert(!spansMeet(0,0,5,0,1));
    // the only element at the point is the neighbour whatever its level (old drawings)
    assert(joined(1,3000,200,0,200,1));
    // two elements at the point: the level decides
    assert(!joined(2,3000,200,0,200,1));assert(joined(2,3000,200,3000,200,1));
    std::puts("size follow rules OK");return 0;
}
