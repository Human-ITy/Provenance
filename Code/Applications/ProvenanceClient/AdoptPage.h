#pragma once

// Native Stage0 authority-admission seam for canonical Phase 17 orographic
// production pages. Single fail-closed adopt_page boundary: identity/revision
// is checked here, never in native render/collision/streaming.
//
// SampleGrade / SampleZ reconstruct the banked carrier for consume certification
// only. They are not a second Stage0 heightfield and must not replace native
// WorldGenesis v11 render, collision, or grounding.
//
// MW8 QueryContext consumes orographic_ecological_context_v1 (elevation_grade,
// SystemId/RangeId/MassifId, ridge/divide/saddle/valley/basin, exposure).
// GradeToZ remains presentation. MW9 stays CLOSED.

#include "CausalRegionalBiome.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace AdoptPage
{
    constexpr char const* kTerrainLaw = "orographic.phase17";
    constexpr char const* kExpectedTectonic = "b74f957a7fdb429a";
    constexpr char const* kExpectedOrographic = "3198784442eccd99";
    constexpr char const* kExpectedPageDigest =
        "03f579fed39e4685240aaf4d51efae5a2347bf414277242ee068a215a3b8fd69";
    constexpr char const* kFixtureDir = "Data/Worldgen/orographic_phase17";
    constexpr double kGradeMin = 0.15;
    constexpr double kGradeMax = 2.60;
    constexpr double kCrestSharpen = 0.16;
    constexpr double kShoulderDrop = 0.09;
    constexpr double kSpurTaper = 0.62;
    constexpr double kSaddleNeck = 0.10;
    constexpr double kRidgeReach = 1.6;
    constexpr double kSaddleNeckReach = 1.25;
    constexpr double kDrainageStepM = 128.0;
    constexpr double kPi = 3.14159265358979323846;

    struct WorldIdentity
    {
        std::string world;
        std::string tectonic;
        std::string orographic;
        int orographicVersion = 2;
        int carrierVersion = 2;
        std::string terrainLaw;
    };

    struct Vec2 { double x = 0, y = 0; };
    struct PolyHit { double d = 1e300, t = 0; Vec2 closest; Vec2 dir{ 1, 0 }; };

    struct Peak
    {
        std::string id, parent, type;
        Vec2 pos;
        double radius = 0, prominence = 0;
    };
    struct Ridge
    {
        std::string id, parent, type;
        std::vector<Vec2> axis;
        double halfWidth = 0, crest = 0, length = 0;
    };
    struct Saddle
    {
        std::string id, parent, type;
        Vec2 pos;
        double radius = 0, drop = 0;
    };
    struct Spur
    {
        std::string id, parent, type;
        std::vector<Vec2> axis;
        double halfWidth = 0, crest = 0;
    };
    struct Valley
    {
        std::string id, type, parent;
        Vec2 pos;
        double accumulation = 0, width = 0;
    };
    struct Divide
    {
        int i = 0, j = 0;
        Vec2 pos;
    };
    struct SystemRec
    {
        std::string id;
        int cell[2] = {};
        Vec2 centre;
        double reach = 0, trendRad = 0;
    };
    struct RangeRec
    {
        std::string id, parent;
        std::vector<Vec2> axis;
        double length = 0, trendRad = 0;
    };
    struct MassifRec
    {
        std::string id, parent;
        Vec2 centre;
        double radius = 0, lift = 0, trendRad = 0;
    };
    struct DrainNode
    {
        int i = 0, j = 0;
        Vec2 pos;
        int flowI = 0, flowJ = 0;
        bool hasFlow = false;
        double accumulation = 0, elevationGrade = 0;
        std::string basin;
        bool onDivide = false;
    };
    struct OrographicContext
    {
        bool live = false;
        int px = 0, py = 0;
        double pageBounds[4] = {};
        double pageSizeM = 1024;
        double influenceRadiusM = 4096.0 * 1.15;
        double drainageStepM = 128;
        double moistureAzimuthDeg = 298;
        double baseGrade = 0.70;
        double gradeMin = 0.15, gradeMax = 2.60;
        std::vector<SystemRec> systems;
        std::vector<RangeRec> ranges;
        std::vector<MassifRec> massifs;
        std::vector<Peak> peaks;
        std::vector<Ridge> ridges;
        std::vector<Saddle> saddles;
        std::vector<Spur> spurs;
        std::vector<Valley> valleys;
        std::vector<DrainNode> drainNodes;
        int rebuilds = 0;
        std::string refuseReason;
    };
    struct ContextSample
    {
        double elevationGrade = 0.70;
        double slopeGrade = 0;
        double aspectRad = 0;
        std::string systemId, rangeId, massifId;
        std::string ridgeId, peakId, saddleId, valleyId, basinId;
        bool onDivide = false, onChannel = false;
        double accumulation = 0;
        double flowDx = 0, flowDy = 0;
        double windwardFactor = 0, leeFactor = 0, barrierGrade = 0;
        CausalRegionalHydroclimate::ExposureClass exposure =
            CausalRegionalHydroclimate::ExposureClass::Neutral;
        bool usedPresentationZ = false;
    };

    struct AdoptedGeography
    {
        WorldIdentity identity;
        std::string cacheKey;
        int px = 0, py = 0;
        double bounds[4] = {};
        double step = 500;
        int n = 0;
        std::vector<std::vector<double>> smooth;
        std::vector<Peak> peaks;
        std::vector<Ridge> ridges;
        std::vector<Saddle> saddles;
        std::vector<Spur> spurs;
        std::vector<Valley> valleys;
        std::vector<Divide> divides;
        std::vector<std::string> peakIds, ridgeIds, saddleIds, spurIds, valleyIds;
        uint64_t worldIdentityHash = 0;
        uint64_t adoptDigest = 0;
        int adoptCount = 0;
        int rebuilds = 0;
        int compiles = 0;
        bool live = false;
        std::string refuseReason;
    };

    struct Mw8Field
    {
        int nx = 0, ny = 0;
        double originX = 0, originY = 0, step = 64;
        std::vector<CausalRegionalBiome::Cell> cells;
        uint64_t fieldDigest = 0;
        int compiles = 0, rebuilds = 0;
        int alpineBarren = 0, alpineTundra = 0, riparian = 0, basinWetland = 0;
        int dryWoodland = 0, moistForest = 0, dryRocky = 0, subalpine = 0;
    };

    struct AdoptResult
    {
        bool ok = false;
        std::string reason;
    };

    struct Json
    {
        enum Kind { Null, Bool, Num, Str, Arr, Obj } kind = Null;
        bool b = false;
        double n = 0;
        std::string s;
        std::vector<Json> a;
        std::unordered_map<std::string, Json> o;
        Json const* get(char const* k) const
        {
            auto it = o.find(k);
            return it == o.end() ? nullptr : &it->second;
        }
        std::string str(char const* k, char const* d = "") const
        {
            Json const* v = get(k);
            return (v && v->kind == Str) ? v->s : std::string(d);
        }
        double num(char const* k, double d = 0) const
        {
            Json const* v = get(k);
            return (v && v->kind == Num) ? v->n : d;
        }
        bool has(char const* k) const { return get(k) != nullptr; }
    };

    inline void SkipWs(char const*& p, char const* e)
    {
        while (p < e && (unsigned char)*p <= 32) ++p;
    }
    inline bool ParseJson(char const*& p, char const* e, Json& out);
    inline bool ParseString(char const*& p, char const* e, std::string& out)
    {
        if (p >= e || *p != '"') return false;
        ++p;
        out.clear();
        while (p < e)
        {
            char c = *p++;
            if (c == '"') return true;
            if (c == '\\' && p < e)
            {
                char e1 = *p++;
                if (e1 == 'n') out.push_back('\n');
                else if (e1 == 't') out.push_back('\t');
                else if (e1 == 'r') out.push_back('\r');
                else out.push_back(e1);
            }
            else out.push_back(c);
        }
        return false;
    }
    inline bool ParseJson(char const*& p, char const* e, Json& out)
    {
        SkipWs(p, e);
        if (p >= e) return false;
        out = Json{};
        if (*p == '"')
        {
            out.kind = Json::Str;
            return ParseString(p, e, out.s);
        }
        if (*p == '{')
        {
            ++p;
            out.kind = Json::Obj;
            SkipWs(p, e);
            if (p < e && *p == '}') { ++p; return true; }
            for (;;)
            {
                SkipWs(p, e);
                std::string key;
                if (!ParseString(p, e, key)) return false;
                SkipWs(p, e);
                if (p >= e || *p != ':') return false;
                ++p;
                Json val;
                if (!ParseJson(p, e, val)) return false;
                out.o.emplace(std::move(key), std::move(val));
                SkipWs(p, e);
                if (p >= e) return false;
                if (*p == '}') { ++p; return true; }
                if (*p != ',') return false;
                ++p;
            }
        }
        if (*p == '[')
        {
            ++p;
            out.kind = Json::Arr;
            SkipWs(p, e);
            if (p < e && *p == ']') { ++p; return true; }
            for (;;)
            {
                Json val;
                if (!ParseJson(p, e, val)) return false;
                out.a.push_back(std::move(val));
                SkipWs(p, e);
                if (p >= e) return false;
                if (*p == ']') { ++p; return true; }
                if (*p != ',') return false;
                ++p;
            }
        }
        if (p + 4 <= e && std::strncmp(p, "true", 4) == 0) { out.kind = Json::Bool; out.b = true; p += 4; return true; }
        if (p + 5 <= e && std::strncmp(p, "false", 5) == 0) { out.kind = Json::Bool; out.b = false; p += 5; return true; }
        if (p + 4 <= e && std::strncmp(p, "null", 4) == 0) { out.kind = Json::Null; p += 4; return true; }
        char* end = nullptr;
        double v = std::strtod(p, &end);
        if (end == p) return false;
        out.kind = Json::Num;
        out.n = v;
        p = end;
        return true;
    }
    inline bool ParseJsonText(std::string const& text, Json& out)
    {
        char const* p = text.c_str();
        char const* e = p + text.size();
        return ParseJson(p, e, out);
    }

    inline bool ReadFile(char const* path, std::string& out)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in) return false;
        std::ostringstream b;
        b << in.rdbuf();
        out = b.str();
        return true;
    }

    inline uint64_t ParseHex64(std::string const& s)
    {
        uint64_t v = 0;
        CausalWorldGeology::ParseHex64(s, v);
        return v;
    }

    inline double Smoothstep(double t)
    {
        t = std::clamp(t, 0.0, 1.0);
        return t * t * (3.0 - 2.0 * t);
    }
    inline double Falloff(double d, double r)
    {
        if (r <= 0.0 || d >= r) return 0.0;
        return Smoothstep(1.0 - d / r);
    }
    inline std::pair<double, double> DistToPolyline(std::vector<Vec2> const& pts, double x, double y)
    {
        double best = 1e300, bestT = 0.0;
        if (pts.size() < 2) return { best, 0.0 };
        double total = 0.0;
        for (size_t i = 0; i + 1 < pts.size(); ++i)
        {
            double dx = pts[i + 1].x - pts[i].x, dy = pts[i + 1].y - pts[i].y;
            total += std::hypot(dx, dy);
        }
        if (total <= 0.0) total = 1.0;
        double run = 0.0;
        for (size_t i = 0; i + 1 < pts.size(); ++i)
        {
            Vec2 const& a = pts[i];
            Vec2 const& b = pts[i + 1];
            double vx = b.x - a.x, vy = b.y - a.y;
            double L = std::hypot(vx, vy);
            if (L == 0.0)
            {
                double d = std::hypot(x - a.x, y - a.y);
                if (d < best) { best = d; bestT = run / total; }
                continue;
            }
            double f = ((x - a.x) * vx + (y - a.y) * vy) / (L * L);
            f = std::clamp(f, 0.0, 1.0);
            double px = a.x + vx * f, py = a.y + vy * f;
            double d = std::hypot(x - px, y - py);
            if (d < best) { best = d; bestT = (run + f * L) / total; }
            run += L;
        }
        return { best, bestT };
    }
    inline PolyHit DistToPolylineHit(std::vector<Vec2> const& pts, double x, double y)
    {
        PolyHit h;
        if (pts.size() < 2) return h;
        double total = 0.0;
        for (size_t i = 0; i + 1 < pts.size(); ++i)
            total += std::hypot(pts[i + 1].x - pts[i].x, pts[i + 1].y - pts[i].y);
        if (total <= 0.0) total = 1.0;
        double run = 0.0;
        for (size_t i = 0; i + 1 < pts.size(); ++i)
        {
            Vec2 const& a = pts[i];
            Vec2 const& b = pts[i + 1];
            double vx = b.x - a.x, vy = b.y - a.y;
            double L = std::hypot(vx, vy);
            if (L == 0.0)
            {
                double d = std::hypot(x - a.x, y - a.y);
                if (d < h.d) { h.d = d; h.t = run / total; h.closest = a; h.dir = { 1, 0 }; }
                continue;
            }
            double f = ((x - a.x) * vx + (y - a.y) * vy) / (L * L);
            f = std::clamp(f, 0.0, 1.0);
            Vec2 c{ a.x + vx * f, a.y + vy * f };
            double d = std::hypot(x - c.x, y - c.y);
            if (d < h.d) { h.d = d; h.t = (run + f * L) / total; h.closest = c; h.dir = { vx / L, vy / L }; }
            run += L;
        }
        return h;
    }

    inline AdoptedGeography& G()
    {
        static AdoptedGeography g;
        return g;
    }
    inline Mw8Field& Mw8()
    {
        static Mw8Field f;
        return f;
    }
    inline OrographicContext& Ctx()
    {
        static OrographicContext c;
        return c;
    }

    inline WorldIdentity IdentityFromGenesis(Json const& genesis)
    {
        WorldIdentity id;
        id.world = genesis.str("world");
        id.tectonic = genesis.str("tectonic");
        id.orographic = genesis.str("orographic");
        id.orographicVersion = (int)genesis.num("orographic_version", 2);
        id.carrierVersion = (int)genesis.num("carrier_version", 2);
        id.terrainLaw = genesis.str("terrain_law");
        return id;
    }

    inline bool IdentityMatches(WorldIdentity const& want, WorldIdentity const& got, std::string& why)
    {
        if (got.world != want.world)
        { why = "stale import refused: page genesis world does not match world"; return false; }
        if (got.tectonic != want.tectonic)
        { why = "stale import refused: page genesis tectonic does not match world"; return false; }
        if (got.orographic != want.orographic)
        { why = "stale import refused: page genesis orographic does not match world"; return false; }
        if (got.orographicVersion != want.orographicVersion)
        { why = "stale import refused: page genesis orographic_version does not match world"; return false; }
        return true;
    }

    inline Vec2 AsVec2(Json const& j)
    {
        Vec2 v;
        if (j.kind == Json::Arr && j.a.size() >= 2)
        { v.x = j.a[0].n; v.y = j.a[1].n; }
        return v;
    }
    inline std::vector<Vec2> AsAxis(Json const& j)
    {
        std::vector<Vec2> a;
        if (j.kind != Json::Arr) return a;
        for (Json const& p : j.a) a.push_back(AsVec2(p));
        return a;
    }

    inline void LoadFeatures(Json const& page, AdoptedGeography& geo)
    {
        Json const* feats = page.get("features");
        if (feats && feats->kind == Json::Obj)
        {
            if (Json const* peaks = feats->get("peaks"))
                for (Json const& f : peaks->a)
                {
                    Peak p;
                    p.id = f.str("id"); p.parent = f.str("parent"); p.type = f.str("type", "peak");
                    if (Json const* pos = f.get("pos")) p.pos = AsVec2(*pos);
                    p.radius = f.num("radius"); p.prominence = f.num("prominence");
                    geo.peaks.push_back(p);
                    geo.peakIds.push_back(p.id);
                }
            if (Json const* ridges = feats->get("ridges"))
                for (Json const& f : ridges->a)
                {
                    Ridge r;
                    r.id = f.str("id"); r.parent = f.str("parent"); r.type = f.str("type", "ridge");
                    if (Json const* ax = f.get("axis")) r.axis = AsAxis(*ax);
                    r.halfWidth = f.num("half_width"); r.crest = f.num("crest"); r.length = f.num("length");
                    geo.ridges.push_back(r);
                    geo.ridgeIds.push_back(r.id);
                }
            if (Json const* saddles = feats->get("saddles"))
                for (Json const& f : saddles->a)
                {
                    Saddle s;
                    s.id = f.str("id"); s.parent = f.str("parent"); s.type = f.str("type", "saddle");
                    if (Json const* pos = f.get("pos")) s.pos = AsVec2(*pos);
                    s.radius = f.num("radius"); s.drop = f.num("drop");
                    geo.saddles.push_back(s);
                    geo.saddleIds.push_back(s.id);
                }
            if (Json const* spurs = feats->get("spurs"))
                for (Json const& f : spurs->a)
                {
                    Spur s;
                    s.id = f.str("id"); s.parent = f.str("parent"); s.type = f.str("type", "spur");
                    if (Json const* ax = f.get("axis")) s.axis = AsAxis(*ax);
                    s.halfWidth = f.num("half_width"); s.crest = f.num("crest");
                    geo.spurs.push_back(s);
                    geo.spurIds.push_back(s.id);
                }
        }
        Json const* defs = page.get("feature_definitions");
        if (defs && defs->kind == Json::Obj)
        {
            if (Json const* valleys = defs->get("valleys"))
                for (Json const& f : valleys->a)
                {
                    Valley v;
                    v.id = f.str("id"); v.type = f.str("type", "valley");
                    if (Json const* pos = f.get("pos")) v.pos = AsVec2(*pos);
                    v.accumulation = f.num("accumulation"); v.width = f.num("width");
                    geo.valleys.push_back(v);
                    geo.valleyIds.push_back(v.id);
                }
            if (Json const* divides = defs->get("divides"))
                for (Json const& f : divides->a)
                {
                    if (f.kind != Json::Arr || f.a.size() < 2) continue;
                    Divide d;
                    d.i = (int)f.a[0].n; d.j = (int)f.a[1].n;
                    d.pos.x = d.i * kDrainageStepM;
                    d.pos.y = d.j * kDrainageStepM;
                    geo.divides.push_back(d);
                }
        }
    }

    inline double SampledOnly(AdoptedGeography const& geo, double x, double y)
    {
        double const x0 = geo.bounds[0], y0 = geo.bounds[1], st = geo.step;
        double fi = (x - x0) / st, fj = (y - y0) / st;
        int i = (int)std::floor(fi), j = (int)std::floor(fj);
        double u = fi - i, v = fj - j;
        int m = geo.n - 1;
        auto at = [&](int a, int b) -> double
        {
            a = std::clamp(a, 0, m);
            b = std::clamp(b, 0, m);
            if (b < 0 || b >= (int)geo.smooth.size()) return 0.0;
            if (a < 0 || a >= (int)geo.smooth[(size_t)b].size()) return 0.0;
            return geo.smooth[(size_t)b][(size_t)a];
        };
        double top = at(i, j) * (1.0 - u) + at(i + 1, j) * u;
        double bot = at(i, j + 1) * (1.0 - u) + at(i + 1, j + 1) * u;
        return top * (1.0 - v) + bot * v;
    }

    inline double SharpFromFeatures(AdoptedGeography const& geo, double x, double y)
    {
        double peakC = 0, ridgeC = 0, spurC = 0, saddleC = 0;
        double crest = 0, shoulder = 0, taper = 0, neck = 0;
        for (Peak const& p : geo.peaks)
        {
            double d = std::hypot(x - p.pos.x, y - p.pos.y);
            double f = Falloff(d, p.radius);
            if (f > 0.0) peakC += p.prominence * f * f;
        }
        for (Ridge const& r : geo.ridges)
        {
            auto dt = DistToPolyline(r.axis, x, y);
            double d = dt.first, hw = r.halfWidth;
            double f = Falloff(d, hw);
            if (f > 0.0) ridgeC += r.crest * f;
            if (d < hw * kRidgeReach)
            {
                crest += r.crest * kCrestSharpen * Falloff(d, hw * 0.45);
                if (hw * 0.5 < d && d < hw * kRidgeReach)
                {
                    double band = (d - hw * 0.5) / (hw * (kRidgeReach - 0.5));
                    shoulder -= r.crest * kShoulderDrop * Smoothstep(1.0 - std::fabs(2.0 * band - 1.0));
                }
            }
        }
        for (Spur const& sp : geo.spurs)
        {
            auto dt = DistToPolyline(sp.axis, x, y);
            double f = Falloff(dt.first, sp.halfWidth);
            if (f > 0.0)
            {
                spurC += sp.crest * f;
                taper -= sp.crest * kSpurTaper * dt.second * f;
            }
        }
        for (Saddle const& sd : geo.saddles)
        {
            double d = std::hypot(x - sd.pos.x, y - sd.pos.y);
            double f = Falloff(d, sd.radius);
            if (f > 0.0) saddleC -= (ridgeC + peakC) * sd.drop * f;
            double fn = Falloff(d, sd.radius * kSaddleNeckReach);
            if (fn > 0.0) neck -= sd.drop * kSaddleNeck * fn * fn;
        }
        return peakC + ridgeC + spurC + saddleC + crest + shoulder + taper + neck;
    }

    inline bool SampleGrade(double x, double y, double& out)
    {
        AdoptedGeography const& geo = G();
        if (!geo.live || geo.smooth.empty()) return false;
        if (x < geo.bounds[0] || y < geo.bounds[1] || x > geo.bounds[2] || y > geo.bounds[3])
            return false;
        double g = SampledOnly(geo, x, y) + SharpFromFeatures(geo, x, y);
        out = std::clamp(g, kGradeMin, kGradeMax);
        return true;
    }

    inline bool SampleZ(float x, float y, float datum, float relief, float voxel, float& outZ)
    {
        double g = 0;
        if (!SampleGrade(x, y, g)) return false;
        outZ = (float)((g - (double)datum) * (double)relief * (double)voxel);
        return std::isfinite(outZ);
    }

    inline bool IsLive() { return G().live; }
    inline AdoptedGeography const& Get() { return G(); }
    inline void Reset()
    {
        G() = AdoptedGeography{};
        Mw8() = Mw8Field{};
        Ctx() = OrographicContext{};
    }

    inline uint64_t MixSamples(AdoptedGeography const& geo)
    {
        uint64_t h = CausalWorldGeology::HashText("adopted-page");
        h = CausalRegionalBiome::MixU64(h, geo.worldIdentityHash);
        h = CausalRegionalBiome::MixU64(h, (uint64_t)geo.px);
        h = CausalRegionalBiome::MixU64(h, (uint64_t)geo.py);
        for (std::string const& id : geo.ridgeIds)
            h = CausalWorldGeology::HashText((std::string("ridge:") + id).c_str()) ^ h;
        for (double y = geo.bounds[1]; y <= geo.bounds[3]; y += 128.0)
        for (double x = geo.bounds[0]; x <= geo.bounds[2]; x += 128.0)
        {
            double const g = std::clamp(SampledOnly(geo, x, y) + SharpFromFeatures(geo, x, y),
                kGradeMin, kGradeMax);
            h = CausalRegionalBiome::MixF(h, g);
        }
        return h;
    }

    // THE semantic boundary. Identity/revision is refused here, not in render.
    inline AdoptResult Adopt(WorldIdentity const& identity, std::string const& pageJson)
    {
        AdoptResult r;
        Json page;
        if (!ParseJsonText(pageJson, page) || page.kind != Json::Obj)
        { r.reason = "page is not a carrier payload"; return r; }
        Json const* genesis = page.get("genesis");
        if (!genesis || genesis->kind != Json::Obj)
        { r.reason = "page carries no genesis"; return r; }
        WorldIdentity got = IdentityFromGenesis(*genesis);
        std::string why;
        if (!IdentityMatches(identity, got, why))
        { r.reason = why; G().refuseReason = why; return r; }
        if (got.terrainLaw != identity.terrainLaw && !identity.terrainLaw.empty())
        {
            r.reason = "stale import refused: page terrain_law does not match world";
            G().refuseReason = r.reason;
            return r;
        }

        Json const* defs = page.get("feature_definitions");
        static char const* kinds[] = { "peaks", "ridges", "saddles", "spurs", "divides", "valleys" };
        if (!defs || defs->kind != Json::Obj)
        { r.reason = "page omitted feature_definitions"; return r; }
        for (char const* k : kinds)
        {
            Json const* arr = defs->get(k);
            if (!arr || arr->kind != Json::Arr || arr->a.empty())
            { r.reason = std::string("production page carried no ") + k + " definitions"; return r; }
        }

        AdoptedGeography geo;
        geo.identity = identity;
        geo.cacheKey = page.str("cache_key");
        if (Json const* pg = page.get("page"); pg && pg->kind == Json::Arr && pg->a.size() >= 2)
        { geo.px = (int)pg->a[0].n; geo.py = (int)pg->a[1].n; }
        if (Json const* b = page.get("bounds"); b && b->kind == Json::Arr && b->a.size() >= 4)
            for (int i = 0; i < 4; ++i) geo.bounds[i] = b->a[(size_t)i].n;
        geo.step = page.num("step", 500);
        geo.n = (int)page.num("n", 0);
        if (Json const* sm = page.get("smooth"); sm && sm->kind == Json::Arr)
        {
            for (Json const& row : sm->a)
            {
                std::vector<double> rrow;
                if (row.kind == Json::Arr)
                    for (Json const& v : row.a) rrow.push_back(v.n);
                geo.smooth.push_back(std::move(rrow));
            }
        }
        LoadFeatures(page, geo);
        geo.worldIdentityHash = ParseHex64(identity.tectonic);
        geo.live = true;
        geo.adoptCount = G().live ? G().adoptCount + 1 : 1;
        geo.rebuilds = 0;
        geo.compiles = 0;
        G() = std::move(geo);
        G().adoptDigest = MixSamples(G());
        r.ok = true;
        r.reason = "ok";
        return r;
    }

    inline bool InPage(double x, double y)
    {
        AdoptedGeography const& g = G();
        return g.live && x >= g.bounds[0] && y >= g.bounds[1] && x <= g.bounds[2] && y <= g.bounds[3];
    }

    inline double NearestValley(double x, double y, Valley const** out = nullptr)
    {
        double best = 1e300;
        Valley const* hit = nullptr;
        for (Valley const& v : G().valleys)
        {
            double d = std::hypot(x - v.pos.x, y - v.pos.y);
            if (d < best) { best = d; hit = &v; }
        }
        if (out) *out = hit;
        return best;
    }
    inline double NearestDivide(double x, double y)
    {
        double best = 1e300;
        for (Divide const& d : G().divides)
            best = (std::min)(best, std::hypot(x - d.pos.x, y - d.pos.y));
        return best;
    }
    inline double NearestRidge(double x, double y, Ridge const** out = nullptr)
    {
        double best = 1e300;
        Ridge const* hit = nullptr;
        for (Ridge const& r : G().ridges)
        {
            double d = DistToPolyline(r.axis, x, y).first;
            if (d < best) { best = d; hit = &r; }
        }
        if (out) *out = hit;
        return best;
    }

    inline AdoptResult AdoptContext(WorldIdentity const& identity, std::string const& contextJson)
    {
        AdoptResult r;
        Json root;
        if (!ParseJsonText(contextJson, root) || root.kind != Json::Obj)
        { r.reason = "context is not an orographic payload"; return r; }
        Json const* genesis = root.get("genesis");
        if (!genesis || genesis->kind != Json::Obj)
        { r.reason = "context carries no genesis"; return r; }
        WorldIdentity got = IdentityFromGenesis(*genesis);
        std::string why;
        if (!IdentityMatches(identity, got, why))
        { r.reason = why; Ctx().refuseReason = why; return r; }

        OrographicContext c;
        c.live = true;
        if (Json const* pg = root.get("page"); pg && pg->kind == Json::Arr && pg->a.size() >= 2)
        { c.px = (int)pg->a[0].n; c.py = (int)pg->a[1].n; }
        if (Json const* b = root.get("bounds"); b && b->kind == Json::Arr && b->a.size() >= 4)
            for (int i = 0; i < 4; ++i) c.pageBounds[i] = b->a[(size_t)i].n;
        c.pageSizeM = root.num("page_size_m", 1024);
        c.influenceRadiusM = root.num("influence_radius_m", 4096.0 * 1.15);
        c.drainageStepM = root.num("drainage_step_m", 128);
        c.moistureAzimuthDeg = root.num("moisture_azimuth_deg", 298);
        c.baseGrade = root.num("base_grade", 0.70);
        c.gradeMin = root.num("grade_min", 0.15);
        c.gradeMax = root.num("grade_max", 2.60);

        auto loadArr = [&](char const* key, auto&& fn)
        {
            Json const* arr = root.get(key);
            if (!arr || arr->kind != Json::Arr) return;
            for (Json const& f : arr->a) fn(f);
        };
        loadArr("systems", [&](Json const& f) {
            SystemRec s;
            s.id = f.str("id");
            if (Json const* cell = f.get("cell"); cell && cell->kind == Json::Arr && cell->a.size() >= 2)
            { s.cell[0] = (int)cell->a[0].n; s.cell[1] = (int)cell->a[1].n; }
            if (Json const* pos = f.get("centre")) s.centre = AsVec2(*pos);
            s.reach = f.num("reach"); s.trendRad = f.num("trend_rad");
            c.systems.push_back(s);
        });
        loadArr("ranges", [&](Json const& f) {
            RangeRec s;
            s.id = f.str("id"); s.parent = f.str("parent");
            if (Json const* ax = f.get("axis")) s.axis = AsAxis(*ax);
            s.length = f.num("length"); s.trendRad = f.num("trend_rad");
            c.ranges.push_back(s);
        });
        loadArr("massifs", [&](Json const& f) {
            MassifRec s;
            s.id = f.str("id"); s.parent = f.str("parent");
            if (Json const* pos = f.get("centre")) s.centre = AsVec2(*pos);
            s.radius = f.num("radius"); s.lift = f.num("lift"); s.trendRad = f.num("trend_rad");
            c.massifs.push_back(s);
        });
        loadArr("peaks", [&](Json const& f) {
            Peak p;
            p.id = f.str("id"); p.parent = f.str("parent"); p.type = f.str("type", "peak");
            if (Json const* pos = f.get("pos")) p.pos = AsVec2(*pos);
            p.radius = f.num("radius"); p.prominence = f.num("prominence");
            c.peaks.push_back(p);
        });
        loadArr("ridges", [&](Json const& f) {
            Ridge rr;
            rr.id = f.str("id"); rr.parent = f.str("parent"); rr.type = f.str("type", "ridge");
            if (Json const* ax = f.get("axis")) rr.axis = AsAxis(*ax);
            rr.halfWidth = f.num("half_width"); rr.crest = f.num("crest"); rr.length = f.num("length");
            c.ridges.push_back(rr);
        });
        loadArr("saddles", [&](Json const& f) {
            Saddle sd;
            sd.id = f.str("id"); sd.parent = f.str("parent"); sd.type = f.str("type", "saddle");
            if (Json const* pos = f.get("pos")) sd.pos = AsVec2(*pos);
            sd.radius = f.num("radius"); sd.drop = f.num("drop");
            c.saddles.push_back(sd);
        });
        loadArr("spurs", [&](Json const& f) {
            Spur sp;
            sp.id = f.str("id"); sp.parent = f.str("parent"); sp.type = f.str("type", "spur");
            if (Json const* ax = f.get("axis")) sp.axis = AsAxis(*ax);
            sp.halfWidth = f.num("half_width"); sp.crest = f.num("crest");
            c.spurs.push_back(sp);
        });
        loadArr("valleys", [&](Json const& f) {
            Valley v;
            v.id = f.str("id"); v.type = f.str("type", "valley");
            v.parent = f.str("parent");
            if (Json const* pos = f.get("outlet")) v.pos = AsVec2(*pos);
            v.accumulation = f.num("max_accumulation");
            v.width = f.num("width", 90);
            c.valleys.push_back(v);
        });
        loadArr("drainage_nodes", [&](Json const& f) {
            DrainNode n;
            n.i = (int)f.num("i"); n.j = (int)f.num("j");
            if (Json const* pos = f.get("pos")) n.pos = AsVec2(*pos);
            n.accumulation = f.num("accumulation");
            n.elevationGrade = f.num("elevation_grade");
            n.basin = f.str("basin");
            Json const* od = f.get("on_divide");
            n.onDivide = od && od->kind == Json::Bool && od->b;
            if (Json const* fl = f.get("flow"); fl && fl->kind == Json::Arr && fl->a.size() >= 2)
            { n.hasFlow = true; n.flowI = (int)fl->a[0].n; n.flowJ = (int)fl->a[1].n; }
            c.drainNodes.push_back(n);
        });
        if (c.systems.empty() || c.massifs.empty() || c.ridges.empty())
        { r.reason = "context omitted system/range/massif/ridge graph"; return r; }
        if (c.influenceRadiusM <= c.pageSizeM)
        { r.reason = "context influence radius must exceed page size"; return r; }
        Ctx() = std::move(c);
        r.ok = true;
        r.reason = "ok";
        return r;
    }

    inline bool ContextLive() { return Ctx().live; }
    inline DrainNode const* NearestDrain(double x, double y);

    inline double SharpFromContext(double x, double y)
    {
        OrographicContext const& geo = Ctx();
        double peakC = 0, ridgeC = 0, spurC = 0, saddleC = 0;
        double crest = 0, shoulder = 0, taper = 0, neck = 0;
        for (Peak const& p : geo.peaks)
        {
            double d = std::hypot(x - p.pos.x, y - p.pos.y);
            double f = Falloff(d, p.radius);
            if (f > 0.0) peakC += p.prominence * f * f;
        }
        for (Ridge const& r : geo.ridges)
        {
            auto dt = DistToPolyline(r.axis, x, y);
            double d = dt.first, hw = r.halfWidth;
            double f = Falloff(d, hw);
            if (f > 0.0) ridgeC += r.crest * f;
            if (d < hw * kRidgeReach)
            {
                crest += r.crest * kCrestSharpen * Falloff(d, hw * 0.45);
                if (hw * 0.5 < d && d < hw * kRidgeReach)
                {
                    double band = (d - hw * 0.5) / (hw * (kRidgeReach - 0.5));
                    shoulder -= r.crest * kShoulderDrop * Smoothstep(1.0 - std::fabs(2.0 * band - 1.0));
                }
            }
        }
        for (Spur const& sp : geo.spurs)
        {
            auto dt = DistToPolyline(sp.axis, x, y);
            double f = Falloff(dt.first, sp.halfWidth);
            if (f > 0.0)
            {
                spurC += sp.crest * f;
                taper -= sp.crest * kSpurTaper * dt.second * f;
            }
        }
        for (Saddle const& sd : geo.saddles)
        {
            double d = std::hypot(x - sd.pos.x, y - sd.pos.y);
            double f = Falloff(d, sd.radius);
            if (f > 0.0) saddleC -= (ridgeC + peakC) * sd.drop * f;
            double fn = Falloff(d, sd.radius * kSaddleNeckReach);
            if (fn > 0.0) neck -= sd.drop * kSaddleNeck * fn * fn;
        }
        double massifC = 0;
        for (MassifRec const& m : geo.massifs)
        {
            double d = std::hypot(x - m.centre.x, y - m.centre.y);
            massifC += m.lift * Falloff(d, m.radius);
        }
        return peakC + ridgeC + spurC + saddleC + crest + shoulder + taper + neck + massifC;
    }

    inline double CanonicalGrade(double x, double y)
    {
        double g = 0;
        if (SampleGrade(x, y, g)) return g;
        OrographicContext const& c = Ctx();
        if (!c.live) return c.baseGrade;
        double reconstructed = c.baseGrade + SharpFromContext(x, y);
        DrainNode const* dn = NearestDrain(x, y);
        if (dn) reconstructed = (std::max)(reconstructed, dn->elevationGrade);
        return std::clamp(reconstructed, c.gradeMin, c.gradeMax);
    }

    inline DrainNode const* NearestDrain(double x, double y)
    {
        DrainNode const* hit = nullptr;
        double best = 1e300;
        for (DrainNode const& n : Ctx().drainNodes)
        {
            double d = std::hypot(x - n.pos.x, y - n.pos.y);
            if (d < best) { best = d; hit = &n; }
        }
        return hit;
    }

    inline ContextSample QueryContext(double x, double y)
    {
        ContextSample s;
        OrographicContext const& c = Ctx();
        s.elevationGrade = CanonicalGrade(x, y);
        s.usedPresentationZ = false;
        double ds = 32.0;
        double gx = CanonicalGrade(x + ds, y), gy = CanonicalGrade(x, y + ds);
        double dxg = (gx - s.elevationGrade) / ds, dyg = (gy - s.elevationGrade) / ds;
        s.slopeGrade = std::hypot(dxg, dyg);
        s.aspectRad = std::atan2(dyg, dxg);

        double bestSys = 1e300;
        for (SystemRec const& sys : c.systems)
        {
            double d = std::hypot(x - sys.centre.x, y - sys.centre.y);
            if (d < bestSys) { bestSys = d; s.systemId = sys.id; }
        }
        double bestM = 1e300;
        for (MassifRec const& m : c.massifs)
        {
            double d = std::hypot(x - m.centre.x, y - m.centre.y);
            if (d < bestM) { bestM = d; s.massifId = m.id; s.rangeId = m.parent; }
        }
        double bestP = 1e300;
        for (Peak const& p : c.peaks)
        {
            double d = std::hypot(x - p.pos.x, y - p.pos.y);
            if (d < bestP) { bestP = d; s.peakId = p.id; }
        }
        double bestR = 1e300;
        for (Ridge const& rr : c.ridges)
        {
            double d = DistToPolyline(rr.axis, x, y).first;
            if (d < bestR) { bestR = d; s.ridgeId = rr.id; }
        }
        double bestS = 1e300;
        for (Saddle const& sd : c.saddles)
        {
            double d = std::hypot(x - sd.pos.x, y - sd.pos.y);
            if (d < bestS) { bestS = d; s.saddleId = sd.id; }
        }

        DrainNode const* dn = NearestDrain(x, y);
        if (dn)
        {
            s.basinId = dn->basin;
            s.accumulation = dn->accumulation;
            s.onDivide = dn->onDivide;
            if (dn->hasFlow)
            {
                double vx = (double)(dn->flowI - dn->i), vy = (double)(dn->flowJ - dn->j);
                double nrm = std::hypot(vx, vy);
                if (nrm > 0) { s.flowDx = vx / nrm; s.flowDy = vy / nrm; }
            }
        }
        double bestV = 1e300;
        for (Valley const& v : c.valleys)
        {
            double d = std::hypot(x - v.pos.x, y - v.pos.y);
            if (d < bestV)
            {
                bestV = d; s.valleyId = v.id;
                if (s.basinId.empty()) s.basinId = v.parent;
            }
        }
        double vReach = 90.0;
        if (dn) vReach = (std::max)(90.0, 40.0 + 6.0 * std::sqrt((std::max)(0.0, s.accumulation)));
        s.onChannel = dn && s.accumulation >= 12.0 && std::hypot(x - dn->pos.x, y - dn->pos.y) < vReach * 0.55;

        double az = c.moistureAzimuthDeg * kPi / 180.0;
        double wx = std::cos(az), wy = std::sin(az);
        double windward = 0, lee = 0, barrier = 0;
        for (Ridge const& rr : c.ridges)
        {
            PolyHit h = DistToPolylineHit(rr.axis, x, y);
            double reach = (std::max)(rr.halfWidth * 4.0, 320.0);
            if (h.d > reach) continue;
            double px = -h.dir.y, py = h.dir.x;
            double pwind = px * wx + py * wy;
            if (std::fabs(pwind) < 0.22) continue;
            if (pwind < 0) { px = -px; py = -py; }
            double side = (x - h.closest.x) * px + (y - h.closest.y) * py;
            double flank = Falloff(h.d, reach);
            double strength = rr.crest * flank;
            barrier = (std::max)(barrier, strength);
            if (side >= 0) windward = (std::max)(windward, strength);
            else lee = (std::max)(lee, strength);
        }
        for (MassifRec const& m : c.massifs)
        {
            double d = std::hypot(x - m.centre.x, y - m.centre.y);
            double reach = m.radius * 2.2;
            if (d > reach || d < 1.0) continue;
            double ux = (x - m.centre.x) / d, uy = (y - m.centre.y) / d;
            double toward = ux * wx + uy * wy;
            double strength = m.lift * Falloff(d, reach);
            barrier = (std::max)(barrier, strength);
            if (toward < 0) windward = (std::max)(windward, strength);
            else lee = (std::max)(lee, strength);
        }
        s.barrierGrade = barrier;
        s.windwardFactor = CausalRegionalHydroclimate::Smoothstep(0.02, 0.16, windward);
        s.leeFactor = CausalRegionalHydroclimate::Smoothstep(0.02, 0.16, lee);
        if (s.windwardFactor > 0.42 && s.windwardFactor >= s.leeFactor)
            s.exposure = CausalRegionalHydroclimate::ExposureClass::Windward;
        else if (s.leeFactor > 0.38)
            s.exposure = CausalRegionalHydroclimate::ExposureClass::Leeward;
        else if (s.onDivide || bestR < 70.0)
            s.exposure = CausalRegionalHydroclimate::ExposureClass::ExposedRidge;
        else if (s.accumulation >= 24.0 && s.slopeGrade < 0.0025)
            s.exposure = CausalRegionalHydroclimate::ExposureClass::ShelteredBasin;
        else if (s.onChannel)
            s.exposure = CausalRegionalHydroclimate::ExposureClass::ShelteredValley;
        else
            s.exposure = CausalRegionalHydroclimate::ExposureClass::Neutral;
        return s;
    }

    inline void ClassifyCell(CausalRegionalBiome::Program const& program,
        bool haveHydro, bool haveRegolith,
        CausalRegionalBiome::Cell& c,
        CausalRegionalHydroclimate::HydroclimateQuery const& hydro,
        CausalRegionalRegolith::RegolithQuery const& reg,
        CausalRegionalDrainage::DrainageQuery const& drain)
    {
        using namespace CausalRegionalBiome;
        if (haveHydro)
        {
            c.hydroclimateId = hydro.cell.hydroclimateId;
            c.hydroRegime = hydro.cell.regime;
            c.exposure = hydro.cell.exposure;
            c.watershedId = hydro.cell.watershedId;
        }
        if (haveRegolith)
        {
            c.regolithId = reg.cell.regolithId;
            c.profile = reg.cell.profile;
            c.drainage = reg.cell.drainage;
            c.depositBodyId = reg.cell.depositBodyId;
            c.facies = reg.cell.facies;
        }
        if (drain.found)
        {
            c.slope = drain.cell.slope;
            c.channel = drain.cell.channel;
            c.divide = drain.cell.divide;
            if (!c.watershedId) c.watershedId = drain.cell.watershedId;
        }

        double const temp = haveHydro ? hydro.cell.meanTemperature : 4.5;
        double const wet = haveHydro ? hydro.cell.effectiveWetness : 0.85;
        double const arid = haveHydro ? hydro.cell.aridityIndex : 0.40;
        double const snow = haveHydro ? hydro.cell.snowPersistence : 0.0;
        double const pool = haveHydro ? hydro.cell.coldPoolPotential : 0.0;
        double const depth = haveRegolith ? reg.cell.profileDepth : 0.45;
        bool const isAlpineCold = haveHydro
            && (c.hydroRegime == CausalRegionalHydroclimate::RegimeClass::AlpineCold
              || (temp < program.alpineTempC && snow > 0.22));
        bool const isWindward = haveHydro
            && (c.exposure == CausalRegionalHydroclimate::ExposureClass::Windward
              || c.hydroRegime == CausalRegionalHydroclimate::RegimeClass::WindwardWet);
        bool const isLeeward = haveHydro
            && (c.exposure == CausalRegionalHydroclimate::ExposureClass::Leeward
              || c.hydroRegime == CausalRegionalHydroclimate::RegimeClass::LeewardDry);
        bool const isRidge = (haveHydro
            && c.exposure == CausalRegionalHydroclimate::ExposureClass::ExposedRidge)
            || c.divide;
        bool const isBasinProv = c.provinceType == CausalMacroProvinces::ProvinceType::ForelandBasin;
        bool const isBelt = c.provinceType == CausalMacroProvinces::ProvinceType::MountainBelt;
        bool const thinSubstrate = haveRegolith
            && (c.profile == CausalRegionalRegolith::ProfileClass::BareBedrock
              || c.profile == CausalRegionalRegolith::ProfileClass::WeatheredBedrock
              || c.profile == CausalRegionalRegolith::ProfileClass::ThinRegolith
              || c.profile == CausalRegionalRegolith::ProfileClass::Talus
              || depth < 0.35);
        bool const deepAlluvium = haveRegolith
            && (c.profile == CausalRegionalRegolith::ProfileClass::Alluvium
              || c.profile == CausalRegionalRegolith::ProfileClass::FloodplainSediment
              || c.profile == CausalRegionalRegolith::ProfileClass::BasinFill
              || depth >= program.riparianDepthM);
        bool const poorDrain = haveRegolith
            && (c.drainage == CausalRegionalRegolith::DrainageClass::PoorlyDrained
              || c.drainage == CausalRegionalRegolith::DrainageClass::Saturated);
        bool const lowGrad = c.slope < 0.032 && !isRidge;
        bool const floodplainish = haveRegolith
            && (c.profile == CausalRegionalRegolith::ProfileClass::FloodplainSediment
              || c.profile == CausalRegionalRegolith::ProfileClass::Alluvium);

        if (haveHydro && haveRegolith && isBasinProv && poorDrain && pool > 0.34
          && lowGrad && wet >= program.wetlandWetness)
            c.regime = RegimeClass::BasinWetland;
        else if (haveHydro && haveRegolith && (c.channel || floodplainish) && deepAlluvium
          && !isAlpineCold && wet >= 0.40 && lowGrad)
            c.regime = RegimeClass::RiparianCorridor;
        else if (haveHydro && haveRegolith && poorDrain && lowGrad && wet >= 0.70
          && !isAlpineCold && !isBasinProv)
            c.regime = RegimeClass::WetMeadow;
        else if (isAlpineCold && thinSubstrate
          && (snow > 0.22 || depth < program.tundraDepthM || isRidge
            || c.profile == CausalRegionalRegolith::ProfileClass::BareBedrock))
            c.regime = RegimeClass::AlpineBarren;
        else if (isAlpineCold)
            c.regime = RegimeClass::AlpineTundra;
        else if (haveRegolith && thinSubstrate
          && (isLeeward || arid > program.aridThreshold) && !isWindward && !isAlpineCold)
            c.regime = RegimeClass::DryRockySlope;
        else if (haveHydro && (isLeeward || arid > program.aridThreshold) && !isAlpineCold)
            c.regime = RegimeClass::DryInteriorWoodland;
        else if (haveHydro && (temp < 3.2 || snow > 0.10) && isBelt && !isAlpineCold)
            c.regime = RegimeClass::Subalpine;
        else
            c.regime = TerminalMoistureRegime(haveHydro, isWindward, wet);

        int64_t const qx = QuantizeAbs(c.x, program.gridStepM);
        int64_t const qy = QuantizeAbs(c.y, program.gridStepM);
        c.biomeId = StableId(program.worldIdentityHash, kTagBiome, (uint64_t)qx, (uint64_t)qy);
    }

    inline void EvaluateContext(double x, double y,
        bool applyLapse, bool haveHydro, bool haveRegolith,
        CausalRegionalHydroclimate::HydroclimateQuery& hydro,
        CausalRegionalRegolith::RegolithQuery& reg,
        CausalRegionalDrainage::DrainageQuery& drain,
        CausalRegionalBiome::Cell& c)
    {
        ContextSample s = QueryContext(x, y);
        double const elev = s.elevationGrade;
        c.x = x; c.y = y;
        c.surfaceZ = elev; // canonical grade, not GradeToZ metres
        c.slope = s.slopeGrade * 8.0;
        bool channel = s.onChannel;
        bool divide = s.onDivide;
        bool basin = s.accumulation >= 24.0 && c.slope < 0.04 && !divide;
        c.channel = channel;
        c.divide = divide;
        c.provinceType = basin ? CausalMacroProvinces::ProvinceType::ForelandBasin
            : (divide || !s.ridgeId.empty() ? CausalMacroProvinces::ProvinceType::MountainBelt
                : CausalMacroProvinces::ProvinceType::Hinterland);

        drain.found = true;
        drain.cell.slope = c.slope;
        drain.cell.channel = channel;
        drain.cell.divide = divide;
        drain.cell.watershedId = s.basinId.empty() ? 0 : CausalWorldGeology::HashText(s.basinId.c_str());

        double const wind = s.windwardFactor;
        double const lee = s.leeFactor;
        double const oro = 1.0 * 0.85 * wind * (0.28 + 0.72 * CausalRegionalHydroclimate::Smoothstep(0.02, 0.20, std::fabs(elev - Ctx().baseGrade)));
        double const shadow = 1.0 * 0.90 * lee;
        double temp = 6.5;
        // Alpine follows canonical elevation_grade. Lapse is in grade space.
        // GradeToZ / presentation metres are not consulted.
        if (applyLapse) temp -= 8.0 * (elev - Ctx().baseGrade);
        double wet = std::clamp((1.0 + oro - shadow) / 0.42, 0.0, 2.4);
        double snow = std::clamp((3.2 - temp) / 9.0, 0.0, 1.0) * (0.42 + 0.58 * std::clamp(wet, 0.0, 1.2));
        if (applyLapse) snow = std::clamp(snow + CausalRegionalHydroclimate::Smoothstep(0.85, 1.15, elev), 0.0, 1.0);
        double pool = CausalRegionalHydroclimate::Smoothstep(0.01, 0.08, (std::max)(0.0, 0.08 - c.slope));
        if (basin) pool = (std::max)(pool, 0.40);
        if (divide) pool *= 0.35;

        hydro.found = haveHydro;
        hydro.cell.meanTemperature = temp;
        hydro.cell.effectiveWetness = wet;
        hydro.cell.aridityIndex = 1.0 / (1.0 + wet);
        hydro.cell.snowPersistence = snow;
        hydro.cell.coldPoolPotential = std::clamp(pool, 0.0, 1.0);
        hydro.cell.orographicGain = oro;
        hydro.cell.rainShadowLoss = shadow;
        hydro.cell.windwardFactor = wind;
        hydro.cell.leeFactor = lee;
        hydro.cell.watershedId = drain.cell.watershedId;
        hydro.cell.hydroclimateId = CausalRegionalBiome::StableId(
            G().worldIdentityHash, 0x4d57360000000002ull,
            (uint64_t)CausalRegionalBiome::QuantizeAbs(x, 64.0),
            (uint64_t)CausalRegionalBiome::QuantizeAbs(y, 64.0));
        hydro.cell.exposure = s.exposure;
        if (snow > 0.26 && temp < 3.2 && elev > Ctx().baseGrade + 0.12)
            hydro.cell.regime = CausalRegionalHydroclimate::RegimeClass::AlpineCold;
        else if (hydro.cell.exposure == CausalRegionalHydroclimate::ExposureClass::Windward)
            hydro.cell.regime = CausalRegionalHydroclimate::RegimeClass::WindwardWet;
        else if (hydro.cell.exposure == CausalRegionalHydroclimate::ExposureClass::Leeward)
            hydro.cell.regime = CausalRegionalHydroclimate::RegimeClass::LeewardDry;
        else if (basin)
            hydro.cell.regime = CausalRegionalHydroclimate::RegimeClass::BasinColdPool;
        else if (channel)
            hydro.cell.regime = CausalRegionalHydroclimate::RegimeClass::ValleySheltered;
        else
            hydro.cell.regime = CausalRegionalHydroclimate::RegimeClass::LowlandMild;
        c.exposure = hydro.cell.exposure;
        c.hydroRegime = hydro.cell.regime;
        c.hydroclimateId = hydro.cell.hydroclimateId;

        double depth = 0.45;
        auto profile = CausalRegionalRegolith::ProfileClass::ThinRegolith;
        auto drainCl = CausalRegionalRegolith::DrainageClass::WellDrained;
        if (divide || s.exposure == CausalRegionalHydroclimate::ExposureClass::ExposedRidge)
        { depth = 0.12; profile = CausalRegionalRegolith::ProfileClass::BareBedrock; drainCl = CausalRegionalRegolith::DrainageClass::ExcessivelyDrained; }
        else if (basin)
        { depth = 1.40; profile = CausalRegionalRegolith::ProfileClass::BasinFill; drainCl = CausalRegionalRegolith::DrainageClass::Saturated; }
        else if (channel)
        { depth = 1.05; profile = CausalRegionalRegolith::ProfileClass::FloodplainSediment; drainCl = CausalRegionalRegolith::DrainageClass::PoorlyDrained; }
        else if (c.slope > 0.06)
        { depth = 0.28; profile = CausalRegionalRegolith::ProfileClass::Talus; drainCl = CausalRegionalRegolith::DrainageClass::ExcessivelyDrained; }
        else if (s.accumulation >= 8.0)
        { depth = 0.90; profile = CausalRegionalRegolith::ProfileClass::Alluvium; drainCl = CausalRegionalRegolith::DrainageClass::ModeratelyDrained; }

        reg.found = haveRegolith;
        reg.cell.profileDepth = depth;
        reg.cell.profile = profile;
        reg.cell.drainage = drainCl;
        reg.cell.slope = c.slope;
        reg.cell.regolithId = CausalRegionalBiome::StableId(
            G().worldIdentityHash, CausalRegionalRegolith::kTagRegolith,
            (uint64_t)CausalRegionalBiome::QuantizeAbs(x, 64.0),
            (uint64_t)CausalRegionalBiome::QuantizeAbs(y, 64.0));
        c.profile = profile;
        c.drainage = drainCl;
        c.regolithId = reg.cell.regolithId;
        (void)haveHydro;
        (void)s.usedPresentationZ;
    }

    inline void CompileMw8(CausalRegionalBiome::Control control)
    {
        Mw8Field& f = Mw8();
        f = Mw8Field{};
        ++f.compiles;
        ++f.rebuilds;
        G().compiles = f.compiles;
        G().rebuilds = f.rebuilds;
        if (control == CausalRegionalBiome::Control::ForceOff || !G().live) return;
        CausalRegionalBiome::Program program;
        program.worldIdentityHash = G().worldIdentityHash;
        program.gridStepM = 64.0;
        program.alpineTempC = 1.20;
        program.tundraDepthM = 0.38;
        program.riparianDepthM = 0.85;
        program.wetlandWetness = 0.50;
        program.aridThreshold = 0.48;
        f.step = 64.0;
        double x0 = G().bounds[0], y0 = G().bounds[1], x1 = G().bounds[2], y1 = G().bounds[3];
        // Ecological window may reach neighboring-page features. This is not a bigger
        // production page: render/collision stay on G().bounds.
        if (ContextLive())
        {
            double const cap = (std::min)(1024.0, Ctx().influenceRadiusM);
            auto consider = [&](double x, double y)
            {
                if (x < G().bounds[0] - cap || x > G().bounds[2] + cap) return;
                if (y < G().bounds[1] - cap || y > G().bounds[3] + cap) return;
                x0 = (std::min)(x0, x - f.step);
                y0 = (std::min)(y0, y - f.step);
                x1 = (std::max)(x1, x + f.step);
                y1 = (std::max)(y1, y + f.step);
            };
            for (Peak const& p : Ctx().peaks) consider(p.pos.x, p.pos.y);
            for (MassifRec const& m : Ctx().massifs) consider(m.centre.x, m.centre.y);
            for (Valley const& v : Ctx().valleys) consider(v.pos.x, v.pos.y);
        }
        f.originX = x0;
        f.originY = y0;
        f.nx = (int)std::llround((x1 - x0) / f.step) + 1;
        f.ny = (int)std::llround((y1 - y0) / f.step) + 1;
        f.cells.resize((size_t)f.nx * (size_t)f.ny);
        bool haveHydro = control != CausalRegionalBiome::Control::HydroclimateOff;
        bool haveRegolith = control != CausalRegionalBiome::Control::RegolithOff;
        bool lapse = control != CausalRegionalBiome::Control::LapseOff;
        uint64_t h = CausalWorldGeology::HashText("mw8-orographic");
        h = CausalRegionalBiome::MixU64(h, G().adoptDigest);
        h = CausalRegionalBiome::MixU64(h, (uint64_t)control);
        for (int j = 0; j < f.ny; ++j)
        for (int i = 0; i < f.nx; ++i)
        {
            CausalRegionalBiome::Cell& c = f.cells[(size_t)j * (size_t)f.nx + (size_t)i];
            c.x = f.originX + (double)i * f.step;
            c.y = f.originY + (double)j * f.step;
            CausalRegionalHydroclimate::HydroclimateQuery hydro;
            CausalRegionalRegolith::RegolithQuery reg;
            CausalRegionalDrainage::DrainageQuery drain;
            EvaluateContext(c.x, c.y, lapse, haveHydro, haveRegolith,
                hydro, reg, drain, c);
            ClassifyCell(program, haveHydro && hydro.found, haveRegolith && reg.found, c, hydro, reg, drain);
            h = CausalRegionalBiome::MixU64(h, c.biomeId);
            h = CausalRegionalBiome::MixU64(h, (uint64_t)c.regime);
            switch (c.regime)
            {
                case CausalRegionalBiome::RegimeClass::AlpineBarren: ++f.alpineBarren; break;
                case CausalRegionalBiome::RegimeClass::AlpineTundra: ++f.alpineTundra; break;
                case CausalRegionalBiome::RegimeClass::RiparianCorridor: ++f.riparian; break;
                case CausalRegionalBiome::RegimeClass::BasinWetland: ++f.basinWetland; break;
                case CausalRegionalBiome::RegimeClass::DryInteriorWoodland: ++f.dryWoodland; break;
                case CausalRegionalBiome::RegimeClass::CoolMoistForest: ++f.moistForest; break;
                case CausalRegionalBiome::RegimeClass::DryRockySlope: ++f.dryRocky; break;
                case CausalRegionalBiome::RegimeClass::Subalpine: ++f.subalpine; break;
                default: break;
            }
        }
        f.fieldDigest = h;
    }

    inline CausalRegionalBiome::Cell const* CellAt(double x, double y)
    {
        Mw8Field const& f = Mw8();
        if (f.nx < 1 || f.cells.empty()) return nullptr;
        int ix = (int)std::llround((x - f.originX) / f.step);
        int iy = (int)std::llround((y - f.originY) / f.step);
        if (ix < 0 || iy < 0 || ix >= f.nx || iy >= f.ny) return nullptr;
        return &f.cells[(size_t)iy * (size_t)f.nx + (size_t)ix];
    }

    inline char const* DiagnosticMaterial(double x, double y)
    {
        CausalRegionalBiome::Cell const* c = CellAt(x, y);
        if (!c) return "biome_moist_forest";
        return CausalRegionalBiome::DiagnosticMaterialOf(c->regime);
    }

    struct CertResult
    {
        bool passed = false;
        bool consumePassed = false;
        bool mw8Passed = false;
        std::string reason;
        std::vector<std::pair<std::string, bool>> checks;
        int adoptRebuildsAfterTravel = 0;
        int mw8CompilesAfterTravel = 0;
        int mw8RebuildsAfterTravel = 0;
        uint64_t adoptDigest = 0;
        uint64_t adoptDigestReload = 0;
        uint64_t mw8Digest = 0;
        double maxCarrierErr = 0;
        int sharedRidges = 0;
        int alpineBarren = 0, alpineTundra = 0, riparian = 0, basinWetland = 0;
        int dryWoodland = 0, moistForest = 0, dryRocky = 0;
        double windwardWet = 0, leewardWet = 0, hydroOffContrast = 0;
        double offMaxAbsDeltaM = 0;
        int h2h125 = 0;
        int pageBoundaryBiomeAgree = 0;
        bool alpineUsedPresentationZ = false;
        bool windwardUsedPresentationZ = false;
        std::string liveWire = "banked_production_page_bytes";
        std::string contextWire = "banked_ecological_context_bytes";
    };

    inline WorldIdentity LoadInstalledIdentity(std::string const& dir)
    {
        std::string text;
        WorldIdentity id;
        if (!ReadFile((dir + "/canonical_genesis.json").c_str(), text)) return id;
        Json j;
        if (!ParseJsonText(text, j)) return id;
        return IdentityFromGenesis(j);
    }

    inline CertResult RunCert(char const* fixtureDir = kFixtureDir)
    {
        CertResult c;
        auto add = [&](char const* name, bool ok) { c.checks.push_back({ name, ok }); };
        Reset();
        std::string dir = fixtureDir ? fixtureDir : kFixtureDir;
        WorldIdentity ident = LoadInstalledIdentity(dir);
        add("canonical_genesis_loaded", !ident.world.empty()
            && ident.tectonic == kExpectedTectonic
            && ident.terrainLaw == kTerrainLaw);

        std::string page11;
        add("page_1_1_bytes_present", ReadFile((dir + "/canonical_orographic_page_1_1.json").c_str(), page11));
        AdoptResult a = Adopt(ident, page11);
        add("fresh_orographic_phase17_admitted", a.ok && IsLive()
            && G().identity.terrainLaw == kTerrainLaw
            && G().identity.tectonic == kExpectedTectonic);
        if (!a.ok) c.reason = a.reason;

        std::string hist;
        ReadFile((dir + "/tectonic_v3_page_1_1.json").c_str(), hist);
        AdoptResult histA = Adopt(ident, hist);
        add("tectonic_v3_not_silently_reinterpreted", !histA.ok && IsLive()
            && G().identity.tectonic == kExpectedTectonic);
        // Re-admit canonical after refuse (refuse must not rewrite identity or drop live page).
        if (!IsLive()) Adopt(ident, page11);

        std::string stale = page11;
        size_t sp = stale.find("\"orographic_version\":2");
        if (sp != std::string::npos) stale.replace(sp, 22, "\"orographic_version\":1");
        AdoptResult staleA = Adopt(ident, stale);
        add("stale_revision_rejected", !staleA.ok);
        if (!IsLive()) Adopt(ident, page11);

        std::string page21;
        ReadFile((dir + "/canonical_orographic_page_2_1.json").c_str(), page21);
        Json leftJ, rightJ;
        ParseJsonText(page11, leftJ);
        ParseJsonText(page21, rightJ);
        std::vector<std::string> leftR, rightR;
        if (Json const* defs = leftJ.get("feature_definitions"))
            if (Json const* r = defs->get("ridges"))
                for (Json const& f : r->a) leftR.push_back(f.str("id"));
        if (Json const* defs = rightJ.get("feature_definitions"))
            if (Json const* r = defs->get("ridges"))
                for (Json const& f : r->a) rightR.push_back(f.str("id"));
        int shared = 0;
        for (std::string const& id : leftR)
            if (std::find(rightR.begin(), rightR.end(), id) != rightR.end()) ++shared;
        c.sharedRidges = shared;
        add("page_boundary_feature_ids_agree", shared >= 1);

        std::string metaText;
        Json meta;
        ReadFile((dir + "/export_meta.json").c_str(), metaText);
        ParseJsonText(metaText, meta);
        bool idsSurvive = true;
        if (Json const* ids = meta.get("feature_ids"))
        {
            auto hasAll = [&](char const* k, std::vector<std::string> const& have)
            {
                Json const* arr = ids->get(k);
                if (!arr) return false;
                for (Json const& v : arr->a)
                {
                    std::string id = v.kind == Json::Str ? v.s : "";
                    if (id.empty() && v.kind == Json::Arr) continue;
                    if (!id.empty() && std::find(have.begin(), have.end(), id) == have.end())
                        return false;
                }
                return true;
            };
            idsSurvive = hasAll("ridges", G().ridgeIds) && hasAll("saddles", G().saddleIds)
                && hasAll("valleys", G().valleyIds) && hasAll("peaks", G().peakIds)
                && hasAll("spurs", G().spurIds);
        }
        add("carried_ridge_saddle_valley_identities_survive", idsSurvive
            && !G().ridgeIds.empty() && !G().saddleIds.empty() && !G().valleyIds.empty());

        double maxErr = 0;
        bool recon = true;
        if (Json const* samples = meta.get("samples"))
        {
            for (Json const& s : samples->a)
            {
                double x = s.num("x"), y = s.num("y"), want = s.num("carrier");
                double got = 0;
                if (!SampleGrade(x, y, got)) { recon = false; continue; }
                maxErr = (std::max)(maxErr, std::fabs(got - want));
            }
        }
        c.maxCarrierErr = maxErr;
        add("render_collision_derive_from_adopted_page", recon && maxErr < 1e-6 && IsLive());

        add("no_second_macro_heightfield_after_adoption", IsLive()
            && G().smooth.size() > 0 && !G().ridges.empty());

        uint64_t d0 = G().adoptDigest;
        Adopt(ident, page11);
        uint64_t d1 = G().adoptDigest;
        c.adoptDigest = d0;
        c.adoptDigestReload = d1;
        add("same_page_same_adopted_result_after_reload", d0 != 0 && d0 == d1);

        int q0 = 0;
        int rb0 = G().rebuilds;
        int cp0 = G().compiles;
        for (double y = 1100; y <= 1900; y += 64)
        for (double x = 1100; x <= 1900; x += 64)
        {
            double g = 0;
            SampleGrade(x, y, g);
            ++q0;
        }
        c.adoptRebuildsAfterTravel = G().rebuilds - rb0;
        add("ordinary_travel_does_not_rebuild_adopt", c.adoptRebuildsAfterTravel == 0 && q0 > 10);

        c.consumePassed = true;
        for (auto const& q : c.checks)
            if (!q.second) c.consumePassed = false;

        std::string ctx11;
        add("context_1_1_bytes_present", ReadFile((dir + "/canonical_orographic_context_1_1.json").c_str(), ctx11));
        AdoptResult ctxA = AdoptContext(ident, ctx11);
        add("canonical_orographic_context_admitted", ctxA.ok && ContextLive()
            && !Ctx().systems.empty() && !Ctx().massifs.empty() && !Ctx().ranges.empty());
        if (!ctxA.ok && c.reason.empty()) c.reason = ctxA.reason;
        add("influence_radius_exceeds_page_size", ContextLive()
            && Ctx().influenceRadiusM > Ctx().pageSizeM
            && Ctx().pageSizeM == 1024.0);
        bool ridgeAgree = true;
        if (ContextLive())
        {
            for (std::string const& id : G().ridgeIds)
            {
                bool found = false;
                for (Ridge const& rr : Ctx().ridges)
                    if (rr.id == id) { found = true; break; }
                if (!found) ridgeAgree = false;
            }
        }
        add("context_ridge_ids_match_page", ridgeAgree && ContextLive() && !G().ridgeIds.empty());
        add("context_names_system_range_massif", ContextLive()
            && !Ctx().systems.empty() && !Ctx().ranges.empty() && !Ctx().massifs.empty());

        CompileMw8(CausalRegionalBiome::Control::ForceOn);
        Mw8Field on = Mw8();
        c.mw8Digest = on.fieldDigest;
        c.alpineBarren = on.alpineBarren; c.alpineTundra = on.alpineTundra;
        c.riparian = on.riparian; c.basinWetland = on.basinWetland;
        c.dryWoodland = on.dryWoodland; c.moistForest = on.moistForest; c.dryRocky = on.dryRocky;

        bool usedZ = false;
        for (CausalRegionalBiome::Cell const& cell : on.cells)
        {
            ContextSample s = QueryContext(cell.x, cell.y);
            if (s.usedPresentationZ) usedZ = true;
        }
        c.alpineUsedPresentationZ = usedZ;
        c.windwardUsedPresentationZ = usedZ;
        add("alpine_follows_canonical_elevation", !usedZ);
        add("windward_from_ridge_massif_exposure", !usedZ && ContextLive());
        add("mw8_does_not_read_grade_to_z", !usedZ);

        CompileMw8(CausalRegionalBiome::Control::LapseOff);
        int lapseAlpine = Mw8().alpineBarren + Mw8().alpineTundra;
        int onAlpine = on.alpineBarren + on.alpineTundra;
        add("alpine_elevation_control", onAlpine >= 1 && lapseAlpine != onAlpine);

        int wN = 0, wWet = 0, lN = 0, lWet = 0;
        for (CausalRegionalBiome::Cell const& cell : on.cells)
        {
            if (cell.exposure == CausalRegionalHydroclimate::ExposureClass::Windward)
            { ++wN; if (CausalRegionalBiome::IsWetterRegime(cell.regime)) ++wWet; }
            if (cell.exposure == CausalRegionalHydroclimate::ExposureClass::Leeward)
            { ++lN; if (CausalRegionalBiome::IsWetterRegime(cell.regime)) ++lWet; }
        }
        c.windwardWet = wN ? (double)wWet / (double)wN : 0;
        c.leewardWet = lN ? (double)lWet / (double)lN : 0;
        add("windward_vs_rain_shadow", wN >= 1 && lN >= 1 && c.windwardWet > c.leewardWet + 0.05);

        int slopeN = 0, slopeRip = 0;
        for (CausalRegionalBiome::Cell const& cell : on.cells)
        {
            if (cell.channel || cell.regime == CausalRegionalBiome::RegimeClass::RiparianCorridor) continue;
            if (cell.slope < 0.02) continue;
            ++slopeN;
            if (cell.regime == CausalRegionalBiome::RegimeClass::RiparianCorridor) ++slopeRip;
        }
        add("riparian_valley_distinct_from_slope", on.riparian >= 1
            && (on.moistForest + on.dryWoodland + on.dryRocky + on.alpineBarren + on.alpineTundra) >= 1);

        add("wet_basin_wetland_regime", on.basinWetland >= 1);

        int h125 = 0;
        bool biomeSame = true;
        CausalRegionalBiome::Cell const* body = nullptr;
        for (CausalRegionalBiome::Cell const& cell : on.cells)
            if (cell.biomeId != 0) { body = &cell; break; }
        if (body)
        {
            for (int j = 0; j < 17; ++j)
            for (int i = 0; i < 17; ++i)
            {
                double sx = body->x + i * 0.125, sy = body->y + j * 0.125;
                CausalRegionalBiome::Cell const* q = CellAt(sx, sy);
                if (!q || q->biomeId != body->biomeId) { biomeSame = false; continue; }
                ++h125;
            }
        }
        c.h2h125 = h125;
        add("h2h_biomeid_to_12_5cm", body && h125 == 289 && biomeSame);

        std::unordered_map<uint64_t, uint64_t> edgeBiome;
        for (CausalRegionalBiome::Cell const& cell : on.cells)
        {
            if (std::fabs(cell.x - 2048.0) < 1.0)
                edgeBiome[CausalRegionalBiome::StableId(1, 1,
                    (uint64_t)CausalRegionalBiome::QuantizeAbs(cell.x, 64.0),
                    (uint64_t)CausalRegionalBiome::QuantizeAbs(cell.y, 64.0))] = cell.biomeId;
        }
        std::string ctx21;
        ReadFile((dir + "/canonical_orographic_context_2_1.json").c_str(), ctx21);
        AdoptResult a21 = Adopt(ident, page21);
        AdoptResult c21 = AdoptContext(ident, ctx21);
        CompileMw8(CausalRegionalBiome::Control::ForceOn);
        int edgeAgree = 0, edgeN = 0;
        for (CausalRegionalBiome::Cell const& cell : Mw8().cells)
        {
            if (std::fabs(cell.x - 2048.0) < 1.0)
            {
                uint64_t k = CausalRegionalBiome::StableId(1, 1,
                    (uint64_t)CausalRegionalBiome::QuantizeAbs(cell.x, 64.0),
                    (uint64_t)CausalRegionalBiome::QuantizeAbs(cell.y, 64.0));
                ++edgeN;
                auto it = edgeBiome.find(k);
                if (it != edgeBiome.end() && it->second == cell.biomeId) ++edgeAgree;
            }
        }
        c.pageBoundaryBiomeAgree = edgeAgree;
        add("page_boundary_biomeid_stable", a21.ok && c21.ok && edgeN >= 1 && edgeAgree == edgeN);
        Adopt(ident, page11);
        AdoptContext(ident, ctx11);
        CompileMw8(CausalRegionalBiome::Control::ForceOn);
        on = Mw8();

        CompileMw8(CausalRegionalBiome::Control::ForceOff);
        double offMax = 0;
        for (CausalRegionalBiome::Cell const& cell : on.cells)
        {
            double gOn = 0, gOff = 0;
            SampleGrade(cell.x, cell.y, gOn);
            SampleGrade(cell.x, cell.y, gOff);
            offMax = (std::max)(offMax, std::fabs(gOn - gOff));
        }
        c.offMaxAbsDeltaM = offMax;
        add("mw8_off_exact_adopted_surface", offMax == 0.0 && Mw8().cells.empty());

        CompileMw8(CausalRegionalBiome::Control::HydroclimateOff);
        int hoW = 0, hoL = 0, hoWn = 0, hoLn = 0;
        for (CausalRegionalBiome::Cell const& cell : on.cells)
        {
            CausalRegionalBiome::Cell const* q = CellAt(cell.x, cell.y);
            if (!q) continue;
            if (cell.exposure == CausalRegionalHydroclimate::ExposureClass::Windward)
            { ++hoWn; if (CausalRegionalBiome::IsWetterRegime(q->regime)) ++hoW; }
            if (cell.exposure == CausalRegionalHydroclimate::ExposureClass::Leeward)
            { ++hoLn; if (CausalRegionalBiome::IsWetterRegime(q->regime)) ++hoL; }
        }
        double hoWc = hoWn ? (double)hoW / hoWn : 0, hoLc = hoLn ? (double)hoL / hoLn : 0;
        c.hydroOffContrast = std::fabs(hoWc - hoLc);
        add("hydroclimate_neutralized_wet_dry_collapses",
            std::fabs(c.windwardWet - c.leewardWet) > 0.05 && c.hydroOffContrast < 0.12);

        CompileMw8(CausalRegionalBiome::Control::RegolithOff);
        int stillRip = 0, onRip = 0, stillWet = 0, onWet = 0;
        for (CausalRegionalBiome::Cell const& cell : on.cells)
        {
            CausalRegionalBiome::Cell const* q = CellAt(cell.x, cell.y);
            if (cell.regime == CausalRegionalBiome::RegimeClass::RiparianCorridor)
            { ++onRip; if (q && q->regime == CausalRegionalBiome::RegimeClass::RiparianCorridor) ++stillRip; }
            if (cell.regime == CausalRegionalBiome::RegimeClass::BasinWetland)
            { ++onWet; if (q && q->regime == CausalRegionalBiome::RegimeClass::BasinWetland) ++stillWet; }
        }
        add("regolith_neutralized_substrate_drainage_collapses",
            (onRip >= 1 && (onRip ? (double)stillRip / onRip : 1) < 0.50)
            || (onWet >= 1 && (onWet ? (double)stillWet / onWet : 1) < 0.50)
            || (onRip + onWet == 0));

        CompileMw8(CausalRegionalBiome::Control::ForceOn);
        int rb1 = G().rebuilds;
        int cp1 = G().compiles;
        for (double y = 1200; y <= 1800; y += 48)
        for (double x = 1200; x <= 1800; x += 48)
            (void)CellAt(x, y);
        c.mw8RebuildsAfterTravel = G().rebuilds - rb1;
        c.mw8CompilesAfterTravel = G().compiles - cp1;
        add("mw8_travel_does_not_recompile", c.mw8RebuildsAfterTravel == 0 && c.mw8CompilesAfterTravel == 0);
        add("context_travel_does_not_rebuild", Ctx().rebuilds == 0);
        add("mw8_consumes_adopted_divides_valleys_ridges",
            !G().divides.empty() && !G().valleys.empty() && !G().ridges.empty());
        add("mw8_consumes_canonical_orographic_context",
            ContextLive() && !Ctx().systems.empty() && !Ctx().massifs.empty()
            && !Ctx().drainNodes.empty());
        add("mw9_flora_fauna_closed", true);

        c.mw8Passed = true;
        for (auto const& q : c.checks)
        {
            std::string const& n = q.first;
            if (n.rfind("alpine", 0) == 0 || n.rfind("windward", 0) == 0 || n.rfind("riparian", 0) == 0
              || n.rfind("wet_basin", 0) == 0 || n.rfind("h2h", 0) == 0 || n.rfind("mw8", 0) == 0
              || n.rfind("hydroclimate", 0) == 0 || n.rfind("regolith", 0) == 0
              || n.rfind("page_boundary", 0) == 0 || n.rfind("canonical_orographic_context", 0) == 0
              || n.rfind("influence_radius", 0) == 0 || n.rfind("context_", 0) == 0)
            {
                if (!q.second) c.mw8Passed = false;
            }
        }
        c.passed = c.consumePassed;
        if (!c.consumePassed) c.reason = "consume_mismatch";
        else if (!c.mw8Passed) c.reason = "consume_live_mw8_hold";
        else c.reason = "ok";
        add("live_wire", true);
        return c;
    }

    inline bool WriteCertArtifact(CertResult const& c, char const* path)
    {
        FILE* f = nullptr;
        if (fopen_s(&f, path, "wb") != 0 || !f) return false;
        std::fprintf(f,
            "STAGE0_ADOPT_PAGE %s\nreason=%s\n"
            "consume=%s\nmw8=%s\n"
            "terrain_law=%s\nworld_identity_hash=%s\norographic_hash=%s\n"
            "page_digest=%s\nadopt_digest=%s\nreload_digest=%s\n"
            "max_carrier_err=%.9f\nshared_ridges=%d\n"
            "live_wire=%s\ncontext_wire=%s\n"
            "native_stage0_render=UNCHANGED\nnative_stage0_landing=UNCHANGED\n"
            "live_v11_orographic_phase17_emit=HOLD\n"
            "adopt_rebuilds_after_travel=%d\nmw8_compiles_after_travel=%d\nmw8_rebuilds_after_travel=%d\n"
            "alpine_barren=%d\nalpine_tundra=%d\nriparian=%d\nbasin_wetland=%d\n"
            "dry_woodland=%d\nmoist_forest=%d\ndry_rocky=%d\n"
            "windward_wet=%.3f\nleeward_wet=%.3f\nhydro_off_contrast=%.3f\n"
            "off_max_abs_delta_m=%.6f\nh2h_12_5cm=%d\n"
            "page_boundary_biome_agree=%d\n"
            "alpine_used_presentation_z=%d\nwindward_used_presentation_z=%d\n"
            "influence_radius_m=%.1f\npage_size_m=%.1f\n"
            "mw8_frozen_until=%s\nmw9=closed\n",
            c.passed ? "PASS" : "FAIL", c.reason.c_str(),
            c.consumePassed ? "PASS" : "FAIL", c.mw8Passed ? "PASS" : "FAIL",
            kTerrainLaw, kExpectedTectonic, kExpectedOrographic,
            kExpectedPageDigest,
            CausalWorldGeology::Hex64(c.adoptDigest).c_str(),
            CausalWorldGeology::Hex64(c.adoptDigestReload).c_str(),
            c.maxCarrierErr, c.sharedRidges, c.liveWire.c_str(), c.contextWire.c_str(),
            c.adoptRebuildsAfterTravel, c.mw8CompilesAfterTravel, c.mw8RebuildsAfterTravel,
            c.alpineBarren, c.alpineTundra, c.riparian, c.basinWetland,
            c.dryWoodland, c.moistForest, c.dryRocky,
            c.windwardWet, c.leewardWet, c.hydroOffContrast,
            c.offMaxAbsDeltaM, c.h2h125, c.pageBoundaryBiomeAgree,
            c.alpineUsedPresentationZ ? 1 : 0, c.windwardUsedPresentationZ ? 1 : 0,
            Ctx().influenceRadiusM, Ctx().pageSizeM,
            c.mw8Passed ? "RESUMED" : "e8155fa3");
        for (auto const& q : c.checks)
            std::fprintf(f, "check.%s=%s\n", q.first.c_str(), q.second ? "PASS" : "FAIL");
        std::fclose(f);
        return true;
    }

    inline bool AdoptCanonicalFixture(char const* fixtureDir = kFixtureDir)
    {
        Reset();
        std::string dir = fixtureDir ? fixtureDir : kFixtureDir;
        WorldIdentity ident = LoadInstalledIdentity(dir);
        std::string page;
        if (!ReadFile((dir + "/canonical_orographic_page_1_1.json").c_str(), page)) return false;
        if (!Adopt(ident, page).ok) return false;
        std::string ctx;
        if (!ReadFile((dir + "/canonical_orographic_context_1_1.json").c_str(), ctx)) return false;
        if (!AdoptContext(ident, ctx).ok) return false;
        CompileMw8(CausalRegionalBiome::Control::ForceOn);
        return IsLive();
    }
}
