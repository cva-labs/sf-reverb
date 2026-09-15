#pragma once
// Superformula Reverb - geometry-driven reverberation engine
// Copyright (C) 2026 CVA Labs
// SPDX-License-Identifier: AGPL-3.0-or-later
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>

namespace sf {
constexpr float pi = 3.14159265358979323846f;
struct Shape { float m=6, n1=1.2f, n2=3.8f, n3=3.8f, vertical=4, size=12; };
inline float radius(float angle,float m,float n1,float n2,float n3) {
    const float a=std::pow(std::abs(std::cos(m*angle/4)),n2);
    const float b=std::pow(std::abs(std::sin(m*angle/4)),n3);
    return std::clamp(std::pow(std::max(a+b,0.0001f),-1.0f/n1),0.18f,2.8f);
}
struct Vec { float x,y,z; };
inline Vec point(float t,float p,const Shape& s) {
    float r=radius(t,s.m,s.n1,s.n2,s.n3);
    float v=radius(p,s.vertical,s.n1,s.n2,s.n3);
    return {r*v*std::cos(p)*std::cos(t),r*v*std::cos(p)*std::sin(t),v*std::sin(p)};
}
struct Material { const char* name; float absorption,cutoff; unsigned colour; };
inline constexpr std::array<Material,8> materials {{
    {"Metal",0.035f,14500,0xff1d3461},{"Stone",0.065f,10000,0xff6f1a07},
    {"Silk",0.46f,2600,0xffaf9164},{"Bamboo",0.19f,6500,0xff1d3461},
    {"Gourd",0.24f,4500,0xff6f1a07},{"Clay",0.12f,7800,0xffaf9164},
    {"Hide",0.36f,3400,0xff6f1a07},{"Wood",0.22f,5400,0xff1d3461}
}};
// Eight-line orthogonal feedback delay network. Geometry samples set travel paths;
// material absorption sets loss per reflection. All storage is prepared off audio thread.
class Engine {
    std::array<std::vector<float>,8> lines;
    std::array<float,8> delay{},target{},low{},gain{},gainTarget{};
    std::vector<float> pre,preSide;
    int write=0,preWrite=0,length=0;
    float sr=48000, damp=0.3f,dampTarget=0.3f,wet=0.3f,wetTarget=0.3f;
    float preDelay=0,preTarget=0;
    static float read(const std::vector<float>& b,int pos,float d) {
        float r=float(pos)-d; if(r<0) r+=float(b.size());
        int i=int(r); float f=r-float(i);
        return b[size_t(i)]*(1-f)+b[(size_t(i)+1)%b.size()]*f;
    }
public:
    void prepare(float sampleRate) {
        sr=sampleRate; length=int(sr*2)+8;
        for(auto& l:lines) l.assign(size_t(length),0);
        pre.assign(size_t(sr*0.3f)+8,0); preSide.assign(pre.size(),0); low.fill(0); write=preWrite=0;
        configure({},0,1,0.3f,15); delay=target; gain=gainTarget; damp=dampTarget; wet=wetTarget; preDelay=preTarget;
    }
    void configure(const Shape& s,int material,float decay,float mix,float predelay) {
        const auto& mat=materials[size_t(std::clamp(material,0,7))];
        for(int i=0;i<8;++i) {
            auto q=point(-pi+(float(i)+0.37f)*2*pi/8, -1.1f+float(i)*2.2f/7,s);
            float distance=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z)*s.size;
            float seconds=std::clamp((distance*2+float(i)*0.73f)/343.0f,0.009f,1.8f);
            target[size_t(i)]=seconds*sr;
            gainTarget[size_t(i)]=std::clamp(std::pow(1-mat.absorption,1.0f/decay)*std::exp(-seconds*0.35f),0.0f,0.995f);
        }
        dampTarget=1-std::exp(-2*pi*std::min(mat.cutoff,sr*0.4f)/sr);
        wetTarget=mix; preTarget=predelay*sr/1000;
    }
    void process(float& l,float& r) {
        constexpr float smooth=0.0008f;
        wet+=(wetTarget-wet)*smooth; damp+=(dampTarget-damp)*smooth;
        preDelay+=(preTarget-preDelay)*smooth;
        float dryL=l,dryR=r;
        pre[size_t(preWrite)]=(l+r)*0.5f;
        preSide[size_t(preWrite)]=(l-r)*0.5f;
        float input=read(pre,preWrite,preDelay);
        float side=read(preSide,preWrite,preDelay);
        preWrite=(preWrite+1)%int(pre.size());
        std::array<float,8> taps{};
        for(int i=0;i<8;++i) {
            size_t j=size_t(i); delay[j]+=(target[j]-delay[j])*smooth;
            gain[j]+=(gainTarget[j]-gain[j])*smooth;
            float v=read(lines[j],write,delay[j]);
            low[j]+=damp*(v-low[j]); taps[j]=low[j];
        }
        float outL=(taps[0]+taps[2]-taps[4]+taps[6])*0.35f;
        float outR=(taps[1]-taps[3]+taps[5]+taps[7])*0.35f;
        for(int step=1;step<8;step*=2)
            for(int base=0;base<8;base+=step*2)
                for(int j=0;j<step;++j) { float a=taps[size_t(base+j)],b=taps[size_t(base+j+step)]; taps[size_t(base+j)]=a+b; taps[size_t(base+j+step)]=a-b; }
        for(int i=0;i<8;++i) lines[size_t(i)][size_t(write)]=input*(i%2? -0.22f:0.22f)+side*(i%4<2?0.22f:-0.22f)+taps[size_t(i)]*0.35355339f*gain[size_t(i)];
        write=(write+1)%length;
        l=dryL*(1-wet)+outL*wet; r=dryR*(1-wet)+outR*wet;
    }
};
}
