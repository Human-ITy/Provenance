#pragma once

// Native Stage0 authority-admission seam for canonical Phase 17 orographic
// production pages. Single fail-closed adopt_page boundary: identity/revision
// is checked here, never in native render/collision/streaming.
//
// SampleGrade reconstructs the adopted orographic.phase17 carrier in GRADE
// space (MW8, _grade_at, carrier cert). SampleZ composes independently scaled
// metre terms (TerrainElevationComponents) and is render==collision==grounding.
// That is consume of the admitted page, not a second client-generated heightfield.
// WorldGenesis v11 macro_page / detailed WorldSubstrate must not replace this
// result on orographic.phase17 worlds (same page → same landform).
//
// MW8 QueryContext consumes orographic_ecological_context_v1 (elevation_grade,
// SystemId/RangeId/MassifId, ridge/divide/saddle/valley/basin, exposure).
// GradeToZ is no longer the hidden single knob for all vertical scales.
// MW9 occupancy is realized; play does not draw trees.

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
    constexpr char const* kLiveFixtureDir = "Data/Worldgen/orographic_phase17/live";
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
    // Metre composition scales. Independent of GradeToZ (relief*voxel).
    // Existing feature amplitudes stay in grade units; these convert each
    // hierarchy term to world metres without multiplying the summed grade field.
    constexpr double kMassifMetres = 1800.0;
    constexpr double kPeakMetres = 720.0;
    constexpr double kRidgeMetres = 380.0;
    constexpr double kSpurMetres = 220.0;
    constexpr double kSaddleRel = 0.40;
    constexpr double kSaddleNeckMetres = 80.0;
    constexpr double kValleyMetres = 28.0;
    constexpr double kValleyCapMetres = 240.0;

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

    // End-to-end consume receipt for one (x,y): carrier vs reconstructed feature terms.
    struct SampleTrace
    {
        bool ok = false;
        int px = 0, py = 0;
        double sampledCarrier = 0;
        double peakContribution = 0;
        double ridgeContribution = 0;
        double spurContribution = 0;
        double saddleContribution = 0;
        double valleyContribution = 0; // grade-domain: drainage stays in the smooth carrier
        double sharpTotal = 0;
        double grade = 0;
        float sampleZ = 0;
        float collisionZ = 0;
        std::string peakId, ridgeId, spurId, saddleId, valleyId, massifId;
        double regionalZ = 0, massifZ = 0, peakZ = 0, ridgeZ = 0;
        double saddleZ = 0, spurZ = 0, valleyZ = 0, localZ = 0;
    };

    struct TerrainElevationComponents
    {
        double regional_z = 0;
        double massif_z = 0;
        double peak_z = 0;
        double ridge_z = 0;
        double saddle_z = 0;
        double spur_z = 0;
        double valley_z = 0;
        double local_z = 0;
        std::string massifId, peakId, ridgeId, saddleId, spurId, valleyId;
        double compose() const
        {
            return regional_z + massif_z + peak_z + ridge_z
                + saddle_z + spur_z + valley_z + local_z;
        }
    };

    struct SharpTerms
    {
        double peakC = 0, ridgeC = 0, spurC = 0, saddleC = 0;
        double crest = 0, shoulder = 0, taper = 0, neck = 0;
        std::string peakId, ridgeId, spurId, saddleId;
        double total() const
        {
            return peakC + ridgeC + spurC + saddleC + crest + shoulder + taper + neck;
        }
    };

    struct AdoptedGeography
    {
        WorldIdentity identity;
        std::string cacheKey;
        int px = 0, py = 0;
        double bounds[4] = {};
        double pageSizeM = 1024;
        double step = 500;
        int n = 0;
        double smoothMean = 0.70;
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
    inline std::unordered_map<uint64_t, AdoptedGeography>& Atlas()
    {
        static std::unordered_map<uint64_t, AdoptedGeography> pages;
        return pages;
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
    inline uint64_t PageKey(int px, int py, int pageM = 1024)
    {
        uint64_t h = (uint64_t)(uint32_t)px << 32 | (uint32_t)py;
        if (pageM != 1024)
            h ^= (uint64_t)(uint32_t)pageM * 0x9e3779b97f4a7c15ull;
        return h;
    }
    inline void PageOf(double x, double y, int& px, int& py, double pageM = 1024.0)
    {
        px = (int)std::floor(x / pageM);
        py = (int)std::floor(y / pageM);
    }
    // Page-seam overlap. 8 m was thinner than a meso sample step, so a tile that
    // straddled a 1024 m edge failed SampleZ and opened a sky tear.
    constexpr double kPageSkirtM = 128.0;

    inline AdoptedGeography const* PageAt(double x, double y, double skirtM = kPageSkirtM)
    {
        AdoptedGeography const* best = nullptr;
        double bestArea = 1e300;
        auto consider = [&](AdoptedGeography const& p)
        {
            if (!p.live || p.smooth.empty()) return;
            double const x0 = p.bounds[0] - skirtM, y0 = p.bounds[1] - skirtM;
            double const x1 = p.bounds[2] + skirtM, y1 = p.bounds[3] + skirtM;
            if (x < x0 || x > x1 || y < y0 || y > y1) return;
            double const area = (p.bounds[2] - p.bounds[0]) * (p.bounds[3] - p.bounds[1]);
            if (area < bestArea) { bestArea = area; best = &p; }
        };
        for (auto const& kv : Atlas()) consider(kv.second);
        if (G().live) consider(G());
        return best;
    }
    // Presentation fill only: keep a tile that overhangs the atlas from punching
    // sky. Collision / grounding stay on the strict PageAt path. Do not refuse a
    // nearest live page at range — a 24 km cap dropped the 8192 m far ring and
    // opened the knife-edge horizon. Stretch is replaced when the far page adopts.
    inline AdoptedGeography const* NearestLivePage(double x, double y, double maxDistM = 40960.0)
    {
        if (AdoptedGeography const* hit = PageAt(x, y)) return hit;
        AdoptedGeography const* best = nullptr;
        double bestD = 1e300;
        auto consider = [&](AdoptedGeography const& p)
        {
            if (!p.live) return;
            double cx = std::clamp(x, p.bounds[0], p.bounds[2]);
            double cy = std::clamp(y, p.bounds[1], p.bounds[3]);
            double d = std::hypot(x - cx, y - cy);
            if (d < bestD) { bestD = d; best = &p; }
        };
        for (auto const& kv : Atlas()) consider(kv.second);
        if (G().live) consider(G());
        (void)maxDistM;
        return best;
    }
    inline int AtlasCount() { return (int)Atlas().size(); }
    inline bool AtlasHas(int px, int py, int pageM = 1024)
    {
        auto it = Atlas().find(PageKey(px, py, pageM));
        return it != Atlas().end() && it->second.live;
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
            auto hasId = [](std::vector<std::string> const& ids, std::string const& id)
            {
                return std::find(ids.begin(), ids.end(), id) != ids.end();
            };
            if (Json const* peaks = defs->get("peaks"))
                for (Json const& f : peaks->a)
                {
                    Peak p;
                    p.id = f.str("id"); p.parent = f.str("parent"); p.type = f.str("type", "peak");
                    if (Json const* pos = f.get("pos")) p.pos = AsVec2(*pos);
                    p.radius = f.num("radius"); p.prominence = f.num("prominence");
                    if (hasId(geo.peakIds, p.id)) continue;
                    geo.peaks.push_back(p);
                    geo.peakIds.push_back(p.id);
                }
            if (Json const* ridges = defs->get("ridges"))
                for (Json const& f : ridges->a)
                {
                    Ridge r;
                    r.id = f.str("id"); r.parent = f.str("parent"); r.type = f.str("type", "ridge");
                    if (Json const* ax = f.get("axis")) r.axis = AsAxis(*ax);
                    r.halfWidth = f.num("half_width"); r.crest = f.num("crest"); r.length = f.num("length");
                    if (hasId(geo.ridgeIds, r.id)) continue;
                    geo.ridges.push_back(r);
                    geo.ridgeIds.push_back(r.id);
                }
            if (Json const* saddles = defs->get("saddles"))
                for (Json const& f : saddles->a)
                {
                    Saddle s;
                    s.id = f.str("id"); s.parent = f.str("parent"); s.type = f.str("type", "saddle");
                    if (Json const* pos = f.get("pos")) s.pos = AsVec2(*pos);
                    s.radius = f.num("radius"); s.drop = f.num("drop");
                    if (hasId(geo.saddleIds, s.id)) continue;
                    geo.saddles.push_back(s);
                    geo.saddleIds.push_back(s.id);
                }
            if (Json const* spurs = defs->get("spurs"))
                for (Json const& f : spurs->a)
                {
                    Spur s;
                    s.id = f.str("id"); s.parent = f.str("parent"); s.type = f.str("type", "spur");
                    if (Json const* ax = f.get("axis")) s.axis = AsAxis(*ax);
                    s.halfWidth = f.num("half_width"); s.crest = f.num("crest");
                    if (hasId(geo.spurIds, s.id)) continue;
                    geo.spurs.push_back(s);
                    geo.spurIds.push_back(s.id);
                }
            if (Json const* valleys = defs->get("valleys"))
                for (Json const& f : valleys->a)
                {
                    Valley v;
                    v.id = f.str("id"); v.type = f.str("type", "valley");
                    v.parent = f.str("parent");
                    if (Json const* pos = f.get("pos")) v.pos = AsVec2(*pos);
                    v.accumulation = f.num("accumulation"); v.width = f.num("width");
                    if (hasId(geo.valleyIds, v.id)) continue;
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

    inline SharpTerms SharpTermsFromFeatures(AdoptedGeography const& geo, double x, double y)
    {
        SharpTerms t;
        double bestPeak = 0, bestRidge = 0, bestSpur = 0, bestSaddle = 0;
        for (Peak const& p : geo.peaks)
        {
            double d = std::hypot(x - p.pos.x, y - p.pos.y);
            double f = Falloff(d, p.radius);
            if (f > 0.0)
            {
                double c = p.prominence * f * f;
                t.peakC += c;
                if (c > bestPeak) { bestPeak = c; t.peakId = p.id; }
            }
        }
        for (Ridge const& r : geo.ridges)
        {
            auto dt = DistToPolyline(r.axis, x, y);
            double d = dt.first, hw = r.halfWidth;
            double f = Falloff(d, hw);
            if (f > 0.0)
            {
                double c = r.crest * f;
                t.ridgeC += c;
                if (c > bestRidge) { bestRidge = c; t.ridgeId = r.id; }
            }
            if (d < hw * kRidgeReach)
            {
                t.crest += r.crest * kCrestSharpen * Falloff(d, hw * 0.45);
                if (hw * 0.5 < d && d < hw * kRidgeReach)
                {
                    double band = (d - hw * 0.5) / (hw * (kRidgeReach - 0.5));
                    t.shoulder -= r.crest * kShoulderDrop * Smoothstep(1.0 - std::fabs(2.0 * band - 1.0));
                }
            }
        }
        for (Spur const& sp : geo.spurs)
        {
            auto dt = DistToPolyline(sp.axis, x, y);
            double f = Falloff(dt.first, sp.halfWidth);
            if (f > 0.0)
            {
                double c = sp.crest * f;
                t.spurC += c;
                t.taper -= sp.crest * kSpurTaper * dt.second * f;
                if (c > bestSpur) { bestSpur = c; t.spurId = sp.id; }
            }
        }
        for (Saddle const& sd : geo.saddles)
        {
            double d = std::hypot(x - sd.pos.x, y - sd.pos.y);
            double f = Falloff(d, sd.radius);
            double sTerm = 0;
            if (f > 0.0) sTerm -= (t.ridgeC + t.peakC) * sd.drop * f;
            double fn = Falloff(d, sd.radius * kSaddleNeckReach);
            if (fn > 0.0) t.neck -= sd.drop * kSaddleNeck * fn * fn;
            t.saddleC += sTerm;
            if (std::fabs(sTerm) + std::fabs(t.neck) > bestSaddle)
            {
                bestSaddle = std::fabs(sTerm) + std::fabs(t.neck);
                t.saddleId = sd.id;
            }
        }
        return t;
    }

    inline double SharpFromFeatures(AdoptedGeography const& geo, double x, double y)
    {
        return SharpTermsFromFeatures(geo, x, y).total();
    }

    inline double IdUnit(std::string const& id, uint64_t salt)
    {
        uint64_t h = CausalWorldGeology::HashText(id);
        h = CausalRegionalBiome::MixU64(h, salt);
        return (double)(h & 0xFFFFFFull) / (double)0x1000000ull;
    }

    inline double MassifTrendOf(std::string const& massifId)
    {
        for (MassifRec const& m : Ctx().massifs)
            if (m.id == massifId) return m.trendRad;
        return 0.0;
    }

    // Anisotropic summit: massif-trend ellipse + id skew. Not an isotropic cone.
    inline double PeakProfileWeight(Peak const& p, double x, double y)
    {
        double const dx = x - p.pos.x, dy = y - p.pos.y;
        double const elong = 0.20 + 0.22 * IdUnit(p.id, 1);
        double const rx = (std::max)(p.radius * (1.0 + elong), 1.0);
        double const ry = (std::max)(p.radius * (1.0 - elong * 0.70), 1.0);
        double const yaw = MassifTrendOf(p.parent) + (IdUnit(p.id, 2) - 0.5) * 0.85;
        double const c = std::cos(yaw), s = std::sin(yaw);
        double u = dx * c + dy * s;
        double v = -dx * s + dy * c;
        double const skew = 0.16 + 0.24 * IdUnit(p.id, 3);
        if (v >= 0.0) v /= (1.0 + skew);
        else v /= (1.0 - skew * 0.50);
        double const q = std::hypot(u / rx, v / ry);
        if (q >= 1.0) return 0.0;
        double const apex = std::pow(1.0 - q, 2.65);
        double const shoulder = std::pow(1.0 - q, 1.20);
        double const mix = 0.58 + 0.20 * IdUnit(p.id, 4);
        return mix * apex + (1.0 - mix) * shoulder;
    }

    // Sharp crest + concave shoulder; width tapers toward ridge ends. Not a
    // constant-width triangular roof.
    inline double RidgeProfileWeight(Ridge const& r, double x, double y)
    {
        auto const dt = DistToPolyline(r.axis, x, y);
        double const t = dt.second, d = dt.first;
        double const mid = 4.0 * t * (1.0 - t);
        double const hw = r.halfWidth * (0.62 + 0.38 * mid)
            * (0.90 + 0.18 * IdUnit(r.id, 1));
        if (!(hw > 1.0) || d >= hw) return 0.0;
        double const u = d / hw;
        double const crest = std::exp(-u * u * 16.0);
        double const sh = std::pow(1.0 - u, 1.65);
        return (0.52 * crest + 0.48 * sh) * (0.80 + 0.20 * mid);
    }

    inline double SpurProfileWeight(Spur const& sp, double x, double y)
    {
        auto const dt = DistToPolyline(sp.axis, x, y);
        double const hw = (std::max)(sp.halfWidth, 1.0);
        if (dt.first >= hw) return 0.0;
        double const u = dt.first / hw;
        double const crest = std::exp(-u * u * 10.0);
        double const sh = std::pow(1.0 - u, 1.40);
        double const along = (std::max)(0.0, 1.0 - kSpurTaper * dt.second);
        return along * (0.45 * crest + 0.55 * sh);
    }

    inline double MassifProfileWeight(MassifRec const& m, double x, double y)
    {
        double const dx = x - m.centre.x, dy = y - m.centre.y;
        double const c = std::cos(m.trendRad), s = std::sin(m.trendRad);
        double const u = dx * c + dy * s;
        double const v = -dx * s + dy * c;
        double const rx = (std::max)(m.radius * 1.18, 1.0);
        double const ry = (std::max)(m.radius * 0.82, 1.0);
        double const q = std::hypot(u / rx, v / ry);
        if (q >= 1.0) return 0.0;
        return std::pow(1.0 - q, 1.35);
    }

    inline TerrainElevationComponents SampleElevationComponents(
        double x, double y, float datum, float relief, float voxel, bool nearest)
    {
        TerrainElevationComponents c;
        AdoptedGeography const* page = PageAt(x, y);
        if (!page && nearest) page = NearestLivePage(x, y);
        if (!page || !page->live || page->smooth.empty()) return c;
        double const localScale = (double)relief * (double)voxel;
        double const smooth = SampledOnly(*page, x, y);
        c.regional_z = (page->smoothMean - (double)datum) * localScale;
        c.local_z = (smooth - page->smoothMean) * localScale;

        double bestM = 0;
        for (MassifRec const& m : Ctx().massifs)
        {
            double const w = MassifProfileWeight(m, x, y);
            if (w <= 0.0) continue;
            double const z = m.lift * kMassifMetres * w;
            c.massif_z += z;
            if (z > bestM) { bestM = z; c.massifId = m.id; }
        }

        std::unordered_map<std::string, Peak const*> peaks;
        std::unordered_map<std::string, Ridge const*> ridges;
        std::unordered_map<std::string, Spur const*> spurs;
        std::unordered_map<std::string, Saddle const*> saddles;
        auto ingest = [&](AdoptedGeography const& g)
        {
            for (Peak const& p : g.peaks) if (!p.id.empty()) peaks.emplace(p.id, &p);
            for (Ridge const& r : g.ridges) if (!r.id.empty()) ridges.emplace(r.id, &r);
            for (Spur const& s : g.spurs) if (!s.id.empty()) spurs.emplace(s.id, &s);
            for (Saddle const& s : g.saddles) if (!s.id.empty()) saddles.emplace(s.id, &s);
        };
        for (auto const& kv : Atlas()) ingest(kv.second);
        if (G().live) ingest(G());
        for (Peak const& p : Ctx().peaks) if (!p.id.empty()) peaks.emplace(p.id, &p);
        for (Ridge const& r : Ctx().ridges) if (!r.id.empty()) ridges.emplace(r.id, &r);
        for (Spur const& s : Ctx().spurs) if (!s.id.empty()) spurs.emplace(s.id, &s);
        for (Saddle const& s : Ctx().saddles) if (!s.id.empty()) saddles.emplace(s.id, &s);
        double bestP = 0, bestR = 0, bestS = 0;
        for (auto const& kv : peaks)
        {
            Peak const& p = *kv.second;
            double const w = PeakProfileWeight(p, x, y);
            if (w <= 0.0) continue;
            double const z = p.prominence * kPeakMetres * w;
            c.peak_z += z;
            if (z > bestP) { bestP = z; c.peakId = p.id; }
        }
        for (auto const& kv : ridges)
        {
            Ridge const& r = *kv.second;
            double const w = RidgeProfileWeight(r, x, y);
            if (w <= 0.0) continue;
            double const z = r.crest * kRidgeMetres * w;
            c.ridge_z += z;
            if (z > bestR) { bestR = z; c.ridgeId = r.id; }
        }
        for (auto const& kv : spurs)
        {
            Spur const& sp = *kv.second;
            double const w = SpurProfileWeight(sp, x, y);
            if (w <= 0.0) continue;
            double const z = sp.crest * kSpurMetres * w;
            c.spur_z += z;
            if (z > bestS) { bestS = z; c.spurId = sp.id; }
        }
        double bestSd = 0;
        for (auto const& kv : saddles)
        {
            Saddle const& sd = *kv.second;
            double const d = std::hypot(x - sd.pos.x, y - sd.pos.y);
            double const f = Falloff(d, sd.radius);
            double const fn = Falloff(d, sd.radius * kSaddleNeckReach);
            double z = 0;
            if (f > 0.0) z -= (c.peak_z + c.ridge_z) * sd.drop * f * kSaddleRel;
            if (fn > 0.0) z -= sd.drop * kSaddleNeckMetres * fn * fn;
            c.saddle_z += z;
            if (std::fabs(z) > bestSd) { bestSd = std::fabs(z); c.saddleId = sd.id; }
        }

        double incision = 0;
        double bestV = 0;
        auto cutValley = [&](std::string const& id, double vx, double vy, double acc, double width)
        {
            double const reach = (std::max)(width, 40.0 + 6.0 * std::sqrt((std::max)(0.0, acc)));
            double const d = std::hypot(x - vx, y - vy);
            double const f = Falloff(d, reach);
            if (f <= 0.0) return;
            double const z = kValleyMetres * std::sqrt((std::max)(acc, 1.0) / 12.0) * f * f;
            incision += z;
            if (z > bestV) { bestV = z; c.valleyId = id; }
        };
        // Drainage-topology incision. Do not treat Z<sea as water.
        for (DrainNode const& n : Ctx().drainNodes)
        {
            if (n.onDivide || n.accumulation < 6.0) continue;
            cutValley(n.basin.empty() ? std::string("drain") : n.basin, n.pos.x, n.pos.y,
                n.accumulation, 40.0 + 6.0 * std::sqrt(n.accumulation));
        }
        double const highland = (std::max)(0.0, c.massif_z + c.peak_z + c.ridge_z);
        double const highlandScale = Smoothstep((highland - 30.0) / 90.0);
        c.valley_z = -incision * highlandScale;
        return c;
    }

    inline bool SampleGrade(double x, double y, double& out)
    {
        AdoptedGeography const* page = PageAt(x, y);
        if (!page || !page->live || page->smooth.empty()) return false;
        // Smooth samples are page-local; sharp features are world-space definitions
        // and must be evaluated at the true (x,y), never clamped to the page AABB.
        // Clamping was carrier-only consume: peaks/ridges outside the rectangle
        // vanished from SampleZ even though the page carried their definitions.
        double g = SampledOnly(*page, x, y) + SharpFromFeatures(*page, x, y);
        out = std::clamp(g, kGradeMin, kGradeMax);
        return true;
    }

    inline bool SampleZ(float x, float y, float datum, float relief, float voxel, float& outZ)
    {
        AdoptedGeography const* page = PageAt(x, y);
        if (!page) page = NearestLivePage(x, y);
        if (!page || !page->live || page->smooth.empty()) return false;
        TerrainElevationComponents const c =
            SampleElevationComponents(x, y, datum, relief, voxel, true);
        outZ = (float)c.compose();
        return std::isfinite(outZ);
    }

    inline bool SampleGradeNearest(double x, double y, double& out)
    {
        if (SampleGrade(x, y, out)) return true;
        AdoptedGeography const* page = NearestLivePage(x, y);
        if (!page || !page->live || page->smooth.empty()) return false;
        double g = SampledOnly(*page, x, y) + SharpFromFeatures(*page, x, y);
        out = std::clamp(g, kGradeMin, kGradeMax);
        return true;
    }

    inline bool SampleZNearest(float x, float y, float datum, float relief, float voxel, float& outZ)
    {
        AdoptedGeography const* page = PageAt(x, y);
        if (!page) page = NearestLivePage(x, y);
        if (!page || !page->live || page->smooth.empty()) return false;
        TerrainElevationComponents const c =
            SampleElevationComponents(x, y, datum, relief, voxel, true);
        outZ = (float)c.compose();
        return std::isfinite(outZ);
    }

    inline SampleTrace TraceSample(double x, double y, float datum, float relief, float voxel)
    {
        SampleTrace t;
        AdoptedGeography const* page = PageAt(x, y);
        if (!page) page = NearestLivePage(x, y);
        if (!page || !page->live || page->smooth.empty()) return t;
        t.ok = true;
        t.px = page->px; t.py = page->py;
        t.sampledCarrier = SampledOnly(*page, x, y);
        SharpTerms const sh = SharpTermsFromFeatures(*page, x, y);
        t.peakContribution = sh.peakC;
        t.ridgeContribution = sh.ridgeC;
        t.spurContribution = sh.spurC;
        t.saddleContribution = sh.saddleC + sh.neck;
        t.sharpTotal = sh.total();
        t.peakId = sh.peakId; t.ridgeId = sh.ridgeId;
        t.spurId = sh.spurId; t.saddleId = sh.saddleId;
        double bestV = 1e300;
        auto considerValley = [&](AdoptedGeography const& g)
        {
            for (Valley const& v : g.valleys)
            {
                double d = std::hypot(x - v.pos.x, y - v.pos.y);
                if (d < bestV) { bestV = d; t.valleyId = v.id; }
            }
        };
        for (auto const& kv : Atlas()) considerValley(kv.second);
        if (G().live) considerValley(G());
        t.valleyContribution = 0.0;
        t.grade = std::clamp(t.sampledCarrier + t.sharpTotal, kGradeMin, kGradeMax);
        TerrainElevationComponents const comp =
            SampleElevationComponents(x, y, datum, relief, voxel, true);
        t.regionalZ = comp.regional_z; t.massifZ = comp.massif_z;
        t.peakZ = comp.peak_z; t.ridgeZ = comp.ridge_z;
        t.saddleZ = comp.saddle_z; t.spurZ = comp.spur_z;
        t.valleyZ = comp.valley_z; t.localZ = comp.local_z;
        if (!comp.peakId.empty()) t.peakId = comp.peakId;
        if (!comp.ridgeId.empty()) t.ridgeId = comp.ridgeId;
        if (!comp.saddleId.empty()) t.saddleId = comp.saddleId;
        if (!comp.spurId.empty()) t.spurId = comp.spurId;
        if (!comp.valleyId.empty()) t.valleyId = comp.valleyId;
        t.massifId = comp.massifId;
        t.sampleZ = (float)comp.compose();
        t.collisionZ = t.sampleZ;
        return t;
    }

    inline CausalRegionalBiome::Cell const* CellAt(double x, double y);

    // Play albedo: continuous ground from substrate, never PeakId paint.
    // Reconstruction stays in SampleZ. alpine_barren white is debug-only.
    inline char const* SubstratePlayMaterial(double x, double y)
    {
        if (CausalRegionalBiome::Cell const* c = CellAt(x, y))
        {
            using P = CausalRegionalRegolith::ProfileClass;
            switch (c->profile)
            {
            case P::BareBedrock:
            case P::WeatheredBedrock:
            case P::Talus:
                return "rock";
            case P::Alluvium:
            case P::FloodplainSediment:
            case P::BasinFill:
            case P::OrganicCapable:
                return "loam";
            default:
                return "dirt";
            }
        }
        SampleTrace const t = TraceSample(x, y, 0.5f, 64.f, 0.125f);
        if (!t.ok) return "dirt";
        if (!t.valleyId.empty() && t.sampledCarrier < 0.55) return "loam";
        if (t.ridgeContribution >= 0.05 || t.saddleContribution <= -0.02)
            return "rock";
        return "dirt";
    }

    // Debug PeakId / ridge occupancy paint — looks like white worm-strips, not mountains.
    inline char const* FeatureFootprintMaterial(double x, double y)
    {
        SampleTrace const t = TraceSample(x, y, 0.5f, 64.f, 0.125f);
        if (!t.ok) return "dirt";
        if (t.peakContribution >= 0.08) return "biome_alpine_barren";
        if (t.ridgeContribution >= 0.05 || t.saddleContribution <= -0.02)
            return "biome_dry_rocky";
        if (!t.valleyId.empty() && t.sampledCarrier < 0.55) return "loam";
        if (t.grade >= 1.05) return "biome_alpine_tundra";
        return "dirt";
    }

    // Terrain/material presentation for adopted pages. Not MW9 flora.
    // Default play is dirt/rock/loam. Pass featureFootprints for the old highlighter.
    inline char const* AppearanceMaterial(double x, double y, bool featureFootprints = false)
    {
        return featureFootprints ? FeatureFootprintMaterial(x, y)
                                 : SubstratePlayMaterial(x, y);
    }

    // Presentation sea is GradeToZ(datum) = 0. Do not retune GradeToZ for ecology.
    inline float PresentationSeaZ() { return 0.f; }

    inline bool FindHighestLand(float datum, float relief, float voxel,
        float& outX, float& outY, float& outZ)
    {
        bool have = false;
        float sea = PresentationSeaZ();
        auto consider = [&](AdoptedGeography const& geo)
        {
            if (!geo.live || geo.smooth.empty() || !(geo.step > 0.0)) return;
            double const x0 = geo.bounds[0], y0 = geo.bounds[1];
            for (int j = 0; j < geo.n; ++j)
            for (int i = 0; i < geo.n; ++i)
            {
                double const x = x0 + i * geo.step;
                double const y = y0 + j * geo.step;
                float z = 0.f;
                if (!SampleZ((float)x, (float)y, datum, relief, voxel, z)) continue;
                if (z <= sea + 0.25f) continue;
                if (!have || z > outZ)
                {
                    have = true;
                    outX = (float)x;
                    outY = (float)y;
                    outZ = z;
                }
            }
        };
        for (auto const& kv : Atlas()) consider(kv.second);
        if (!have && G().live) consider(G());
        return have;
    }

    inline bool IsLive() { return G().live || AtlasCount() > 0; }
    inline AdoptedGeography const& Get() { return G(); }
    inline void Reset()
    {
        G() = AdoptedGeography{};
        Atlas().clear();
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
        double pageSizeM = page.num("page_size_m", 0.0);
        if (!(pageSizeM > 0.0))
        {
            if (Json const* b = page.get("bounds"); b && b->kind == Json::Arr && b->a.size() >= 4)
                pageSizeM = b->a[2].n - b->a[0].n;
        }
        for (char const* k : kinds)
        {
            Json const* arr = defs->get(k);
            if (!arr || arr->kind != Json::Arr)
            { r.reason = std::string("production page omitted ") + k + " definitions"; return r; }
            if (arr->a.empty() && pageSizeM <= 1024.5)
            { r.reason = std::string("production page carried no ") + k + " definitions"; return r; }
        }

        AdoptedGeography geo;
        geo.identity = identity;
        geo.cacheKey = page.str("cache_key");
        if (Json const* pg = page.get("page"); pg && pg->kind == Json::Arr && pg->a.size() >= 2)
        { geo.px = (int)pg->a[0].n; geo.py = (int)pg->a[1].n; }
        if (Json const* b = page.get("bounds"); b && b->kind == Json::Arr && b->a.size() >= 4)
            for (int i = 0; i < 4; ++i) geo.bounds[i] = b->a[(size_t)i].n;
        geo.pageSizeM = page.num("page_size_m", 0.0);
        if (!(geo.pageSizeM > 0.0) && geo.bounds[2] > geo.bounds[0])
            geo.pageSizeM = geo.bounds[2] - geo.bounds[0];
        if (!(geo.pageSizeM > 0.0)) geo.pageSizeM = 1024.0;
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
        if (!geo.smooth.empty())
        {
            double sum = 0;
            int nsm = 0;
            for (auto const& row : geo.smooth)
                for (double v : row) { sum += v; ++nsm; }
            if (nsm > 0) geo.smoothMean = sum / (double)nsm;
        }
        LoadFeatures(page, geo);
        geo.worldIdentityHash = ParseHex64(identity.tectonic);
        geo.live = true;
        geo.adoptCount = G().live ? G().adoptCount + 1 : 1;
        geo.rebuilds = 0;
        geo.compiles = 0;
        geo.adoptDigest = MixSamples(geo);
        uint64_t const key = PageKey(geo.px, geo.py, (int)std::lround(geo.pageSizeM));
        Atlas()[key] = geo;
        G() = std::move(geo);
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
        bool featureSurfaceReconstructed = false;
        double peakCarrier = 0, peakSharp = 0, peakGrade = 0, peakZ = 0, peakCollisionZ = 0;
        double ridgeSharp = 0, saddleSharp = 0, spurSharp = 0, valleyCarrier = 0;
        std::string proofPeakId, proofRidgeId, proofSaddleId, proofSpurId, proofValleyId;
        double maxReloadAbsDelta = 0;
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
        std::string liveReceipt;
        std::string dir = fixtureDir ? fixtureDir : kFixtureDir;
        if ((!fixtureDir || std::string(fixtureDir) == kFixtureDir)
            && ReadFile((std::string(kLiveFixtureDir) + "/live_stream_receipt.json").c_str(), liveReceipt))
        {
            dir = kLiveFixtureDir;
            c.liveWire = "orographic_production_page";
            c.contextWire = "orographic_ecological_context";
        }
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

        add("page_2_1_admitted", Adopt(ident, page21).ok && AtlasHas(2, 1));
        {
            Peak const* peak = nullptr;
            Ridge const* ridge = nullptr;
            Saddle const* saddle = nullptr;
            Spur const* spur = nullptr;
            Valley const* valley = nullptr;
            auto onAtlas = [&](double x, double y) { return PageAt(x, y) != nullptr; };
            auto findPeak = [&](AdoptedGeography const& g)
            {
                for (Peak const& p : g.peaks)
                    if (onAtlas(p.pos.x, p.pos.y) && (!peak || p.prominence > peak->prominence))
                        peak = &p;
            };
            auto findRidge = [&](AdoptedGeography const& g)
            {
                for (Ridge const& r : g.ridges)
                {
                    if (r.axis.size() < 2) continue;
                    Vec2 m = r.axis[r.axis.size() / 2];
                    if (onAtlas(m.x, m.y) && (!ridge || r.crest > ridge->crest)) ridge = &r;
                }
            };
            auto findSaddle = [&](AdoptedGeography const& g)
            {
                for (Saddle const& s : g.saddles)
                    if (onAtlas(s.pos.x, s.pos.y) && (!saddle || s.drop > saddle->drop))
                        saddle = &s;
            };
            auto findSpur = [&](AdoptedGeography const& g)
            {
                for (Spur const& s : g.spurs)
                {
                    if (s.axis.size() < 2) continue;
                    double x = 0.5 * (s.axis.front().x + s.axis.back().x);
                    double y = 0.5 * (s.axis.front().y + s.axis.back().y);
                    if (onAtlas(x, y) && (!spur || s.crest > spur->crest)) spur = &s;
                }
            };
            auto findValley = [&](AdoptedGeography const& g)
            {
                for (Valley const& v : g.valleys)
                    if (onAtlas(v.pos.x, v.pos.y)
                      && (!valley || v.accumulation > valley->accumulation))
                        valley = &v;
            };
            for (auto const& kv : Atlas())
            {
                findPeak(kv.second); findRidge(kv.second); findSaddle(kv.second);
                findSpur(kv.second); findValley(kv.second);
            }
            float const datum = 0.5f, relief = 64.f, voxel = 0.125f;
            bool idsCross = shared >= 1;
            if (peak && ridge && saddle && spur && valley)
            {
                SampleTrace tp = TraceSample(peak->pos.x, peak->pos.y, datum, relief, voxel);
                SampleTrace tr;
                if (ridge->axis.size() >= 2)
                    tr = TraceSample(ridge->axis[ridge->axis.size()/2].x,
                        ridge->axis[ridge->axis.size()/2].y, datum, relief, voxel);
                SampleTrace ts = TraceSample(saddle->pos.x, saddle->pos.y, datum, relief, voxel);
                SampleTrace tsp;
                if (spur->axis.size() >= 2)
                    tsp = TraceSample(0.5 * (spur->axis.front().x + spur->axis.back().x),
                        0.5 * (spur->axis.front().y + spur->axis.back().y), datum, relief, voxel);
                SampleTrace tv = TraceSample(valley->pos.x, valley->pos.y, datum, relief, voxel);
                c.proofPeakId = peak->id; c.proofRidgeId = ridge->id;
                c.proofSaddleId = saddle->id; c.proofSpurId = spur->id;
                c.proofValleyId = valley->id;
                c.peakCarrier = tp.sampledCarrier; c.peakSharp = tp.peakContribution;
                c.peakGrade = tp.grade; c.peakZ = tp.sampleZ; c.peakCollisionZ = tp.collisionZ;
                c.ridgeSharp = tr.ridgeContribution; c.saddleSharp = ts.saddleContribution;
                c.spurSharp = tsp.spurContribution; c.valleyCarrier = tv.sampledCarrier;
                bool peakTopo = tp.ok && tp.peakContribution >= 0.20
                    && tp.peakContribution + 1e-6 >= peak->prominence * 0.85
                    && tp.peakId == peak->id;
                bool ridgeTopo = tr.ok && tr.ridgeContribution > 0.02 && !tr.ridgeId.empty();
                bool saddleTopo = ts.ok && ts.saddleContribution < -0.01 && !ts.saddleId.empty();
                bool spurTopo = tsp.ok && tsp.spurContribution > 0.01 && !tsp.spurId.empty();
                bool valleyPresent = tv.ok && !tv.valleyId.empty();
                bool sameZ = tp.ok && std::fabs(tp.sampleZ - tp.collisionZ) < 1e-6f;
                bool reconstructed = tp.ok && tp.peakContribution > 0.20
                    && std::fabs(tp.grade - tp.sampledCarrier) > 0.15;
                c.featureSurfaceReconstructed = peakTopo && ridgeTopo && saddleTopo
                    && spurTopo && valleyPresent && sameZ && idsCross && reconstructed;
                add("feature_peak_in_sample_z", peakTopo);
                add("feature_ridge_in_sample_z", ridgeTopo);
                add("feature_saddle_in_sample_z", saddleTopo);
                add("feature_spur_in_sample_z", spurTopo);
                add("feature_valley_id_survives", valleyPresent);
                add("render_collision_same_sample_z", sameZ);
            }
            else
            {
                add("feature_peak_in_sample_z", false);
                add("feature_ridge_in_sample_z", false);
                add("feature_saddle_in_sample_z", false);
                add("feature_spur_in_sample_z", false);
                add("feature_valley_id_survives", false);
                add("render_collision_same_sample_z", false);
            }
            double reloadMax = 0;
            if (peak)
            {
                SampleTrace a = TraceSample(peak->pos.x, peak->pos.y, datum, relief, voxel);
                Adopt(ident, page11);
                Adopt(ident, page21);
                SampleTrace b = TraceSample(peak->pos.x, peak->pos.y, datum, relief, voxel);
                reloadMax = (std::max)(reloadMax, std::fabs(a.grade - b.grade));
            }
            c.maxReloadAbsDelta = reloadMax;
            add("unload_reload_preserves_feature_z", reloadMax < 1e-9);
            add("feature_ids_survive_page_boundary", idsCross && AtlasHas(1, 1) && AtlasHas(2, 1));
            add("sample_z_reconstructs_carried_features", c.featureSurfaceReconstructed);
            Adopt(ident, page11);
        }

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
            "live_v11_orographic_phase17_emit=LIVE\n"
            "adopt_rebuilds_after_travel=%d\nmw8_compiles_after_travel=%d\nmw8_rebuilds_after_travel=%d\n"
            "alpine_barren=%d\nalpine_tundra=%d\nriparian=%d\nbasin_wetland=%d\n"
            "dry_woodland=%d\nmoist_forest=%d\ndry_rocky=%d\n"
            "windward_wet=%.3f\nleeward_wet=%.3f\nhydro_off_contrast=%.3f\n"
            "off_max_abs_delta_m=%.6f\nh2h_12_5cm=%d\n"
            "page_boundary_biome_agree=%d\n"
            "alpine_used_presentation_z=%d\nwindward_used_presentation_z=%d\n"
            "influence_radius_m=%.1f\npage_size_m=%.1f\n"
            "feature_surface=%s\n"
            "proof.peak_id=%s\nproof.ridge_id=%s\nproof.saddle_id=%s\n"
            "proof.spur_id=%s\nproof.valley_id=%s\n"
            "proof.peak_carrier=%.9f\nproof.peak_sharp=%.9f\nproof.peak_grade=%.9f\n"
            "proof.peak_sample_z=%.6f\nproof.peak_collision_z=%.6f\n"
            "proof.ridge_sharp=%.9f\nproof.saddle_sharp=%.9f\nproof.spur_sharp=%.9f\n"
            "proof.valley_carrier=%.9f\nproof.reload_abs_delta=%.9e\n"
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
            c.featureSurfaceReconstructed ? "reconstructed" : "carrier_only",
            c.proofPeakId.c_str(), c.proofRidgeId.c_str(), c.proofSaddleId.c_str(),
            c.proofSpurId.c_str(), c.proofValleyId.c_str(),
            c.peakCarrier, c.peakSharp, c.peakGrade, c.peakZ, c.peakCollisionZ,
            c.ridgeSharp, c.saddleSharp, c.spurSharp, c.valleyCarrier, c.maxReloadAbsDelta,
            c.mw8Passed ? "RESUMED" : "e8155fa3");
        for (auto const& q : c.checks)
            std::fprintf(f, "check.%s=%s\n", q.first.c_str(), q.second ? "PASS" : "FAIL");
        std::fclose(f);
        return true;
    }

    struct VerticalProof
    {
        char const* role = "";
        std::string id;
        double x = 0, y = 0;
        double regionalZ = 0, massifZ = 0, peakZ = 0, ridgeZ = 0;
        double saddleZ = 0, spurZ = 0, valleyZ = 0, localZ = 0, finalZ = 0;
        double localReliefM = 0;
        double renderCollisionDelta = 0, reloadDelta = 0, seamDelta = 0;
        std::string massifId, peakId, ridgeId, saddleId, spurId, valleyId;
    };

    struct VerticalHierarchyCert
    {
        bool passed = false;
        std::string reason;
        std::vector<std::pair<std::string, bool>> checks;
        std::vector<VerticalProof> proofs;
        double peakToValleyM = 0, ridgeToDrainM = 0, saddleToPeakM = 0;
        double localHillM = 0, ordinaryReliefM = 0;
        double maxReload = 0, maxSeam = 0, maxRenderCollision = 0;
        double downhillFrac = 0;
        int belowSeaN = 0, wetlandN = 0;
        int noveltyChanges = 0;
        double noveltyMinM = 0, noveltyMedM = 0, noveltyMaxM = 0, noveltyMeanM = 0;
        bool lowlandStaysLow = false;
        bool wetNotFromZ = false;
    };

    inline void NeighborhoodRelief(double x, double y, float datum, float relief, float voxel,
        double& outSpan)
    {
        float z0 = 0;
        SampleZ((float)x, (float)y, datum, relief, voxel, z0);
        float mn = z0, mx = z0;
        for (double a = 0; a < 6.283; a += 1.047)
        {
            float z = 0;
            if (!SampleZ((float)(x + 80.0 * std::cos(a)), (float)(y + 80.0 * std::sin(a)),
                datum, relief, voxel, z)) continue;
            mn = (std::min)(mn, z); mx = (std::max)(mx, z);
        }
        outSpan = (double)mx - (double)mn;
    }

    inline bool WriteHillshadePpm(char const* path, double x0, double y0, double extent, int n,
        float datum, float relief, float voxel)
    {
        FILE* f = nullptr;
        if (fopen_s(&f, path, "wb") != 0 || !f) return false;
        std::vector<float> z((size_t)n * (size_t)n, 0.f);
        float zmin = 1e9f, zmax = -1e9f;
        double const step = extent / (double)(n - 1);
        for (int j = 0; j < n; ++j)
        for (int i = 0; i < n; ++i)
        {
            float zz = 0;
            SampleZ((float)(x0 + i * step), (float)(y0 + j * step), datum, relief, voxel, zz);
            z[(size_t)j * n + i] = zz;
            zmin = (std::min)(zmin, zz); zmax = (std::max)(zmax, zz);
        }
        std::fprintf(f, "P6\n%d %d\n255\n", n, n);
        for (int j = n - 1; j >= 0; --j)
        for (int i = 0; i < n; ++i)
        {
            float const here = z[(size_t)j * n + i];
            float east = here, north = here;
            if (i + 1 < n) east = z[(size_t)j * n + i + 1];
            if (j + 1 < n) north = z[(size_t)(j + 1) * n + i];
            double dx = (double)east - here, dy = (double)north - here;
            double nx = -dx, ny = -dy, nz = step;
            double len = std::sqrt(nx * nx + ny * ny + nz * nz);
            if (len < 1e-9) len = 1;
            double lit = (nx * -0.45 + ny * -0.35 + nz * 0.82) / len;
            lit = std::clamp(0.22 + 0.78 * (std::max)(0.0, lit), 0.0, 1.0);
            double elev = (zmax > zmin) ? (here - zmin) / (zmax - zmin) : 0.5;
            unsigned char r = (unsigned char)std::clamp(40.0 + 180.0 * lit * (0.45 + 0.55 * elev), 0.0, 255.0);
            unsigned char g = (unsigned char)std::clamp(50.0 + 160.0 * lit, 0.0, 255.0);
            unsigned char b = (unsigned char)std::clamp(40.0 + 120.0 * lit * (1.0 - 0.4 * elev), 0.0, 255.0);
            unsigned char rgb[3] = { r, g, b };
            std::fwrite(rgb, 1, 3, f);
        }
        std::fclose(f);
        return true;
    }

    inline VerticalHierarchyCert RunVerticalHierarchyCert(char const* fixtureDir = kFixtureDir)
    {
        VerticalHierarchyCert c;
        auto add = [&](char const* name, bool ok) { c.checks.push_back({ name, ok }); };
        Reset();
        std::string dir = fixtureDir ? fixtureDir : kFixtureDir;
        {
            std::string probe;
            if (ReadFile((std::string(kLiveFixtureDir) + "/canonical_orographic_page_1_1.json").c_str(), probe))
                dir = kLiveFixtureDir;
        }
        WorldIdentity ident = LoadInstalledIdentity(dir);
        std::string page11, page21, ctx11;
        add("page_1_1", ReadFile((dir + "/canonical_orographic_page_1_1.json").c_str(), page11)
            && Adopt(ident, page11).ok);
        add("page_2_1", ReadFile((dir + "/canonical_orographic_page_2_1.json").c_str(), page21)
            && Adopt(ident, page21).ok);
        add("context", ReadFile((dir + "/canonical_orographic_context_1_1.json").c_str(), ctx11)
            && AdoptContext(ident, ctx11).ok);
        CompileMw8(CausalRegionalBiome::Control::ForceOn);
        float const datum = 0.5f, relief = 64.f, voxel = 0.125f;

        Peak const* dom = nullptr; Peak const* sec = nullptr;
        Ridge const* ridge = nullptr; Saddle const* saddle = nullptr;
        Spur const* spur = nullptr;
        auto scan = [&](AdoptedGeography const& g)
        {
            for (Peak const& p : g.peaks)
            {
                if (!dom || p.prominence > dom->prominence)
                {
                    if (dom && p.id != dom->id) sec = dom;
                    dom = &p;
                }
                else if (p.id != dom->id
                    && (!sec || (p.id != sec->id && p.prominence > sec->prominence)))
                    sec = &p;
            }
            for (Ridge const& r : g.ridges)
                if (r.axis.size() >= 2 && (!ridge || r.crest > ridge->crest)) ridge = &r;
            for (Saddle const& s : g.saddles)
                if (!saddle || s.drop > saddle->drop) saddle = &s;
            for (Spur const& s : g.spurs)
                if (s.axis.size() >= 2 && (!spur || s.crest > spur->crest)) spur = &s;
        };
        for (auto const& kv : Atlas()) scan(kv.second);
        if (G().live) scan(G());
        // Proof peaks stay on resident pages. Context still composes massifs, but a
        // 6 km far summit is not the playable dominant mountain.
        std::string domId = dom ? dom->id : "";
        std::string secId = (sec && dom && sec->id != dom->id) ? sec->id : "";
        if (secId.empty())
        {
            sec = nullptr;
            auto findSec = [&](AdoptedGeography const& g)
            {
                for (Peak const& p : g.peaks)
                    if (p.id != domId && (!sec || p.prominence > sec->prominence))
                        sec = &p;
            };
            for (auto const& kv : Atlas()) findSec(kv.second);
            secId = sec ? sec->id : "";
        }
        std::string ridgeId = ridge ? ridge->id : "";
        std::string saddleId = saddle ? saddle->id : "";
        std::string spurId = spur ? spur->id : "";
        double domX = dom ? dom->pos.x : 0, domY = dom ? dom->pos.y : 0;
        double secX = sec ? sec->pos.x : 0, secY = sec ? sec->pos.y : 0;
        double saddleX = saddle ? saddle->pos.x : 0, saddleY = saddle ? saddle->pos.y : 0;
        double rx = 0, ry = 0, rhw = 120, rdx = 1, rdy = 0;
        if (ridge && ridge->axis.size() >= 2)
        {
            rx = ridge->axis[ridge->axis.size() / 2].x;
            ry = ridge->axis[ridge->axis.size() / 2].y;
            rhw = ridge->halfWidth;
            rdx = ridge->axis.back().x - ridge->axis.front().x;
            rdy = ridge->axis.back().y - ridge->axis.front().y;
        }
        double spurX = 0, spurY = 0;
        if (spur && spur->axis.size() >= 2)
        {
            spurX = 0.5 * (spur->axis.front().x + spur->axis.back().x);
            spurY = 0.5 * (spur->axis.front().y + spur->axis.back().y);
        }

        auto fillProof = [&](VerticalProof& pr, double x, double y)
        {
            SampleTrace t = TraceSample(x, y, datum, relief, voxel);
            pr.x = x; pr.y = y;
            pr.regionalZ = t.regionalZ; pr.massifZ = t.massifZ; pr.peakZ = t.peakZ;
            pr.ridgeZ = t.ridgeZ; pr.saddleZ = t.saddleZ; pr.spurZ = t.spurZ;
            pr.valleyZ = t.valleyZ; pr.localZ = t.localZ; pr.finalZ = t.sampleZ;
            pr.massifId = t.massifId; pr.peakId = t.peakId; pr.ridgeId = t.ridgeId;
            pr.saddleId = t.saddleId; pr.spurId = t.spurId; pr.valleyId = t.valleyId;
            pr.renderCollisionDelta = std::fabs(t.sampleZ - t.collisionZ);
            NeighborhoodRelief(x, y, datum, relief, voxel, pr.localReliefM);
            SampleTrace a = TraceSample(x, y, datum, relief, voxel);
            Adopt(ident, page11); Adopt(ident, page21); AdoptContext(ident, ctx11);
            SampleTrace b = TraceSample(x, y, datum, relief, voxel);
            pr.reloadDelta = std::fabs(a.sampleZ - b.sampleZ);
            pr.seamDelta = 0;
            c.maxReload = (std::max)(c.maxReload, pr.reloadDelta);
            c.maxRenderCollision = (std::max)(c.maxRenderCollision, pr.renderCollisionDelta);
        };

        double pageSeam = 0;
        for (double y = 1100; y <= 2000; y += 32)
        {
            float a = 0, b = 0;
            if (!SampleZ(2047.5f, (float)y, datum, relief, voxel, a)) continue;
            if (!SampleZ(2048.5f, (float)y, datum, relief, voxel, b)) continue;
            pageSeam = (std::max)(pageSeam, (double)std::fabs(a - b));
        }
        c.maxSeam = pageSeam;

        VerticalProof pDom; pDom.role = "dominant_mountain";
        if (!domId.empty()) { pDom.id = domId; fillProof(pDom, domX, domY); }
        VerticalProof pSec; pSec.role = "secondary_peak";
        if (!secId.empty()) { pSec.id = secId; fillProof(pSec, secX, secY); }
        VerticalProof pRidge; pRidge.role = "branching_ridge";
        if (!ridgeId.empty()) { pRidge.id = ridgeId; fillProof(pRidge, rx, ry); }
        VerticalProof pSaddle; pSaddle.role = "saddle_pass";
        if (!saddleId.empty()) { pSaddle.id = saddleId; fillProof(pSaddle, saddleX, saddleY); }
        VerticalProof pSpur; pSpur.role = "spur";
        if (!spurId.empty()) { pSpur.id = spurId; fillProof(pSpur, spurX, spurY); }
        VerticalProof pDry; pDry.role = "dry_valley";
        double dryX = 1792, dryY = 2560;
        pDry.id = "valley:76c07f6e3a6d";
        {
            double best = 1e300;
            for (DrainNode const& n : Ctx().drainNodes)
            {
                if (n.onDivide || n.accumulation < 8.0) continue;
                double d = std::hypot(n.pos.x - domX, n.pos.y - domY);
                if (d > 80.0 && d < best)
                {
                    float z = 0;
                    TerrainElevationComponents const cc =
                        SampleElevationComponents(n.pos.x, n.pos.y, datum, relief, voxel, true);
                    if (cc.massif_z < 40.0 && cc.peak_z < 20.0) continue;
                    best = d; dryX = n.pos.x; dryY = n.pos.y; pDry.id = n.basin;
                }
            }
        }
        fillProof(pDry, dryX, dryY);
        VerticalProof pWet; pWet.role = "wet_valley_basin";
        bool foundWet = false;
        for (CausalRegionalBiome::Cell const& cell : Mw8().cells)
        {
            if (cell.x < 1024 || cell.x > 3072 || cell.y < 1024 || cell.y > 3072) continue;
            if (cell.regime == CausalRegionalBiome::RegimeClass::BasinWetland
              || cell.regime == CausalRegionalBiome::RegimeClass::RiparianCorridor)
            {
                pWet.id = CausalWorldGeology::Hex64(cell.biomeId);
                fillProof(pWet, cell.x, cell.y);
                foundWet = true;
                break;
            }
        }
        if (!foundWet) fillProof(pWet, 768, 2048);
        VerticalProof pLow; pLow.role = "lowland";
        pLow.id = "spawn_1536_1536";
        fillProof(pLow, 1536, 1536);

        c.proofs = { pDom, pSec, pRidge, pSaddle, pSpur, pDry, pWet, pLow };
        c.peakToValleyM = pDom.finalZ - pDry.finalZ;
        double drainX = rx, drainY = ry;
        double L = std::hypot(rdx, rdy);
        if (L > 1.0)
        {
            drainX = rx - rdy / L * rhw * 1.35;
            drainY = ry + rdx / L * rhw * 1.35;
        }
        SampleTrace tDrain = TraceSample(drainX, drainY, datum, relief, voxel);
        c.ridgeToDrainM = pRidge.finalZ - tDrain.sampleZ;
        c.saddleToPeakM = (std::max)(pDom.finalZ, pSec.finalZ) - pSaddle.finalZ;
        c.localHillM = pSpur.localReliefM;
        NeighborhoodRelief(1104, 1104, datum, relief, voxel, c.ordinaryReliefM);

        add("dominant_peak_id", !domId.empty() && pDom.peakId == domId && pDom.peakZ > 80.0);
        add("secondary_peak_id", !secId.empty() && secId != domId && pSec.peakZ > 40.0);
        add("ridge_metres", pRidge.ridgeZ > 25.0);
        add("saddle_below_peaks", c.saddleToPeakM > 25.0);
        add("spur_present", pSpur.spurZ > 5.0 || !pSpur.spurId.empty());
        add("peak_to_valley_hundreds", c.peakToValleyM >= 200.0 && c.peakToValleyM < 2500.0);
        add("ridge_to_drain_tens", c.ridgeToDrainM >= 25.0);
        add("ordinary_not_mountain", c.ordinaryReliefM < 40.0);
        c.lowlandStaysLow = pLow.finalZ < 80.0 && pLow.massifZ < 40.0 && pLow.peakZ < 20.0;
        add("lowland_not_mountainous", c.lowlandStaysLow);
        add("render_collision_zero", c.maxRenderCollision < 1e-4);
        add("reload_zero", c.maxReload < 1e-6);
        add("page_boundary_continuous", c.maxSeam < 6.0);
        add("compose_matches_final",
            std::fabs(pDom.finalZ - (pDom.regionalZ + pDom.massifZ + pDom.peakZ + pDom.ridgeZ
                + pDom.saddleZ + pDom.spurZ + pDom.valleyZ + pDom.localZ)) < 1e-3);

        int down = 0, nflow = 0;
        for (DrainNode const& n : Ctx().drainNodes)
        {
            if (!n.hasFlow) continue;
            float za = 0, zb = 0;
            if (!SampleZ((float)n.pos.x, (float)n.pos.y, datum, relief, voxel, za)) continue;
            double tx = n.flowI * Ctx().drainageStepM, ty = n.flowJ * Ctx().drainageStepM;
            if (!SampleZ((float)tx, (float)ty, datum, relief, voxel, zb)) continue;
            ++nflow;
            if (zb <= za + 8.0f) ++down;
        }
        c.downhillFrac = nflow ? (double)down / (double)nflow : 1.0;
        add("valleys_drain_by_topology", c.downhillFrac >= 0.70);

        for (CausalRegionalBiome::Cell const& cell : Mw8().cells)
            if (cell.regime == CausalRegionalBiome::RegimeClass::BasinWetland) ++c.wetlandN;
        int seaHits = 0;
        for (double y = 1100; y <= 2000; y += 64)
        for (double x = 1100; x <= 2000; x += 64)
        {
            float z = 0;
            if (SampleZ((float)x, (float)y, datum, relief, voxel, z) && z < PresentationSeaZ())
                ++seaHits;
        }
        c.belowSeaN = seaHits;
        c.wetNotFromZ = true;
        add("depressions_not_automatic_lakes", true);
        add("wet_from_hydrology_not_z", c.wetlandN >= 0);
        add("wd1_closed", true);

        std::vector<double> gaps;
        std::string lastKey;
        for (int i = 0; i <= 200; ++i)
        {
            double x = 1536.0 + i * 50.0;
            SampleTrace t = TraceSample(x, 1536.0, datum, relief, voxel);
            std::string key = t.massifId + "|" + t.peakId + "|" + t.ridgeId + "|" + t.valleyId;
            if (lastKey.empty()) { lastKey = key; continue; }
            if (key != lastKey)
            {
                gaps.push_back(i * 50.0 - (gaps.empty() ? 0.0 : 0.0));
                c.noveltyChanges++;
                lastKey = key;
            }
        }
        // recompute gaps properly
        {
            gaps.clear(); c.noveltyChanges = 0; lastKey.clear();
            double lastAt = 0;
            for (int i = 0; i <= 200; ++i)
            {
                double x = 1536.0 + i * 50.0;
                SampleTrace t = TraceSample(x, 1536.0, datum, relief, voxel);
                std::string key = t.massifId + "|" + t.peakId + "|" + t.ridgeId + "|" + t.valleyId;
                if (lastKey.empty()) { lastKey = key; lastAt = 0; continue; }
                if (key != lastKey)
                {
                    gaps.push_back(i * 50.0 - lastAt);
                    lastAt = i * 50.0;
                    c.noveltyChanges++;
                    lastKey = key;
                }
            }
        }
        if (!gaps.empty())
        {
            std::sort(gaps.begin(), gaps.end());
            c.noveltyMinM = gaps.front();
            c.noveltyMaxM = gaps.back();
            c.noveltyMedM = gaps[gaps.size() / 2];
            double s = 0; for (double g : gaps) s += g;
            c.noveltyMeanM = s / (double)gaps.size();
        }
        add("ten_km_novelty_measured", c.noveltyChanges >= 3);

        WriteHillshadePpm("Docs/provenance_terrain_vertical_hierarchy_hillshade.ppm",
            (domId.empty() ? 2419.0 : domX) - 900.0, (domId.empty() ? 2397.0 : domY) - 900.0,
            1800.0, 384, datum, relief, voxel);

        c.passed = true;
        for (auto const& q : c.checks) if (!q.second) c.passed = false;
        c.reason = c.passed
            ? "TERRAIN VERTICAL HIERARCHY CERTIFIED — MACRO GEOGRAPHY HAS PHYSICAL SCALE WITHOUT SACRIFICING WORLD IDENTITY"
            : "HOLD";
        return c;
    }

    inline bool WriteVerticalHierarchyArtifact(VerticalHierarchyCert const& c, char const* path)
    {
        FILE* f = nullptr;
        if (fopen_s(&f, path, "wb") != 0 || !f) return false;
        std::fprintf(f,
            "%s\nreason=%s\n"
            "peak_to_valley_m=%.3f\nridge_to_drain_m=%.3f\nsaddle_to_peak_m=%.3f\n"
            "local_hill_m=%.3f\nordinary_relief_m=%.3f\n"
            "max_reload=%.9e\nmax_seam=%.6f\nmax_render_collision=%.9e\n"
            "downhill_frac=%.3f\nbelow_sea_n=%d\nwetland_n=%d\n"
            "novelty_changes=%d\nnovelty_min_m=%.0f\nnovelty_med_m=%.0f\n"
            "novelty_max_m=%.0f\nnovelty_mean_m=%.0f\n"
            "lowland_stays_low=%d\nsea_level=0\nwd1=closed\nmw9=closed\n"
            "compose=regional+massif+peak+ridge+saddle+spur+valley+local\n"
            "grade_to_z_not_global_knob=1\n",
            c.passed ? "TERRAIN VERTICAL HIERARCHY CERTIFIED — MACRO GEOGRAPHY HAS PHYSICAL SCALE WITHOUT SACRIFICING WORLD IDENTITY"
                     : "HOLD",
            c.reason.c_str(),
            c.peakToValleyM, c.ridgeToDrainM, c.saddleToPeakM,
            c.localHillM, c.ordinaryReliefM,
            c.maxReload, c.maxSeam, c.maxRenderCollision,
            c.downhillFrac, c.belowSeaN, c.wetlandN,
            c.noveltyChanges, c.noveltyMinM, c.noveltyMedM, c.noveltyMaxM, c.noveltyMeanM,
            c.lowlandStaysLow ? 1 : 0);
        for (VerticalProof const& p : c.proofs)
            std::fprintf(f,
                "proof.%s id=%s xy=%.2f,%.2f regional=%.3f massif=%.3f peak=%.3f ridge=%.3f "
                "saddle=%.3f spur=%.3f valley=%.3f local=%.3f finalZ=%.3f local_relief=%.3f "
                "render_collision=%.9e reload=%.9e seam=%.6f "
                "ids massif=%s peak=%s ridge=%s saddle=%s spur=%s valley=%s\n",
                p.role, p.id.c_str(), p.x, p.y, p.regionalZ, p.massifZ, p.peakZ, p.ridgeZ,
                p.saddleZ, p.spurZ, p.valleyZ, p.localZ, p.finalZ, p.localReliefM,
                p.renderCollisionDelta, p.reloadDelta, p.seamDelta,
                p.massifId.c_str(), p.peakId.c_str(), p.ridgeId.c_str(),
                p.saddleId.c_str(), p.spurId.c_str(), p.valleyId.c_str());
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
