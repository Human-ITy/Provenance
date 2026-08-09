// ProvenanceClient — P3b dual contour / Hermite / QEF cavity extractor.
//
// fill samples (+ optional halo)
//   → sign-changing primal lattice EDGES at iso
//   → Hermite (interpolated position + gradient normal)
//   → one QEF vertex per active dual cell (mass-point fallback)
//   → for each sign-changing edge: 4 incident dual verts → one quad
//   → canonical owner = lex-min dual-cell of those four (only owner emits)
//
// Optional focus sphere: extract cavity air OR open skin near the dig mouth.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace DualContourQef
{
    struct Vec3
    {
        float x = 0.f, y = 0.f, z = 0.f;
        Vec3() = default;
        Vec3( float X, float Y, float Z ) : x( X ), y( Y ), z( Z ) {}
        Vec3 operator+( Vec3 const& o ) const { return { x + o.x, y + o.y, z + o.z }; }
        Vec3 operator-( Vec3 const& o ) const { return { x - o.x, y - o.y, z - o.z }; }
        Vec3 operator*( float s ) const { return { x * s, y * s, z * s }; }
    };

    inline float Dot( Vec3 const& a, Vec3 const& b ) { return a.x * b.x + a.y * b.y + a.z * b.z; }
    inline float Len( Vec3 const& v ) { return std::sqrt( Dot( v, v ) ); }
    inline Vec3 Norm( Vec3 v )
    {
        float const L = Len( v );
        return ( L > 1e-8f ) ? v * ( 1.f / L ) : Vec3{ 0.f, 0.f, 1.f };
    }
    inline bool Finite( Vec3 const& v )
    {
        return std::isfinite( v.x ) && std::isfinite( v.y ) && std::isfinite( v.z );
    }

    struct Tri { Vec3 a, b, c; };

    struct ExtractStats
    {
        int tris = 0;
        int edgesEmitted = 0;
        int haloMissing = 0;
        int gradDegraded = 0;
        int zeroHermiteSkip = 0;
        int rejectedLongEdge = 0;
        int qefMassFallback = 0;
        // Primal X/Y edges that cross this column's +max face (world-cell seam).
        int boundaryEdgesEmitted = 0;
    };

    inline void AccumulateStats( ExtractStats& dst, ExtractStats const& src )
    {
        dst.tris += src.tris;
        dst.edgesEmitted += src.edgesEmitted;
        dst.haloMissing += src.haloMissing;
        dst.gradDegraded += src.gradDegraded;
        dst.zeroHermiteSkip += src.zeroHermiteSkip;
        dst.rejectedLongEdge += src.rejectedLongEdge;
        dst.qefMassFallback += src.qefMassFallback;
        dst.boundaryEdgesEmitted += src.boundaryEdgesEmitted;
    }

    struct IFillField
    {
        virtual ~IFillField() = default;
        // Lattice indices relative to home column origin (may be outside 0..w-1 for halo).
        virtual bool TrySample( int c, int r, int k, int& outFill ) const = 0;
    };

    inline Vec3 LatticeWorld( int c, int r, int k,
        float originX, float originY, float crestZ,
        float du, float dv, float edge, int kz )
    {
        return {
            originX + ( (float)c + 0.5f ) * du,
            originY + ( (float)r + 0.5f ) * dv,
            crestZ - ( (float)( kz - 1 - k ) + 0.5f ) * edge
        };
    }

    // Density = fill - iso. Unknown samples return NaN — never invent solid or air.
    // (Legacy bug: returning 0.f made Solid(dens)>=0 treat missing as solid/at-ISO.)
    inline float SampleDensity( IFillField const& field, int c, int r, int k, int iso, ExtractStats& st )
    {
        int fill = 0;
        if ( !field.TrySample( c, r, k, fill ) )
        {
            ++st.haloMissing;
            return std::numeric_limits<float>::quiet_NaN();
        }
        return (float)fill - (float)iso;
    }

    inline bool IsUnknown( float dens ) { return !std::isfinite( dens ); }
    inline bool Solid( float dens ) { return dens >= 0.f; } // only valid when !IsUnknown(dens)

    struct Hermite { Vec3 p, n; };

    inline bool EdgeHermite( IFillField const& field,
        int c0, int r0, int k0, int c1, int r1, int k1,
        float originX, float originY, float crestZ,
        float du, float dv, float edge, int kz, int iso,
        Hermite& out, ExtractStats& st )
    {
        float const d0 = SampleDensity( field, c0, r0, k0, iso, st );
        float const d1 = SampleDensity( field, c1, r1, k1, iso, st );
        if ( IsUnknown( d0 ) || IsUnknown( d1 ) ) { return false; }
        if ( Solid( d0 ) == Solid( d1 ) ) { return false; }
        float t = 0.5f;
        float const den = d0 - d1;
        if ( std::fabs( den ) > 1e-6f ) { t = d0 / den; }
        t = std::clamp( t, 0.02f, 0.98f );
        Vec3 const p0 = LatticeWorld( c0, r0, k0, originX, originY, crestZ, du, dv, edge, kz );
        Vec3 const p1 = LatticeWorld( c1, r1, k1, originX, originY, crestZ, du, dv, edge, kz );
        out.p = p0 + ( p1 - p0 ) * t;

        // Central-difference gradient on density (points toward air / out of solid).
        // Unknown neighbor samples degrade to edge direction — do not invent density.
        auto dens = [&]( int c, int r, int k ) { return SampleDensity( field, c, r, k, iso, st ); };
        int const cm = ( c0 + c1 ) / 2, rm = ( r0 + r1 ) / 2, km = ( k0 + k1 ) / 2;
        float const gx0 = dens( cm + 1, rm, km ), gx1 = dens( cm - 1, rm, km );
        float const gy0 = dens( cm, rm + 1, km ), gy1 = dens( cm, rm - 1, km );
        float const gz0 = dens( cm, rm, km + 1 ), gz1 = dens( cm, rm, km - 1 );
        Vec3 g{ 0.f, 0.f, 0.f };
        bool gradOk = true;
        if ( IsUnknown( gx0 ) || IsUnknown( gx1 ) ) { gradOk = false; }
        else { g.x = gx0 - gx1; }
        if ( IsUnknown( gy0 ) || IsUnknown( gy1 ) ) { gradOk = false; }
        else { g.y = gy0 - gy1; }
        if ( IsUnknown( gz0 ) || IsUnknown( gz1 ) ) { gradOk = false; }
        else { g.z = gz0 - gz1; }
        if ( !gradOk || Len( g ) < 1e-5f )
        {
            ++st.gradDegraded;
            g = p1 - p0;
            if ( Solid( d0 ) ) { g = g * -1.f; } // from solid toward air along edge
        }
        out.n = Norm( g );
        return Finite( out.p ) && Finite( out.n );
    }

    inline bool SolveQef( Hermite const* H, int nH, Vec3 const& cellMin, Vec3 const& cellMax,
        Vec3& outP, ExtractStats& stats )
    {
        if ( nH <= 0 )
        {
            ++stats.zeroHermiteSkip;
            return false;
        }
        Vec3 mass{};
        int nMass = 0;
        for ( int i = 0; i < nH; ++i )
        {
            if ( !Finite( H[i].p ) || !Finite( H[i].n ) ) { continue; }
            mass = mass + H[i].p;
            ++nMass;
        }
        if ( nMass <= 0 )
        {
            ++stats.zeroHermiteSkip;
            return false;
        }
        mass = mass * ( 1.f / (float)nMass );

        // 3×3 ATA / ATb accumulate (plane constraints n·(x-p)=0).
        float ATA[9] = {};
        float ATb[3] = {};
        for ( int i = 0; i < nH; ++i )
        {
            if ( !Finite( H[i].p ) || !Finite( H[i].n ) ) { continue; }
            float const nx = H[i].n.x, ny = H[i].n.y, nz = H[i].n.z;
            float const d = Dot( H[i].n, H[i].p );
            ATA[0] += nx * nx; ATA[1] += nx * ny; ATA[2] += nx * nz;
            ATA[3] += ny * nx; ATA[4] += ny * ny; ATA[5] += ny * nz;
            ATA[6] += nz * nx; ATA[7] += nz * ny; ATA[8] += nz * nz;
            ATb[0] += nx * d; ATb[1] += ny * d; ATb[2] += nz * d;
        }
        // Tiny ridge toward mass point for stability.
        constexpr float kRidge = 1e-3f;
        ATA[0] += kRidge; ATA[4] += kRidge; ATA[8] += kRidge;
        ATb[0] += kRidge * mass.x; ATb[1] += kRidge * mass.y; ATb[2] += kRidge * mass.z;

        // Cramer's / direct 3×3 solve.
        auto det3 = []( float a00, float a01, float a02,
            float a10, float a11, float a12,
            float a20, float a21, float a22 )
        {
            return a00 * ( a11 * a22 - a12 * a21 )
                 - a01 * ( a10 * a22 - a12 * a20 )
                 + a02 * ( a10 * a21 - a11 * a20 );
        };
        float const D = det3( ATA[0], ATA[1], ATA[2], ATA[3], ATA[4], ATA[5], ATA[6], ATA[7], ATA[8] );
        Vec3 solved = mass;
        if ( std::fabs( D ) > 1e-8f )
        {
            float const Dx = det3( ATb[0], ATA[1], ATA[2], ATb[1], ATA[4], ATA[5], ATb[2], ATA[7], ATA[8] );
            float const Dy = det3( ATA[0], ATb[0], ATA[2], ATA[3], ATb[1], ATA[5], ATA[6], ATb[2], ATA[8] );
            float const Dz = det3( ATA[0], ATA[1], ATb[0], ATA[3], ATA[4], ATb[1], ATA[6], ATA[7], ATb[2] );
            solved = { Dx / D, Dy / D, Dz / D };
        }
        else
        {
            ++stats.qefMassFallback;
        }
        if ( !Finite( solved ) )
        {
            ++stats.qefMassFallback;
            solved = mass;
        }
        // Clamp to dual-cell bounds expanded slightly.
        float const pad = 0.02f;
        solved.x = std::clamp( solved.x, cellMin.x - pad, cellMax.x + pad );
        solved.y = std::clamp( solved.y, cellMin.y - pad, cellMax.y + pad );
        solved.z = std::clamp( solved.z, cellMin.z - pad, cellMax.z + pad );
        // Reject runaway vs mass.
        if ( Len( solved - mass ) > Len( cellMax - cellMin ) * 1.5f + 0.05f )
        {
            ++stats.qefMassFallback;
            solved = mass;
        }
        outP = solved;
        return true;
    }

    inline void EmitQuad( Vec3 const& v0, Vec3 const& v1, Vec3 const& v2, Vec3 const& v3,
        Vec3 const& towardAir, float maxEdge, ExtractStats& stats, std::vector<Tri>& out )
    {
        if ( !Finite( v0 ) || !Finite( v1 ) || !Finite( v2 ) || !Finite( v3 ) )
        {
            ++stats.rejectedLongEdge;
            return;
        }
        auto tooLong = [&]( Vec3 const& a, Vec3 const& b )
        {
            return Len( a - b ) > maxEdge;
        };
        if ( tooLong( v0, v1 ) || tooLong( v1, v3 ) || tooLong( v3, v2 ) || tooLong( v2, v0 )
          || tooLong( v0, v3 ) || tooLong( v1, v2 ) )
        {
            ++stats.rejectedLongEdge;
            return;
        }
        ++stats.edgesEmitted;

        auto emit = [&]( Vec3 a, Vec3 b, Vec3 c )
        {
            Vec3 const e0 = b - a, e1 = c - a;
            Vec3 n{ e0.y * e1.z - e0.z * e1.y, e0.z * e1.x - e0.x * e1.z, e0.x * e1.y - e0.y * e1.x };
            if ( Dot( n, towardAir ) < 0.f ) { std::swap( b, c ); }
            out.push_back( { a, b, c } );
            ++stats.tris;
        };
        emit( v0, v1, v2 );
        emit( v0, v2, v3 );
    }

    inline bool InFocus( Vec3 const& p, float fx, float fy, float fz, float fr )
    {
        if ( fr <= 0.f ) { return true; }
        float const dx = p.x - fx, dy = p.y - fy, dz = p.z - fz;
        return ( dx * dx + dy * dy + dz * dz ) <= fr * fr;
    }

    // Extract isosurface for one column lattice. focusRM<=0 → whole column (rare).
    // With focus: include dual cells whose centre is in focus OR that have cavity air in focus.
    inline ExtractStats ExtractCell(
        IFillField const& field,
        int w, int h, int kz,
        float originX, float originY, float crestZ,
        float edge, int iso,
        std::vector<Tri>& outTris,
        float focusX = 0.f, float focusY = 0.f, float focusZ = 0.f, float focusRM = 0.f )
    {
        ExtractStats stats{};
        if ( w <= 1 || h <= 1 || kz <= 1 || edge <= 1e-5f ) { return stats; }
        float const du = 1.f / (float)w;
        float const dv = 1.f / (float)h;
        float maxEdge = edge * 2.25f; // tighter — long dual edges = SPIRE-scale triangle strips

        // Dual-cell feature verts: index (c,r,k) for c in [0,w-1), etc. — corners of primal.
        // We store features for dual cells c∈[-1,w], etc. sparse via hash-less dense grid with sentinel.
        int const gw = w + 2, gh = h + 2, gk = kz + 2;
        auto idx = [&]( int c, int r, int k ) {
            return ( ( k + 1 ) * gh + ( r + 1 ) ) * gw + ( c + 1 );
        };
        std::vector<uint8_t> hasVert( (size_t)gw * gh * gk, 0 );
        std::vector<Vec3> vert( (size_t)gw * gh * gk );

        auto dens = [&]( int c, int r, int k ) {
            return SampleDensity( field, c, r, k, iso, stats );
        };

        // Build feature verts for dual cells that own at least one crossing edge.
        for ( int k = -1; k < kz; ++k )
        {
            for ( int r = -1; r < h; ++r )
            {
                for ( int c = -1; c < w; ++c )
                {
                    Hermite H[12];
                    int nH = 0;
                    // 12 edges of the dual cell cube from (c,r,k) to (c+1,r+1,k+1) in primal corner space.
                    // Primal samples at integer corners of this dual cell.
                    int const cc[8] = { c, c + 1, c + 1, c, c, c + 1, c + 1, c };
                    int const rr[8] = { r, r, r + 1, r + 1, r, r, r + 1, r + 1 };
                    int const kk[8] = { k, k, k, k, k + 1, k + 1, k + 1, k + 1 };
                    static int const edges[12][2] = {
                        { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },
                        { 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 },
                        { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
                    };
                    Vec3 cellCentre = LatticeWorld( c, r, k, originX, originY, crestZ, du, dv, edge, kz );
                    // Approximate dual centre.
                    Vec3 cellMax = LatticeWorld( c + 1, r + 1, k + 1, originX, originY, crestZ, du, dv, edge, kz );
                    Vec3 mid = ( cellCentre + cellMax ) * 0.5f;

                    bool anyAir = false, anySolid = false, anyUnknown = false;
                    for ( int i = 0; i < 8; ++i )
                    {
                        float const d = dens( cc[i], rr[i], kk[i] );
                        if ( IsUnknown( d ) ) { anyUnknown = true; break; }
                        if ( Solid( d ) ) { anySolid = true; }
                        else { anyAir = true; }
                    }
                    // Fail-closed: dual cells that touch unavailable samples do not invent a feature.
                    if ( anyUnknown || !( anyAir && anySolid ) ) { continue; }

                    if ( focusRM > 0.f )
                    {
                        // Keep if dual mid in focus, OR any corner (open skin / cavity) in focus.
                        bool keep = InFocus( mid, focusX, focusY, focusZ, focusRM );
                        if ( !keep )
                        {
                            for ( int i = 0; i < 8 && !keep; ++i )
                            {
                                Vec3 const p = LatticeWorld( cc[i], rr[i], kk[i], originX, originY, crestZ, du, dv, edge, kz );
                                keep = InFocus( p, focusX, focusY, focusZ, focusRM );
                            }
                        }
                        if ( !keep ) { continue; }
                    }

                    for ( int e = 0; e < 12; ++e )
                    {
                        int const i0 = edges[e][0], i1 = edges[e][1];
                        Hermite h{};
                        if ( EdgeHermite( field,
                            cc[i0], rr[i0], kk[i0], cc[i1], rr[i1], kk[i1],
                            originX, originY, crestZ, du, dv, edge, kz, iso, h, stats ) )
                        {
                            if ( nH < 12 ) { H[nH++] = h; }
                        }
                    }
                    Vec3 feature{};
                    if ( !SolveQef( H, nH, cellCentre, cellMax, feature, stats ) ) { continue; }
                    int const id = idx( c, r, k );
                    hasVert[(size_t)id] = 1;
                    vert[(size_t)id] = feature;
                }
            }
        }

        auto vertAt = [&]( int c, int r, int k, Vec3& out ) -> bool
        {
            int const id = idx( c, r, k );
            if ( id < 0 || id >= (int)hasVert.size() || !hasVert[(size_t)id] ) { return false; }
            out = vert[(size_t)id];
            return true;
        };

        // Emit quads: for each primal sign-changing edge, 4 dual cells share it.
        // World-cell seam ownership: each column emits its +max face edges (c=w-1→w, r=h-1→h)
        // and never the -min face (c=-1→0 / r=-1→0). Interior edges use lex-min dual owner.
        auto tryEdge = [&]( int c0, int r0, int k0, int c1, int r1, int k1,
            int dc0, int dr0, int dk0, int dc1, int dr1, int dk1,
            int dc2, int dr2, int dk2, int dc3, int dr3, int dk3,
            bool worldSeamEdge )
        {
            float const d0 = dens( c0, r0, k0 );
            float const d1 = dens( c1, r1, k1 );
            if ( IsUnknown( d0 ) || IsUnknown( d1 ) ) { return; }
            if ( Solid( d0 ) == Solid( d1 ) ) { return; }

            // Lex-min dual cell among the four is the owner (within this column extract).
            int oc[4] = { dc0, dc1, dc2, dc3 };
            int orr[4] = { dr0, dr1, dr2, dr3 };
            int ok[4] = { dk0, dk1, dk2, dk3 };
            int owner = 0;
            for ( int i = 1; i < 4; ++i )
            {
                if ( oc[i] < oc[owner]
                  || ( oc[i] == oc[owner] && orr[i] < orr[owner] )
                  || ( oc[i] == oc[owner] && orr[i] == orr[owner] && ok[i] < ok[owner] ) )
                {
                    owner = i;
                }
            }
            // Only emit when this invocation's "primary" dual matches owner — we call once per edge
            // from the primary dual (dc0,dr0,dk0) convention: first dual is (min corner).
            if ( !( dc0 == oc[owner] && dr0 == orr[owner] && dk0 == ok[owner] ) ) { return; }

            Vec3 v[4];
            for ( int i = 0; i < 4; ++i )
            {
                if ( !vertAt( oc[i], orr[i], ok[i], v[i] ) ) { return; }
            }
            Vec3 const p0 = LatticeWorld( c0, r0, k0, originX, originY, crestZ, du, dv, edge, kz );
            Vec3 const p1 = LatticeWorld( c1, r1, k1, originX, originY, crestZ, du, dv, edge, kz );
            Vec3 towardAir = Solid( d0 ) ? ( p1 - p0 ) : ( p0 - p1 );
            int const edgesBefore = stats.edgesEmitted;
            EmitQuad( v[0], v[1], v[2], v[3], towardAir, maxEdge, stats, outTris );
            if ( worldSeamEdge && stats.edgesEmitted > edgesBefore )
            {
                ++stats.boundaryEdgesEmitted;
            }
        };

        // X-edges: interior c=0..w-2 plus +X world seam c=w-1→w (owned by this column).
        for ( int k = 0; k < kz; ++k )
        for ( int r = 0; r < h; ++r )
        for ( int c = 0; c < w; ++c )
        {
            tryEdge( c, r, k, c + 1, r, k,
                c, r - 1, k - 1,  c, r, k - 1,  c, r, k,  c, r - 1, k,
                /*worldSeamEdge=*/c == w - 1 );
        }
        // Y-edges: interior r=0..h-2 plus +Y world seam r=h-1→h (owned by this column).
        for ( int k = 0; k < kz; ++k )
        for ( int r = 0; r < h; ++r )
        for ( int c = 0; c < w; ++c )
        {
            tryEdge( c, r, k, c, r + 1, k,
                c - 1, r, k - 1,  c, r, k - 1,  c, r, k,  c - 1, r, k,
                /*worldSeamEdge=*/r == h - 1 );
        }
        // Z-edges (vertical in lattice index; world Z down from crest) — no world-cell seam.
        for ( int k = 0; k < kz - 1; ++k )
        for ( int r = 0; r < h; ++r )
        for ( int c = 0; c < w; ++c )
        {
            tryEdge( c, r, k, c, r, k + 1,
                c - 1, r - 1, k,  c, r - 1, k,  c, r, k,  c - 1, r, k,
                /*worldSeamEdge=*/false );
        }

        return stats;
    }
} // namespace DualContourQef
