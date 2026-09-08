#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

// Opt-in end-to-end frame intervals, not GPU timings. Pair the next update's
// delta with the preceding update's movement/work metadata, not new input.
namespace EE::OutcropLab
{
    struct MovementFrameCapture
    {
        enum class Region {Lab,Creek,Outer};
        struct Context {bool moving=false,busy=false,ready=false;Region region=Region::Lab;double walkMs=0,uploadMs=0;};
        struct Sample {double frameMs=0;Context context;};
        struct Stats {size_t count=0,over20=0,over33=0;double mean=0,p95=0,p99=0,maximum=0;};
        static constexpr size_t Limit=8192;
        std::vector<Sample> samples;
        Context previous;
        bool running=false,hasPrevious=false,finished=false,wrote=false,writeAttempted=false;
        double seconds=0;
        Stats moving,idle;
        static char const* Name(Region r){return r==Region::Lab?"lab":r==Region::Creek?"creek":"outer";}
        void Start(){samples.clear();samples.reserve(Limit);running=true;hasPrevious=false;finished=wrote=writeAttempted=false;seconds=0;moving={};idle={};}
        Stats Summarize(bool wantMoving)const
        {
            Stats s;std::vector<double> times;
            for(auto const& v:samples)if(v.context.moving==wantMoving&&!v.context.busy)
            {times.push_back(v.frameMs);s.mean+=v.frameMs;s.over20+=v.frameMs>20;s.over33+=v.frameMs>33.333333;}
            s.count=times.size();if(times.empty())return s;
            std::sort(times.begin(),times.end());s.mean/=double(s.count);
            s.p95=times[size_t(std::ceil(.95*s.count))-1];s.p99=times[size_t(std::ceil(.99*s.count))-1];s.maximum=times.back();return s;
        }
        void Finish(){running=false;hasPrevious=false;finished=true;moving=Summarize(true);idle=Summarize(false);}
        void Observe(double frameMs,Context current)
        {
            if(!running)return;
            if(hasPrevious&&previous.ready&&std::isfinite(frameMs)&&frameMs>0)
            {
                samples.push_back({frameMs,previous});seconds+=frameMs*.001;
                if(seconds>=30||samples.size()>=Limit){Finish();return;}
            }
            previous=current;hasPrevious=true;
        }
        bool Write(char const* path)
        {
            if(!finished)return false;writeAttempted=true;
            FILE* file=nullptr;if(fopen_s(&file,path,"w")!=0||!file)return false;
            bool ok=std::fprintf(file,"sample,region,movement_input,action_or_upload,frame_ms,walk_cpu_ms,upload_cpu_ms\n")>=0;
            for(size_t i=0;i<samples.size();++i){auto const& v=samples[i];ok=(std::fprintf(file,"%zu,%s,%d,%d,%.6f,%.6f,%.6f\n",i,Name(v.context.region),v.context.moving?1:0,v.context.busy?1:0,v.frameMs,v.context.walkMs,v.context.uploadMs)>=0)&&ok;}
            wrote=std::fclose(file)==0&&ok;return wrote;
        }
    };
}
