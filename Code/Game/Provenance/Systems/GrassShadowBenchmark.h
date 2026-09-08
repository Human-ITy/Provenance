#pragma once

// Presentation instrumentation only. Never part of matter or fingerprints.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace EE::OutcropLab
{
    struct GrassShadowBenchmark
    {
        struct Stats { size_t count=0; double mean=0,p95=0,p99=0,maximum=0; };
        std::vector<double> samples[4];
        int phase=-1;
        double elapsed=0;
        bool finished=false,pass=false,wrote=false;
        Stats offResult,onResult;
        std::string outputPath="GrassShadowBenchmark.csv";
        bool Running() const { return phase>=0&&phase<4; }
        static bool Shadows(int phase) { return phase==1||phase==2; }
        static Stats Summarize(std::vector<double> values)
        {
            Stats s; s.count=values.size(); if(values.empty())return s;
            for(double v:values)s.mean+=v; s.mean/=double(s.count);
            std::sort(values.begin(),values.end());
            s.p95=values[size_t(std::ceil(.95*double(s.count)))-1];
            s.p99=values[size_t(std::ceil(.99*double(s.count)))-1];
            s.maximum=values.back(); return s;
        }
        Stats Combined(bool on) const
        {
            std::vector<double> all;
            for(int i=0;i<4;++i)if(Shadows(i)==on)all.insert(all.end(),samples[i].begin(),samples[i].end());
            return Summarize(std::move(all));
        }
        void Start()
        {
            for(auto& v:samples){v.clear();v.reserve(10000);}
            phase=0;elapsed=0;finished=pass=wrote=false;
        }
        // Call before switching presentation state. Three seconds settle all
        // patch rebuilds and pipelines; seven seconds sample each ABBA leg.
        // A test is invalidated by player input / pending simulation work in
        // the adapter. It does not steer, reset, or modify the player's world.
        bool Advance(double seconds,bool ready)
        {
            if(!Running())return false;
            if(!std::isfinite(seconds)||seconds<=0)return false;
            if(!ready){elapsed=0;samples[phase].clear();return false;}
            elapsed+=seconds;
            if(elapsed>3&&samples[phase].size()<10000)samples[phase].push_back(seconds*1000);
            if(elapsed<10)return false;
            ++phase;elapsed=0;
            if(phase==4)
            {
                finished=true;offResult=Combined(false);onResult=Combined(true);
                auto const& off=offResult;auto const& on=onResult;
                // Frame-time envelope, not GPU timing. Vsync can hide costs.
                // Keep the feature opt-in until visual + uncapped validation.
                pass=off.count>=120&&on.count>=120&&on.mean<=off.mean+1.0&&on.p95<=off.p95+2.0&&on.p99<=off.p99+4.0;
                FILE* file=nullptr;
                if(fopen_s(&file,outputPath.c_str(),"w")==0&&file)
                {
                    std::fprintf(file,"phase,shadows,sample,frame_ms\n");
                    for(int i=0;i<4;++i)for(size_t j=0;j<samples[i].size();++j)
                        std::fprintf(file,"%d,%d,%zu,%.6f\n",i,Shadows(i)?1:0,j,samples[i][j]);
                    wrote=std::fclose(file)==0;
                }
            }
            return true;
        }
    };
}
