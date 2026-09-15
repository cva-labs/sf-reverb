#include "Engine.h"
// Copyright (C) 2026 CVA Labs
// SPDX-License-Identifier: AGPL-3.0-or-later
#include <iostream>
#include <cstdlib>
static void require(bool ok,const char* message) { if(!ok) { std::cerr<<message<<'\n'; std::exit(1); } }
double render(sf::Shape s,int material,float sr=48000) {
    sf::Engine engine; engine.prepare(sr); engine.configure(s,material,1,1,0);
    double energy=0;
    for(int i=0;i<int(sr*5);++i) { float l=i==int(sr/2)?1.0f:0.0f,r=l; engine.process(l,r); require(std::isfinite(l)&&std::isfinite(r),"Nonfinite output"); require(std::abs(l)<4&&std::abs(r)<4,"Unstable output"); if(i>int(sr*0.6f)) energy+=l*l+r*r; }
    return energy;
}
int main() {
    sf::Shape sphere{0,2,2,2,0,12};
    for(int i=0;i<100;++i) { auto v=sf::point(i*0.07f,i*0.03f,sphere); require(std::abs(v.x*v.x+v.y*v.y+v.z*v.z-1)<0.0001f,"Sphere geometry incorrect"); }
    double metal=render({},0),silk=render({},2); require(metal>silk*1.2,"Materials must alter decay");
    sf::Shape other{12,0.3f,6,0.3f,8,40}; double changed=render(other,0); require(std::abs(metal-changed)>0.00001,"Geometry must change response");
    for(int m=0;m<8;++m) for(float sr:{44100.f,96000.f}) require(render(other,m,sr)>0,"Silent reverb");
    sf::Engine engine; engine.prepare(48000); engine.configure({},0,1,0,0);
    for(int i=0;i<48000;++i) { float l=0,r=0; engine.process(l,r); }
    float l=0.25f,r=-0.5f; engine.process(l,r); require(std::abs(l-0.25f)<0.00001f&&std::abs(r+0.5f)<0.00001f,"Dry signal altered");
    std::cout<<"PASS: geometry, material decay, shape response, stability at 44.1/48/96 kHz, dry path\n";
}
