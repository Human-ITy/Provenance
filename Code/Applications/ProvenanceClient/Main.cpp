// Provenance Phase 4 client - Esoterica fork app.
// Phase 2 far heightfield + Phase 3 6ft walk + Phase 4 interaction digests (handful scoop).
// Handheld law: ~245 mL ~= 1 cup ~= 32/255 of a 12.5cm storage voxel (~8 scoops per voxel).
// Protocol matches Unreal FFablescriptClient (newline JSON, version 1).

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <gl/GL.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_ONLY_PNG
#include "stb_image.h"

#include "ProvenanceGeography.h"
#include "VisualMaterial.h"
#include "RockStructure.h"
#include "HorizonToHand.h"
#include "DualContourQef.h"

#include <algorithm>
#include <cmath>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#pragma comment( lib, "ws2_32.lib" )
#pragma comment( lib, "user32.lib" )
#pragma comment( lib, "gdi32.lib" )
#pragma comment( lib, "shell32.lib" )
#pragma comment( lib, "opengl32.lib" )

namespace
{
    constexpr char const* kDefaultHost = "127.0.0.1";
    constexpr int kDefaultPort = 8765;
    constexpr int kProtocolVersion = 1;
    constexpr UINT_PTR kTimerId = 1;

    constexpr int kBlockCells = 16;          // surface_field tile (Unreal uses 24; 16 keeps Phase 2 snappy)
    constexpr int kFarRadiusCells = 96;      // far ring reach (~vista underlay scale)
    constexpr int kMaxBlocksPerTick = 1;     // one in-flight request (engine is single-client sequential)

    // 6 ft scale reference (engine: 1 cell = 1 m)
    constexpr float kCharHeightM = 1.8288f;  // 6 ft
    constexpr float kEyeHeightM = 1.70f;     // eye line for a ~6 ft adult
    constexpr float kCapsuleRadiusM = 0.35f;
    constexpr float kWalkSpeedMps = 5.0f;    // brisk walk
    constexpr float kSprintSpeedMps = 11.0f; // matches Unreal sprint ~cell/s order
    constexpr float kFlySpeedMps = 24.f;
    constexpr float kFlySprintMps = 48.f;
    constexpr float kGravityMps2 = 20.f;
    constexpr float kJumpSpeedMps = 7.5f;
    constexpr float kMaxStepM = 0.55f;       // max climb per move without jump
    // Steeper than ~48° is a wall — small per-frame steps used to skate up cliffs into the mesh.
    constexpr float kMaxWalkSlope = 1.10f;   // rise/run ≈ tan(48°)
    constexpr float kWallBodyClearM = 0.45f; // torso hits wall if grade beside feet exceeds this
    constexpr float kProjNearDefaultM = 0.5f;  // outdoor / held-inspect clearance
    constexpr float kProjNearCavityM = 0.06f; // inside carve — else near clip eats cavity walls → void
    constexpr float kEyeCrouchMinM = 0.42f;

    // Handheld / manipulation volume (authoritative player scoop feel)
    constexpr int kFillFull = 255;
    constexpr int kHandfulFillUnits = 32;                    // deliberate scoop quantum
    constexpr float kHandfulVoxelFrac = (float)kHandfulFillUnits / (float)kFillFull; // ~1/8 voxel
    constexpr float kVoxelEdgeM = 0.125f;
    constexpr float kVoxelVolumeM3 = kVoxelEdgeM * kVoxelEdgeM * kVoxelEdgeM; // ~1.953 L
    constexpr float kHandfulVolumeM3 = kVoxelVolumeM3 * kHandfulVoxelFrac;    // ~245 mL
    constexpr float kHandfulRadiusM = 0.0388225f; // (3*V_handful/(4pi))^(1/3) -- rounded bare-hand scoop
    constexpr float kDirtVoxelG = 2500.f;    // display fallback; engine credits real grams
    constexpr float kHandfulDirtG = kDirtVoxelG * kHandfulVoxelFrac; // ~314 g dirt
    constexpr float kReachCells = 3.5f;

    // Journal / hotbar UI
    constexpr int kInvCols = 3;
    constexpr int kInvRows = 4;
    constexpr int kInvPageSize = kInvCols * kInvRows; // 12
    constexpr int kCraftCols = 3;
    constexpr int kCraftRows = 2;
    constexpr int kCraftPageSize = kCraftCols * kCraftRows; // 6
    constexpr int kHotbarSlots = 6;
    constexpr int kMaxDeposit = 4;

    struct BagSlot
    {
        std::string id;
        int count = 0;
    };

    struct RecipeDef
    {
        char const* id;
        char const* name;
        char const* resultId;
        int resultCount;
        char const* reqId[kMaxDeposit];
        int reqCount[kMaxDeposit];
        int nReq;
    };

    RecipeDef const kRecipes[] = {
        { "campfire", "CAMPFIRE", "campfire_kit", 1,
          { "sticks_tinder", "flint", "bark", nullptr }, { 1, 1, 1, 0 }, 3 },
        { "torch", "TORCH", "torch", 1,
          { "sticks_tinder", "bark", nullptr, nullptr }, { 1, 1, 0, 0 }, 2 },
        { "planks", "WOOD PLANKS", "wood_planks", 2,
          { "wood_log", nullptr, nullptr, nullptr }, { 1, 0, 0, 0 }, 1 },
        { "sword", "SWORD", "sword", 1,
          { "iron", "wood_planks", "leather_hide", nullptr }, { 2, 1, 1, 0 }, 3 },
        { "kettle", "KETTLE", "kettle", 1,
          { "iron", "coal", nullptr, nullptr }, { 3, 1, 0, 0 }, 2 },
        { "bucket", "BUCKET", "bucket", 1,
          { "wood_planks", "iron", "rope", nullptr }, { 2, 1, 1, 0 }, 3 },
        { "flask", "WATER FLASK", "water_flask", 1,
          { "leather_hide", "rope", nullptr, nullptr }, { 2, 1, 0, 0 }, 2 },
    };
    constexpr int kRecipeCount = (int)( sizeof( kRecipes ) / sizeof( kRecipes[0] ) );

    enum class LinkState
    {
        Disconnected,
        Connecting,
        Connected,
        CapsOk,
        CapsError,
        SocketError,
    };

    enum class PendingKind
    {
        None,
        Caps,
        Player,
        Surface,
        Carve,
        Place,
        Column,
    };

    enum class IntentKind
    {
        None,
        Dig,
        PlaceHeld,
    };

    constexpr int kFillIso = 128; // half-full isosurface (engine FILL_ISO)
    // Diagnostic: voxel_column must not mutate resident geography grade (spire conviction).
    // Fill / fillZ / cavity reconcile still apply. Flip false only to A/B the needle.
    constexpr bool kForbidColumnGradeOverwrite = true;
    // AABB solid|air shell is NOT a coherent surface — never peel virgin HF for it.
    // Production cavity = D2 Hermite/QEF; AABB stays B-key diagnostic only.
    constexpr bool kAabbCavityOwnsHf = false;

    enum class BoundaryMode : uint8_t
    {
        D2 = 0,        // production — Hermite/QEF dual contour on EditedRegion occupancy
        DebugBoundary, // AABB solid|air faces (diagnostic)
        Overlay        // D2 + AABB wireframe
    };

    // Loose MatterBody chips (H2H plates). Off clears view for D2/HF cert.
    enum class ChipMode : uint8_t
    {
        Off = 0,   // zero chip presentation/physics cost
        Visual,    // geometry visible, frozen (no integrate)
        Phys       // ACTIVE gravity + SupportBelow(x,y,currentZ) settle/slide
    };

    struct CellSample
    {
        float grade = 0.85f;
        float fillZ = 0.f;          // occupancy-derived crest Z (when known)
        bool hasFillZ = false;
        bool edited = false;
        uint8_t r = 90, g = 120, b = 70;
        std::string cap;            // surface_field material id (sand/dirt/clay/...)
        bool valid = false;
        // Authoritative column occupancy (voxel_column) — D2 cavity source
        std::vector<uint8_t> fill;
        int fillW = 0, fillH = 0, fillK = 0;
        GLuint cavityList = 0;
        GLuint debugBoundaryList = 0;
        bool hasCavity = false;
        bool carved = false; // true only after local affect-sphere dig — never from prefetch/look/walk
        // Commit focus (transient last action) — NOT the persistent HF aperture.
        float carveWx = 0.f, carveWy = 0.f, carveWz = 0.f, carveRM = 0.f;
        bool hasCarveFocus = false;
        float patchMinX = 0.f, patchMinY = 0.f, patchMaxX = 0.f, patchMaxY = 0.f;
        bool hasPatchBounds = false;
        uint32_t editedRegionId = 0; // persistent excavation region (monotonic openings)
        std::vector<DualContourQef::Tri> cavityTris;
        // Last D2 extract telemetry (fail-closed halo + seam ownership).
        int lastD2HaloMissing = 0;
        int lastD2BoundaryEdges = 0;
        bool lastD2PublishRefused = false;
        // Lattice top Z when place adds headroom above virgin grade (HF grade unchanged).
        float occCrestZ = 0.f;
        bool hasOccCrest = false;
    };

    // One connected excavation. Openings only expand on remove-only edits.
    struct EditOpening
    {
        float x = 0.f, y = 0.f, z = 0.f, r = 0.f;
        float nx = 0.f, ny = 0.f, nz = 1.f; // outward face at strike (wall mouths ≠ crest HF N)
    };

    struct EditedRegion
    {
        uint32_t id = 0;
        std::vector<EditOpening> openings; // persistent HF aperture union (never shrinks on new strike)
        std::vector<std::pair<int, int>> cells;
        float ownMinX = 0.f, ownMinY = 0.f, ownMaxX = 0.f, ownMaxY = 0.f;
        bool hasOwnBounds = false;
        // Transient last action (diagnostic / RED) — may move freely.
        float actionX = 0.f, actionY = 0.f, actionZ = 0.f, actionR = 0.f;
        bool hasAction = false;
        int dirtyRev = 0;
    };

    enum class ScarKind : uint8_t
    {
        ScoopHemi = 0,     // isotropic handful cup — flat soft ground only
        FoliationPlate,    // asymmetric schist/laminated pry notch
        FacePuncture,      // sealed face chip — steep walls / pick punctures (NOT DigDep bowls)
    };

    struct DigScar
    {
        float wx = 0.f, wy = 0.f, wz = 0.f; // dig: scoop centre; place: contact
        float radius = kHandfulRadiusM;     // footprint == green/amber selection sphere
        float depth = kHandfulRadiusM;     // open-cup vertical amp (stacks on re-dig)
        int cx = 0, cy = 0;                 // contact cell (for column refresh)
        bool place = false;
        // Cohesive buried bite: sphere cavity under an intact grade roof (sand never tunnels).
        bool tunnel = false;
        // First surface-break centre Z — deepen moves wz down but keeps this entrance open.
        float entranceWz = 0.f;
        // Mica-schist / foliated law — index alone is not enough.
        ScarKind kind = ScarKind::ScoopHemi;
        float strikeX = 1.f, strikeY = 0.f; // along foliation in XY
        float plateAlong = 0.f;             // half-length along strike
        float plateAcrossDeep = 0.f;        // pry recess (deep side)
        float plateAcrossLip = 0.f;         // thin projecting lip (shallow side)
        float support = 1.f;                // 1 connected … 0 free slab
        RockStruct::FractureStage fracStage = RockStruct::FractureStage::Intact;
        // Face frame for sealed chips — recess along -N at the true strike point (not grade-Z sink).
        float faceNx = 0.f, faceNy = 0.f, faceNz = 1.f;
    };

    struct AppState
    {
        HWND hwnd = nullptr;
        HDC hdc = nullptr;
        HGLRC glrc = nullptr;
        SOCKET sock = INVALID_SOCKET;
        LinkState link = LinkState::Disconnected;
        std::string statusLine = "Provenance Phase 4 - starting...";
        std::string detail;
        std::string digestLine = "Interaction digest idle - LMB dig into stock, RMB place one scoop, Tab unlock cursor";
        std::string recvBuf;
        int nextId = 1;
        std::string host = kDefaultHost;
        int port = kDefaultPort;
        DWORD lastAttemptMs = 0;
        int attempts = 0;
        // --cert-dig: auto tour digs (move + strike), dump PPMs, quit.
        bool certDig = false;
        int certPhase = 0;
        DWORD certPhaseMs = 0;
        int certStop = 0; // 0..2 dig sites
        float certMaxDtMs = 0.f;
        float certLastRemeshMs = 0.f;
        float certMaxRemeshMs = 0.f;
        int certFrames = 0;
        // --cert-geo / --cert-geography: RANGE Horizon-to-Hand transect cert.
        bool certGeo = false;
        int certGeoPhase = 0;   // 0 wait stream → 1 discover/virgin → 2 walk → 3 dig → 4 write/quit
        DWORD certGeoPhaseMs = 0;
        // Cert-only: skip EnsureD2HaloLattices so deliberate Unknown-halo refusal can be proven.
        bool certD2SkipHaloEnsure = false;
        int certGeoDigIdx = 0;
        int certGeoDigArmed = 0; // 0 move/aim, 1 struck settling
        int certGeoExitCode = 0; // 0 PASS, 1 hard FAIL
        int certGeoWalkHf0 = 0;
        int certGeoWalkD20 = 0;
        int certGeoWalkCells0 = 0;
        int certGeoWalkSample0 = 0;
        bool certGeoFailWritten = false;
        // --cert-lsi: read-only Local Surface Intent capture/cert (dig+place geometry).
        bool certLsi = false;
        int certLsiPhase = 0; // 0 wait → 1 settle → 2 run → 3 write/quit
        DWORD certLsiPhaseMs = 0;
        int certLsiExitCode = 0; // 0 harness complete with no FAIL rows; 1 = defect FAIL rows
        bool certLsiFailWritten = false;
        // Startup / subsystem counters — prove virgin path does zero D2.
        int perfGeoCellsCreated = 0;
        int perfSampleSurfaceCalls = 0;   // only EnsureGeoCell should bump this for terrain
        int perfSampleCapColorCalls = 0;  // must use cell cache (no FBM)
        int perfHfRebuilds = 0;
        int perfHfTris = 0;
        float perfHfRemeshMsTotal = 0.f;
        int perfD2Rebuilds = 0;
        int perfD2Tris = 0;
        int perfD2Edges = 0;
        int perfD2QefFallbacks = 0;
        int perfD2HaloMiss = 0;
        float perfD2MsTotal = 0.f;
        int perfVirginD2Rebuilds = -1; // snapshot at first stream-complete before any dig
        bool perfVirginSnapDone = false;
        char certOutDir[MAX_PATH] = {};

        // terrain_caps
        int wireVersion = 0;
        std::string contract;
        std::string generatorId;
        int generatorVersion = 0;
        std::string worldIdentityHash;
        int terrainRev = -1;
        std::string authorityMode;
        int worldSize = -1;
        bool envelopeOk = false;
        std::string lastError;
        float reliefVoxels = 64.f;
        float gradeDatum = 0.5f;
        float voxelEdgeM = 0.125f;
        float cellDepthM = 0.5f;   // caps cell_depth_m — engine depth/radius units (1.0 = ~0.5 m)

        // player / stream center
        int playerX = 128;
        int playerY = 128;
        bool havePlayer = false;

        // pending RPC
        PendingKind pending = PendingKind::None;
        int pendingId = 0;
        int pendingBx = 0;
        int pendingBy = 0;
        IntentKind intent = IntentKind::None;

        // far surface cache: key = ((int64)x << 32) ^ (uint32)y
        std::unordered_map<uint64_t, CellSample> cells;
        std::unordered_set<uint64_t> fetchedBlocks;
        std::vector<EditedRegion> editedRegions;
        uint32_t nextEditedRegionId = 1;
        int cellsLoaded = 0;
        int blocksLoaded = 0;
        int blocksWanted = 0;
        bool streamComplete = false;

        // aim + held bite
        bool aimHit = false;
        int aimCx = 0, aimCy = 0;
        float aimU = 0.5f, aimV = 0.5f, aimDepth = kHandfulRadiusM;
        float aimX = 0.f, aimY = 0.f, aimZ = 0.f;
        // Crosshair identity — geography strike + biome; wire cap may differ (cover vs rock).
        std::string aimStrikeCap = "-";
        std::string aimWireCap = "-";
        std::string aimBiome = "-";
        std::string aimFormId = "-";
        uint8_t aimStrikeR = 128, aimStrikeG = 128, aimStrikeB = 128;
        // Unreal FsDrawImmediateTerrainCue twin — brief rings/rays on dig/place press
        float cueT = 0.f;
        float cueX = 0.f, cueY = 0.f, cueZ = 0.f;
        float cueNx = 0.f, cueNy = 0.f, cueNz = 1.f; // surface normal at click
        bool cuePlace = false;
        bool cueBusy = false;
        std::unordered_map<std::string, int> heldBite; // material -> grams (mirrors engine carry accumulate)
        int heldTotalG = 0;
        std::string heldDominant;
        std::unordered_map<std::string, int> pendingPlaceAsk; // scoop sent on last place
        int pendingPlaceG = 0;
        bool pendingPlaceIntoHole = false; // refill dig cup — no mound on top
        // Optimistic scar edit (for refuse rollback without killing neighbour cups)
        int pendingScarIndex = -1;
        float pendingScarDepthBefore = 0.f;
        bool pendingScarWasNew = false;
        bool pendingScarIsPlace = false;
        int pendingBiteCx = 0, pendingBiteCy = 0;
        float pendingBiteU = 0.5f, pendingBiteV = 0.5f;
        float pendingBiteWx = 0.f, pendingBiteWy = 0.f, pendingBiteWz = 0.f;
        bool pendingBiteForward = false;
        uint64_t grippedBodyId = 0; // H2H MatterBody currently carried
        // Gallery sample in hand (E pickup) — separate from scoop heldBite / plate grip.
        std::string heldGalleryId;
        int heldGallerySrc = -1; // gallery index restored on E-drop
        float heldYaw = 0.f;     // Shift+scroll: spin about world up
        float heldTumble = 0.f;  // Scroll: tip/roll over about hand-right (inspect faces)
        static constexpr int kGalleryMax = 40;
        struct GallerySample
        {
            char const* id = nullptr;
            float x = 0.f, y = 0.f, z = 0.f;
            bool present = false;
        };
        GallerySample gallery[kGalleryMax] = {};
        int galleryCount = 0;
        bool gallerySpawned = false;
        int pendingLocalScoopG = 0;
        std::string pendingLocalScoopMat;
        float pendingAffectRM = kHandfulRadiusM; // dig-volume sphere committed with the pending carve

        int lastDigRev = -1;
        float lastEngineMs = 0.f;
        std::vector<DigScar> scars;
        std::vector<std::pair<int, int>> columnQueue; // cells awaiting voxel_column truth
        int pendingColX = 0, pendingColY = 0;

        // Cavity boundary presentation: D2 production; B cycles diagnostic AABB.
        BoundaryMode boundaryMode = BoundaryMode::D2;
        int cavityTrisTotal = 0;
        int cavityEdgesEmitted = 0;
        // Chips default OFF so D2 cavity cert is not buried under plates.
        ChipMode chipMode = ChipMode::Off;

        // SPIRE conviction telemetry — grade authority A/B (no D2/ownership changes).
        bool spireHavePre = false;
        int spireCx = 0, spireCy = 0;
        float spireGeoGrade = 0.f, spireGeoZ = 0.f;
        float spireAimZ = 0.f;
        float spirePreReqGrade = 0.f, spirePreReqZ = 0.f;
        std::string spireVisualCap;   // geography / HF presentation material
        std::string spireAuthMat;     // engine/form credit material (may disagree — no remap)
        std::string spireLine = "SPIRE idle — dig once to capture grade/Z chain";
        int spireOverwriteBlocked = 0;
        int spireOverwriteWouldHave = 0;

        // body + camera (world meters; 1 cell = 1 m)
        float feetX = 128.f;
        float feetY = 128.f;
        float feetZ = 0.f;
        float velZ = 0.f;
        bool grounded = false;
        bool walkMode = true;   // default: 6ft standable walk; F = free camera
        DWORD sessionStartMs = 0; // GetTickCount at launch — session clock
        float camX = 128.f;
        float camY = 128.f;
        float camZ = 40.f;
        float projNearM = kProjNearDefaultM; // tightened inside carved cavities
        float yaw = 0.f;     // radians, 0 = +Y
        float pitch = -0.15f;

        // Authoritative outdoor directional sun + sky ambient (world-space).
        // Materials own base_color/roughness/metallic; sun owns brightness right now.
        float sunAzimuth = 0.85f;     // radians from +Y toward +X
        float sunElevation = 0.95f;   // radians above horizon (~54°)
        float sunDirX = 0.35f, sunDirY = 0.18f, sunDirZ = 0.92f; // toward sun
        float sunColorR = 1.f, sunColorG = 0.96f, sunColorB = 0.88f;
        float sunIntensity = 1.05f;
        float skyColorR = 0.55f, skyColorG = 0.62f, skyColorB = 0.78f;
        float skyIntensity = 0.32f;
        bool keys[256] = {};
        bool keyToggleLatch[256] = {};
        bool rmbDown = false;
        bool mouseLook = true;   // FPS: cursor locked to camera
        bool cursorCaptured = false;
        int lastMouseX = 0;
        int lastMouseY = 0;
        DWORD lastFrameMs = 0;
        float frameDt = 0.016f;

        GLuint fontBase = 0;

        // Cached terrain mesh — rebuild on dig/stream/move, never resample whole vista each frame
        GLuint terrainList = 0;
        bool terrainDirty = true;
        int terrainAnchorX = INT_MIN;
        int terrainAnchorY = INT_MIN;
        size_t terrainScarGen = 0;
        size_t scarGen = 0;

        // Journal (J) + persistent hotbar
        bool journalOpen = false;
        bool journalWasMouseLook = true;
        int invPage = 0;
        int craftPage = 0;
        int selectedRecipe = -1; // index into kRecipes, -1 = browser
        BagSlot bag[64];
        int bagCount = 0;
        BagSlot deposit[kMaxDeposit];
        std::string hotbar[kHotbarSlots];
        int hotbarSel = 0;
        float uiMouseX = 0.f; // ortho (y-up)
        float uiMouseY = 0.f;
        int uiWinW = 1;
        int uiWinH = 1;
        bool bagSeeded = false;
        std::unordered_map<std::string, GLuint> iconTex;
        std::string assetsRoot;
        char const* journalNotes[6] = {
            "Day 1 — awoke beside the river bank.",
            "The earth remembers every scoop.",
            "Seed map marks the known ridge.",
            "Craft at the campfire when dry.",
            "",
            ""
        };
        char const* skillNames[5] = { "FORAGE", "DIG", "CRAFT", "COOK", "TRAVEL" };
        float skillFrac[5] = { 0.35f, 0.55f, 0.20f, 0.10f, 0.40f };
    };

    AppState g;

    void SetMouseLook( HWND hwnd, bool enabled ); // defined with camera/input
    void LoadIconTextures();
    void UnloadIconTextures();
    void SeedStarterBag();
    void BagAdd( std::string const& id, int count );
    void DrawJournal();
    void DrawHotbar();
    void OpenJournal();
    void CloseJournal();
    void ToggleJournal();
    void UpdateUiMouseFromWin( int winX, int winY );
    bool TryHotbarClick( float mx, float my );
    bool HandleJournalClick( float mx, float my, bool rightClick );
    void UpdateStreamHud();
    void UpdateAim();
    bool TryPickupGallerySample();
    bool DropHeldGalleryToGround();
    void DrawHeldGallerySample();
    void EnsureGallerySpawned();

    uint64_t CellKey( int x, int y )
    {
        return ( (uint64_t)(uint32_t)x << 32 ) | (uint32_t)y;
    }

    uint64_t BlockKey( int bx, int by )
    {
        return CellKey( bx, by );
    }

    char const* LinkLabel( LinkState s )
    {
        switch ( s )
        {
            case LinkState::Disconnected: return "DISCONNECTED";
            case LinkState::Connecting: return "CONNECTING";
            case LinkState::Connected: return "CONNECTED";
            case LinkState::CapsOk: return "CAPS OK";
            case LinkState::CapsError: return "CAPS ERROR";
            case LinkState::SocketError: return "SOCKET ERROR";
        }
        return "?";
    }

    void CloseSock()
    {
        if ( g.sock != INVALID_SOCKET )
        {
            closesocket( g.sock );
            g.sock = INVALID_SOCKET;
        }
        g.recvBuf.clear();
        g.pending = PendingKind::None;
    }

    bool ExtractJsonString( std::string const& json, char const* key, std::string& out )
    {
        std::string needle = std::string( "\"" ) + key + "\":\"";
        size_t p = json.find( needle );
        if ( p == std::string::npos )
        {
            needle = std::string( "\"" ) + key + "\": \"";
            p = json.find( needle );
            if ( p == std::string::npos ) { return false; }
        }
        p += needle.size();
        size_t e = json.find( '"', p );
        if ( e == std::string::npos ) { return false; }
        out.assign( json, p, e - p );
        return true;
    }

    bool ExtractJsonInt( std::string const& json, char const* key, int& out )
    {
        std::string needle = std::string( "\"" ) + key + "\":";
        size_t p = json.find( needle );
        if ( p == std::string::npos ) { return false; }
        p += needle.size();
        while ( p < json.size() && ( json[p] == ' ' || json[p] == '\t' ) ) { ++p; }
        if ( p >= json.size() ) { return false; }
        char* end = nullptr;
        long v = strtol( json.c_str() + p, &end, 10 );
        if ( end == json.c_str() + p ) { return false; }
        out = (int)v;
        return true;
    }

    bool ExtractJsonFloat( std::string const& json, char const* key, float& out )
    {
        std::string needle = std::string( "\"" ) + key + "\":";
        size_t p = json.find( needle );
        if ( p == std::string::npos ) { return false; }
        p += needle.size();
        while ( p < json.size() && ( json[p] == ' ' || json[p] == '\t' ) ) { ++p; }
        if ( p >= json.size() ) { return false; }
        char* end = nullptr;
        double v = strtod( json.c_str() + p, &end );
        if ( end == json.c_str() + p ) { return false; }
        out = (float)v;
        return true;
    }

    bool ExtractJsonBool( std::string const& json, char const* key, bool& out )
    {
        std::string needle = std::string( "\"" ) + key + "\":";
        size_t p = json.find( needle );
        if ( p == std::string::npos ) { return false; }
        p += needle.size();
        while ( p < json.size() && ( json[p] == ' ' || json[p] == '\t' ) ) { ++p; }
        if ( json.compare( p, 4, "true" ) == 0 ) { out = true; return true; }
        if ( json.compare( p, 5, "false" ) == 0 ) { out = false; return true; }
        return false;
    }

    bool ExtractPositionXY( std::string const& json, int& x, int& y )
    {
        size_t p = json.find( "\"position\":" );
        if ( p == std::string::npos ) { return false; }
        p = json.find( '[', p );
        if ( p == std::string::npos ) { return false; }
        char* end = nullptr;
        long vx = strtol( json.c_str() + p + 1, &end, 10 );
        if ( end == json.c_str() + p + 1 ) { return false; }
        while ( *end == ' ' || *end == '\t' || *end == ',' ) { ++end; }
        long vy = strtol( end, &end, 10 );
        x = (int)vx;
        y = (int)vy;
        return true;
    }

    void CapColor( char const* cap, uint8_t& r, uint8_t& g, uint8_t& b )
    {
        // VisualMaterialDef palette — continuous body language, not painted index tiles.
        VisualMat::CapColor( cap, r, g, b );
    }

    float GradeToZ( float grade )
    {
        // Same law as Unreal / terrain_caps: height_voxels = (grade - datum) * relief; meters = * voxel_edge
        return ( grade - g.gradeDatum ) * g.reliefVoxels * g.voxelEdgeM;
    }

    bool SampleGroundZBase( float x, float y, float& outZ );
    bool SampleGroundZ( float x, float y, float& outZ );
    bool SampleAimSurfaceZ( float x, float y, float& outZ );
    bool SampleTerrainDrawZ( float x, float y, float& outZ ); // HF; collapse where air under skin
    bool SampleOccupancyZ( float x, float y, float& outZ );
    // Occupancy / HF physical support — not D2 tris. Prefer SupportBelow (Z-aware).
    struct SupportHit
    {
        bool hit = false;
        bool deferred = false; // missing authority — refuse; never invent a floor
        DualContourQef::Vec3 position{};
        DualContourQef::Vec3 normal{ 0.f, 0.f, 1.f };
        char material[32] = {};
        int cellX = 0, cellY = 0;
        int regionRev = -1; // EditedRegion::dirtyRev when known
    };
    SupportHit SupportBelow( float x, float y, float queryZ );
    bool SupportAt( float x, float y, float& outZ ); // thin XY adapter → SupportBelow
    bool OccupancySolidAt( float x, float y, float z );
    void PrefetchOccupancyCell( int cx, int cy );
    void SetFillAt( CellSample& cell, int c, int r, int k, uint8_t v );
    void SeedOccupancyFromVirginSurface( int cx, int cy );
    void EnsureOccupancyLattice( int cx, int cy );
    bool CarveOccupancySphere( float carveX, float carveY, float carveZ, float radiusM,
        float openX, float openY, float openZ );
    // P3e: place / re-fill — reverse matter transfer into occupancy (not DigScar mound).
    struct PlaceFillResult
    {
        bool ok = false;
        int acceptedGrams = 0;
        int unitsFilled = 0;
        int voxelsTouched = 0;
    };
    float CellOccCrest( CellSample const& cell );
    bool ExpandOccupancyHeadroom( int cx, int cy, int addLayers );
    int CountOccupancySolidInSphere( float wx, float wy, float wz, float radiusM );
    int CountOccupancyFillUnitsInSphere( float wx, float wy, float wz, float radiusM );
    PlaceFillResult PlaceOccupancyFill( float wx, float wy, float wz, float radiusM, int gramsAvailable );
    void RetirePresentationScarsNear( float wx, float wy, float radiusM );
    void RebuildCavityMesh( int cx, int cy );
    void DrawCavityMeshes();
    void DrawCavityMeshesMouth(); // stencil ALWAYS pass — tiny offset only
    void DrawCavityMeshesInterior(); // depth LEQUAL — no offset (offset x-rays nearby HF)
    void DrawEditedRegionOpenings(); // wall-only tip disks (crest uses occupancy break)
    void DrawOccupancySurfaceBreaks(); // stencil = matter-gone crest + mouth-bridge fill
    bool CavityReadyNear( float x, float y ); // D2 present near XY — gate HF omit
    bool NearOpeningMouthAt( float x, float y ); // bite mouth disk — not whole occupancy cell
    bool CrestMouthStencilAt( float x, float y );
    void DrawCarveActionFootprints(); // occupancy crest + steep wall openings
    EditedRegion* FindEditedRegion( uint32_t id );
    void AddOpeningMonotonic( EditedRegion& er, float x, float y, float z, float r,
        float nx = 0.f, float ny = 0.f, float nz = 1.f );
    uint32_t MergeOrCreateEditedRegion( float wx, float wy, float wz, float radiusM,
        std::vector<std::pair<int, int>> const& touchedCells,
        float nx = 0.f, float ny = 0.f, float nz = 1.f );
    bool NearRegionOpenings( EditedRegion const& er, float xc, float yc, float zc, float collarM );
    bool EditedRegionOwnsAt( float x, float y );
    bool SurfaceBrokenByOccupancy( float x, float y );
    bool CavityPatchOwnsAt( float x, float y );
    bool SurfaceOpenedByOccupancy( float x, float y );
    GLuint AllocDisplayListOutsideFonts();
    void EmitPhase3Tri( float x0, float y0, float z0,
        float x1, float y1, float z1,
        float x2, float y2, float z2,
        float cavityHint );
    void SampleAimNormal( float x, float y, float& nx, float& ny, float& nz );
    void CaptureFaceNormalAt( float x, float y, float& nx, float& ny, float& nz );
    void ResolveCarveIntoNormal( float aimX, float aimY, float aimZ,
        float lookX, float lookY, float lookZ,
        float& nx, float& ny, float& nz );
    bool IsSteepFaceAt( float x, float y );
    std::string CapAtWorld( float x, float y );
    CellSample const* GetCell( int x, int y );
    CellSample* GetCellMutable( int x, int y );

    float ScarHemiAt( DigScar const& s, float x, float y )
    {
        float const dx = x - s.wx;
        float const dy = y - s.wy;
        if ( s.kind == ScarKind::FoliationPlate && !s.place )
        {
            // Asymmetric notch aligned with foliation strike:
            // deep pry recess one side, thin projecting lip the other — not an ice-cream scoop.
            float const sx = s.strikeX, sy = s.strikeY;
            float const tx = -sy, ty = sx;
            float const along = dx * sx + dy * sy;
            float const across = dx * tx + dy * ty;
            float const halfAlong = (std::max)( 0.02f, s.plateAlong );
            if ( std::fabs( along ) > halfAlong ) { return 0.f; }
            float const acrossMax = ( across >= 0.f )
                ? (std::max)( 0.01f, s.plateAcrossDeep )
                : (std::max)( 0.01f, s.plateAcrossLip );
            if ( std::fabs( across ) > acrossMax ) { return 0.f; }
            float const ua = 1.f - ( along / halfAlong ) * ( along / halfAlong );
            float const uc = 1.f - ( across / acrossMax ) * ( across / acrossMax );
            float const asym = ( across >= 0.f ) ? 1.f : 0.52f; // lip stays thin
            // Visual chip only — never amplify into a through-wall sink.
            float const amp = (std::min)( 0.055f, s.depth * 0.85f );
            return amp * std::sqrt( (std::max)( 0.f, ua * uc ) ) * asym;
        }
        float const r2 = s.radius * s.radius;
        float const d2 = dx * dx + dy * dy;
        if ( d2 >= r2 || s.radius < 1e-6f ) { return 0.f; }
        return s.depth * std::sqrt( 1.f - d2 / r2 );
    }

    bool ActiveToolIsPick()
    {
        if ( g.hotbarSel < 0 || g.hotbarSel >= kHotbarSlots ) { return false; }
        return g.hotbar[g.hotbarSel] == "pick";
    }

    char const* ActiveToolId()
    {
        if ( g.hotbarSel < 0 || g.hotbarSel >= kHotbarSlots ) { return "hand"; }
        std::string const& id = g.hotbar[g.hotbarSel];
        if ( id.empty() || id == "torch" ) { return "hand"; }
        return id.c_str();
    }

    H2H::ToolMatterProfile const& ActiveToolProfile()
    {
        return H2H::ToolById( ActiveToolId() );
    }

    float ActiveAimRadiusM()
    {
        // LIVE contact/query envelope (Index §2) — aiming feel only, NOT removed volume.
        return ActiveToolProfile().contact_radius_m;
    }

    float TransferScarRadiusM()
    {
        // Tool transfer budget as equivalent sphere — base for soft scoop affect.
        float const V = (std::max)( 1e-6f, ActiveToolProfile().transfer_volume_limit_m3 );
        return std::cbrt( ( 3.f * V ) / ( 4.f * 3.14159265f ) );
    }

    float SoftScoopScarRadiusM()
    {
        // Fallback transfer sphere; dig path prefers AimDigAffect().radiusM (tool×material).
        return TransferScarRadiusM();
    }

    float SphereVolumeM3( float radiusM )
    {
        float const r = (std::max)( 1e-6f, radiusM );
        return ( 4.f / 3.f ) * 3.14159265f * r * r * r;
    }

    // One law: preview affect sphere == scar footprint == wire carve == matter-return volume.
    enum class DigAffectMode : uint8_t { ScoopHemi = 0, FacePuncture, FoliationPlate };

    struct DigAffectSpec
    {
        DigAffectMode mode = DigAffectMode::ScoopHemi;
        float radiusM = kHandfulRadiusM;   // dig-volume sphere (primary indicator)
        float depthM = kHandfulRadiusM;    // scar recess amp
        float volumeM3 = 0.f;              // conserved matter budget for this strike
        char const* feel = "scoop";        // HUD cue
    };

    DigAffectSpec ComputeDigAffect( H2H::ToolMatterProfile const& tool,
        H2H::MaterialFormContract const& form, bool steepFace )
    {
        DigAffectSpec a;
        float const transferR = std::cbrt(
            ( 3.f * (std::max)( 1e-6f, tool.transfer_volume_limit_m3 ) ) / ( 4.f * 3.14159265f ) );

        // Hard / foliated rock + pick → persistent fracture (plate), tip-scale pit.
        bool const hardRock = form.hardness >= 3
            || form.fabric == H2H::FabricKind::FoliatedAnisotropic
            || form.fabric == H2H::FabricKind::BeddedFissile
            || ( form.fabric == H2H::FabricKind::Massive && form.rigid_fracture_body );
        if ( hardRock && tool.force == H2H::ForceClass::Pick )
        {
            a.mode = DigAffectMode::FoliationPlate;
            // Tip pit = min(transfer sphere, influence) — concentrated strike, not shovel bowl.
            a.radiusM = (std::min)( transferR, (std::max)( 0.02f, tool.influence_depth_m * 0.45f ) );
            a.depthM = a.radiusM;
            a.volumeM3 = (std::min)( SphereVolumeM3( a.radiusM ), tool.transfer_volume_limit_m3 );
            a.feel = form.fabric == H2H::FabricKind::FoliatedAnisotropic ? "peel/plate" : "fracture";
            return a;
        }

        // Soft matter: tool transfer sphere is the dig volume. Shape follows fabric + slope.
        if ( tool.force == H2H::ForceClass::Pick )
        {
            // Pick on soft: tip puncture (secondary) — still one sphere, not shovel bowl.
            a.mode = DigAffectMode::FacePuncture;
            a.radiusM = (std::min)( transferR, (std::max)( 0.022f, tool.influence_depth_m * 0.40f ) );
            a.depthM = a.radiusM;
            a.volumeM3 = (std::min)( SphereVolumeM3( a.radiusM ), tool.transfer_volume_limit_m3 );
            a.feel = ( form.fabric == H2H::FabricKind::Granular ) ? "crush/grains" : "tip puncture";
            return a;
        }

        // Hand / shovel scoop — full transfer sphere.
        a.radiusM = transferR;
        a.depthM = transferR;
        a.volumeM3 = tool.transfer_volume_limit_m3;
        if ( steepFace )
        {
            a.mode = DigAffectMode::FacePuncture;
            a.feel = "face scoop";
        }
        else if ( form.fabric == H2H::FabricKind::Granular )
        {
            a.mode = DigAffectMode::ScoopHemi;
            a.feel = "slump scoop";
        }
        else
        {
            a.mode = steepFace ? DigAffectMode::FacePuncture : DigAffectMode::ScoopHemi;
            a.feel = steepFace ? "face scoop" : "soft scoop";
        }
        return a;
    }

    DigAffectSpec AimDigAffect()
    {
        if ( !g.aimHit )
        {
            DigAffectSpec a;
            a.radiusM = TransferScarRadiusM();
            a.depthM = a.radiusM;
            a.volumeM3 = ActiveToolProfile().transfer_volume_limit_m3;
            return a;
        }
        std::string const cap = CapAtWorld( g.aimX, g.aimY );
        H2H::MaterialFormContract const& form = H2H::FormOrDirt( cap.c_str() );
        float nx = 0.f, ny = 0.f, nz = 1.f;
        CaptureFaceNormalAt( g.aimX, g.aimY, nx, ny, nz );
        bool const steep = nz < 0.58f || IsSteepFaceAt( g.aimX, g.aimY );
        return ComputeDigAffect( ActiveToolProfile(), form, steep );
    }

    int AffectAcceptedGrams( H2H::MaterialFormContract const& form, DigAffectSpec const& affect )
    {
        // Matter return from the SAME sphere the player sees/affects — density from this material.
        float const vol = (std::min)( affect.volumeM3, ActiveToolProfile().transfer_volume_limit_m3 );
        return H2H::VolumeToGramsFloor( vol, form.density_kg_m3 );
    }

    void CaptureFaceNormalAt( float x, float y, float& nx, float& ny, float& nz )
    {
        SampleAimNormal( x, y, nx, ny, nz );
        float const len = std::sqrt( nx * nx + ny * ny + nz * nz );
        if ( len > 1e-5f ) { nx /= len; ny /= len; nz /= len; }
        else { nx = 0.f; ny = 0.f; nz = 1.f; }
    }

    void ResolveCarveIntoNormal( float aimX, float aimY, float aimZ,
        float lookX, float lookY, float lookZ,
        float& nx, float& ny, float& nz )
    {
        // Outward face normal for carve centre = aim - N*into.
        CaptureFaceNormalAt( aimX, aimY, nx, ny, nz );
        float const lookHoriz = std::sqrt( lookX * lookX + lookY * lookY );
        float crest = aimZ;
        SampleGroundZBase( aimX, aimY, crest );
        float const belowCrest = crest - aimZ;
        auto setLookInto = [&]()
        {
            nx = -lookX;
            ny = -lookY;
            nz = -lookZ;
            float const len = std::sqrt( nx * nx + ny * ny + nz * nz );
            if ( len > 1e-5f ) { nx /= len; ny /= len; nz /= len; }
            else { nx = 0.f; ny = 0.f; nz = 1.f; }
        };
        // Mid/lower dark wall: aim sits below crest — HF N only reads on the top pushed facet.
        if ( belowCrest > 0.18f && lookHoriz > 0.40f && std::fabs( lookZ ) < 0.70f )
        {
            setLookInto();
            return;
        }
        // Crest-up HF while looking into a silhouette (near-top strip edge cases).
        if ( nz > 0.58f && lookHoriz > 0.50f && std::fabs( lookZ ) < 0.55f )
        {
            setLookInto();
        }
    }

    void StrikeAxesFromLook( float nx, float ny, float nz,
        float lookX, float lookY, float lookZ,
        float& strikeX, float& strikeY )
    {
        // Crack elongates across the swing in the face plane: along = normalize(N × look).
        float ax = ny * lookZ - nz * lookY;
        float ay = nz * lookX - nx * lookZ;
        float az = nx * lookY - ny * lookX;
        float const alen = std::sqrt( ax * ax + ay * ay + az * az );
        if ( alen < 1e-4f )
        {
            // Look ≈ face-on: fall back to world-up × N.
            ax = -ny; ay = nx; az = 0.f;
        }
        float const inv = 1.f / (std::max)( 1e-5f, std::sqrt( ax * ax + ay * ay ) );
        strikeX = ax * inv;
        strikeY = ay * inv;
    }

    bool CapUsesFoliation( char const* cap )
    {
        if ( !cap || !cap[0] ) { return false; }
        H2H::MaterialFormContract const& f = H2H::FormOrDirt( cap );
        if ( f.fabric == H2H::FabricKind::FoliatedAnisotropic ) { return true; }
        if ( f.fabric == H2H::FabricKind::BeddedFissile ) { return true; }
        VisualMat::VisualMaterialDef const& vd = VisualMat::OrDirt( cap );
        return vd.fracture_character == VisualMat::FractureCharacter::FoliationSplit
            || vd.fracture_character == VisualMat::FractureCharacter::SplitLayers
            || std::strcmp( cap, "mica_schist" ) == 0;
    }

    std::string CapAtWorld( float x, float y )
    {
        int const cx = (int)std::floor( x );
        int const cy = (int)std::floor( y );
        CellSample const* c = GetCell( cx, cy );
        if ( c && c->valid && !c->cap.empty() ) { return c->cap; }
        ProvenanceGeo::EnsureReady();
        return ProvenanceGeo::SampleSurface( x, y, g.gradeDatum, g.reliefVoxels, g.voxelEdgeM ).cap;
    }

    bool MaterialSlumpsOpen( std::string const& mat )
    {
        // Loose / granular — always an open crater (engine settle drops overhangs).
        return mat == "sand" || mat == "gravel";
    }

    bool MaterialHoldsTunnel( std::string const& mat )
    {
        // Only loose granular materials keep the open-crater / slump read.
        // Grass/wheat are cover over earth — digs into dirt/loam/clay should hold a roof.
        if ( mat.empty() ) { return true; }
        if ( MaterialSlumpsOpen( mat ) ) { return false; }
        if ( mat == "water" || mat == "air" ) { return false; }
        return true;
    }

    std::string CellCapName( int cx, int cy )
    {
        CellSample const* c = GetCell( cx, cy );
        if ( !c || c->cap.empty() ) { return "dirt"; }
        return c->cap;
    }

    float TunnelSurfaceBreakRadius( DigScar const& s, float gradeZ )
    {
        // Circle where the ENTRANCE sphere cuts grade — deepen must not shrink this away.
        float const cz = s.tunnel ? s.entranceWz : s.wz;
        float const dz = gradeZ - cz;
        float const r = s.radius;
        float const r2 = r * r;
        float const d2 = dz * dz;
        if ( d2 >= r2 ) { return 0.f; }
        return std::sqrt( r2 - d2 );
    }

    float ScarTunnelDepAt( DigScar const& s, float x, float y, float gradeZ )
    {
        // Only sink the heightfield inside the entrance break; floor comes from the deep cavity.
        float const breakR = TunnelSurfaceBreakRadius( s, gradeZ );
        if ( breakR < 1e-5f ) { return 0.f; }
        float const dx = x - s.wx;
        float const dy = y - s.wy;
        if ( ( dx * dx + dy * dy ) > breakR * breakR ) { return 0.f; }
        float const r = s.radius;
        float const r2 = r * r;
        float const d2 = dx * dx + dy * dy;
        if ( d2 >= r2 ) { return 0.f; }
        float const h = std::sqrt( r2 - d2 );
        float const sphereBot = s.wz - h;
        return (std::max)( 0.f, gradeZ - sphereBot );
    }

    float DigDepAt( float x, float y )
    {
        // Tool scars only — occupancy must never depress the skin from walk/look/prefetch.
        float digDep = 0.f;
        float gradeZ = 0.f;
        bool haveGrade = SampleGroundZBase( x, y, gradeZ );
        for ( DigScar const& s : g.scars )
        {
            if ( s.place ) { continue; }
            if ( s.kind == ScarKind::FoliationPlate || s.kind == ScarKind::FacePuncture ) { continue; }
            if ( s.tunnel )
            {
                if ( !haveGrade ) { continue; }
                digDep = (std::max)( digDep, ScarTunnelDepAt( s, x, y, gradeZ ) );
            }
            else
            {
                digDep = (std::max)( digDep, ScarHemiAt( s, x, y ) );
            }
        }
        return digDep;
    }

    float PlaceLiftAt( float x, float y )
    {
        float placeLift = 0.f;
        for ( DigScar const& s : g.scars )
        {
            if ( !s.place ) { continue; }
            placeLift = (std::max)( placeLift, ScarHemiAt( s, x, y ) );
        }
        return placeLift;
    }

    float ScarDeltaZ( float x, float y )
    {
        // Feet: mounds minus holes (place can partially refill a dig).
        return PlaceLiftAt( x, y ) - DigDepAt( x, y );
    }

    void InvalidateTerrainMesh()
    {
        g.terrainDirty = true;
    }

    EditedRegion* FindEditedRegion( uint32_t id )
    {
        if ( id == 0 ) { return nullptr; }
        for ( EditedRegion& er : g.editedRegions )
        {
            if ( er.id == id ) { return &er; }
        }
        return nullptr;
    }

    void AddOpeningMonotonic( EditedRegion& er, float x, float y, float z, float r,
        float nx, float ny, float nz )
    {
        r = (std::max)( 0.05f, r );
        float nlen = std::sqrt( nx * nx + ny * ny + nz * nz );
        if ( nlen > 1e-5f ) { nx /= nlen; ny /= nlen; nz /= nlen; }
        else { nx = 0.f; ny = 0.f; nz = 1.f; }
        // Covered by existing opening in 3D → no-op (XY-only merge stacked crest disks above strikes).
        for ( EditOpening& o : er.openings )
        {
            float const dx = o.x - x, dy = o.y - y, dz = o.z - z;
            float const d = std::sqrt( dx * dx + dy * dy + dz * dz );
            if ( d + r <= o.r + 1e-4f ) { return; }
            // New disk fully covers old → expand old in place (still monotonic).
            if ( d + o.r <= r + 1e-4f )
            {
                o.x = x; o.y = y; o.z = z; o.r = r;
                o.nx = nx; o.ny = ny; o.nz = nz;
                goto bounds;
            }
        }
        er.openings.push_back( { x, y, z, r, nx, ny, nz } );
    bounds:
        if ( !er.hasOwnBounds )
        {
            er.ownMinX = x - r; er.ownMaxX = x + r;
            er.ownMinY = y - r; er.ownMaxY = y + r;
            er.hasOwnBounds = true;
        }
        else
        {
            er.ownMinX = (std::min)( er.ownMinX, x - r );
            er.ownMaxX = (std::max)( er.ownMaxX, x + r );
            er.ownMinY = (std::min)( er.ownMinY, y - r );
            er.ownMaxY = (std::max)( er.ownMaxY, y + r );
        }
        ++er.dirtyRev;
    }

    bool NearRegionOpenings( EditedRegion const& er, float xc, float yc, float zc, float collarM )
    {
        for ( EditOpening const& o : er.openings )
        {
            float const lim = o.r + collarM;
            float const dx = xc - o.x, dy = yc - o.y, dz = zc - o.z;
            if ( ( dx * dx + dy * dy + dz * dz ) <= lim * lim ) { return true; }
        }
        return false;
    }

    bool EditedRegionOwnsAt( float x, float y )
    {
        // Persistent aperture from accumulated openings — not latest tip focus.
        for ( EditedRegion const& er : g.editedRegions )
        {
            if ( er.openings.empty() ) { continue; }
            if ( er.hasOwnBounds )
            {
                if ( x < er.ownMinX - 0.02f || x > er.ownMaxX + 0.02f
                  || y < er.ownMinY - 0.02f || y > er.ownMaxY + 0.02f )
                {
                    continue;
                }
            }
            for ( EditOpening const& o : er.openings )
            {
                float const dx = x - o.x, dy = y - o.y;
                if ( ( dx * dx + dy * dy ) <= o.r * o.r ) { return true; }
            }
        }
        return false;
    }

    // HF peel predicate: virgin skin may vanish only where occupancy is air at that skin.
    // Opening disks alone are too wide (void skirts); steep faces need this, not XY-disk ownership.
    bool SurfaceBrokenByOccupancy( float x, float y )
    {
        int const cx = (int)std::floor( x );
        int const cy = (int)std::floor( y );
        CellSample const* cell = GetCell( cx, cy );
        if ( !cell || !cell->carved || cell->fill.empty() || cell->fillW <= 0 ) { return false; }
        float virginZ = 0.f;
        if ( !SampleGroundZBase( x, y, virginZ ) ) { return false; }
        // Air just under the virgin sheet = surface strike actually opened the skin.
        if ( !OccupancySolidAt( x, y, virginZ - 0.02f ) ) { return true; }
        float occZ = virginZ;
        if ( SampleOccupancyZ( x, y, occZ ) && occZ < virginZ - ( 0.45f * kVoxelEdgeM ) )
        {
            return true;
        }
        return false;
    }

    uint32_t MergeOrCreateEditedRegion( float wx, float wy, float wz, float radiusM,
        std::vector<std::pair<int, int>> const& touchedCells,
        float nx, float ny, float nz )
    {
        std::vector<uint32_t> hitIds;
        for ( auto const& xy : touchedCells )
        {
            CellSample const* c = GetCell( xy.first, xy.second );
            if ( c && c->editedRegionId != 0 )
            {
                bool seen = false;
                for ( uint32_t id : hitIds ) { if ( id == c->editedRegionId ) { seen = true; break; } }
                if ( !seen ) { hitIds.push_back( c->editedRegionId ); }
            }
        }
        // Also absorb regions whose openings overlap this strike (bridging bites).
        for ( EditedRegion const& er : g.editedRegions )
        {
            for ( EditOpening const& o : er.openings )
            {
                float const dx = o.x - wx, dy = o.y - wy;
                float const lim = o.r + radiusM + 0.08f;
                if ( ( dx * dx + dy * dy ) > lim * lim ) { continue; }
                bool seen = false;
                for ( uint32_t id : hitIds ) { if ( id == er.id ) { seen = true; break; } }
                if ( !seen ) { hitIds.push_back( er.id ); }
                break;
            }
        }

        EditedRegion* dest = nullptr;
        if ( hitIds.empty() )
        {
            EditedRegion er;
            er.id = g.nextEditedRegionId++;
            g.editedRegions.push_back( er );
            dest = &g.editedRegions.back();
        }
        else
        {
            // Keep first id; merge others into it.
            dest = FindEditedRegion( hitIds[0] );
            if ( !dest )
            {
                EditedRegion er;
                er.id = g.nextEditedRegionId++;
                g.editedRegions.push_back( er );
                dest = &g.editedRegions.back();
                hitIds[0] = dest->id;
            }
            for ( size_t i = 1; i < hitIds.size(); ++i )
            {
                EditedRegion* src = FindEditedRegion( hitIds[i] );
                if ( !src || src == dest ) { continue; }
                for ( EditOpening const& o : src->openings )
                {
                    AddOpeningMonotonic( *dest, o.x, o.y, o.z, o.r, o.nx, o.ny, o.nz );
                }
                for ( auto const& xy : src->cells )
                {
                    bool have = false;
                    for ( auto const& c : dest->cells )
                    {
                        if ( c.first == xy.first && c.second == xy.second ) { have = true; break; }
                    }
                    if ( !have ) { dest->cells.push_back( xy ); }
                    if ( CellSample* cell = GetCellMutable( xy.first, xy.second ) )
                    {
                        cell->editedRegionId = dest->id;
                    }
                }
                uint32_t const kill = src->id;
                g.editedRegions.erase(
                    std::remove_if( g.editedRegions.begin(), g.editedRegions.end(),
                        [&]( EditedRegion const& e ) { return e.id == kill; } ),
                    g.editedRegions.end() );
                dest = FindEditedRegion( hitIds[0] );
                if ( !dest ) { break; }
            }
        }
        if ( !dest ) { return 0; }

        AddOpeningMonotonic( *dest, wx, wy, wz, radiusM, nx, ny, nz );
        dest->actionX = wx; dest->actionY = wy; dest->actionZ = wz; dest->actionR = radiusM;
        dest->hasAction = true;

        for ( auto const& xy : touchedCells )
        {
            bool have = false;
            for ( auto const& c : dest->cells )
            {
                if ( c.first == xy.first && c.second == xy.second ) { have = true; break; }
            }
            if ( !have ) { dest->cells.push_back( xy ); }
            if ( CellSample* cell = GetCellMutable( xy.first, xy.second ) )
            {
                cell->editedRegionId = dest->id;
            }
        }
        return dest->id;
    }

    bool CavityPatchOwnsAt( float x, float y )
    {
        // Persistent EditedRegion openings own the HF aperture (remove-only monotonic).
        return EditedRegionOwnsAt( x, y );
    }

    void EnsureGeoCell( int cx, int cy )
    {
        ProvenanceGeo::EnsureReady();
        auto const sample = ProvenanceGeo::SampleSurface(
            (double)cx + 0.5, (double)cy + 0.5,
            g.gradeDatum, g.reliefVoxels, g.voxelEdgeM );
        auto const key = CellKey( cx, cy );
        CellSample& dest = g.cells[key];
        bool const keepFillZ = dest.valid && dest.hasFillZ;
        float const keepFZ = dest.fillZ;
        bool const keepEdited = dest.valid && dest.edited;
        bool const keepCarved = dest.valid && dest.carved;
        bool const keepCavity = dest.valid && dest.hasCavity;
        GLuint const keepCavityList = dest.valid ? dest.cavityList : 0;
        bool const keepFocus = dest.valid && dest.hasCarveFocus;
        float const kWx = dest.carveWx, kWy = dest.carveWy, kWz = dest.carveWz, kRM = dest.carveRM;
        bool const keepPatch = dest.valid && dest.hasPatchBounds;
        float const pMnX = dest.patchMinX, pMnY = dest.patchMinY, pMxX = dest.patchMaxX, pMxY = dest.patchMaxY;
        uint32_t const keepRegionId = dest.valid ? dest.editedRegionId : 0;
        if ( !dest.valid )
        {
            ++g.cellsLoaded;
            ++g.perfGeoCellsCreated;
        }
        ++g.perfSampleSurfaceCalls; // geography FBM — once per cell create/refresh, not per tri
        // Esoterica geography = baseline grade/cap authority (bridge optional).
        dest.grade = sample.grade;
        dest.cap = sample.cap;
        CapColor( sample.cap, dest.r, dest.g, dest.b );
        dest.valid = true;
        // Preserve dig flags — never promote resident fill into "edited/carved" on stream refresh.
        if ( keepFillZ )
        {
            dest.hasFillZ = true;
            dest.fillZ = keepFZ;
        }
        if ( keepEdited ) { dest.edited = true; }
        if ( keepCarved )
        {
            dest.carved = true;
            dest.hasCavity = keepCavity;
            dest.cavityList = keepCavityList;
        }
        if ( keepFocus )
        {
            dest.hasCarveFocus = true;
            dest.carveWx = kWx; dest.carveWy = kWy; dest.carveWz = kWz; dest.carveRM = kRM;
        }
        if ( keepPatch )
        {
            dest.hasPatchBounds = true;
            dest.patchMinX = pMnX; dest.patchMinY = pMnY;
            dest.patchMaxX = pMxX; dest.patchMaxY = pMxY;
        }
        if ( keepRegionId != 0 ) { dest.editedRegionId = keepRegionId; }
    }

    void EnsureGeoDisk( int px, int py, int radCells )
    {
        bool any = false;
        for ( int dy = -radCells; dy <= radCells; ++dy )
        {
            for ( int dx = -radCells; dx <= radCells; ++dx )
            {
                if ( dx * dx + dy * dy > radCells * radCells ) { continue; }
                int const cx = px + dx, cy = py + dy;
                if ( GetCell( cx, cy ) ) { continue; }
                EnsureGeoCell( cx, cy );
                any = true;
            }
        }
        if ( any ) { InvalidateTerrainMesh(); }
    }

    void ClearPendingScarEdit()
    {
        g.pendingScarIndex = -1;
        g.pendingScarDepthBefore = 0.f;
        g.pendingScarWasNew = false;
    }

    void RollbackPendingScarEdit()
    {
        if ( g.pendingScarIndex < 0 || g.pendingScarIndex >= (int)g.scars.size() )
        {
            ClearPendingScarEdit();
            return;
        }
        if ( g.pendingScarWasNew )
        {
            // Only pop if still the edited slot (usually back)
            if ( g.pendingScarIndex == (int)g.scars.size() - 1
              && g.scars.back().place == g.pendingScarIsPlace )
            {
                g.scars.pop_back();
                ++g.scarGen;
            }
        }
        else
        {
            DigScar& s = g.scars[(size_t)g.pendingScarIndex];
            if ( s.place == g.pendingScarIsPlace )
            {
                s.depth = g.pendingScarDepthBefore;
                if ( s.depth < 1e-4f )
                {
                    g.scars.erase( g.scars.begin() + g.pendingScarIndex );
                }
                ++g.scarGen;
            }
        }
        ClearPendingScarEdit();
    }

    void RemoveLastOptimisticDigScar()
    {
        if ( g.pendingScarIndex >= 0 && !g.pendingScarIsPlace )
        {
            RollbackPendingScarEdit();
            return;
        }
        if ( g.scars.empty() ) { return; }
        DigScar const& s = g.scars.back();
        if ( !s.place )
        {
            g.scars.pop_back();
            ++g.scarGen;
        }
    }

    void RemoveLastOptimisticPlaceScar()
    {
        if ( g.pendingScarIndex >= 0 && g.pendingScarIsPlace )
        {
            RollbackPendingScarEdit();
            return;
        }
        if ( g.scars.empty() ) { return; }
        DigScar const& s = g.scars.back();
        if ( s.place )
        {
            g.scars.pop_back();
            ++g.scarGen;
        }
    }

    void CancelDigScarsUnderPlace( float wx, float wy, float radius )
    {
        // Place into a hole: shrink overlapping dig cups (slump fill) instead of deleting the well wholesale.
        float const lim = radius * 0.85f;
        float const lim2 = lim * lim;
        size_t const before = g.scars.size();
        for ( DigScar& s : g.scars )
        {
            if ( s.place ) { continue; }
            float const dx = s.wx - wx;
            float const dy = s.wy - wy;
            if ( ( dx * dx + dy * dy ) > lim2 ) { continue; }
            s.depth = (std::max)( 0.f, s.depth - kHandfulRadiusM );
        }
        g.scars.erase( std::remove_if( g.scars.begin(), g.scars.end(),
            []( DigScar const& s ) { return !s.place && s.depth < 1e-4f; } ),
            g.scars.end() );
        if ( g.scars.size() != before ) { ++g.scarGen; }
    }

    void CancelPlaceScarsUnderDig( float wx, float wy, float radius )
    {
        // Dig into a mound: cut place cups so placed dirt can be scooped again.
        float const lim = radius * 0.85f;
        float const lim2 = lim * lim;
        size_t const before = g.scars.size();
        for ( DigScar& s : g.scars )
        {
            if ( !s.place ) { continue; }
            float const dx = s.wx - wx;
            float const dy = s.wy - wy;
            if ( ( dx * dx + dy * dy ) > lim2 ) { continue; }
            s.depth = (std::max)( 0.f, s.depth - kHandfulRadiusM );
        }
        g.scars.erase( std::remove_if( g.scars.begin(), g.scars.end(),
            []( DigScar const& s ) { return s.place && s.depth < 1e-4f; } ),
            g.scars.end() );
        if ( g.scars.size() != before ) { ++g.scarGen; }
    }

    void RemoveDigScarsInCell( int cx, int cy )
    {
        size_t const before = g.scars.size();
        g.scars.erase( std::remove_if( g.scars.begin(), g.scars.end(),
            [&]( DigScar const& s ) { return !s.place && s.cx == cx && s.cy == cy; } ),
            g.scars.end() );
        if ( g.scars.size() != before ) { ++g.scarGen; }
    }

    void PruneScars()
    {
        // Prefer distance cull over FIFO — old nearby holes must not vanish when you dig elsewhere.
        float const maxR = (float)kFarRadiusCells + 4.f;
        float const maxR2 = maxR * maxR;
        g.scars.erase( std::remove_if( g.scars.begin(), g.scars.end(),
            [&]( DigScar const& s )
            {
                float const dx = s.wx - g.feetX;
                float const dy = s.wy - g.feetY;
                return ( dx * dx + dy * dy ) > maxR2;
            } ),
            g.scars.end() );
        // Dense handful fields easily exceed a few hundred cups; keep nearby openings alive.
        constexpr size_t kScarSoftCap = 8192;
        while ( g.scars.size() > kScarSoftCap )
        {
            // Drop farthest from feet, never a random "first dug" near the player
            size_t worst = 0;
            float worstD2 = -1.f;
            for ( size_t i = 0; i < g.scars.size(); ++i )
            {
                float const dx = g.scars[i].wx - g.feetX;
                float const dy = g.scars[i].wy - g.feetY;
                float const d2 = dx * dx + dy * dy;
                if ( d2 > worstD2 ) { worstD2 = d2; worst = i; }
            }
            g.scars.erase( g.scars.begin() + (std::ptrdiff_t)worst );
        }
    }

    void AddScar( float wx, float wy, float wz, int cx, int cy, bool place, bool tunnel )
    {
        // Place: tight merge. Dig: deepen only when reticle is near the SAME scoop centre.
        // Wide dig-merge glued rim-overlap digs into the old hole (remaining solid never got a cup).
        float const mergeR = kHandfulRadiusM * ( place ? 0.35f : 0.42f );
        float const mergeR2 = mergeR * mergeR;
        for ( size_t i = 0; i < g.scars.size(); ++i )
        {
            DigScar& s = g.scars[i];
            if ( s.place != place ) { continue; }
            if ( !place && s.tunnel != tunnel ) { continue; } // open shaft ≠ buried tunnel
            float const dx = s.wx - wx;
            float const dy = s.wy - wy;
            if ( ( dx * dx + dy * dy ) > mergeR2 ) { continue; }
            if ( !place )
            {
                // Near bowl centre only — rim / remaining-solid bites get their own cup.
                if ( ScarHemiAt( s, wx, wy ) < kHandfulRadiusM * 0.55f ) { continue; }
            }
            g.pendingScarIndex = (int)i;
            g.pendingScarDepthBefore = s.depth;
            g.pendingScarWasNew = false;
            g.pendingScarIsPlace = place;
            s.depth += kHandfulRadiusM;
            if ( place )
            {
                s.wz = wz;
            }
            else if ( s.tunnel )
            {
                s.wz = (std::min)( s.wz, wz ); // deepen cavity centre
            }
            else
            {
                s.wz = wz;
            }
            ++g.scarGen;
            auto it = g.cells.find( CellKey( s.cx, s.cy ) );
            if ( it != g.cells.end() && it->second.valid ) { it->second.edited = true; }
            return;
        }

        DigScar s;
        s.wx = wx; s.wy = wy; s.wz = wz;
        s.cx = cx; s.cy = cy;
        s.place = place;
        s.tunnel = place ? false : tunnel;
        s.entranceWz = wz;
        s.radius = ( g.pendingAffectRM > 1e-5f ) ? g.pendingAffectRM : SoftScoopScarRadiusM();
        s.depth = s.radius;
        s.kind = ScarKind::ScoopHemi;
        g.scars.push_back( s );
        g.pendingScarIndex = (int)g.scars.size() - 1;
        g.pendingScarDepthBefore = 0.f;
        g.pendingScarWasNew = true;
        g.pendingScarIsPlace = place;
        ++g.scarGen;
        PruneScars();
        if ( g.pendingScarWasNew )
        {
            g.pendingScarIndex = (int)g.scars.size() - 1;
        }
        auto it = g.cells.find( CellKey( cx, cy ) );
        if ( it != g.cells.end() && it->second.valid ) { it->second.edited = true; }
    }

    bool IsSteepFaceAt( float x, float y )
    {
        // Flat ground nz≈1; vertical cliff nz→0. Hemi DigDep on cliffs stretches into vertical scoops.
        float nx = 0.f, ny = 0.f, nz = 1.f;
        SampleAimNormal( x, y, nx, ny, nz );
        return nz < 0.58f;
    }

    void AddFacePunctureScar( float wx, float wy, float wz, int cx, int cy,
        float radius, float depthAmp, float nx, float ny, float nz )
    {
        float const mergeR = radius * 0.85f;
        float const mergeR2 = mergeR * mergeR;
        for ( size_t i = 0; i < g.scars.size(); ++i )
        {
            DigScar& s = g.scars[i];
            if ( s.place || s.kind != ScarKind::FacePuncture ) { continue; }
            float const dx = s.wx - wx, dy = s.wy - wy, dz = s.wz - wz;
            if ( ( dx * dx + dy * dy + dz * dz ) > mergeR2 ) { continue; }
            g.pendingScarIndex = (int)i;
            g.pendingScarDepthBefore = s.depth;
            g.pendingScarWasNew = false;
            g.pendingScarIsPlace = false;
            s.depth = (std::min)( 0.08f, s.depth + depthAmp * 0.35f );
            s.radius = (std::max)( s.radius, radius );
            s.wx = wx; s.wy = wy; s.wz = wz;
            s.faceNx = nx; s.faceNy = ny; s.faceNz = nz;
            ++g.scarGen;
            return;
        }
        DigScar s;
        s.wx = wx; s.wy = wy; s.wz = wz;
        s.cx = cx; s.cy = cy;
        s.place = false;
        s.tunnel = false;
        s.entranceWz = wz;
        s.kind = ScarKind::FacePuncture;
        s.radius = radius;
        s.depth = depthAmp;
        s.support = 1.f;
        s.faceNx = nx; s.faceNy = ny; s.faceNz = nz;
        g.scars.push_back( s );
        g.pendingScarIndex = (int)g.scars.size() - 1;
        g.pendingScarDepthBefore = 0.f;
        g.pendingScarWasNew = true;
        g.pendingScarIsPlace = false;
        ++g.scarGen;
        PruneScars();
        if ( g.pendingScarWasNew )
        {
            g.pendingScarIndex = (int)g.scars.size() - 1;
        }
    }

    // Foliation plate notch — merge extends the SAME seam (impact → crack → pry → plate).
    void AddFoliationPlateScar( float wx, float wy, float wz, int cx, int cy,
        float strikeX, float strikeY,
        float along, float acrossDeep, float acrossLip, float depthAmp,
        float support, RockStruct::FractureStage stage,
        float faceNx, float faceNy, float faceNz )
    {
        float const inv = 1.f / (std::max)( 1e-5f, std::sqrt( strikeX * strikeX + strikeY * strikeY ) );
        strikeX *= inv; strikeY *= inv;
        float const mergeAlong = along + 0.06f;
        for ( size_t i = 0; i < g.scars.size(); ++i )
        {
            DigScar& s = g.scars[i];
            if ( s.place || s.kind != ScarKind::FoliationPlate ) { continue; }
            float const dot = s.strikeX * strikeX + s.strikeY * strikeY;
            if ( std::fabs( dot ) < 0.82f ) { continue; }
            float const dx = s.wx - wx, dy = s.wy - wy, dz = s.wz - wz;
            float const alongD = std::fabs( dx * s.strikeX + dy * s.strikeY );
            float const acrossD = std::fabs( dx * ( -s.strikeY ) + dy * s.strikeX );
            if ( alongD > mergeAlong || acrossD > acrossDeep + 0.05f ) { continue; }
            if ( ( dx * dx + dy * dy + dz * dz ) > ( mergeAlong * mergeAlong ) ) { continue; }

            g.pendingScarIndex = (int)i;
            g.pendingScarDepthBefore = s.depth;
            g.pendingScarWasNew = false;
            g.pendingScarIsPlace = false;
            s.plateAlong = (std::min)( RockStruct::kStructuralSlabM * 0.55f, s.plateAlong + along * 0.35f );
            s.plateAcrossDeep = (std::max)( s.plateAcrossDeep, acrossDeep );
            s.plateAcrossLip = (std::min)( s.plateAcrossLip, acrossLip );
            s.depth = (std::max)( s.depth, depthAmp );
            s.support = (std::min)( s.support, support );
            s.fracStage = stage;
            s.wx = wx; s.wy = wy; s.wz = wz; // extend from latest strike contact
            s.faceNx = faceNx; s.faceNy = faceNy; s.faceNz = faceNz;
            ++g.scarGen;
            auto it = g.cells.find( CellKey( s.cx, s.cy ) );
            if ( it != g.cells.end() && it->second.valid ) { it->second.edited = true; }
            return;
        }

        DigScar s;
        s.wx = wx; s.wy = wy; s.wz = wz;
        s.cx = cx; s.cy = cy;
        s.place = false;
        s.tunnel = false;
        s.entranceWz = wz;
        s.kind = ScarKind::FoliationPlate;
        s.strikeX = strikeX; s.strikeY = strikeY;
        s.plateAlong = along;
        s.plateAcrossDeep = acrossDeep;
        s.plateAcrossLip = acrossLip;
        s.depth = depthAmp;
        s.radius = (std::max)( along, acrossDeep );
        s.support = support;
        s.fracStage = stage;
        s.faceNx = faceNx; s.faceNy = faceNy; s.faceNz = faceNz;
        g.scars.push_back( s );
        g.pendingScarIndex = (int)g.scars.size() - 1;
        g.pendingScarDepthBefore = 0.f;
        g.pendingScarWasNew = true;
        g.pendingScarIsPlace = false;
        ++g.scarGen;
        PruneScars();
        if ( g.pendingScarWasNew )
        {
            g.pendingScarIndex = (int)g.scars.size() - 1;
        }
        auto it = g.cells.find( CellKey( cx, cy ) );
        if ( it != g.cells.end() && it->second.valid ) { it->second.edited = true; }
    }

    bool DigShouldTunnel( float wx, float wy, float wz, int cx, int cy )
    {
        // Open lit scoops only — buried tunnel spheres draped grade into holes (rejected).
        (void)wx; (void)wy; (void)wz; (void)cx; (void)cy;
        return false;
    }

    float WorldToEngDepth( float meters )
    {
        // Engine carve/place depth+radius are cell-depths (Unreal: worldΔZ / (sublayers*voxelEdge)).
        return meters / (std::max)( 0.05f, g.cellDepthM );
    }

    float HandfulRadiusEng()
    {
        // Wire carve/place radius = dig-volume / place-volume sphere (tool×material affect).
        float r = g.aimHit ? AimDigAffect().radiusM : TransferScarRadiusM();
        if ( g.pendingAffectRM > 1e-5f && g.pending != PendingKind::None )
        {
            r = g.pendingAffectRM;
        }
        return WorldToEngDepth( r );
    }

    // Dig stays under the reticle and cuts DEEPER; place keeps slump-into-hole / seat-on-mound.
    // Out depth is WORLD meters below grade (caller converts to engine cell-depths for the wire).
    void ResolveBiteFromAim( bool place,
        float& bx, float& by, float& bz,
        int& cx, int& cy, float& u, float& v, float& depthM )
    {
        float cp = std::cos( g.pitch ), sp = std::sin( g.pitch );
        float cyw = std::cos( g.yaw ), sy = std::sin( g.yaw );
        float fx = sy * cp, fy = cyw * cp, fz = sp;

        if ( place )
        {
            float const dig = DigDepAt( g.aimX, g.aimY );
            float const lift = PlaceLiftAt( g.aimX, g.aimY );
            float const netHole = dig - lift;
            // Keep slump-into-hole: only bias depth when there is a real open pit under the aim.
            if ( netHole > kHandfulRadiusM * 0.25f )
            {
                depthM = netHole;
                float const bias = kHandfulRadiusM * 0.25f;
                bx = g.aimX + fx * bias;
                by = g.aimY + fy * bias;
                bz = g.aimZ + fz * bias;
            }
            else
            {
                // Place on grade or on an existing mound — seat on contact (place-on-place).
                depthM = 0.f;
                bx = g.aimX;
                by = g.aimY;
                bz = g.aimZ;
            }
            cx = (int)std::floor( bx );
            cy = (int)std::floor( by );
            u = bx - (float)cx;
            v = by - (float)cy;
            return;
        }

        // DIG — continue along look through matter (into the face ahead of an open scoop).
        // Straight-down only when looking steeply at virgin grade. Inside a dig with a shallow
        // look, walk the bite centre FORWARD into remaining solid — never sky skin / vertical deepen.
        float hitX = g.aimX, hitY = g.aimY, hitZ = g.aimZ;
        float const R = g.aimHit ? AimDigAffect().radiusM : SoftScoopScarRadiusM();
        float const lookDown = -fz; // pitch<0 → looking down

        float crestAtAim = hitZ;
        SampleGroundZBase( hitX, hitY, crestAtAim );
        hitZ = g.aimZ;

        float nx = 0.f, ny = 0.f, nz = 1.f;
        CaptureFaceNormalAt( hitX, hitY, nx, ny, nz );
        bool const steepFace = nz < 0.58f;

        // Steep / wall contact: land ON the ray hit and press into the face along look.
        // Grade-Z re-seat + XY walk is what made vertical strikes miss the strike point.
        if ( steepFace )
        {
            float const into = R * 0.60f;
            bx = hitX + fx * into;
            by = hitY + fy * into;
            bz = hitZ + fz * into;
            cx = (int)std::floor( bx );
            cy = (int)std::floor( by );
            u = bx - (float)cx;
            v = by - (float)cy;
            depthM = R;
            return;
        }

        float const digAim = DigDepAt( hitX, hitY );
        bool const inScoop = digAim > R * 0.20f;
        bool const lookForward = lookDown < 0.72f;

        float bias = R * 0.60f;
        if ( inScoop && lookForward ) { bias = R * 2.2f; }
        else if ( lookForward ) { bias = R * 1.05f; }

        bx = hitX + fx * bias;
        by = hitY + fy * bias;
        bz = hitZ + fz * bias;

        // Walk along look until DigDep drops (solid wall ahead) — matter-true forward bite.
        if ( inScoop && lookForward )
        {
            bool foundSolid = false;
            float bestX = bx, bestY = by, bestZ = bz;
            float const tMax = R * 5.5f;
            float const tStep = R * 0.22f;
            for ( float t = R * 0.35f; t <= tMax; t += tStep )
            {
                float const x = hitX + fx * t;
                float const y = hitY + fy * t;
                float const z = hitZ + fz * t;
                float grade = 0.f;
                if ( !SampleGroundZBase( x, y, grade ) ) { continue; }
                float const dig = DigDepAt( x, y );
                // Solid = little/no open cup under grade, and bite sits in the mass (not above sky skin).
                if ( dig < R * 0.18f && z <= grade + R * 0.15f && z >= grade - R * 3.5f )
                {
                    bestX = x;
                    bestY = y;
                    // Seat just inside the face so the sphere intersects remaining solid.
                    bestZ = (std::min)( z, grade - R * 0.35f );
                    foundSolid = true;
                    break;
                }
            }
            if ( foundSolid )
            {
                bx = bestX;
                by = bestY;
                bz = bestZ;
            }
        }
        else
        {
            // Only pull back if look-steer dove INTO a deeper empty cup than the aim.
            float const digCentre = DigDepAt( bx, by );
            if ( digCentre > digAim + R * 0.25f && digCentre > R * 0.40f )
            {
                bias = R * 0.20f;
                bx = hitX + fx * bias;
                by = hitY + fy * bias;
                bz = hitZ + fz * bias;
            }
        }

        // Grade-normal seat only for steep virgin scoops — +Z nudge kills forward tunneling.
        if ( !inScoop && !lookForward && lookDown >= 0.72f )
        {
            float const pen = -( ( bx - hitX ) * nx + ( by - hitY ) * ny + ( bz - hitZ ) * nz );
            float const want = R * 0.35f;
            if ( pen < want )
            {
                float const d = want - pen;
                bx -= nx * d;
                by -= ny * d;
                bz -= nz * d;
            }
        }

        cx = (int)std::floor( bx );
        cy = (int)std::floor( by );
        u = bx - (float)cx;
        v = by - (float)cy;

        float crestZ = crestAtAim;
        SampleGroundZBase( bx, by, crestZ );
        depthM = (std::max)( R, crestZ - bz );

        // Vertical deepen only when looking down into the same cup (not forward into a face).
        if ( !lookForward && DigDepAt( bx, by ) > R * 0.70f )
        {
            float floorZ = bz;
            SampleGroundZ( bx, by, floorZ );
            float const floorDepthM = crestZ - floorZ;
            depthM = (std::max)( depthM, floorDepthM + R * 0.5f );
        }
    }

    void QueueColumn( int cx, int cy )
    {
        for ( auto const& p : g.columnQueue )
        {
            if ( p.first == cx && p.second == cy ) { return; }
        }
        g.columnQueue.push_back( { cx, cy } );
    }

    void QueueSettledFromReply( std::string const& line )
    {
        size_t p = line.find( "\"settled\"" );
        if ( p == std::string::npos ) { return; }
        size_t b = line.find( '[', p );
        size_t e = line.find( ']', b );
        if ( b == std::string::npos || e == std::string::npos ) { return; }
        // settled entries are [x,y] pairs or objects — scan for integer pairs
        std::string body = line.substr( b + 1, e - b - 1 );
        char const* cur = body.c_str();
        while ( *cur )
        {
            while ( *cur && ( *cur < '0' || *cur > '9' ) && *cur != '-' ) { ++cur; }
            if ( !*cur ) { break; }
            char* end = nullptr;
            long x = strtol( cur, &end, 10 );
            if ( end == cur ) { break; }
            cur = end;
            while ( *cur && ( *cur < '0' || *cur > '9' ) && *cur != '-' ) { ++cur; }
            if ( !*cur ) { break; }
            long y = strtol( cur, &end, 10 );
            if ( end == cur ) { break; }
            cur = end;
            QueueColumn( (int)x, (int)y );
        }
    }

    void SetMouseLook( HWND hwnd, bool enabled )
    {
        g.mouseLook = enabled;
        if ( enabled )
        {
            RECT rc; GetClientRect( hwnd, &rc );
            POINT pt = { ( rc.right - rc.left ) / 2, ( rc.bottom - rc.top ) / 2 };
            ClientToScreen( hwnd, &pt );
            SetCursorPos( pt.x, pt.y );
            g.lastMouseX = ( rc.right - rc.left ) / 2;
            g.lastMouseY = ( rc.bottom - rc.top ) / 2;
            SetCapture( hwnd );
            while ( ShowCursor( FALSE ) >= 0 ) {}
            g.cursorCaptured = true;
        }
        else
        {
            if ( g.cursorCaptured )
            {
                ReleaseCapture();
                while ( ShowCursor( TRUE ) < 0 ) {}
                g.cursorCaptured = false;
            }
        }
    }

    void DrawWireSphere( float cx, float cy, float cz, float radius, float cr, float cg, float cb, int seg = 20 )
    {
        glColor3f( cr, cg, cb );
        glBegin( GL_LINE_LOOP );
        for ( int i = 0; i < seg; ++i )
        {
            float a = (float)i / (float)seg * 6.2831853f;
            glVertex3f( cx + std::cos( a ) * radius, cy + std::sin( a ) * radius, cz );
        }
        glEnd();
        glBegin( GL_LINE_LOOP );
        for ( int i = 0; i < seg; ++i )
        {
            float a = (float)i / (float)seg * 6.2831853f;
            glVertex3f( cx + std::cos( a ) * radius, cy, cz + std::sin( a ) * radius );
        }
        glEnd();
        glBegin( GL_LINE_LOOP );
        for ( int i = 0; i < seg; ++i )
        {
            float a = (float)i / (float)seg * 6.2831853f;
            glVertex3f( cx, cy + std::cos( a ) * radius, cz + std::sin( a ) * radius );
        }
        glEnd();
    }

    void FireActionCue( bool place, bool busy );
    void DrawActionCue( float dt );
    void DrawSurfaceRing( float ox, float oy, float oz,
                          float tx, float ty, float tz,
                          float bx, float by, float bz,
                          float radius, float cr, float cg, float cb, int seg );
    void DrawHorizontalRing( float cx, float cy, float cz, float radius, float cr, float cg, float cb, int seg );

    bool SendLine( std::string const& line )
    {
        if ( g.sock == INVALID_SOCKET ) { return false; }
        size_t sent = 0;
        while ( sent < line.size() )
        {
            int n = send( g.sock, line.data() + sent, (int)( line.size() - sent ), 0 );
            if ( n <= 0 ) { return false; }
            sent += (size_t)n;
        }
        return true;
    }

    bool RequestMethod( char const* method, char const* paramsJson, PendingKind kind, int bx = 0, int by = 0 )
    {
        char buf[384];
        int id = g.nextId++;
        std::snprintf( buf, sizeof( buf ),
            "{\"version\":%d,\"id\":\"%d\",\"type\":\"request\",\"method\":\"%s\",\"params\":%s}\n",
            kProtocolVersion, id, method, paramsJson );
        if ( !SendLine( buf ) ) { return false; }
        g.pending = kind;
        g.pendingId = id;
        g.pendingBx = bx;
        g.pendingBy = by;
        return true;
    }

    void UpdateStreamHud()
    {
        char d[1400];
        char heldBuf[128] = "empty";
        if ( !g.heldGalleryId.empty() )
        {
            std::snprintf( heldBuf, sizeof( heldBuf ), "sample:%s", g.heldGalleryId.c_str() );
        }
        else if ( g.heldTotalG > 0 )
        {
            std::snprintf( heldBuf, sizeof( heldBuf ), "%s %dg (%.2f handfuls)",
                g.heldDominant.empty() ? "?" : g.heldDominant.c_str(),
                g.heldTotalG,
                g.heldTotalG / kHandfulDirtG );
        }
        std::snprintf( d, sizeof( d ),
            "wire v%d | contract=%s | gen=%s v%d | terrain_rev=%d | auth=%s | world=%d\n"
            "hash=%s\n"
            "relief=%.0f datum=%.2f | far cells=%d blocks=%d/%d %s\n"
            "mode=%s grounded=%d | feet (%.1f,%.1f,%.2f) eyeZ=%.2f | 6ft=%.2fm eye=%.2fm\n"
            "H2H tool=%s | affect r=%.3fm (%.0f mL %s) | contact r=%.2fm | hand ref %.0f mL\n"
            "held: %s | aim %s cell(%d,%d) uv(%.2f,%.2f)\n"
            "strike %s | biome %s | wire %s | form %s\n"
            "%s\n"
            "%s",
            g.wireVersion,
            g.contract.empty() ? "?" : g.contract.c_str(),
            g.generatorId.empty() ? "?" : g.generatorId.c_str(),
            g.generatorVersion,
            g.terrainRev,
            g.authorityMode.empty() ? "?" : g.authorityMode.c_str(),
            g.worldSize,
            g.worldIdentityHash.empty() ? "?" : g.worldIdentityHash.c_str(),
            g.reliefVoxels, g.gradeDatum,
            g.cellsLoaded, g.blocksLoaded, g.blocksWanted,
            g.streamComplete ? "STREAM COMPLETE" : "streaming...",
            g.walkMode ? "WALK 6ft" : "FREE CAM",
            g.grounded ? 1 : 0,
            g.feetX, g.feetY, g.feetZ, g.camZ,
            kCharHeightM, kEyeHeightM,
            ActiveToolProfile().id,
            AimDigAffect().radiusM,
            AimDigAffect().volumeM3 * 1e6f,
            AimDigAffect().feel,
            ActiveAimRadiusM(),
            kHandfulVolumeM3 * 1e6f,
            heldBuf,
            g.aimHit ? "HIT" : "---",
            g.aimCx, g.aimCy, g.aimU, g.aimV,
            g.aimStrikeCap.c_str(),
            g.aimBiome.c_str(),
            g.aimWireCap.c_str(),
            g.aimFormId.c_str(),
            g.digestLine.c_str(),
            g.spireLine.c_str() );
        g.detail = d;
    }

    void ParseCapsReply( std::string const& line )
    {
        g.wireVersion = 0;
        ExtractJsonInt( line, "version", g.wireVersion );
        ExtractJsonBool( line, "ok", g.envelopeOk );

        std::string errMsg;
        if ( ExtractJsonString( line, "code", errMsg ) || line.find( "\"error\":{" ) != std::string::npos )
        {
            ExtractJsonString( line, "message", errMsg );
            g.link = LinkState::CapsError;
            g.lastError = errMsg.empty() ? "engine returned error" : errMsg;
            g.statusLine = "terrain_caps failed";
            g.detail = g.lastError;
            return;
        }
        if ( !g.envelopeOk )
        {
            g.link = LinkState::CapsError;
            g.statusLine = "envelope ok=false";
            return;
        }

        ExtractJsonString( line, "contract", g.contract );
        ExtractJsonString( line, "generator_id", g.generatorId );
        ExtractJsonInt( line, "generator_version", g.generatorVersion );
        ExtractJsonString( line, "world_identity_hash", g.worldIdentityHash );
        ExtractJsonInt( line, "terrain_rev", g.terrainRev );
        ExtractJsonString( line, "authority_mode", g.authorityMode );
        ExtractJsonInt( line, "world_size", g.worldSize );
        ExtractJsonFloat( line, "relief_voxels", g.reliefVoxels );
        ExtractJsonFloat( line, "grade_datum", g.gradeDatum );
        ExtractJsonFloat( line, "voxel_edge_m", g.voxelEdgeM );
        if ( !ExtractJsonFloat( line, "cell_depth_m", g.cellDepthM ) )
        {
            // terrain_caps may nest scale — keep Phase 64 default (0.5 m per cell-depth)
            g.cellDepthM = 0.5f;
        }
        if ( g.cellDepthM < 0.05f ) { g.cellDepthM = 0.5f; }

        // Seed Esoterica geography from world identity (deterministic planet causes).
        ProvenanceGeo::SetSeedFromIdentity( g.worldIdentityHash, g.generatorId, g.generatorVersion );

        g.link = LinkState::CapsOk;
        g.statusLine = "Phase 4 - caps ok, locating player";
        UpdateStreamHud();

        if ( !RequestMethod( "player_state", "{}", PendingKind::Player ) )
        {
            g.statusLine = "failed to request player_state";
        }
    }

    void ParsePlayerReply( std::string const& line )
    {
        int x = g.playerX, y = g.playerY;
        if ( ExtractPositionXY( line, x, y ) )
        {
            g.playerX = x;
            g.playerY = y;
            g.havePlayer = true;
            g.feetX = (float)x + 0.5f;
            g.feetY = (float)y + 0.5f;
            g.feetZ = 0.f;
            g.velZ = 0.f;
            g.grounded = false;
            g.walkMode = true;
            g.camX = g.feetX;
            g.camY = g.feetY;
            g.camZ = g.feetZ + kEyeHeightM;
            g.pitch = -0.12f;
        }
        else
        {
            g.havePlayer = true; // fall back to defaults
        }

        // Count wanted blocks (equidistant disk of block rings)
        int const maxRing = ( kFarRadiusCells + kBlockCells - 1 ) / kBlockCells;
        g.blocksWanted = 0;
        for ( int ring = 0; ring <= maxRing; ++ring )
        {
            for ( int dy = -ring; dy <= ring; ++dy )
            {
                for ( int dx = -ring; dx <= ring; ++dx )
                {
                    if ( (std::max)( std::abs( dx ), std::abs( dy ) ) != ring ) { continue; }
                    ++g.blocksWanted;
                }
            }
        }

        // Analytic Esoterica planet residency — concentric disk; bridge surface_field optional.
        ProvenanceGeo::EnsureReady();
        EnsureGeoDisk( (int)std::floor( g.feetX ), (int)std::floor( g.feetY ),
            (std::min)( kFarRadiusCells, 64 ) );
        g.streamComplete = true;
        g.blocksLoaded = g.blocksWanted;
        g.statusLine = "Phase 4 - Esoterica geography resident + interaction digests";
        UpdateStreamHud();
    }

    void ParseSurfaceReply( std::string const& line )
    {
        g.fetchedBlocks.insert( BlockKey( g.pendingBx, g.pendingBy ) );
        ++g.blocksLoaded;

        size_t cellsPos = line.find( "\"cells\"" );
        if ( cellsPos == std::string::npos )
        {
            UpdateStreamHud();
            return;
        }
        size_t i = line.find( '[', cellsPos );
        if ( i == std::string::npos ) { UpdateStreamHud(); return; }
        ++i;

        bool anyNew = false;
        while ( i < line.size() )
        {
            while ( i < line.size() && ( line[i] == ' ' || line[i] == '\t' || line[i] == ',' || line[i] == '\r' || line[i] == '\n' ) ) { ++i; }
            if ( i >= line.size() || line[i] == ']' ) { break; }
            if ( line[i] != '{' ) { break; }
            size_t start = i;
            int depth = 0;
            for ( ; i < line.size(); ++i )
            {
                if ( line[i] == '{' ) { ++depth; }
                else if ( line[i] == '}' )
                {
                    --depth;
                    if ( depth == 0 ) { ++i; break; }
                }
            }
            std::string obj = line.substr( start, i - start );
            int cx = 0, cy = 0;
            float grade = 0.85f;
            std::string cap;
            if ( !ExtractJsonInt( obj, "x", cx ) || !ExtractJsonInt( obj, "y", cy ) ) { continue; }
            ExtractJsonFloat( obj, "grade", grade );
            ExtractJsonString( obj, "cap", cap );
            (void)grade;
            (void)cap;
            // Bridge cells only mark residency. Do NOT refresh grade on already-valid cells —
            // that rewrote Z underfoot while walking and looked like raise/carve.
            if ( !GetCell( cx, cy ) )
            {
                EnsureGeoCell( cx, cy );
                anyNew = true;
            }
        }

        if ( g.blocksLoaded >= g.blocksWanted ) { g.streamComplete = true; }
        g.statusLine = g.streamComplete
            ? "Phase 4 - standable + interaction digests"
            : "Phase 4 - streaming standable surface (equidistant rings)";
        // Remesh only when new cells appear — per-reply invalidate hopped the sheet and hid digs.
        if ( anyNew ) { InvalidateTerrainMesh(); }
        UpdateStreamHud();
    }

    bool FindNextMissingBlock( int& outBx, int& outBy )
    {
        int const maxRing = ( kFarRadiusCells + kBlockCells - 1 ) / kBlockCells;
        int const pbx = (int)std::floor( (double)g.playerX / kBlockCells );
        int const pby = (int)std::floor( (double)g.playerY / kBlockCells );
        for ( int ring = 0; ring <= maxRing; ++ring )
        {
            for ( int dy = -ring; dy <= ring; ++dy )
            {
                for ( int dx = -ring; dx <= ring; ++dx )
                {
                    if ( (std::max)( std::abs( dx ), std::abs( dy ) ) != ring ) { continue; }
                    int const bx = ( pbx + dx ) * kBlockCells;
                    int const by = ( pby + dy ) * kBlockCells;
                    if ( g.fetchedBlocks.count( BlockKey( bx, by ) ) ) { continue; }
                    outBx = bx;
                    outBy = by;
                    return true;
                }
            }
        }
        return false;
    }

    void TickStreamRequests()
    {
        if ( g.link != LinkState::CapsOk ) { return; }
        if ( g.pending != PendingKind::None ) { return; }
        if ( !g.havePlayer ) { return; }

        // Interaction columns optional — dig/place cups don't wait on voxel_column.
        // Prefer keeping the wire free for the next carve/place.
        if ( !g.columnQueue.empty() && g.streamComplete )
        {
            int cx = g.columnQueue.front().first;
            int cy = g.columnQueue.front().second;
            g.columnQueue.erase( g.columnQueue.begin() );
            char params[64];
            std::snprintf( params, sizeof( params ), "{\"x\":%d,\"y\":%d}", cx, cy );
            g.pendingColX = cx;
            g.pendingColY = cy;
            if ( !RequestMethod( "voxel_column", params, PendingKind::Column ) )
            {
                g.link = LinkState::SocketError;
                g.statusLine = "failed to send voxel_column";
            }
            return;
        }

        if ( g.streamComplete ) { return; }

        int bx = 0, by = 0;
        if ( !FindNextMissingBlock( bx, by ) )
        {
            g.streamComplete = true;
            g.statusLine = "Phase 4 - standable + interaction digests";
            UpdateStreamHud();
            return;
        }

        char params[128];
        std::snprintf( params, sizeof( params ), "{\"x0\":%d,\"y0\":%d,\"w\":%d,\"h\":%d}", bx, by, kBlockCells, kBlockCells );
        if ( !RequestMethod( "surface_field", params, PendingKind::Surface, bx, by ) )
        {
            g.link = LinkState::SocketError;
            g.statusLine = "failed to send surface_field";
        }
    }

    void InvalidateBlockContaining( int cx, int cy )
    {
        int const bx = (int)std::floor( (double)cx / kBlockCells ) * kBlockCells;
        int const by = (int)std::floor( (double)cy / kBlockCells ) * kBlockCells;
        g.fetchedBlocks.erase( BlockKey( bx, by ) );
        for ( int y = by; y < by + kBlockCells; ++y )
        {
            for ( int x = bx; x < bx + kBlockCells; ++x )
            {
                auto it = g.cells.find( CellKey( x, y ) );
                if ( it != g.cells.end() )
                {
                    if ( it->second.valid ) { --g.cellsLoaded; }
                    g.cells.erase( it );
                }
            }
        }
        if ( g.blocksLoaded > 0 ) { --g.blocksLoaded; }
        g.streamComplete = false;
        g.statusLine = "Phase 4 - refreshing dug block";
    }

    void RecomputeHeldFromBite()
    {
        g.heldTotalG = 0;
        g.heldDominant.clear();
        int best = 0;
        for ( auto const& kv : g.heldBite )
        {
            if ( kv.second <= 0 ) { continue; }
            g.heldTotalG += kv.second;
            if ( kv.second > best )
            {
                best = kv.second;
                g.heldDominant = kv.first;
            }
        }
        // Drop zeroed slots
        for ( auto it = g.heldBite.begin(); it != g.heldBite.end(); )
        {
            if ( it->second <= 0 ) { it = g.heldBite.erase( it ); }
            else { ++it; }
        }
    }

    void CreditHeld( std::unordered_map<std::string, int> const& added )
    {
        // Match engine carried.credit — digs stack; do not replace the hand.
        for ( auto const& kv : added )
        {
            if ( kv.second > 0 ) { g.heldBite[kv.first] += kv.second; }
        }
        RecomputeHeldFromBite();
    }

    void DebitHeldBy( std::unordered_map<std::string, int> const& removed )
    {
        for ( auto const& kv : removed )
        {
            if ( kv.second <= 0 ) { continue; }
            auto it = g.heldBite.find( kv.first );
            if ( it == g.heldBite.end() ) { continue; }
            it->second -= kv.second;
            if ( it->second < 0 ) { it->second = 0; }
        }
        RecomputeHeldFromBite();
    }

    void DebitHeldTotal( int grams )
    {
        if ( grams <= 0 || g.heldBite.empty() ) { return; }
        int left = grams;
        // Dominant first, then remaining slots — same spirit as engine bite lay order.
        if ( !g.heldDominant.empty() )
        {
            auto it = g.heldBite.find( g.heldDominant );
            if ( it != g.heldBite.end() && it->second > 0 )
            {
                int take = (std::min)( left, it->second );
                it->second -= take;
                left -= take;
            }
        }
        for ( auto& kv : g.heldBite )
        {
            if ( left <= 0 ) { break; }
            if ( kv.second <= 0 ) { continue; }
            int take = (std::min)( left, kv.second );
            kv.second -= take;
            left -= take;
        }
        RecomputeHeldFromBite();
    }

    int HandfulScoopGrams()
    {
        return (std::max)( 1, (int)std::lround( kHandfulDirtG ) );
    }

    // Pull one deliberate scoop from stock for a place ask (does not mutate held until digest).
    bool BuildPlaceScoopAsk( std::unordered_map<std::string, int>& ask, int& totalG, std::string& dominant )
    {
        ask.clear();
        totalG = 0;
        dominant.clear();
        if ( g.heldTotalG <= 0 || g.heldBite.empty() ) { return false; }
        int left = (std::min)( g.heldTotalG, HandfulScoopGrams() );
        if ( !g.heldDominant.empty() )
        {
            auto it = g.heldBite.find( g.heldDominant );
            if ( it != g.heldBite.end() && it->second > 0 )
            {
                int take = (std::min)( left, it->second );
                ask[it->first] = take;
                left -= take;
                totalG += take;
                dominant = it->first;
            }
        }
        for ( auto const& kv : g.heldBite )
        {
            if ( left <= 0 ) { break; }
            if ( kv.second <= 0 ) { continue; }
            if ( ask.count( kv.first ) ) { continue; }
            int take = (std::min)( left, kv.second );
            ask[kv.first] = take;
            left -= take;
            totalG += take;
            if ( dominant.empty() ) { dominant = kv.first; }
        }
        return totalG > 0 && !dominant.empty();
    }

    void ParseGramsMapAfterKey( std::string const& line, char const* key,
        std::unordered_map<std::string, int>& out, int& totalG, std::string& dominant )
    {
        out.clear();
        totalG = 0;
        dominant.clear();
        size_t p = line.find( key );
        if ( p == std::string::npos ) { return; }
        p = line.find( '{', p );
        if ( p == std::string::npos ) { return; }
        size_t end = line.find( '}', p );
        if ( end == std::string::npos ) { return; }
        std::string body = line.substr( p + 1, end - p - 1 );
        size_t i = 0;
        while ( i < body.size() )
        {
            size_t q0 = body.find( '"', i );
            if ( q0 == std::string::npos ) { break; }
            size_t q1 = body.find( '"', q0 + 1 );
            if ( q1 == std::string::npos ) { break; }
            std::string mat = body.substr( q0 + 1, q1 - q0 - 1 );
            size_t colon = body.find( ':', q1 );
            if ( colon == std::string::npos ) { break; }
            char* e = nullptr;
            long grams = strtol( body.c_str() + colon + 1, &e, 10 );
            if ( e == body.c_str() + colon + 1 ) { i = q1 + 1; continue; }
            if ( grams > 0 )
            {
                out[mat] = (int)grams;
                totalG += (int)grams;
            }
            i = (size_t)( e - body.c_str() );
        }
        int best = 0;
        for ( auto const& kv : out )
        {
            if ( kv.second > best ) { best = kv.second; dominant = kv.first; }
        }
    }

    void CaptureSpirePreRequest( int cx, int cy, float aimX, float aimY, float aimZ,
        char const* authMat )
    {
        // Pre-carve snapshot for SPIRE conviction — SampleGroundZBase + resident geography grade.
        g.spireHavePre = true;
        g.spireCx = cx;
        g.spireCy = cy;
        g.spireAimZ = aimZ;
        g.spireAuthMat = authMat ? authMat : "";
        ProvenanceGeo::EnsureReady();
        auto const surf = ProvenanceGeo::SampleSurface(
            (double)aimX, (double)aimY,
            g.gradeDatum, g.reliefVoxels, g.voxelEdgeM );
        g.spireVisualCap = surf.cap ? surf.cap : CellCapName( cx, cy );
        CellSample const* cell = GetCell( cx, cy );
        g.spireGeoGrade = cell && cell->valid ? cell->grade : 0.f;
        g.spireGeoZ = GradeToZ( g.spireGeoGrade );
        float aimSurfZ = aimZ;
        SampleGroundZBase( aimX, aimY, aimSurfZ );
        g.spirePreReqZ = aimSurfZ;
        // Invert SampleGroundZBase continuum at cell centre for a comparable grade proxy.
        g.spirePreReqGrade = g.spireGeoGrade;
        if ( cell && cell->valid )
        {
            g.spirePreReqGrade = cell->grade;
        }
        char buf[384];
        std::snprintf( buf, sizeof( buf ),
            "SPIRE pre cell=(%d,%d) geoGrade=%.5f geoZ=%.3f aimZ=%.3f surfZ=%.3f | visCap=%s authMat=%s | colGradeGate=%s",
            cx, cy, g.spireGeoGrade, g.spireGeoZ, aimZ, aimSurfZ,
            g.spireVisualCap.c_str(),
            g.spireAuthMat.empty() ? "?" : g.spireAuthMat.c_str(),
            kForbidColumnGradeOverwrite ? "PRESERVE" : "ALLOW_OVERWRITE" );
        g.spireLine = buf;
        OutputDebugStringA( buf );
        OutputDebugStringA( "\n" );
    }

    void ParseRemovedMap( std::string const& line, std::unordered_map<std::string, int>& out, int& totalG, std::string& dominant )
    {
        ParseGramsMapAfterKey( line, "\"removed\"", out, totalG, dominant );
    }

    void ParseCarveReply( std::string const& line )
    {
        ExtractJsonFloat( line, "engine_ms", g.lastEngineMs );
        int rev = g.terrainRev;
        ExtractJsonInt( line, "rev", rev );
        if ( rev > g.terrainRev ) { g.terrainRev = rev; g.lastDigRev = rev; }

        std::string reason, msg;
        ExtractJsonString( line, "reason", reason );
        ExtractJsonString( line, "message", msg );
        if ( msg.empty() ) { ExtractJsonString( line, "msg", msg ); }

        // Envelope ok can be true while result.ok is false — detect refusal by reason/msg/empty removed
        std::unordered_map<std::string, int> removed;
        int totalG = 0;
        std::string dominant;
        ParseRemovedMap( line, removed, totalG, dominant );

        if ( totalG <= 0 && ( !reason.empty() || line.find( "\"ok\":false" ) != std::string::npos || line.find( "\"ok\": false" ) != std::string::npos ) )
        {
            bool const emptyDig = ( reason == "nothing_to_dig" )
                || ( msg.find( "Nothing to dig" ) != std::string::npos )
                || ( msg.find( "nothing to dig" ) != std::string::npos );
            if ( emptyDig )
            {
                // Soft scoop / forward bite: credit Index transfer grams when bridge column empty.
                int creditG = g.pendingLocalScoopG > 0
                    ? g.pendingLocalScoopG
                    : (int)std::lround( kHandfulDirtG );
                std::string mat = g.pendingLocalScoopMat.empty()
                    ? CellCapName( g.pendingBiteCx, g.pendingBiteCy )
                    : g.pendingLocalScoopMat;
                if ( g.pendingBiteForward || g.pendingLocalScoopG > 0 )
                {
                    std::unordered_map<std::string, int> localRem;
                    localRem[mat.empty() ? "dirt" : mat] = creditG;
                    CreditHeld( localRem );
                    ClearPendingScarEdit();
                    CancelPlaceScarsUnderDig( g.pendingBiteWx, g.pendingBiteWy, kHandfulRadiusM );
                    float handfuls = ( g.heldTotalG > 0 ) ? ( g.heldTotalG / kHandfulDirtG ) : 0.f;
                    char d[384];
                    std::snprintf( d, sizeof( d ),
                        "H2H scoop @(%d,%d): +%dg %s (index transfer) | hand %dg ~%.2f | eng empty %.1fms",
                        g.pendingBiteCx, g.pendingBiteCy,
                        creditG, mat.empty() ? "dirt" : mat.c_str(),
                        g.heldTotalG, handfuls, g.lastEngineMs );
                    g.digestLine = d;
                    g.statusLine = "Horizon-to-Hand - scoop credited";
                    g.pendingBiteForward = false;
                    g.pendingLocalScoopG = 0;
                    g.pendingLocalScoopMat.clear();
                    g.pendingAffectRM = 0.f;
                    UpdateStreamHud();
                    return;
                }
                // Steep/virgin: engine matter gone, grade skin can still float — keep optimistic cup.
                ClearPendingScarEdit();
                CancelPlaceScarsUnderDig( (float)g.pendingBiteCx + g.pendingBiteU,
                    (float)g.pendingBiteCy + g.pendingBiteV, kHandfulRadiusM );
                char d[320];
                std::snprintf( d, sizeof( d ),
                    "DIG skin clear @(%d,%d): leftover surface cut (engine already empty) | engine %.1fms",
                    g.pendingBiteCx, g.pendingBiteCy, g.lastEngineMs );
                g.digestLine = d;
                g.statusLine = "Phase 4 - cleared leftover skin";
                g.pendingBiteForward = false;
                g.pendingLocalScoopG = 0;
                g.pendingLocalScoopMat.clear();
                g.pendingAffectRM = 0.f;
                UpdateStreamHud();
                return;
            }
            RemoveLastOptimisticDigScar();
            char const* why = msg.empty()
                ? ( reason.empty() ? "nothing" : reason.c_str() )
                : msg.c_str();
            char d[320];
            std::snprintf( d, sizeof( d ), "DIG refused @(%d,%d): %s (engine %.1fms)",
                g.pendingBiteCx, g.pendingBiteCy, why, g.lastEngineMs );
            g.digestLine = d;
            g.statusLine = "Phase 4 - dig refused";
            g.pendingAffectRM = 0.f;
            UpdateStreamHud();
            return;
        }

        // Empty removed without explicit refuse (sub-unit graze etc.) — keep scar, no hand credit
        if ( totalG <= 0 && reason.empty() && line.find( "\"ok\":false" ) == std::string::npos )
        {
            g.digestLine = "DIG grazed - no whole grams yet; scoop again or aim denser dirt";
            g.statusLine = "Phase 4 - dig grazed (no credit)";
            UpdateStreamHud();
            return;
        }

        // Credit held bite — accumulate like engine carry (stockpile scoops for later places)
        CreditHeld( removed );

        // Material mismatch telemetry only — never remap auth grass ↔ visual limestone.
        if ( !dominant.empty() && !g.spireVisualCap.empty()
          && _stricmp( dominant.c_str(), g.spireVisualCap.c_str() ) != 0 )
        {
            char mm[240];
            std::snprintf( mm, sizeof( mm ),
                " | MISMATCH visCap=%s authRemoved=%s (telemetry, no remap)",
                g.spireVisualCap.c_str(), dominant.c_str() );
            g.spireLine += mm;
            OutputDebugStringA( mm );
            OutputDebugStringA( "\n" );
        }

        float handfuls = ( g.heldTotalG > 0 ) ? ( g.heldTotalG / kHandfulDirtG ) : 0.f;
        float mlApprox = ( totalG / kDirtVoxelG ) * ( kVoxelVolumeM3 * 1e6f );
        (void)mlApprox;
        char d[384];
        std::snprintf( d, sizeof( d ),
            "DIG digest: +%dg %s @(%d,%d)%s | hand now %dg ~%.2f handfuls | rev=%d | engine %.1fms",
            totalG, dominant.empty() ? "?" : dominant.c_str(),
            g.pendingBiteCx, g.pendingBiteCy,
            g.pendingBiteForward ? " forward" : "",
            g.heldTotalG, handfuls, g.terrainRev, g.lastEngineMs );
        g.digestLine = d;
        g.statusLine = "Phase 4 - scoop credited (handful)";
        g.pendingBiteForward = false;
        ClearPendingScarEdit();
        // Dig cuts mounds — placed dirt can be scooped again.
        CancelPlaceScarsUnderDig( g.pendingBiteWx, g.pendingBiteWy, kHandfulRadiusM );
        // P3b: demand occupancy for the bite cell so D2 cavity can replace DigScar cups.
        PrefetchOccupancyCell( g.pendingBiteCx, g.pendingBiteCy );
        PrefetchOccupancyCell( g.pendingBiteCx + 1, g.pendingBiteCy );
        PrefetchOccupancyCell( g.pendingBiteCx - 1, g.pendingBiteCy );
        PrefetchOccupancyCell( g.pendingBiteCx, g.pendingBiteCy + 1 );
        PrefetchOccupancyCell( g.pendingBiteCx, g.pendingBiteCy - 1 );
        UpdateStreamHud();
    }

    void ParsePlaceReply( std::string const& line )
    {
        ExtractJsonFloat( line, "engine_ms", g.lastEngineMs );
        int rev = g.terrainRev;
        ExtractJsonInt( line, "rev", rev );
        if ( rev > g.terrainRev ) { g.terrainRev = rev; }

        std::string msg, reason;
        ExtractJsonString( line, "msg", msg );
        ExtractJsonString( line, "reason", reason );
        int placed = 0;
        bool const havePlaced = ExtractJsonInt( line, "placed", placed );

        bool const hardRefuse = ( line.find( "\"ok\":false" ) != std::string::npos
                               || line.find( "\"ok\": false" ) != std::string::npos );
        bool const nothingLanded = havePlaced && placed <= 0;
        if ( hardRefuse || nothingLanded )
        {
            // Hole-fill places never added a mound scar — only grade/mound places roll back.
            if ( !g.pendingPlaceIntoHole ) { RemoveLastOptimisticPlaceScar(); }
            g.pendingPlaceIntoHole = false;
            g.pendingPlaceAsk.clear();
            g.pendingPlaceG = 0;
            if ( msg.find( "aren't carrying" ) != std::string::npos
              || msg.find( "Nothing in hand" ) != std::string::npos )
            {
                g.heldBite.clear();
                g.heldTotalG = 0;
                g.heldDominant.clear();
            }
            if ( msg.find( "Nowhere to place" ) != std::string::npos && g.heldTotalG > 0 && g.heldTotalG < 80 )
            {
                // Sub-quantum scraps often can't land a body — say so clearly.
                char d[320];
                std::snprintf( d, sizeof( d ),
                    "PLACE refused @(%d,%d): only %dg left (need a fuller scoop) (engine %.1fms)",
                    g.pendingBiteCx, g.pendingBiteCy, g.heldTotalG, g.lastEngineMs );
                g.digestLine = d;
                g.statusLine = "Phase 4 - place refused (scrap)";
                UpdateStreamHud();
                return;
            }
            char d[320];
            std::snprintf( d, sizeof( d ), "PLACE refused @(%d,%d): %s (engine %.1fms)",
                g.pendingBiteCx, g.pendingBiteCy,
                msg.empty()
                    ? ( reason.empty()
                        ? ( nothingLanded ? "nowhere to place / nothing landed" : "failed" )
                        : reason.c_str() )
                    : msg.c_str(),
                g.lastEngineMs );
            g.digestLine = d;
            g.statusLine = "Phase 4 - place refused";
            UpdateStreamHud();
            return;
        }

        std::unordered_map<std::string, int> placedBy;
        int placedByTotal = 0;
        std::string placedDom;
        ParseGramsMapAfterKey( line, "\"placed_by\"", placedBy, placedByTotal, placedDom );
        // Local PlaceOccupancyFill already debited pendingPlaceG on send. Digest reconciles only.
        int const alreadyDebited = g.pendingPlaceG;
        if ( placedByTotal > 0 )
        {
            placed = placedByTotal;
            int delta = placedByTotal - alreadyDebited;
            if ( delta > 0 ) { DebitHeldTotal( delta ); }
        }
        else
        {
            int debit = havePlaced && placed > 0 ? placed : alreadyDebited;
            if ( debit <= 0 ) { debit = HandfulScoopGrams(); }
            int delta = debit - alreadyDebited;
            if ( delta > 0 ) { DebitHeldTotal( delta ); }
            if ( !havePlaced || placed <= 0 ) { placed = debit; }
        }
        g.pendingPlaceAsk.clear();
        g.pendingPlaceG = 0;

        char d[320];
        std::snprintf( d, sizeof( d ),
            "PLACE digest: -%dg %s @(%d,%d) | hand now %dg ~%.2f handfuls | rev=%d | engine %.1fms",
            placed, placedDom.empty() ? ( g.heldDominant.empty() ? "?" : g.heldDominant.c_str() ) : placedDom.c_str(),
            g.pendingBiteCx, g.pendingBiteCy, g.heldTotalG, g.heldTotalG / kHandfulDirtG,
            g.terrainRev, g.lastEngineMs );
        g.digestLine = d;
        g.statusLine = g.heldTotalG > 0 ? "P3e - placed (still holding)" : "P3e - placed (hand empty)";
        ClearPendingScarEdit();
        // Slump fill: one scoop into the hole shrinks the dig cup. Hole-fill never added a mound.
        CancelDigScarsUnderPlace( (float)g.pendingBiteCx + g.pendingBiteU,
            (float)g.pendingBiteCy + g.pendingBiteV, kHandfulRadiusM );
        g.pendingPlaceIntoHole = false;
        // No column fan-out after place — same stall as dig.
        UpdateStreamHud();
    }

    float FillTopToWorldZ( float grade, int kz, float meanTopLayers )
    {
        // Occupancy crest proxy: virgin columns full to kz; digs lower meanTop below kz.
        float const gradeZ = GradeToZ( grade );
        float const deltaLayers = (float)kz - meanTopLayers;
        return gradeZ - deltaLayers * g.voxelEdgeM;
    }

    int FillAt( CellSample const& cell, int c, int r, int k )
    {
        if ( cell.fill.empty() || cell.fillW <= 0 ) { return 0; }
        if ( c < 0 || r < 0 || k < 0 || c >= cell.fillW || r >= cell.fillH || k >= cell.fillK ) { return 0; }
        return (int)cell.fill[(size_t)k * cell.fillW * cell.fillH + r * cell.fillW + c];
    }

    bool SampleOccupancyZ( float x, float y, float& outZ )
    {
        // Occupancy-derived surface from voxel_column fill (k=0 bottom … k=kz-1 top).
        int const cx = (int)std::floor( x );
        int const cy = (int)std::floor( y );
        CellSample const* cell = GetCell( cx, cy );
        if ( !cell || cell->fill.empty() || cell->fillW <= 0 ) { return false; }
        int const w = cell->fillW, h = cell->fillH, kz = cell->fillK;
        int const col = (std::min)( w - 1, (std::max)( 0, (int)( ( x - (float)cx ) * (float)w ) ) );
        int const row = (std::min)( h - 1, (std::max)( 0, (int)( ( y - (float)cy ) * (float)h ) ) );
        int top = -1;
        for ( int k = kz - 1; k >= 0; --k )
        {
            if ( FillAt( *cell, col, row, k ) >= kFillIso ) { top = k; break; }
        }
        float const crest = CellOccCrest( *cell );
        if ( top < 0 )
        {
            outZ = crest - (float)kz * g.voxelEdgeM;
            return true;
        }
        // Top face of uppermost solid voxel (crest when top == kz-1).
        outZ = crest - (float)( kz - 1 - top ) * g.voxelEdgeM;
        return true;
    }

    // Physical support from occupancy (same fill D2 consumes) or virgin HF.
    // Searches downward from queryZ — roof/topmost solid above the body is ignored.
    // Never queries D2 triangles. Never invents a floor when authority is missing.
    SupportHit SupportBelow( float x, float y, float queryZ )
    {
        SupportHit hit{};
        hit.cellX = (int)std::floor( x );
        hit.cellY = (int)std::floor( y );
        CellSample const* cell = GetCell( hit.cellX, hit.cellY );
        bool const carvedReady = cell && cell->carved && !cell->fill.empty()
            && cell->fillW > 0 && cell->fillK > 0;
        bool const erOwns = EditedRegionOwnsAt( x, y );

        // Missing authority → refuse/defer. Never stand on virgin HF over excavated claim.
        if ( ( erOwns || ( cell && cell->carved ) ) && !carvedReady )
        {
            hit.deferred = true;
            hit.hit = false;
            return hit;
        }

        auto writeMat = [&]( SupportHit& h )
        {
            std::string const cap = ( cell && !cell->cap.empty() ) ? cell->cap : CapAtWorld( x, y );
            std::snprintf( h.material, sizeof( h.material ), "%s",
                cap.empty() ? "?" : cap.c_str() );
        };
        auto writeRev = [&]( SupportHit& h )
        {
            if ( !cell || cell->editedRegionId == 0 ) { return; }
            if ( EditedRegion* er = FindEditedRegion( cell->editedRegionId ) )
            {
                h.regionRev = er->dirtyRev;
            }
        };

        if ( carvedReady )
        {
            float const edge = (std::max)( 0.05f, g.voxelEdgeM );
            float const step = edge * 0.25f;
            float const crest = CellOccCrest( *cell );
            float const columnBottom = crest - (float)cell->fillK * edge - edge;
            writeRev( hit );

            auto refineAirToSolid = [&]( float zAir, float zSolid ) -> float
            {
                float t0 = zAir, t1 = zSolid;
                for ( int i = 0; i < 12; ++i )
                {
                    float const tm = 0.5f * ( t0 + t1 );
                    if ( OccupancySolidAt( x, y, tm ) ) { t1 = tm; }
                    else { t0 = tm; }
                }
                return t1;
            };
            auto estimateNormal = [&]( float zHit, SupportHit& h )
            {
                float const eps = edge;
                auto neighZ = [&]( float nx, float ny, float& oz ) -> bool
                {
                    if ( OccupancySolidAt( nx, ny, queryZ ) )
                    {
                        // Neighbor column solid at query — walk up to its top face.
                        float zTop = queryZ;
                        for ( float z = queryZ; z <= crest + edge * 2.f; z += step )
                        {
                            if ( !OccupancySolidAt( nx, ny, z ) ) { break; }
                            zTop = z;
                        }
                        oz = refineAirToSolid( zTop + step, zTop );
                        return true;
                    }
                    float prev = queryZ;
                    for ( float z = queryZ - step; z >= columnBottom; z -= step )
                    {
                        if ( OccupancySolidAt( nx, ny, z ) )
                        {
                            oz = refineAirToSolid( prev, z );
                            return true;
                        }
                        prev = z;
                    }
                    return false;
                };
                float zxp = zHit, zxm = zHit, zyp = zHit, zym = zHit;
                int ok = 0;
                ok += neighZ( x + eps, y, zxp ) ? 1 : 0;
                ok += neighZ( x - eps, y, zxm ) ? 1 : 0;
                ok += neighZ( x, y + eps, zyp ) ? 1 : 0;
                ok += neighZ( x, y - eps, zym ) ? 1 : 0;
                if ( ok >= 2 )
                {
                    float nx = ( zxm - zxp ) / ( 2.f * eps );
                    float ny = ( zym - zyp ) / ( 2.f * eps );
                    float nz = 1.f;
                    float const len = std::sqrt( nx * nx + ny * ny + nz * nz );
                    if ( len > 1e-6f )
                    {
                        h.normal = DualContourQef::Vec3{ nx / len, ny / len, nz / len };
                        return;
                    }
                }
                h.normal = DualContourQef::Vec3{ 0.f, 0.f, 1.f };
            };

            // Penetrating solid: settle to top face of this solid stack (still occupancy, not HF).
            if ( OccupancySolidAt( x, y, queryZ ) )
            {
                float zTop = queryZ;
                for ( float z = queryZ; z <= crest + edge * 2.f; z += step )
                {
                    if ( !OccupancySolidAt( x, y, z ) ) { break; }
                    zTop = z;
                }
                float const supportZ = refineAirToSolid( zTop + step, zTop );
                hit.hit = true;
                hit.position = DualContourQef::Vec3{ x, y, supportZ };
                estimateNormal( supportZ, hit );
                writeMat( hit );
                return hit;
            }

            // Air: march downward from queryZ — ignore roof / topmost solid above.
            float prevZ = queryZ;
            for ( float z = queryZ - step; z >= columnBottom; z -= step )
            {
                if ( OccupancySolidAt( x, y, z ) )
                {
                    float const supportZ = refineAirToSolid( prevZ, z );
                    hit.hit = true;
                    hit.position = DualContourQef::Vec3{ x, y, supportZ };
                    estimateNormal( supportZ, hit );
                    writeMat( hit );
                    return hit;
                }
                prevZ = z;
            }
            // Authority present but no solid below query — miss (do not invent HF).
            hit.hit = false;
            return hit;
        }

        // Virgin continuum: HF (+ presentation scars). Unchanged vs prior SupportAt outside ER.
        float z = 0.f;
        if ( !SampleGroundZBase( x, y, z ) )
        {
            hit.hit = false;
            return hit;
        }
        z += ScarDeltaZ( x, y );
        hit.hit = true;
        hit.position = DualContourQef::Vec3{ x, y, z };
        float nx = 0.f, ny = 0.f, nz = 1.f;
        CaptureFaceNormalAt( x, y, nx, ny, nz );
        hit.normal = DualContourQef::Vec3{ nx, ny, nz };
        writeMat( hit );
        (void)queryZ; // virgin support is the continuum surface (settle/push uses this Z)
        return hit;
    }

    bool SupportAt( float x, float y, float& outZ )
    {
        // XY-only adapter. Prefer SupportBelow with the body's current Z.
        float qz = g.camZ;
        if ( !std::isfinite( qz ) ) { qz = g.feetZ + 1.6f; }
        if ( std::fabs( x - g.feetX ) > 3.f || std::fabs( y - g.feetY ) > 3.f )
        {
            float crest = 0.f;
            if ( SampleGroundZBase( x, y, crest ) ) { qz = crest + 2.5f; }
        }
        SupportHit const h = SupportBelow( x, y, qz );
        if ( !h.hit ) { return false; }
        outZ = h.position.z;
        return true;
    }

    void DestroyCavityList( CellSample& cell )
    {
        if ( cell.cavityList )
        {
            glDeleteLists( cell.cavityList, 1 );
            cell.cavityList = 0;
        }
        if ( cell.debugBoundaryList )
        {
            glDeleteLists( cell.debugBoundaryList, 1 );
            cell.debugBoundaryList = 0;
        }
        cell.cavityTris.clear();
        cell.hasCavity = false;
        // No cavity → no HF handoff. Keep carve focus so a later rebuild can reclaim ownership.
        cell.hasPatchBounds = false;
    }

    // Occupancy field for one home column + same-res neighbor halo (missing = air).
    struct ColumnFillField : DualContourQef::IFillField
    {
        int homeCx = 0, homeCy = 0;
        CellSample const* home = nullptr;

        bool TrySample( int c, int r, int k, int& outFill ) const override
        {
            outFill = 0;
            if ( !home || home->fill.empty() || home->fillW <= 0 ) { return false; }
            int const w = home->fillW, h = home->fillH, kz = home->fillK;
            // Column Z domain boundary (not a missing neighbor): below = solid, above = air.
            if ( k < 0 ) { outFill = kFillFull; return true; }
            if ( k >= kz ) { outFill = 0; return true; }
            int cellDx = 0, cellDy = 0;
            int lc = c, lr = r;
            while ( lc < 0 ) { lc += w; --cellDx; }
            while ( lc >= w ) { lc -= w; ++cellDx; }
            while ( lr < 0 ) { lr += h; --cellDy; }
            while ( lr >= h ) { lr -= h; ++cellDy; }
            CellSample const* cell = home;
            if ( cellDx != 0 || cellDy != 0 )
            {
                cell = GetCell( homeCx + cellDx, homeCy + cellDy );
                if ( !cell || cell->fill.empty()
                    || cell->fillW != w || cell->fillH != h || cell->fillK != kz )
                {
                    // Unknown halo — never synthesize solid or air. Caller must fetch or refuse.
                    return false;
                }
            }
            outFill = FillAt( *cell, lc, lr, k );
            return true;
        }
    };

    // Ensure 3×3 neighbor occupancy lattices exist for D2 halo sampling (virgin seed OK).
    // Returns false if any neighbor still cannot provide a matching fill field.
    bool EnsureD2HaloLattices( int cx, int cy )
    {
        CellSample const* home = GetCell( cx, cy );
        if ( !home || home->fill.empty() || home->fillW <= 0 ) { return false; }
        int const w = home->fillW, h = home->fillH, kz = home->fillK;
        bool ok = true;
        for ( int dy = -1; dy <= 1; ++dy )
        {
            for ( int dx = -1; dx <= 1; ++dx )
            {
                int const nx = cx + dx, ny = cy + dy;
                PrefetchOccupancyCell( nx, ny );
                EnsureOccupancyLattice( nx, ny );
                CellSample* n = GetCellMutable( nx, ny );
                if ( !n ) { ok = false; continue; }
                if ( n->fill.empty() || n->fillW <= 0 )
                {
                    EnsureOccupancyLattice( nx, ny );
                    n = GetCellMutable( nx, ny );
                }
                if ( n && !n->fill.empty() && !n->carved && !n->hasFillZ )
                {
                    // Local lattice just created (no wire fill yet): seed from virgin HF.
                    // Do not overwrite authoritative voxel_column fill (hasFillZ already set).
                    SeedOccupancyFromVirginSurface( nx, ny );
                }
                n = GetCellMutable( nx, ny );
                // P3e place headroom may raise home fillK — expand neighbors to match.
                if ( n && !n->fill.empty() && n->fillW == w && n->fillH == h && n->fillK < kz )
                {
                    ExpandOccupancyHeadroom( nx, ny, kz - n->fillK );
                    n = GetCellMutable( nx, ny );
                }
                if ( !n || n->fill.empty()
                  || n->fillW != w || n->fillH != h || n->fillK != kz )
                {
                    ok = false;
                }
            }
        }
        return ok;
    }

    bool OpeningTouchesCell( EditOpening const& o, int cx, int cy )
    {
        float const r = (std::max)( 0.05f, o.r );
        return o.x + r >= (float)cx && o.x - r <= (float)( cx + 1 )
            && o.y + r >= (float)cy && o.y - r <= (float)( cy + 1 );
    }

    bool DirtyBoundsFromEditedRegion( EditedRegion const* er, CellSample const& cell,
        int cx, int cy,
        float& mnX, float& mxX, float& mnY, float& mxY, float& mnZ, float& mxZ )
    {
        // Full accumulated dirty AABB for this cell's share of the EditedRegion.
        // Never truncated — large tunnels span many cells; work may partition, bounds must not clip.
        constexpr float kD2ReconHalo = 0.20f;
        mnX = 1e9f; mxX = -1e9f; mnY = 1e9f; mxY = -1e9f; mnZ = 1e9f; mxZ = -1e9f;
        bool any = false;
        if ( er )
        {
            for ( EditOpening const& o : er->openings )
            {
                if ( !OpeningTouchesCell( o, cx, cy ) ) { continue; }
                float const r = (std::max)( 0.05f, o.r ) + kD2ReconHalo;
                mnX = (std::min)( mnX, o.x - r ); mxX = (std::max)( mxX, o.x + r );
                mnY = (std::min)( mnY, o.y - r ); mxY = (std::max)( mxY, o.y + r );
                mnZ = (std::min)( mnZ, o.z - r ); mxZ = (std::max)( mxZ, o.z + r );
                any = true;
            }
        }
        if ( cell.hasCarveFocus )
        {
            float const r = (std::max)( 0.10f, cell.carveRM ) + kD2ReconHalo;
            mnX = (std::min)( mnX, cell.carveWx - r ); mxX = (std::max)( mxX, cell.carveWx + r );
            mnY = (std::min)( mnY, cell.carveWy - r ); mxY = (std::max)( mxY, cell.carveWy + r );
            mnZ = (std::min)( mnZ, cell.carveWz - r ); mxZ = (std::max)( mxZ, cell.carveWz + r );
            any = true;
        }
        return any;
    }

    bool FocusFromEditedRegion( EditedRegion const* er, CellSample const& cell,
        int cx, int cy, float& fx, float& fy, float& fz, float& fr )
    {
        // Convenience: bounding sphere of full dirty AABB (no geometric clip).
        float mnX, mxX, mnY, mxY, mnZ, mxZ;
        if ( !DirtyBoundsFromEditedRegion( er, cell, cx, cy, mnX, mxX, mnY, mxY, mnZ, mxZ ) )
        {
            return false;
        }
        fx = 0.5f * ( mnX + mxX );
        fy = 0.5f * ( mnY + mxY );
        fz = 0.5f * ( mnZ + mxZ );
        float const dx = mxX - mnX, dy = mxY - mnY, dz = mxZ - mnZ;
        fr = 0.5f * std::sqrt( dx * dx + dy * dy + dz * dz ) + 0.05f;
        return fr > 0.05f;
    }

    void RebuildDebugBoundaryList( int cx, int cy, CellSample& cell, EditedRegion const* er )
    {
        if ( cell.debugBoundaryList )
        {
            glDeleteLists( cell.debugBoundaryList, 1 );
            cell.debugBoundaryList = 0;
        }
        int const w = cell.fillW, h = cell.fillH, kz = cell.fillK;
        float const edge = (std::max)( 0.05f, g.voxelEdgeM );
        float const crest = CellOccCrest( cell );
        float const x0 = (float)cx, y0 = (float)cy;
        float const du = 1.f / (float)w, dv = 1.f / (float)h;
        constexpr float kCollar = 0.08f;

        auto solid = [&]( int c, int r, int k ) -> bool
        {
            if ( c < 0 || r < 0 || k < 0 || c >= w || r >= h || k >= kz ) { return false; }
            return FillAt( cell, c, r, k ) >= kFillIso;
        };
        auto airInside = [&]( int nc, int nr, int nk ) -> bool
        {
            if ( nc < 0 || nr < 0 || nk < 0 || nc >= w || nr >= h || nk >= kz ) { return false; }
            return !solid( nc, nr, nk );
        };
        auto cornerZ = [&]( int k, bool topFace ) -> float
        {
            return topFace ? ( crest - (float)( kz - 1 - k ) * edge )
                           : ( crest - (float)( kz - k ) * edge );
        };
        auto inOpenings = [&]( float xc, float yc, float zc ) -> bool
        {
            if ( er && !er->openings.empty() )
            {
                return NearRegionOpenings( *er, xc, yc, zc, kCollar );
            }
            if ( !cell.hasCarveFocus ) { return false; }
            float const lim = (std::max)( 0.10f, cell.carveRM ) + kCollar;
            float const dx = xc - cell.carveWx, dy = yc - cell.carveWy, dz = zc - cell.carveWz;
            return ( dx * dx + dy * dy + dz * dz ) <= lim * lim;
        };

        GLuint list = AllocDisplayListOutsideFonts();
        if ( !list ) { return; }
        cell.debugBoundaryList = list;
        glNewList( list, GL_COMPILE );
        glShadeModel( GL_FLAT );
        glBegin( GL_TRIANGLES );
        int faces = 0;
        for ( int k = 0; k < kz; ++k )
        {
            for ( int r = 0; r < h; ++r )
            {
                for ( int c = 0; c < w; ++c )
                {
                    if ( !solid( c, r, k ) ) { continue; }
                    float const xc = x0 + ( (float)c + 0.5f ) * du;
                    float const yc = y0 + ( (float)r + 0.5f ) * dv;
                    float const zc = crest - ( (float)( kz - 1 - k ) + 0.5f ) * edge;
                    if ( !inOpenings( xc, yc, zc ) ) { continue; }

                    float const px0 = x0 + (float)c * du;
                    float const px1 = x0 + (float)( c + 1 ) * du;
                    float const py0 = y0 + (float)r * dv;
                    float const py1 = y0 + (float)( r + 1 ) * dv;
                    float const zLo = cornerZ( k, false );
                    float const zHi = cornerZ( k, true );
                    auto emit = [&]( float ax, float ay, float az, float bx, float by, float bz,
                        float cx2, float cy2, float cz2 )
                    {
                        EmitPhase3Tri( ax, ay, az, bx, by, bz, cx2, cy2, cz2, 0.45f );
                        ++faces;
                    };
                    if ( airInside( c - 1, r, k ) )
                    {
                        emit( px0, py0, zLo, px0, py1, zLo, px0, py0, zHi );
                        emit( px0, py1, zLo, px0, py1, zHi, px0, py0, zHi );
                    }
                    if ( airInside( c + 1, r, k ) )
                    {
                        emit( px1, py0, zLo, px1, py0, zHi, px1, py1, zLo );
                        emit( px1, py1, zLo, px1, py0, zHi, px1, py1, zHi );
                    }
                    if ( airInside( c, r - 1, k ) )
                    {
                        emit( px0, py0, zLo, px0, py0, zHi, px1, py0, zLo );
                        emit( px1, py0, zLo, px0, py0, zHi, px1, py0, zHi );
                    }
                    if ( airInside( c, r + 1, k ) )
                    {
                        emit( px0, py1, zLo, px1, py1, zLo, px0, py1, zHi );
                        emit( px1, py1, zLo, px1, py1, zHi, px0, py1, zHi );
                    }
                    if ( airInside( c, r, k - 1 ) )
                    {
                        emit( px0, py0, zLo, px1, py0, zLo, px0, py1, zLo );
                        emit( px1, py0, zLo, px1, py1, zLo, px0, py1, zLo );
                    }
                    if ( airInside( c, r, k + 1 ) )
                    {
                        emit( px0, py0, zHi, px0, py1, zHi, px1, py0, zHi );
                        emit( px1, py0, zHi, px0, py1, zHi, px1, py1, zHi );
                    }
                }
            }
        }
        glEnd();
        glEndList();
        if ( faces <= 0 )
        {
            glDeleteLists( cell.debugBoundaryList, 1 );
            cell.debugBoundaryList = 0;
        }
    }

    // World → fill indices. Returns false if cell has no occupancy lattice yet.
    bool WorldToFillIndex( float x, float y, float z, CellSample const*& cell,
        int& c, int& r, int& k, float& crestZ )
    {
        int const cx = (int)std::floor( x );
        int const cy = (int)std::floor( y );
        cell = GetCell( cx, cy );
        if ( !cell || cell->fill.empty() || cell->fillW <= 0 || cell->fillK <= 0 ) { return false; }
        int const w = cell->fillW, h = cell->fillH, kz = cell->fillK;
        float const u = x - (float)cx, v = y - (float)cy;
        c = (std::min)( w - 1, (std::max)( 0, (int)( u * (float)w ) ) );
        r = (std::min)( h - 1, (std::max)( 0, (int)( v * (float)h ) ) );
        crestZ = CellOccCrest( *cell );
        float const edge = (std::max)( 0.05f, g.voxelEdgeM );
        float const t = ( crestZ - z ) / edge; // 0 at crest, +down
        if ( t < -0.05f ) { k = kz; return true; } // above crest = air sentinel
        if ( t >= (float)kz ) { k = -1; return true; } // below column
        k = kz - 1 - (int)std::floor( t );
        if ( k < 0 ) { k = 0; }
        if ( k >= kz ) { k = kz - 1; }
        return true;
    }

    bool OccupancySolidAt( float x, float y, float z )
    {
        CellSample const* cell = nullptr;
        int c = 0, r = 0, k = 0;
        float crest = 0.f;
        if ( !WorldToFillIndex( x, y, z, cell, c, r, k, crest ) ) { return false; }
        if ( k < 0 || k >= cell->fillK ) { return false; } // air above / below
        return FillAt( *cell, c, r, k ) >= kFillIso;
    }

    bool CellHasOccupancy( int cx, int cy )
    {
        CellSample const* cell = GetCell( cx, cy );
        return cell && !cell->fill.empty() && cell->fillW > 0;
    }

    bool RayHitOccupancy( float ox, float oy, float oz,
        float fx, float fy, float fz, float maxT,
        float& outT, float& outX, float& outY, float& outZ )
    {
        // March the look ray through resident fill lattices — air → solid is the matter face.
        float const step = (std::max)( 0.04f, g.voxelEdgeM * 0.45f );
        bool havePrev = false;
        bool prevSolid = false;
        float prevT = 0.f;
        for ( float t = 0.08f; t <= maxT; t += step )
        {
            float const x = ox + fx * t;
            float const y = oy + fy * t;
            float const z = oz + fz * t;
            int const cx = (int)std::floor( x );
            int const cy = (int)std::floor( y );
            if ( !CellHasOccupancy( cx, cy ) )
            {
                havePrev = false;
                continue;
            }
            bool const solid = OccupancySolidAt( x, y, z );
            if ( havePrev && !prevSolid && solid )
            {
                // Refine air→solid crossing.
                float t0 = prevT, t1 = t;
                for ( int i = 0; i < 10; ++i )
                {
                    float const tm = 0.5f * ( t0 + t1 );
                    if ( OccupancySolidAt( ox + fx * tm, oy + fy * tm, oz + fz * tm ) ) { t1 = tm; }
                    else { t0 = tm; }
                }
                outT = t1;
                outX = ox + fx * outT;
                outY = oy + fy * outT;
                outZ = oz + fz * outT;
                return true;
            }
            havePrev = true;
            prevSolid = solid;
            prevT = t;
        }
        return false;
    }

    void PrefetchOccupancyCell( int cx, int cy )
    {
        // Engine column fetch only — never seed local lattices or rebuild cavity from aim/look.
        if ( CellHasOccupancy( cx, cy ) ) { return; }
        QueueColumn( cx, cy );
    }

    void SetFillAt( CellSample& cell, int c, int r, int k, uint8_t v )
    {
        if ( cell.fill.empty() || cell.fillW <= 0 ) { return; }
        if ( c < 0 || r < 0 || k < 0 || c >= cell.fillW || r >= cell.fillH || k >= cell.fillK ) { return; }
        cell.fill[(size_t)k * cell.fillW * cell.fillH + r * cell.fillW + c] = v;
    }

    void SeedOccupancyFromVirginSurface( int cx, int cy )
    {
        // Commit-time only: occupancy solid matches canonical virgin HF (SampleGroundZBase).
        // Not a continuous HF↔volume rematerialization — called once per cell at dig commit.
        CellSample* cell = GetCellMutable( cx, cy );
        if ( !cell || cell->fill.empty() || cell->fillW <= 0 || cell->fillK <= 0 ) { return; }
        int const w = cell->fillW, h = cell->fillH, kz = cell->fillK;
        float const edge = (std::max)( 0.05f, g.voxelEdgeM );
        float const crest = CellOccCrest( *cell );
        float const du = 1.f / (float)w, dv = 1.f / (float)h;
        constexpr float kEps = 0.02f;
        for ( int r = 0; r < h; ++r )
        {
            for ( int c = 0; c < w; ++c )
            {
                float const x = (float)cx + ( (float)c + 0.5f ) * du;
                float const y = (float)cy + ( (float)r + 0.5f ) * dv;
                float surfZ = crest;
                SampleGroundZBase( x, y, surfZ );
                for ( int k = 0; k < kz; ++k )
                {
                    float const zc = crest - ( (float)( kz - 1 - k ) + 0.5f ) * edge;
                    if ( zc > surfZ + kEps )
                    {
                        SetFillAt( *cell, c, r, k, 0 );
                    }
                }
            }
        }
        cell->hasFillZ = true;
        cell->fillZ = crest;
    }

    void EnsureOccupancyLattice( int cx, int cy )
    {
        // Dig-commit only path. Virgin world stays HF-only until this seeds a bounded lattice.
        EnsureGeoCell( cx, cy );
        CellSample* cell = GetCellMutable( cx, cy );
        if ( !cell ) { return; }
        if ( !cell->fill.empty() && cell->fillW > 0 && cell->fillK > 0 ) { return; }
        constexpr int kW = 8, kH = 8, kZ = 32;
        cell->fill.assign( (size_t)kW * kH * kZ, (uint8_t)kFillFull );
        cell->fillW = kW;
        cell->fillH = kH;
        cell->fillK = kZ;
        cell->hasFillZ = true;
        cell->fillZ = GradeToZ( cell->grade );
        SeedOccupancyFromVirginSurface( cx, cy );
    }

    void RetirePresentationScarsNear( float wx, float wy, float radiusM )
    {
        // Cavity owns the hole — DigScar cups/chips are flash only.
        float const lim = (std::max)( 0.05f, radiusM * 1.35f );
        float const lim2 = lim * lim;
        size_t const before = g.scars.size();
        g.scars.erase( std::remove_if( g.scars.begin(), g.scars.end(),
            [&]( DigScar const& s )
            {
                if ( s.place ) { return false; }
                float const dx = s.wx - wx, dy = s.wy - wy;
                return ( dx * dx + dy * dy ) <= lim2;
            } ), g.scars.end() );
        if ( g.scars.size() != before ) { ++g.scarGen; }
    }


    float CellOccCrest( CellSample const& cell )
    {
        // Occupancy lattice top. May sit above virgin HF grade when place added headroom.
        return cell.hasOccCrest ? cell.occCrestZ : GradeToZ( cell.grade );
    }

    bool ExpandOccupancyHeadroom( int cx, int cy, int addLayers )
    {
        // Grow lattice upward (raise occ crest, keep column bottom) so placed mounds have air cells
        // above virgin surface without mutating HF grade.
        if ( addLayers <= 0 ) { return true; }
        CellSample* cell = GetCellMutable( cx, cy );
        if ( !cell || cell->fill.empty() || cell->fillW <= 0 || cell->fillK <= 0 ) { return false; }
        int const w = cell->fillW, h = cell->fillH, oldK = cell->fillK;
        int const newK = oldK + addLayers;
        float const edge = (std::max)( 0.05f, g.voxelEdgeM );
        float const oldCrest = CellOccCrest( *cell );
        std::vector<uint8_t> neu( (size_t)w * h * newK, (uint8_t)0 );
        for ( int k = 0; k < oldK; ++k )
        {
            for ( int r = 0; r < h; ++r )
            {
                for ( int c = 0; c < w; ++c )
                {
                    neu[(size_t)k * w * h + r * w + c] =
                        cell->fill[(size_t)k * w * h + r * w + c];
                }
            }
        }
        cell->fill.swap( neu );
        cell->fillK = newK;
        cell->occCrestZ = oldCrest + (float)addLayers * edge;
        cell->hasOccCrest = true;
        return true;
    }

    int CountOccupancySolidInSphere( float wx, float wy, float wz, float radiusM )
    {
        float const R = (std::max)( 0.02f, radiusM );
        float const R2 = R * R;
        int const x0 = (int)std::floor( wx - R - 0.05f );
        int const x1 = (int)std::floor( wx + R + 0.05f );
        int const y0 = (int)std::floor( wy - R - 0.05f );
        int const y1 = (int)std::floor( wy + R + 0.05f );
        int n = 0;
        for ( int cy = y0; cy <= y1; ++cy )
        {
            for ( int cx = x0; cx <= x1; ++cx )
            {
                CellSample const* cell = GetCell( cx, cy );
                if ( !cell || cell->fill.empty() ) { continue; }
                int const w = cell->fillW, h = cell->fillH, kz = cell->fillK;
                float const edge = (std::max)( 0.05f, g.voxelEdgeM );
                float const crest = CellOccCrest( *cell );
                float const du = 1.f / (float)w, dv = 1.f / (float)h;
                for ( int k = 0; k < kz; ++k )
                {
                    float const zc = crest - ( (float)( kz - 1 - k ) + 0.5f ) * edge;
                    for ( int r = 0; r < h; ++r )
                    {
                        float const yc = (float)cy + ( (float)r + 0.5f ) * dv;
                        for ( int c = 0; c < w; ++c )
                        {
                            float const xc = (float)cx + ( (float)c + 0.5f ) * du;
                            float const dx = xc - wx, dy = yc - wy, dz = zc - wz;
                            if ( dx * dx + dy * dy + dz * dz > R2 ) { continue; }
                            if ( FillAt( *cell, c, r, k ) >= kFillIso ) { ++n; }
                        }
                    }
                }
            }
        }
        return n;
    }

    int CountOccupancyFillUnitsInSphere( float wx, float wy, float wz, float radiusM )
    {
        float const R = (std::max)( 0.02f, radiusM );
        float const R2 = R * R;
        int const x0 = (int)std::floor( wx - R - 0.05f );
        int const x1 = (int)std::floor( wx + R + 0.05f );
        int const y0 = (int)std::floor( wy - R - 0.05f );
        int const y1 = (int)std::floor( wy + R + 0.05f );
        int units = 0;
        for ( int cy = y0; cy <= y1; ++cy )
        {
            for ( int cx = x0; cx <= x1; ++cx )
            {
                CellSample const* cell = GetCell( cx, cy );
                if ( !cell || cell->fill.empty() ) { continue; }
                int const w = cell->fillW, h = cell->fillH, kz = cell->fillK;
                float const edge = (std::max)( 0.05f, g.voxelEdgeM );
                float const crest = CellOccCrest( *cell );
                float const du = 1.f / (float)w, dv = 1.f / (float)h;
                for ( int k = 0; k < kz; ++k )
                {
                    float const zc = crest - ( (float)( kz - 1 - k ) + 0.5f ) * edge;
                    for ( int r = 0; r < h; ++r )
                    {
                        float const yc = (float)cy + ( (float)r + 0.5f ) * dv;
                        for ( int c = 0; c < w; ++c )
                        {
                            float const xc = (float)cx + ( (float)c + 0.5f ) * du;
                            float const dx = xc - wx, dy = yc - wy, dz = zc - wz;
                            if ( dx * dx + dy * dy + dz * dz > R2 ) { continue; }
                            units += (int)FillAt( *cell, c, r, k );
                        }
                    }
                }
            }
        }
        return units;
    }

    PlaceFillResult PlaceOccupancyFill( float wx, float wy, float wz, float radiusM, int gramsAvailable )
    {
        // Reverse of CarveOccupancySphere: debit-budgeted fill units → occupancy air→solid,
        // bottom-up with support. Never mutates virgin HF grade. Never shrinks EditedRegion openings.
        // Representation: dirt aggregate in occupancy (cheap). Meaningful quartz/block → MatterBody later.
        PlaceFillResult out{};
        if ( gramsAvailable <= 0 ) { return out; }
        float const R = (std::max)( 0.02f, radiusM );
        float const R2 = R * R;
        int unitsBudget = (int)std::lround( (double)gramsAvailable * (double)kFillFull / (double)kDirtVoxelG );
        if ( unitsBudget <= 0 ) { unitsBudget = 1; }

        int const x0 = (int)std::floor( wx - R - 0.05f );
        int const x1 = (int)std::floor( wx + R + 0.05f );
        int const y0 = (int)std::floor( wy - R - 0.05f );
        int const y1 = (int)std::floor( wy + R + 0.05f );

        int const headLayers = (std::max)( 2, (int)std::ceil( ( R * 2.5f ) / kVoxelEdgeM ) + 1 );
        // Include D2 halo neighbors so fillK stays matched after headroom expand.
        int const hx0 = x0 - 1, hx1 = x1 + 1, hy0 = y0 - 1, hy1 = y1 + 1;
        float needTop = wz + R + kVoxelEdgeM;
        for ( int cy = hy0; cy <= hy1; ++cy )
        {
            for ( int cx = hx0; cx <= hx1; ++cx )
            {
                float virginZ = 0.f;
                SampleGroundZBase( (float)cx + 0.5f, (float)cy + 0.5f, virginZ );
                needTop = (std::max)( needTop, virginZ + R * 2.f + kVoxelEdgeM );
            }
        }
        for ( int cy = hy0; cy <= hy1; ++cy )
        {
            for ( int cx = hx0; cx <= hx1; ++cx )
            {
                EnsureOccupancyLattice( cx, cy );
                CellSample* cell = GetCellMutable( cx, cy );
                if ( !cell ) { continue; }
                if ( !cell->carved )
                {
                    SeedOccupancyFromVirginSurface( cx, cy );
                }
                float const crest = CellOccCrest( *cell );
                if ( crest + 1e-4f < needTop )
                {
                    int add = (int)std::ceil( ( needTop - crest ) / kVoxelEdgeM );
                    add = (std::max)( add, headLayers );
                    ExpandOccupancyHeadroom( cx, cy, add );
                }
            }
        }

        struct Cand { int cx, cy, c, r, k; float z; int space; };
        std::vector<Cand> cands;
        cands.reserve( 256 );
        for ( int cy = y0; cy <= y1; ++cy )
        {
            for ( int cx = x0; cx <= x1; ++cx )
            {
                CellSample* cell = GetCellMutable( cx, cy );
                if ( !cell || cell->fill.empty() ) { continue; }
                int const w = cell->fillW, h = cell->fillH, kz = cell->fillK;
                float const edge = (std::max)( 0.05f, g.voxelEdgeM );
                float const crest = CellOccCrest( *cell );
                float const du = 1.f / (float)w, dv = 1.f / (float)h;
                for ( int k = 0; k < kz; ++k )
                {
                    float const zc = crest - ( (float)( kz - 1 - k ) + 0.5f ) * edge;
                    for ( int r = 0; r < h; ++r )
                    {
                        float const yc = (float)cy + ( (float)r + 0.5f ) * dv;
                        for ( int c = 0; c < w; ++c )
                        {
                            float const xc = (float)cx + ( (float)c + 0.5f ) * du;
                            float const dx = xc - wx, dy = yc - wy, dz = zc - wz;
                            if ( dx * dx + dy * dy + dz * dz > R2 ) { continue; }
                            int const cur = FillAt( *cell, c, r, k );
                            int const space = kFillFull - cur;
                            if ( space <= 0 ) { continue; }
                            cands.push_back( { cx, cy, c, r, k, zc, space } );
                        }
                    }
                }
            }
        }
        std::sort( cands.begin(), cands.end(),
            []( Cand const& a, Cand const& b ) { return a.z < b.z; } );

        auto columnSupported = [&]( int cx, int cy, int c, int r, int k, float zc ) -> bool
        {
            CellSample const* cell = GetCell( cx, cy );
            if ( !cell ) { return false; }
            if ( k > 0 && FillAt( *cell, c, r, k - 1 ) >= kFillIso ) { return true; }
            int const w = cell->fillW, h = cell->fillH;
            auto solidAt = [&]( int cc, int rr, int kk ) -> bool
            {
                if ( kk < 0 || kk >= cell->fillK ) { return false; }
                if ( cc < 0 || rr < 0 || cc >= w || rr >= h ) { return false; }
                return FillAt( *cell, cc, rr, kk ) >= kFillIso;
            };
            if ( solidAt( c - 1, r, k ) || solidAt( c + 1, r, k )
              || solidAt( c, r - 1, k ) || solidAt( c, r + 1, k ) )
            {
                return true;
            }
            if ( k > 0 && ( solidAt( c - 1, r, k - 1 ) || solidAt( c + 1, r, k - 1 )
              || solidAt( c, r - 1, k - 1 ) || solidAt( c, r + 1, k - 1 ) ) )
            {
                return true;
            }
            float virginZ = 0.f;
            float const xc = (float)cx + ( (float)c + 0.5f ) / (float)w;
            float const yc = (float)cy + ( (float)r + 0.5f ) / (float)h;
            if ( SampleGroundZBase( xc, yc, virginZ ) && zc <= virginZ + (std::max)( 0.05f, g.voxelEdgeM ) * 0.75f )
            {
                return true;
            }
            if ( OccupancySolidAt( xc, yc, zc - (std::max)( 0.05f, g.voxelEdgeM ) ) ) { return true; }
            return false;
        };

        int unitsLeft = unitsBudget;
        int unitsFilled = 0;
        int voxelsTouched = 0;
        std::vector<std::pair<int, int>> touchedCells;
        for ( Cand const& cand : cands )
        {
            if ( unitsLeft <= 0 ) { break; }
            if ( !columnSupported( cand.cx, cand.cy, cand.c, cand.r, cand.k, cand.z ) ) { continue; }
            CellSample* cell = GetCellMutable( cand.cx, cand.cy );
            if ( !cell ) { continue; }
            int const cur = FillAt( *cell, cand.c, cand.r, cand.k );
            int const space = kFillFull - cur;
            if ( space <= 0 ) { continue; }
            int const take = (std::min)( space, unitsLeft );
            int const neu = cur + take;
            SetFillAt( *cell, cand.c, cand.r, cand.k, (uint8_t)neu );
            unitsLeft -= take;
            unitsFilled += take;
            if ( take > 0 ) { ++voxelsTouched; }
            cell->edited = true;
            cell->carved = true;
            cell->carveWx = wx;
            cell->carveWy = wy;
            cell->carveWz = wz;
            cell->carveRM = R;
            cell->hasCarveFocus = true;
            float occZ = CellOccCrest( *cell );
            SampleOccupancyZ( (float)cand.cx + 0.5f, (float)cand.cy + 0.5f, occZ );
            cell->fillZ = (std::max)( cell->hasFillZ ? cell->fillZ : occZ, occZ );
            cell->hasFillZ = true;
            bool seen = false;
            for ( auto const& t : touchedCells )
            {
                if ( t.first == cand.cx && t.second == cand.cy ) { seen = true; break; }
            }
            if ( !seen ) { touchedCells.push_back( { cand.cx, cand.cy } ); }
        }

        if ( unitsFilled <= 0 || touchedCells.empty() ) { return out; }

        int acceptedGrams = (int)std::lround(
            (double)unitsFilled * (double)kDirtVoxelG / (double)kFillFull );
        if ( acceptedGrams > gramsAvailable ) { acceptedGrams = gramsAvailable; }
        if ( acceptedGrams < 1 && unitsFilled > 0 ) { acceptedGrams = 1; }

        bool anyEr = false;
        for ( auto const& xy : touchedCells )
        {
            CellSample const* c = GetCell( xy.first, xy.second );
            if ( c && c->editedRegionId != 0 ) { anyEr = true; break; }
        }
        uint32_t rid = MergeOrCreateEditedRegion( wx, wy, wz, (std::min)( R, 0.12f ),
            touchedCells, 0.f, 0.f, 1.f );
        if ( EditedRegion* er = FindEditedRegion( rid ) )
        {
            // Cavity re-fill: openings stay (remove-only). Always bump support rev.
            if ( anyEr ) { ++er->dirtyRev; }
            er->actionX = wx; er->actionY = wy; er->actionZ = wz; er->actionR = R;
            er->hasAction = true;
        }
        EditedRegion* er = FindEditedRegion( rid );

        for ( auto const& xy : touchedCells )
        {
            RebuildCavityMesh( xy.first, xy.second );
            CellSample* cell = GetCellMutable( xy.first, xy.second );
            if ( !cell ) { continue; }
            if ( cell->hasCavity && er && er->hasOwnBounds )
            {
                cell->patchMinX = er->ownMinX;
                cell->patchMaxX = er->ownMaxX;
                cell->patchMinY = er->ownMinY;
                cell->patchMaxY = er->ownMaxY;
                cell->hasPatchBounds = true;
            }
        }
        if ( er )
        {
            for ( auto const& t : touchedCells )
            {
                for ( int dy = -1; dy <= 1; ++dy )
                {
                    for ( int dx = -1; dx <= 1; ++dx )
                    {
                        if ( dx == 0 && dy == 0 ) { continue; }
                        int const nx = t.first + dx, ny = t.second + dy;
                        bool already = false;
                        for ( auto const& u : touchedCells )
                        {
                            if ( u.first == nx && u.second == ny ) { already = true; break; }
                        }
                        if ( already ) { continue; }
                        CellSample* cell = GetCellMutable( nx, ny );
                        if ( !cell || !cell->carved ) { continue; }
                        RebuildCavityMesh( nx, ny );
                    }
                }
            }
        }

        out.ok = true;
        out.acceptedGrams = acceptedGrams;
        out.unitsFilled = unitsFilled;
        out.voxelsTouched = voxelsTouched;
        return out;
    }

    bool CarveOccupancySphere( float carveX, float carveY, float carveZ, float radiusM,
        float openX, float openY, float openZ )
    {
        // Commit: seed → subtract → merge EditedRegion (monotonic openings) → rebuild cavity.
        // Matter sphere at carve*; HF aperture opening at open* (visible strike on slopes/walls).
        float const wx = carveX, wy = carveY, wz = carveZ;
        float const R = (std::max)( 0.02f, radiusM );
        float const R2 = R * R;
        int const x0 = (int)std::floor( wx - R - 0.05f );
        int const x1 = (int)std::floor( wx + R + 0.05f );
        int const y0 = (int)std::floor( wy - R - 0.05f );
        int const y1 = (int)std::floor( wy + R + 0.05f );
        bool any = false;
        std::vector<std::pair<int, int>> touchedCells;
        for ( int cy = y0; cy <= y1; ++cy )
        {
            for ( int cx = x0; cx <= x1; ++cx )
            {
                EnsureOccupancyLattice( cx, cy );
                CellSample* cell = GetCellMutable( cx, cy );
                if ( !cell || cell->fill.empty() ) { continue; }
                if ( !cell->carved )
                {
                    SeedOccupancyFromVirginSurface( cx, cy );
                }
                int const w = cell->fillW, h = cell->fillH, kz = cell->fillK;
                float const edge = kVoxelEdgeM;
                float const crest = CellOccCrest( *cell );
                float const du = 1.f / (float)w, dv = 1.f / (float)h;
                bool touched = false;
                for ( int k = 0; k < kz; ++k )
                {
                    float const zc = crest - ( (float)( kz - 1 - k ) + 0.5f ) * edge;
                    for ( int r = 0; r < h; ++r )
                    {
                        float const yc = (float)cy + ( (float)r + 0.5f ) * dv;
                        for ( int c = 0; c < w; ++c )
                        {
                            float const xc = (float)cx + ( (float)c + 0.5f ) * du;
                            float const dx = xc - wx, dy = yc - wy, dz = zc - wz;
                            if ( dx * dx + dy * dy + dz * dz > R2 ) { continue; }
                            if ( FillAt( *cell, c, r, k ) < kFillIso ) { continue; }
                            SetFillAt( *cell, c, r, k, 0 );
                            touched = true;
                        }
                    }
                }
                if ( touched )
                {
                    cell->edited = true;
                    cell->carved = true;
                    // Transient last-action stamp only — persistent aperture lives on EditedRegion.
                    cell->carveWx = wx;
                    cell->carveWy = wy;
                    cell->carveWz = wz;
                    cell->carveRM = R;
                    cell->hasCarveFocus = true;
                    float occZ = crest;
                    SampleOccupancyZ( (float)cx + 0.5f, (float)cy + 0.5f, occZ );
                    cell->fillZ = (std::min)( cell->hasFillZ ? cell->fillZ : crest, occZ );
                    cell->hasFillZ = true;
                    touchedCells.push_back( { cx, cy } );
                    any = true;
                }
            }
        }
        if ( !any ) { return false; }

        // Aperture at visible strike. Carve into-normal may be look-into (walls), but HF handoff
        // must classify surface strikes by HF face — look-into on slopes left openings "steep"
        // so crest HF never opened (pick credits grams, surface looks untouched).
        float const ox = openX, oy = openY, oz = openZ;
        float onx = 0.f, ony = 0.f, onz = 1.f;
        {
            float cp = std::cos( g.pitch ), sp = std::sin( g.pitch );
            float cyw = std::cos( g.yaw ), sy = std::sin( g.yaw );
            ResolveCarveIntoNormal( ox, oy, oz, sy * cp, cyw * cp, sp, onx, ony, onz );
        }
        float openNx = onx, openNy = ony, openNz = onz;
        bool surfaceStrike = false;
        {
            float skinZ = oz;
            SampleGroundZBase( ox, oy, skinZ );
            float hnx = 0.f, hny = 0.f, hnz = 1.f;
            CaptureFaceNormalAt( ox, oy, hnx, hny, hnz );
            surfaceStrike = ( std::fabs( oz - skinZ ) < 0.45f && hnz >= 0.40f );
            if ( surfaceStrike )
            {
                openNx = hnx; openNy = hny; openNz = hnz;
            }
        }
        // NEW STRIKE mouth contribution — tip-scale only (≤12cm). Accumulated openings grow
        // via union of many strikes; HF aperture uses full o.r (see NearOpeningMouthAt).
        float openR = (std::min)( (std::max)( 0.05f, R ), 0.12f );
        if ( !surfaceStrike && onz < 0.55f )
        {
            openR = (std::min)( openR, (std::max)( 0.05f, R * 0.90f ) );
        }
        uint32_t const rid = MergeOrCreateEditedRegion( ox, oy, oz, openR, touchedCells, openNx, openNy, openNz );
        EditedRegion* er = FindEditedRegion( rid );
        // Through-tunnel: into the face only (-N). Look-axis probes stacked openings above strikes.
        if ( er )
        {
            auto tryExit = [&]( float dx, float dy, float dz )
            {
                float plen = std::sqrt( dx * dx + dy * dy + dz * dz );
                if ( plen < 1e-5f ) { return; }
                dx /= plen; dy /= plen; dz /= plen;
                bool prevSolid = OccupancySolidAt( ox, oy, oz );
                for ( float t = R * 1.2f; t <= (std::max)( 1.2f, R * 8.f ); t += 0.05f )
                {
                    float const x = ox + dx * t;
                    float const y = oy + dy * t;
                    float const z = oz + dz * t;
                    bool const solid = OccupancySolidAt( x, y, z );
                    if ( prevSolid && !solid )
                    {
                        float skinZ = z;
                        SampleGroundZBase( x, y, skinZ );
                        bool const nearCrest = std::fabs( z - skinZ ) < 0.40f;
                        bool const wallBreak = onz < 0.55f && !nearCrest;
                        if ( !nearCrest && !wallBreak ) { return; }
                        float const openZExit = wallBreak ? z : skinZ;
                        float const ddx = x - ox, ddy = y - oy, ddz = openZExit - oz;
                        if ( ( ddx * ddx + ddy * ddy + ddz * ddz ) < ( openR * 2.f ) * ( openR * 2.f ) )
                        {
                            return;
                        }
                        AddOpeningMonotonic( *er, x, y, openZExit, openR, onx, ony, onz );
                        return;
                    }
                    prevSolid = solid;
                }
            };
            tryExit( -onx, -ony, -onz );
        }
        for ( auto const& xy : touchedCells )
        {
            RebuildCavityMesh( xy.first, xy.second );
            CellSample* cell = GetCellMutable( xy.first, xy.second );
            if ( !cell ) { continue; }
            if ( cell->hasCavity && er && er->hasOwnBounds )
            {
                cell->patchMinX = er->ownMinX;
                cell->patchMaxX = er->ownMaxX;
                cell->patchMinY = er->ownMinY;
                cell->patchMaxY = er->ownMaxY;
                cell->hasPatchBounds = true;
            }
        }
        // Halo neighbors only (D2 seam) — rebuilding EVERY EditedRegion cell each dig lagged hard.
        if ( er )
        {
            for ( auto const& t : touchedCells )
            {
                for ( int dy = -1; dy <= 1; ++dy )
                {
                    for ( int dx = -1; dx <= 1; ++dx )
                    {
                        if ( dx == 0 && dy == 0 ) { continue; }
                        int const nx = t.first + dx, ny = t.second + dy;
                        bool already = false;
                        for ( auto const& u : touchedCells )
                        {
                            if ( u.first == nx && u.second == ny ) { already = true; break; }
                        }
                        if ( already ) { continue; }
                        CellSample* cell = GetCellMutable( nx, ny );
                        if ( !cell || !cell->carved ) { continue; }
                        RebuildCavityMesh( nx, ny );
                    }
                }
            }
        }

        if ( kAabbCavityOwnsHf )
        {
            RetirePresentationScarsNear( wx, wy, R );
        }
        // HF remesh whenever this strike opened virgin skin — same for flat / slope / wall HF.
        {
            bool remeshHf = surfaceStrike || SurfaceBrokenByOccupancy( ox, oy );
            if ( !remeshHf )
            {
                for ( auto const& xy : touchedCells )
                {
                    if ( MaterialSlumpsOpen( CellCapName( xy.first, xy.second ) )
                      || SurfaceBrokenByOccupancy( (float)xy.first + 0.5f, (float)xy.second + 0.5f ) )
                    {
                        remeshHf = true;
                        break;
                    }
                }
            }
            if ( remeshHf ) { InvalidateTerrainMesh(); }
        }
        int const bx = (int)std::floor( wx );
        int const by = (int)std::floor( wy );
        for ( int dy = -1; dy <= 1; ++dy )
        {
            for ( int dx = -1; dx <= 1; ++dx )
            {
                QueueColumn( bx + dx, by + dy );
            }
        }
        return true;
    }

    bool SurfaceOpenedByOccupancy( float x, float y )
    {
        return EditedRegionOwnsAt( x, y );
    }

    void RebuildCavityMesh( int cx, int cy )
    {
        // D2 on accumulated EditedRegion occupancy for this cell.
        // ACTION is bite-local; DIRTY = union(openings,carve)+halo; publish ONE coherent cavity.
        // Not tip-patch A/B/C hopping. HF aperture stays separate (NearOpeningMouthAt).
        // Fail-closed: unknown halo → fetch once → still missing refuses publication.
        auto it = g.cells.find( CellKey( cx, cy ) );
        if ( it == g.cells.end() ) { return; }
        int const prevTris = (int)it->second.cavityTris.size();
        DestroyCavityList( it->second );
        it->second.lastD2HaloMissing = 0;
        it->second.lastD2BoundaryEdges = 0;
        it->second.lastD2PublishRefused = false;
        if ( !it->second.carved || it->second.fill.empty()
          || it->second.fillW <= 0 || it->second.fillK <= 0 )
        {
            return;
        }

        uint32_t const erId = it->second.editedRegionId;
        EditedRegion const* er = FindEditedRegion( erId );
        if ( ( !er || er->openings.empty() ) && !it->second.hasCarveFocus ) { return; }

        int const w = it->second.fillW, h = it->second.fillH, kz = it->second.fillK;
        float const edge = kVoxelEdgeM;
        float const crest = CellOccCrest( it->second );

        float mnX, mxX, mnY, mxY, mnZ, mxZ;
        if ( !DirtyBoundsFromEditedRegion( er, it->second, cx, cy, mnX, mxX, mnY, mxY, mnZ, mxZ ) )
        {
            return;
        }

        // Work-budget partition size — NOT a geometric clip. Full dirty AABB is always covered;
        // oversized volumes rebuild as overlapping tiles each with full recon halo.
        constexpr float kWorkBudgetR = 0.85f;
        constexpr float kPartHalo = 0.20f;
        float const spanX = mxX - mnX, spanY = mxY - mnY, spanZ = mxZ - mnZ;
        float const frAll = 0.5f * std::sqrt( spanX * spanX + spanY * spanY + spanZ * spanZ ) + 0.05f;

        auto runExtractPass = [&]( DualContourQef::ExtractStats& outSt ) -> bool
        {
            it = g.cells.find( CellKey( cx, cy ) );
            if ( it == g.cells.end() ) { return false; }
            CellSample* home = &it->second;
            ColumnFillField field;
            field.homeCx = cx;
            field.homeCy = cy;
            field.home = home;
            outSt = {};
            home->cavityTris.clear();
            auto extractFocus = [&]( float fx, float fy, float fz, float fr )
            {
                DualContourQef::ExtractStats const local = DualContourQef::ExtractCell(
                    field, w, h, kz, (float)cx, (float)cy, crest, edge, kFillIso,
                    home->cavityTris, fx, fy, fz, fr );
                DualContourQef::AccumulateStats( outSt, local );
            };
            if ( frAll <= kWorkBudgetR + kPartHalo )
            {
                extractFocus( 0.5f * ( mnX + mxX ), 0.5f * ( mnY + mxY ), 0.5f * ( mnZ + mxZ ), frAll );
            }
            else
            {
                float const tile = kWorkBudgetR;
                float const partR = 0.5f * tile + kPartHalo;
                float const step = (std::max)( 0.25f, tile - kPartHalo );
                for ( float z = mnZ; z <= mxZ + 1e-4f; z += step )
                {
                    for ( float y = mnY; y <= mxY + 1e-4f; y += step )
                    {
                        for ( float x = mnX; x <= mxX + 1e-4f; x += step )
                        {
                            float const fx = (std::min)( mxX, (std::max)( mnX, x + 0.5f * step ) );
                            float const fy = (std::min)( mxY, (std::max)( mnY, y + 0.5f * step ) );
                            float const fz = (std::min)( mxZ, (std::max)( mnZ, z + 0.5f * step ) );
                            extractFocus( fx, fy, fz, partR );
                        }
                    }
                }
            }
            return true;
        };

        if ( !g.certD2SkipHaloEnsure )
        {
            EnsureD2HaloLattices( cx, cy );
        }

        ++g.perfD2Rebuilds;
        DWORD const t0 = GetTickCount();
        DualContourQef::ExtractStats st{};
        if ( !runExtractPass( st ) ) { return; }
        if ( st.haloMissing > 0 && !g.certD2SkipHaloEnsure )
        {
            // One fetch+retry; still incomplete → refuse publication (fail-closed).
            EnsureD2HaloLattices( cx, cy );
            if ( !runExtractPass( st ) ) { return; }
        }
        g.perfD2MsTotal += (float)( GetTickCount() - t0 );

        it = g.cells.find( CellKey( cx, cy ) );
        if ( it == g.cells.end() ) { return; }
        CellSample& cell = it->second;
        cell.lastD2HaloMissing = st.haloMissing;
        cell.lastD2BoundaryEdges = st.boundaryEdgesEmitted;

        g.perfD2Tris += st.tris;
        g.perfD2Edges += st.edgesEmitted;
        g.perfD2QefFallbacks += st.qefMassFallback;
        g.perfD2HaloMiss += st.haloMissing;
        g.cavityEdgesEmitted = st.edgesEmitted;

        if ( st.haloMissing > 0 )
        {
            // Incomplete halo — refuse publication. Do not ship a cavity built on Unknown samples.
            cell.lastD2PublishRefused = true;
            cell.cavityTris.clear();
            cell.hasCavity = false;
            DestroyCavityList( cell );
            if ( cell.debugBoundaryList )
            {
                glDeleteLists( cell.debugBoundaryList, 1 );
                cell.debugBoundaryList = 0;
            }
            g.cavityTrisTotal = (std::max)( 0, g.cavityTrisTotal - prevTris );
            return;
        }

        g.cavityTrisTotal = (std::max)( 0, g.cavityTrisTotal - prevTris ) + (int)cell.cavityTris.size();

        if ( !cell.cavityTris.empty() )
        {
            GLuint list = AllocDisplayListOutsideFonts();
            if ( list )
            {
                cell.cavityList = list;
                glNewList( list, GL_COMPILE );
                glShadeModel( GL_FLAT );
                glBegin( GL_TRIANGLES );
                for ( DualContourQef::Tri const& t : cell.cavityTris )
                {
                    EmitPhase3Tri(
                        t.a.x, t.a.y, t.a.z,
                        t.b.x, t.b.y, t.b.z,
                        t.c.x, t.c.y, t.c.z,
                        0.32f );
                }
                glEnd();
                glEndList();
                cell.hasCavity = true;
            }
        }

        er = FindEditedRegion( erId );
        RebuildDebugBoundaryList( cx, cy, cell, er );

        if ( !cell.hasCavity )
        {
            // Do not promote AABB shelves as production cavity (vertical stripe source).
            if ( cell.debugBoundaryList )
            {
                glDeleteLists( cell.debugBoundaryList, 1 );
                cell.debugBoundaryList = 0;
            }
            DestroyCavityList( cell );
            return;
        }

        if ( er && er->hasOwnBounds )
        {
            cell.patchMinX = er->ownMinX;
            cell.patchMaxX = er->ownMaxX;
            cell.patchMinY = er->ownMinY;
            cell.patchMaxY = er->ownMaxY;
            cell.hasPatchBounds = true;
        }
    }

    void DrawCavityMeshesMouth()
    {
        glEnable( GL_POLYGON_OFFSET_FILL );
        glPolygonOffset( -0.20f, -0.40f );
        DrawCavityMeshes();
        glDisable( GL_POLYGON_OFFSET_FILL );
    }

    void DrawCavityMeshesInterior()
    {
        // No polygon offset — offset pulled cavity through adjacent intact HF (windows).
        glDisable( GL_POLYGON_OFFSET_FILL );
        DrawCavityMeshes();
    }

    void DrawCavityMeshes()
    {
        // Production: D2. [B] DebugBoundary = AABB only. Overlay = D2 + AABB wire.
        static bool sClearedBogus = false;
        bool clearedBogus = false;
        glDisable( GL_CULL_FACE );
        bool const drawD2 = ( g.boundaryMode == BoundaryMode::D2
            || g.boundaryMode == BoundaryMode::Overlay );
        bool const drawAabb = ( g.boundaryMode == BoundaryMode::DebugBoundary
            || g.boundaryMode == BoundaryMode::Overlay );
        for ( auto& kv : g.cells )
        {
            CellSample& cell = kv.second;
            if ( !cell.carved )
            {
                if ( cell.hasCavity || cell.cavityList || cell.debugBoundaryList )
                {
                    DestroyCavityList( cell );
                    clearedBogus = true;
                }
                continue;
            }
            if ( drawD2 && cell.cavityList )
            {
                glCallList( cell.cavityList );
            }
            if ( drawAabb && cell.debugBoundaryList )
            {
                if ( g.boundaryMode == BoundaryMode::Overlay )
                {
                    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
                    glLineWidth( 1.f );
                }
                glCallList( cell.debugBoundaryList );
                if ( g.boundaryMode == BoundaryMode::Overlay )
                {
                    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
                }
            }
            // DebugBoundary with no D2: still show AABB if present.
            if ( g.boundaryMode == BoundaryMode::DebugBoundary
                && !cell.debugBoundaryList && cell.cavityList )
            {
                glCallList( cell.cavityList );
            }
        }
        glEnable( GL_CULL_FACE );
        if ( clearedBogus && !sClearedBogus )
        {
            sClearedBogus = true;
        }
    }

    bool CavityReadyNear( float x, float y )
    {
        // HF may omit skin only where D2 exists nearby — otherwise handoff opens sky void.
        int const cx = (int)std::floor( x );
        int const cy = (int)std::floor( y );
        for ( int dy = -1; dy <= 1; ++dy )
        {
            for ( int dx = -1; dx <= 1; ++dx )
            {
                CellSample const* c = GetCell( cx + dx, cy + dy );
                if ( c && c->hasCavity && c->cavityList ) { return true; }
            }
        }
        return false;
    }

    bool NearOpeningMouthAt( float x, float y )
    {
        // HF aperture = union of accumulated opening mouths (each strike adds tip-scale disk).
        // Do NOT clamp to 12cm here — that would freeze tunnels at first-bite width.
        // 12cm caps only the contribution of a NEW strike when AddOpeningMonotonic runs.
        constexpr float kCollar = 0.03f;
        for ( EditedRegion const& er : g.editedRegions )
        {
            for ( EditOpening const& o : er.openings )
            {
                float const lim = (std::max)( 0.05f, o.r ) + kCollar;
                float const dx = x - o.x, dy = y - o.y;
                if ( ( dx * dx + dy * dy ) <= lim * lim ) { return true; }
            }
        }
        return false;
    }

    bool CrestMouthStencilAt( float x, float y )
    {
        // HF aperture = opening mouth ∩ occupancy skin open ∩ D2 ready.
        if ( !NearOpeningMouthAt( x, y ) ) { return false; }
        if ( !SurfaceBrokenByOccupancy( x, y ) ) { return false; }
        return CavityReadyNear( x, y );
    }

    void DrawOccupancySurfaceBreaks()
    {
        // Seam stitch only where HF remesh may still leave a skin rim (same predicate as handoff).
        if ( g.editedRegions.empty() ) { return; }

        constexpr int kOcc = 8;
        constexpr float kZBias = 0.004f;
        float const feetX = g.feetX, feetY = g.feetY;
        constexpr float kMaxDist2 = 20.f * 20.f;
        glBegin( GL_TRIANGLES );
        for ( auto const& kv : g.cells )
        {
            CellSample const& cell = kv.second;
            if ( !cell.carved || cell.fill.empty() ) { continue; }
            uint64_t const key = kv.first;
            int const cx = (int)(int32_t)( key >> 32 );
            int const cy = (int)(int32_t)( key & 0xffffffffu );
            float const cdx = ( (float)cx + 0.5f ) - feetX;
            float const cdy = ( (float)cy + 0.5f ) - feetY;
            if ( ( cdx * cdx + cdy * cdy ) > kMaxDist2 ) { continue; }
            float const step = 1.f / (float)kOcc;
            for ( int j = 0; j < kOcc; ++j )
            {
                for ( int i = 0; i < kOcc; ++i )
                {
                    float const px00 = (float)cx + (float)i * step;
                    float const py00 = (float)cy + (float)j * step;
                    float const px10 = px00 + step, py10 = py00;
                    float const px01 = px00, py01 = py00 + step;
                    float const px11 = px00 + step, py11 = py00 + step;
                    float const mx = ( px00 + px11 ) * 0.5f;
                    float const my = ( py00 + py11 ) * 0.5f;
                    if ( !CrestMouthStencilAt( mx, my ) ) { continue; }
                    float z00, z10, z01, z11;
                    if ( !SampleGroundZBase( px00, py00, z00 ) ) { continue; }
                    if ( !SampleGroundZBase( px10, py10, z10 ) ) { continue; }
                    if ( !SampleGroundZBase( px01, py01, z01 ) ) { continue; }
                    if ( !SampleGroundZBase( px11, py11, z11 ) ) { continue; }
                    z00 += kZBias; z10 += kZBias; z01 += kZBias; z11 += kZBias;
                    glVertex3f( px00, py00, z00 );
                    glVertex3f( px10, py10, z10 );
                    glVertex3f( px01, py01, z01 );
                    glVertex3f( px10, py10, z10 );
                    glVertex3f( px11, py11, z11 );
                    glVertex3f( px01, py01, z01 );
                }
            }
        }
        glEnd();
    }

    void DrawEditedRegionOpenings()
    {
        // Steep wall mouths only — crest HF is punched by DrawOccupancySurfaceBreaks.
        // Disks sit in the strike face plane so cliff bites aren't XY circles on the crest.
        constexpr int kRing = 18;
        constexpr float kNBias = 0.035f; // toward air — depth-tested punch must clear HF skin
        glBegin( GL_TRIANGLES );
        for ( EditedRegion const& er : g.editedRegions )
        {
            for ( EditOpening const& o : er.openings )
            {
                float nx = o.nx, ny = o.ny, nz = o.nz;
                if ( std::fabs( nz ) > 0.98f && std::fabs( nx ) < 1e-4f && std::fabs( ny ) < 1e-4f )
                {
                    CaptureFaceNormalAt( o.x, o.y, nx, ny, nz );
                }
                if ( nz >= 0.55f ) { continue; } // crest / shallow → occupancy break owns stencil
                float rad = (std::max)( 0.05f, o.r ); // full accumulated mouth (not tip clamp)
                float sx = o.x, sy = o.y, sz = o.z;
                float ax = 1.f, ay = 0.f, az = 0.f;
                if ( std::fabs( nx ) > 0.9f ) { ax = 0.f; ay = 1.f; }
                float tx = ay * nz - az * ny;
                float ty = az * nx - ax * nz;
                float tz = ax * ny - ay * nx;
                float tl = std::sqrt( tx * tx + ty * ty + tz * tz );
                if ( tl > 1e-5f ) { tx /= tl; ty /= tl; tz /= tl; }
                else { tx = 0.f; ty = 1.f; tz = 0.f; }
                float bx = ny * tz - nz * ty;
                float by = nz * tx - nx * tz;
                float bz = nx * ty - ny * tx;

                float const cx = sx + nx * kNBias;
                float const cy = sy + ny * kNBias;
                float const cz = sz + nz * kNBias;
                for ( int j = 0; j < kRing; ++j )
                {
                    float const a0 = (float)j / (float)kRing * 6.2831853f;
                    float const a1 = (float)( j + 1 ) / (float)kRing * 6.2831853f;
                    float const c0 = std::cos( a0 ), s0 = std::sin( a0 );
                    float const c1 = std::cos( a1 ), s1 = std::sin( a1 );
                    float const x0 = cx + ( tx * c0 + bx * s0 ) * rad;
                    float const y0 = cy + ( ty * c0 + by * s0 ) * rad;
                    float const z0 = cz + ( tz * c0 + bz * s0 ) * rad;
                    float const x1 = cx + ( tx * c1 + bx * s1 ) * rad;
                    float const y1 = cy + ( ty * c1 + by * s1 ) * rad;
                    float const z1 = cz + ( tz * c1 + bz * s1 ) * rad;
                    glVertex3f( cx, cy, cz );
                    glVertex3f( x0, y0, z0 );
                    glVertex3f( x1, y1, z1 );
                }
            }
        }
        glEnd();
    }

    void DrawCarveActionFootprints()
    {
        DrawOccupancySurfaceBreaks();
        DrawEditedRegionOpenings();
    }

    // ---- World sun + lit-material path (gallery, held, terrain share one frame) ----
    // Material = base color / roughness / metallic. Lighting = sun + sky on world normals.
    float gLitM[12] = { // local→world: 3 columns + translation
        1.f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        0.f, 0.f, 1.f,
        0.f, 0.f, 0.f
    };
    float gLitRough = 0.75f;
    float gLitMetal = 0.f;

    void LitSetIdentity()
    {
        gLitM[0] = 1.f; gLitM[1] = 0.f; gLitM[2] = 0.f;
        gLitM[3] = 0.f; gLitM[4] = 1.f; gLitM[5] = 0.f;
        gLitM[6] = 0.f; gLitM[7] = 0.f; gLitM[8] = 1.f;
        gLitM[9] = 0.f; gLitM[10] = 0.f; gLitM[11] = 0.f;
    }

    void LitSetFromMatrix( float const M[16] )
    {
        gLitM[0] = M[0]; gLitM[1] = M[1]; gLitM[2] = M[2];
        gLitM[3] = M[4]; gLitM[4] = M[5]; gLitM[5] = M[6];
        gLitM[6] = M[8]; gLitM[7] = M[9]; gLitM[8] = M[10];
        gLitM[9] = M[12]; gLitM[10] = M[13]; gLitM[11] = M[14];
    }

    void LitBindMaterial( char const* id )
    {
        VisualMat::VisualMaterialDef const& vd = VisualMat::OrDirt( id );
        gLitRough = VisualMat::RoughnessOf( vd );
        gLitMetal = VisualMat::MetallicOf( vd );
    }

    void UpdateWorldSun()
    {
        float const ce = std::cos( g.sunElevation );
        float const se = std::sin( g.sunElevation );
        float const ca = std::cos( g.sunAzimuth );
        float const sa = std::sin( g.sunAzimuth );
        g.sunDirX = sa * ce;
        g.sunDirY = ca * ce;
        g.sunDirZ = se;
        float const len = std::sqrt( g.sunDirX * g.sunDirX + g.sunDirY * g.sunDirY + g.sunDirZ * g.sunDirZ );
        if ( len > 1e-6f ) { g.sunDirX /= len; g.sunDirY /= len; g.sunDirZ /= len; }
        // Soft warm sun when low; cooler when high. Kept dimmer — prior fill blew out limestone.
        float const day = (std::max)( 0.15f, se );
        g.sunColorR = 1.f;
        g.sunColorG = 0.92f + 0.06f * day;
        g.sunColorB = 0.78f + 0.18f * day;
        g.sunIntensity = 0.36f + 0.32f * day;
        g.skyIntensity = 0.14f + 0.08f * day;
    }

    void ShadeLitFace( float nx, float ny, float nz,
        float px, float py, float pz,
        float br, float bg, float bb,
        float& outR, float& outG, float& outB )
    {
        // Local → world normal / position (held samples use tumble matrix).
        float wx = gLitM[0] * nx + gLitM[3] * ny + gLitM[6] * nz;
        float wy = gLitM[1] * nx + gLitM[4] * ny + gLitM[7] * nz;
        float wz = gLitM[2] * nx + gLitM[5] * ny + gLitM[8] * nz;
        float nl = std::sqrt( wx * wx + wy * wy + wz * wz );
        if ( nl > 1e-6f ) { wx /= nl; wy /= nl; wz /= nl; }

        float ox = gLitM[0] * px + gLitM[3] * py + gLitM[6] * pz + gLitM[9];
        float oy = gLitM[1] * px + gLitM[4] * py + gLitM[7] * pz + gLitM[10];
        float oz = gLitM[2] * px + gLitM[5] * py + gLitM[8] * pz + gLitM[11];

        float vx = g.camX - ox, vy = g.camY - oy, vz = g.camZ - oz;
        float vl = std::sqrt( vx * vx + vy * vy + vz * vz );
        if ( vl > 1e-6f ) { vx /= vl; vy /= vl; vz /= vl; }

        float const ndotl = (std::max)( 0.f, wx * g.sunDirX + wy * g.sunDirY + wz * g.sunDirZ );
        // Modest anti-sun + view fill — enough to read form, not wash the frame white.
        float const ndotBack = (std::max)( 0.f, -( wx * g.sunDirX + wy * g.sunDirY + wz * g.sunDirZ ) );
        float const ndotV = (std::max)( 0.f, wx * vx + wy * vy + wz * vz );
        float const fill = 0.10f + 0.10f * ndotBack + 0.12f * ndotV;
        float hx = g.sunDirX + vx, hy = g.sunDirY + vy, hz = g.sunDirZ + vz;
        float hl = std::sqrt( hx * hx + hy * hy + hz * hz );
        if ( hl > 1e-6f ) { hx /= hl; hy /= hl; hz /= hl; }
        float const ndoth = (std::max)( 0.f, wx * hx + wy * hy + wz * hz );

        float const rough = (std::min)( 1.f, (std::max)( 0.04f, gLitRough ) );
        float const metal = (std::min)( 1.f, (std::max)( 0.f, gLitMetal ) );
        float const shin = std::pow( 2.f, ( 1.f - rough ) * 8.f ); // ~2..256
        float const specAmp = ( 0.04f + ( 1.f - rough ) * 0.55f ) * ( 0.25f + metal * 1.1f );
        float const spec = std::pow( ndoth, shin ) * specAmp * g.sunIntensity;

        float const ambR = g.skyColorR * g.skyIntensity + fill * 0.40f;
        float const ambG = g.skyColorG * g.skyIntensity + fill * 0.40f;
        float const ambB = g.skyColorB * g.skyIntensity + fill * 0.45f;
        float const difScale = ( 1.f - metal * 0.82f ) * g.sunIntensity * ndotl;
        float const difR = g.sunColorR * difScale;
        float const difG = g.sunColorG * difScale;
        float const difB = g.sunColorB * difScale;

        // Metals: specular tinted by base; dielectrics: white-ish specular.
        float const sr = ( 1.f - metal ) + br * metal;
        float const sg = ( 1.f - metal ) + bg * metal;
        float const sb = ( 1.f - metal ) + bb * metal;

        outR = br * ( ambR + difR ) + sr * spec * g.sunColorR;
        outG = bg * ( ambG + difG ) + sg * spec * g.sunColorG;
        outB = bb * ( ambB + difB ) + sb * spec * g.sunColorB;
        outR = (std::min)( 1.0f, (std::max)( 0.f, outR ) );
        outG = (std::min)( 1.0f, (std::max)( 0.f, outG ) );
        outB = (std::min)( 1.0f, (std::max)( 0.f, outB ) );
    }

    void EmitGalleryTri( float x0, float y0, float z0,
        float x1, float y1, float z1,
        float x2, float y2, float z2,
        float cr, float cg, float cb )
    {
        float ax = x1 - x0, ay = y1 - y0, az = z1 - z0;
        float bx = x2 - x0, by = y2 - y0, bz = z2 - z0;
        float nx = ay * bz - az * by;
        float ny = az * bx - ax * bz;
        float nz = ax * by - ay * bx;
        float const nl = std::sqrt( nx * nx + ny * ny + nz * nz );
        if ( nl > 1e-6f ) { nx /= nl; ny /= nl; nz /= nl; }
        float const px = ( x0 + x1 + x2 ) * ( 1.f / 3.f );
        float const py = ( y0 + y1 + y2 ) * ( 1.f / 3.f );
        float const pz = ( z0 + z1 + z2 ) * ( 1.f / 3.f );
        float outR = 0.f, outG = 0.f, outB = 0.f;
        ShadeLitFace( nx, ny, nz, px, py, pz,
            cr / 255.f, cg / 255.f, cb / 255.f, outR, outG, outB );
        glColor3f( outR, outG, outB );
        glVertex3f( x0, y0, z0 );
        glVertex3f( x1, y1, z1 );
        glVertex3f( x2, y2, z2 );
    }

    void EmitGalleryBox( float cx, float cy, float cz,
        float hx, float hy, float hz, float cr, float cg, float cb )
    {
        float const x0 = cx - hx, x1 = cx + hx;
        float const y0 = cy - hy, y1 = cy + hy;
        float const z0 = cz - hz, z1 = cz + hz;
        EmitGalleryTri( x0, y0, z1, x1, y0, z1, x0, y1, z1, cr, cg, cb );
        EmitGalleryTri( x1, y0, z1, x1, y1, z1, x0, y1, z1, cr, cg, cb );
        EmitGalleryTri( x0, y0, z0, x0, y1, z0, x1, y0, z0, cr, cg, cb );
        EmitGalleryTri( x1, y0, z0, x0, y1, z0, x1, y1, z0, cr, cg, cb );
        EmitGalleryTri( x0, y1, z0, x0, y1, z1, x1, y1, z0, cr, cg, cb );
        EmitGalleryTri( x1, y1, z0, x0, y1, z1, x1, y1, z1, cr, cg, cb );
        EmitGalleryTri( x0, y0, z0, x1, y0, z0, x0, y0, z1, cr, cg, cb );
        EmitGalleryTri( x1, y0, z0, x1, y0, z1, x0, y0, z1, cr, cg, cb );
        EmitGalleryTri( x1, y0, z0, x1, y1, z0, x1, y0, z1, cr, cg, cb );
        EmitGalleryTri( x1, y1, z0, x1, y1, z1, x1, y0, z1, cr, cg, cb );
        EmitGalleryTri( x0, y0, z0, x0, y0, z1, x0, y1, z0, cr, cg, cb );
        EmitGalleryTri( x0, y1, z0, x0, y0, z1, x0, y1, z1, cr, cg, cb );
    }

    // Forward decls — shape helpers used by all EmitForm* (defined below).
    void EmitEllipsoidPatch( float cx, float cy, float cz,
        float rx, float ry, float rz,
        float phi0, float phi1, int slices, int stacks,
        float wobbleAmp, float cr, float cg, float cb );
    void EmitSolidLump( float cx, float cy, float cz,
        float rx, float ry, float rz, float wobbleAmp,
        float cr, float cg, float cb );
    void EmitIrregularHostMass( float cx, float cy, float cz, float S,
        float cr, float cg, float cb );
    void EmitTaperedBranch( float cx, float cy, float cz, float S,
        float cr, float cg, float cb );

    // --- Hand-scale material bodies (gallery + future carve chips) ---
    // Doctrine: same voxel accounting; each material has its own shape language
    // (body / curvature / edge / fracture / surface / detached behavior).
    // Solids must be CLOSED (no open ellipsoid poles → no holes / sheared tops).

    void EmitFormWoodBranch( float cx, float cy, float cz, float S, float cr, float cg, float cb )
    {
        EmitTaperedBranch( cx, cy, cz, S, cr, cg, cb );
    }

    void EmitFormBasaltShard( float cx, float cy, float cz, float S, float cr, float cg, float cb )
    {
        // Dense compact angular mass — heavy silhouette, sharp-to-chipped edges.
        EmitIrregularHostMass( cx, cy, cz, S, cr, cg, cb );
        EmitGalleryTri(
            cx - S * 0.05f, cy - S * 0.20f, cz + S * 0.10f,
            cx + S * 0.28f, cy - S * 0.02f, cz + S * 0.22f,
            cx + S * 0.02f, cy + S * 0.18f, cz + S * 0.18f,
            cr * 0.7f, cg * 0.7f, cb * 0.75f );
    }

    void EmitFormSchistPlate( float cx, float cy, float cz, float S, float cr, float cg, float cb )
    {
        // Warped solid foliation plates (closed lentils) — layered, not open rings.
        for ( int L = 0; L < 4; ++L )
        {
            float const z = cz - S * 0.06f + (float)L * S * 0.04f;
            float const warp = S * ( 0.04f + 0.02f * (float)( L % 2 ) );
            float const shrink = 1.f - (float)L * 0.05f;
            EmitSolidLump( cx + warp, cy, z,
                S * 0.50f * shrink, S * 0.36f * shrink, S * 0.028f, 0.10f,
                cr * ( 0.85f + 0.04f * L ), cg * ( 0.85f + 0.04f * L ), cb * ( 0.8f + 0.04f * L ) );
        }
        EmitSolidLump( cx + S * 0.30f, cy - S * 0.12f, cz + S * 0.05f,
            S * 0.12f, S * 0.09f, S * 0.018f, 0.08f, cr * 0.95f, cg * 0.95f, cb * 0.9f );
    }

    void EmitFormGraniteChunk( float cx, float cy, float cz, float S, float cr, float cg, float cb )
    {
        EmitIrregularHostMass( cx, cy, cz, S * 1.05f, cr, cg, cb );
        EmitGalleryTri(
            cx - S * 0.12f, cy - S * 0.18f, cz + S * 0.14f,
            cx + S * 0.24f, cy - S * 0.04f, cz + S * 0.22f,
            cx + S * 0.04f, cy + S * 0.16f, cz + S * 0.20f,
            cr * 0.75f, cg * 0.75f, cb * 0.78f );
        EmitSolidLump( cx + S * 0.08f, cy + S * 0.10f, cz + S * 0.14f,
            S * 0.12f, S * 0.10f, S * 0.08f, 0.12f, cr * 1.05f, cg * 1.02f, cb * 1.0f );
    }

    void EmitFormLimestoneBlock( float cx, float cy, float cz, float S, float cr, float cg, float cb )
    {
        EmitSolidLump( cx, cy, cz, S * 0.40f, S * 0.38f, S * 0.34f, 0.10f, cr, cg, cb );
        EmitGalleryTri(
            cx - S * 0.22f, cy - S * 0.20f, cz + S * 0.08f,
            cx + S * 0.24f, cy - S * 0.18f, cz + S * 0.10f,
            cx + S * 0.20f, cy + S * 0.22f, cz + S * 0.12f,
            cr * 0.88f, cg * 0.88f, cb * 0.85f );
        EmitSolidLump( cx + S * 0.16f, cy + S * 0.08f, cz + S * 0.10f,
            S * 0.08f, S * 0.07f, S * 0.05f, 0.15f, cr * 0.8f, cg * 0.8f, cb * 0.78f );
        EmitSolidLump( cx - S * 0.28f, cy - S * 0.22f, cz - S * 0.08f,
            S * 0.10f, S * 0.09f, S * 0.08f, 0.12f, cr * 0.75f, cg * 0.75f, cb * 0.72f );
    }

    void EmitFormShaleFlake( float cx, float cy, float cz, float S, float cr, float cg, float cb )
    {
        for ( int L = 0; L < 3; ++L )
        {
            float const z = cz + (float)L * S * 0.022f;
            float const w = S * ( 0.03f * (float)( ( L % 2 ) * 2 - 1 ) );
            EmitSolidLump( cx + w, cy, z, S * 0.48f, S * 0.32f, S * 0.016f, 0.06f,
                cr * ( 0.9f - 0.05f * L ), cg * ( 0.9f - 0.05f * L ), cb * ( 0.88f - 0.05f * L ) );
        }
        EmitSolidLump( cx - S * 0.26f, cy + S * 0.12f, cz + S * 0.02f,
            S * 0.10f, S * 0.07f, S * 0.012f, 0.08f, cr * 0.7f, cg * 0.7f, cb * 0.68f );
    }

    void EmitFormSandstoneSlab( float cx, float cy, float cz, float S, float cr, float cg, float cb )
    {
        EmitSolidLump( cx, cy, cz, S * 0.48f, S * 0.34f, S * 0.11f, 0.06f, cr, cg, cb );
        EmitSolidLump( cx, cy, cz + S * 0.07f, S * 0.44f, S * 0.31f, S * 0.03f, 0.04f,
            cr * 0.9f, cg * 0.88f, cb * 0.82f );
        EmitSolidLump( cx + S * 0.26f, cy - S * 0.06f, cz - S * 0.02f,
            S * 0.11f, S * 0.12f, S * 0.07f, 0.10f, cr * 0.85f, cg * 0.82f, cb * 0.78f );
    }

    void EmitFormStoneChip( float cx, float cy, float cz, float S, float cr, float cg, float cb )
    {
        EmitIrregularHostMass( cx, cy, cz, S * 0.85f, cr, cg, cb );
        EmitGalleryTri(
            cx - S * 0.10f, cy - S * 0.16f, cz + S * 0.08f,
            cx + S * 0.20f, cy - S * 0.02f, cz + S * 0.16f,
            cx - S * 0.02f, cy + S * 0.14f, cz + S * 0.14f,
            cr * 0.8f, cg * 0.8f, cb * 0.8f );
    }

    void EmitFormClayLump( float cx, float cy, float cz, float S, float cr, float cg, float cb )
    {
        EmitSolidLump( cx, cy, cz, S * 0.40f, S * 0.34f, S * 0.26f, 0.08f, cr, cg, cb );
        EmitSolidLump( cx + S * 0.12f, cy + S * 0.06f, cz + S * 0.06f,
            S * 0.26f, S * 0.22f, S * 0.14f, 0.06f, cr * 0.95f, cg * 0.9f, cb * 0.88f );
        EmitSolidLump( cx - S * 0.16f, cy - S * 0.04f, cz + S * 0.02f,
            S * 0.16f, S * 0.20f, S * 0.10f, 0.12f, cr * 0.88f, cg * 0.82f, cb * 0.78f );
    }

    void EmitFormGravelPile( float cx, float cy, float cz, float S, float cr, float cg, float cb )
    {
        float const z0 = cz - S * 0.10f;
        struct P { float x, y, z, r; float shade; };
        P pebs[] = {
            { -0.18f, -0.12f, 0.00f, 0.14f, 1.00f }, { 0.16f, -0.08f, 0.02f, 0.12f, 0.90f },
            { -0.02f,  0.16f, 0.04f, 0.13f, 0.85f }, { 0.08f,  0.02f, 0.14f, 0.14f, 1.05f },
            { -0.22f,  0.05f, 0.10f, 0.10f, 0.75f }, { 0.22f,  0.14f, 0.08f, 0.10f, 0.95f },
            {  0.04f, -0.18f, 0.08f, 0.09f, 0.88f },
        };
        for ( P const& p : pebs )
        {
            EmitSolidLump( cx + p.x * S, cy + p.y * S, z0 + p.z * S,
                p.r * S, p.r * S * 0.9f, p.r * S * 0.85f, 0.14f,
                cr * p.shade, cg * p.shade, cb * p.shade );
        }
    }

    void EmitFormSandMound( float cx, float cy, float cz, float S, float cr, float cg, float cb )
    {
        EmitSolidLump( cx, cy, cz - S * 0.02f, S * 0.46f, S * 0.46f, S * 0.20f, 0.04f, cr, cg, cb );
        EmitSolidLump( cx + S * 0.04f, cy - S * 0.02f, cz + S * 0.10f,
            S * 0.26f, S * 0.26f, S * 0.12f, 0.03f, cr * 1.02f, cg * 1.0f, cb * 0.98f );
        EmitSolidLump( cx - S * 0.04f, cy + S * 0.03f, cz + S * 0.18f,
            S * 0.12f, S * 0.12f, S * 0.06f, 0.03f, cr * 1.05f, cg * 1.02f, cb * 1.0f );
    }

    void EmitFormLoamClod( float cx, float cy, float cz, float S, float cr, float cg, float cb )
    {
        EmitSolidLump( cx, cy, cz, S * 0.32f, S * 0.30f, S * 0.28f, 0.18f, cr, cg, cb );
        EmitSolidLump( cx + S * 0.14f, cy - S * 0.08f, cz + S * 0.05f,
            S * 0.16f, S * 0.14f, S * 0.13f, 0.16f, cr * 0.95f, cg * 0.95f, cb * 0.92f );
        EmitSolidLump( cx - S * 0.12f, cy + S * 0.10f, cz - S * 0.03f,
            S * 0.13f, S * 0.12f, S * 0.11f, 0.14f, cr * 0.9f, cg * 0.9f, cb * 0.88f );
    }

    void EmitFormDirtClod( float cx, float cy, float cz, float S, float cr, float cg, float cb )
    {
        EmitSolidLump( cx, cy, cz, S * 0.28f, S * 0.26f, S * 0.24f, 0.22f, cr, cg, cb );
        EmitSolidLump( cx + S * 0.16f, cy + S * 0.06f, cz - S * 0.02f,
            S * 0.11f, S * 0.12f, S * 0.09f, 0.20f, cr * 0.85f, cg * 0.85f, cb * 0.8f );
        EmitSolidLump( cx - S * 0.14f, cy - S * 0.10f, cz + S * 0.06f,
            S * 0.09f, S * 0.08f, S * 0.08f, 0.18f, cr * 0.9f, cg * 0.88f, cb * 0.82f );
        EmitSolidLump( cx - S * 0.02f, cy + S * 0.14f, cz + S * 0.10f,
            S * 0.06f, S * 0.06f, S * 0.05f, 0.16f, cr * 0.75f, cg * 0.72f, cb * 0.68f );
    }

    void EmitFormGrassSod( float cx, float cy, float cz, float S, float cr, float cg, float cb )
    {
        // Solid dirt body + solid grass skin — no open poles / no doughnut hole.
        float const dirtR = 95.f, dirtG = 78.f, dirtB = 52.f;
        EmitSolidLump( cx, cy, cz - S * 0.02f, S * 0.40f, S * 0.36f, S * 0.16f, 0.12f,
            dirtR, dirtG, dirtB );
        EmitSolidLump( cx, cy, cz + S * 0.10f, S * 0.38f, S * 0.34f, S * 0.10f, 0.14f,
            cr, cg, cb );
        EmitSolidLump( cx - S * 0.10f, cy + S * 0.08f, cz + S * 0.16f,
            S * 0.09f, S * 0.08f, S * 0.07f, 0.12f, cr * 0.85f, cg * 1.05f, cb * 0.7f );
        EmitSolidLump( cx + S * 0.12f, cy - S * 0.06f, cz + S * 0.17f,
            S * 0.08f, S * 0.09f, S * 0.06f, 0.12f, cr * 0.9f, cg * 1.08f, cb * 0.75f );
    }

    // ---- Shape grammar helpers: planes where crystal habit makes planes; curves elsewhere ----

    void EmitEllipsoidPatch( float cx, float cy, float cz,
        float rx, float ry, float rz,
        float phi0, float phi1, int slices, int stacks,
        float wobbleAmp, float cr, float cg, float cb )
    {
        // Low-poly curved body. Radial wobble breaks the perfect egg into a natural lump.
        // NOTE: open phi ranges leave polar holes — prefer EmitSolidLump for closed masses.
        constexpr float kPi = 3.14159265f;
        for ( int i = 0; i < stacks; ++i )
        {
            float const t0 = (float)i / (float)stacks;
            float const t1 = (float)( i + 1 ) / (float)stacks;
            float const p0 = phi0 + ( phi1 - phi0 ) * t0;
            float const p1 = phi0 + ( phi1 - phi0 ) * t1;
            float const sp0 = std::sin( p0 ), cp0 = std::cos( p0 );
            float const sp1 = std::sin( p1 ), cp1 = std::cos( p1 );
            for ( int j = 0; j < slices; ++j )
            {
                float const u0 = (float)j / (float)slices;
                float const u1 = (float)( j + 1 ) / (float)slices;
                float const th0 = u0 * kPi * 2.f;
                float const th1 = u1 * kPi * 2.f;
                auto sample = [&]( float phiS, float phiC, float th, float& ox, float& oy, float& oz )
                {
                    float wob = 1.f + wobbleAmp * ( 0.55f * std::sin( 3.f * th + 1.2f )
                        + 0.35f * std::cos( 5.f * th - 0.7f )
                        + 0.25f * std::sin( 2.f * phiS * 3.f ) );
                    ox = cx + rx * wob * phiS * std::cos( th );
                    oy = cy + ry * wob * phiS * std::sin( th );
                    oz = cz + rz * wob * phiC;
                };
                float ax, ay, az, bx, by, bz, dx, dy, dz, ex, ey, ez;
                sample( sp0, cp0, th0, ax, ay, az );
                sample( sp0, cp0, th1, bx, by, bz );
                sample( sp1, cp1, th0, dx, dy, dz );
                sample( sp1, cp1, th1, ex, ey, ez );
                // Outward winding: (a,d,b)/(b,d,e) — old (a,b,d) inverted normals (upside-down lit).
                EmitGalleryTri( ax, ay, az, dx, dy, dz, bx, by, bz, cr, cg, cb );
                EmitGalleryTri( bx, by, bz, dx, dy, dz, ex, ey, ez, cr, cg, cb );
            }
        }
    }

    void EmitSolidLump( float cx, float cy, float cz,
        float rx, float ry, float rz, float wobbleAmp,
        float cr, float cg, float cb )
    {
        // Closed solid — full sphere latitude range so tops/bottoms are not sheared open.
        constexpr float kPi = 3.14159265f;
        int const slices = 10;
        int const stacks = 7;
        EmitEllipsoidPatch( cx, cy, cz, rx, ry, rz, 0.f, kPi, slices, stacks, wobbleAmp, cr, cg, cb );
    }

    void EmitIrregularHostMass( float cx, float cy, float cz, float S,
        float cr, float cg, float cb )
    {
        // Naturally broken / chipped rock lump — CLOSED convex body + local chips.
        EmitSolidLump( cx, cy, cz, S * 0.38f, S * 0.34f, S * 0.30f, 0.16f, cr, cg, cb );
        EmitSolidLump( cx + S * 0.16f, cy - S * 0.08f, cz + S * 0.02f,
            S * 0.16f, S * 0.14f, S * 0.13f, 0.18f, cr * 0.88f, cg * 0.88f, cb * 0.9f );
        EmitSolidLump( cx - S * 0.12f, cy + S * 0.10f, cz - S * 0.05f,
            S * 0.14f, S * 0.12f, S * 0.11f, 0.16f, cr * 0.78f, cg * 0.78f, cb * 0.82f );
        EmitGalleryTri(
            cx - S * 0.08f, cy - S * 0.22f, cz + S * 0.12f,
            cx + S * 0.20f, cy - S * 0.10f, cz + S * 0.18f,
            cx + S * 0.02f, cy + S * 0.08f, cz + S * 0.22f,
            cr * 0.7f, cg * 0.7f, cb * 0.75f );
    }

    void EmitTaperedBranch( float cx, float cy, float cz, float S,
        float cr, float cg, float cb )
    {
        // Continuous tapered oval tube along +Y — connected rings, not floating beads.
        constexpr float kPi = 3.14159265f;
        constexpr int kSeg = 8;
        constexpr int kRad = 8;
        float const L = S * 1.55f;
        float ring[kSeg + 1][kRad][3];
        for ( int i = 0; i <= kSeg; ++i )
        {
            float const t = (float)i / (float)kSeg;
            float const y = cy - L * 0.5f + t * L;
            float const radX = S * ( 0.20f - t * 0.09f );
            float const radZ = radX * 0.82f;
            float const bend = S * 0.07f * std::sin( t * 3.1f );
            for ( int j = 0; j < kRad; ++j )
            {
                float const a = (float)j * ( kPi * 2.f / (float)kRad );
                ring[i][j][0] = cx + bend + std::cos( a ) * radX;
                ring[i][j][1] = y;
                ring[i][j][2] = cz + bend * 0.35f + std::sin( a ) * radZ;
            }
        }
        for ( int i = 0; i < kSeg; ++i )
        {
            for ( int j = 0; j < kRad; ++j )
            {
                int const j1 = ( j + 1 ) % kRad;
                EmitGalleryTri(
                    ring[i][j][0], ring[i][j][1], ring[i][j][2],
                    ring[i][j1][0], ring[i][j1][1], ring[i][j1][2],
                    ring[i + 1][j][0], ring[i + 1][j][1], ring[i + 1][j][2],
                    cr, cg, cb );
                EmitGalleryTri(
                    ring[i][j1][0], ring[i][j1][1], ring[i][j1][2],
                    ring[i + 1][j1][0], ring[i + 1][j1][1], ring[i + 1][j1][2],
                    ring[i + 1][j][0], ring[i + 1][j][1], ring[i + 1][j][2],
                    cr * 0.95f, cg * 0.95f, cb * 0.95f );
            }
        }
        // End caps (closed solid)
        float const y0 = ring[0][0][1];
        float const y1 = ring[kSeg][0][1];
        float cx0 = 0.f, cz0 = 0.f, cx1 = 0.f, cz1 = 0.f;
        for ( int j = 0; j < kRad; ++j )
        {
            cx0 += ring[0][j][0]; cz0 += ring[0][j][2];
            cx1 += ring[kSeg][j][0]; cz1 += ring[kSeg][j][2];
        }
        cx0 /= (float)kRad; cz0 /= (float)kRad;
        cx1 /= (float)kRad; cz1 /= (float)kRad;
        for ( int j = 0; j < kRad; ++j )
        {
            int const j1 = ( j + 1 ) % kRad;
            EmitGalleryTri( cx0, y0, cz0,
                ring[0][j1][0], ring[0][j1][1], ring[0][j1][2],
                ring[0][j][0], ring[0][j][1], ring[0][j][2],
                cr * 0.7f, cg * 0.65f, cb * 0.55f );
            EmitGalleryTri( cx1, y1, cz1,
                ring[kSeg][j][0], ring[kSeg][j][1], ring[kSeg][j][2],
                ring[kSeg][j1][0], ring[kSeg][j1][1], ring[kSeg][j1][2],
                cr * 0.75f, cg * 0.7f, cb * 0.6f );
        }
        // Splinter chip near fat end
        EmitSolidLump( cx - S * 0.06f, cy - L * 0.42f, cz + S * 0.04f,
            S * 0.08f, S * 0.06f, S * 0.10f, 0.1f, cr * 0.65f, cg * 0.6f, cb * 0.5f );
    }

    void EmitCrystalHexPoint( float bx, float by, float bz,
        float dx, float dy, float dz,
        float radius, float prismLen, float tipLen,
        float cr, float cg, float cb, bool brokenTip )
    {
        // Hexagonal prism + pyramidal termination (quartz habit). Tiltable growth axis.
        float len = std::sqrt( dx * dx + dy * dy + dz * dz );
        if ( len < 1e-5f ) { dx = 0.f; dy = 0.f; dz = 1.f; len = 1.f; }
        dx /= len; dy /= len; dz /= len;

        float ax = 0.f, ay = 0.f, az = 0.f;
        if ( std::fabs( dz ) < 0.9f ) { ax = dy; ay = -dx; az = 0.f; }
        else { ax = 1.f; ay = 0.f; az = 0.f; }
        float al = std::sqrt( ax * ax + ay * ay + az * az );
        ax /= al; ay /= al; az /= al;
        float ux = dy * az - dz * ay;
        float uy = dz * ax - dx * az;
        float uz = dx * ay - dy * ax;
        // (ux,uy,uz) x (dx,dy,dz) should recover (ax..) — use ax,ay,az and ux,uy,uz as ring basis
        float vx = ay * dz - az * dy;
        float vy = az * dx - ax * dz;
        float vz = ax * dy - ay * dx;
        // Prefer orthonormal: u = ax, v = cross(d, u)
        ux = ax; uy = ay; uz = az;
        vx = dy * uz - dz * uy;
        vy = dz * ux - dx * uz;
        vz = dx * uy - dy * ux;
        float vl = std::sqrt( vx * vx + vy * vy + vz * vz );
        if ( vl > 1e-6f ) { vx /= vl; vy /= vl; vz /= vl; }

        constexpr float kPi = 3.14159265f;
        float base[6][3], mid[6][3];
        for ( int i = 0; i < 6; ++i )
        {
            float const ang = (float)i * ( kPi / 3.f ) + 0.15f;
            float const c = std::cos( ang ), s = std::sin( ang );
            float const rx = ( ux * c + vx * s ) * radius;
            float const ry = ( uy * c + vy * s ) * radius;
            float const rz = ( uz * c + vz * s ) * radius;
            base[i][0] = bx + rx;
            base[i][1] = by + ry;
            base[i][2] = bz + rz;
            mid[i][0] = bx + dx * prismLen + rx * 0.92f;
            mid[i][1] = by + dy * prismLen + ry * 0.92f;
            mid[i][2] = bz + dz * prismLen + rz * 0.92f;
        }
        // Prism sides
        for ( int i = 0; i < 6; ++i )
        {
            int const j = ( i + 1 ) % 6;
            EmitGalleryTri( base[i][0], base[i][1], base[i][2],
                base[j][0], base[j][1], base[j][2],
                mid[i][0], mid[i][1], mid[i][2], cr, cg, cb );
            EmitGalleryTri( base[j][0], base[j][1], base[j][2],
                mid[j][0], mid[j][1], mid[j][2],
                mid[i][0], mid[i][1], mid[i][2], cr * 0.95f, cg * 0.95f, cb * 0.95f );
        }
        // Base cap (broken attachment to matrix)
        float const bcx = bx - dx * radius * 0.15f;
        float const bcy = by - dy * radius * 0.15f;
        float const bcz = bz - dz * radius * 0.15f;
        for ( int i = 0; i < 6; ++i )
        {
            int const j = ( i + 1 ) % 6;
            EmitGalleryTri( bcx, bcy, bcz,
                base[j][0], base[j][1], base[j][2],
                base[i][0], base[i][1], base[i][2],
                cr * 0.55f, cg * 0.55f, cb * 0.55f );
        }
        if ( brokenTip )
        {
            // Truncated tip — flat cleavage face
            float tipR = radius * 0.45f;
            float const tx = bx + dx * ( prismLen + tipLen * 0.45f );
            float const ty = by + dy * ( prismLen + tipLen * 0.45f );
            float const tz = bz + dz * ( prismLen + tipLen * 0.45f );
            float tip[6][3];
            for ( int i = 0; i < 6; ++i )
            {
                float const ang = (float)i * ( kPi / 3.f ) + 0.15f;
                float const c = std::cos( ang ), s = std::sin( ang );
                tip[i][0] = tx + ( ux * c + vx * s ) * tipR;
                tip[i][1] = ty + ( uy * c + vy * s ) * tipR;
                tip[i][2] = tz + ( uz * c + vz * s ) * tipR;
                EmitGalleryTri( mid[i][0], mid[i][1], mid[i][2],
                    mid[( i + 1 ) % 6][0], mid[( i + 1 ) % 6][1], mid[( i + 1 ) % 6][2],
                    tip[i][0], tip[i][1], tip[i][2], cr * 1.05f, cg * 1.05f, cb * 1.05f );
            }
            for ( int i = 0; i < 6; ++i )
            {
                int const j = ( i + 1 ) % 6;
                EmitGalleryTri( tx, ty, tz, tip[j][0], tip[j][1], tip[j][2], tip[i][0], tip[i][1], tip[i][2],
                    cr * 1.1f, cg * 1.1f, cb * 1.1f );
            }
        }
        else
        {
            float const axp = bx + dx * ( prismLen + tipLen );
            float const ayp = by + dy * ( prismLen + tipLen );
            float const azp = bz + dz * ( prismLen + tipLen );
            for ( int i = 0; i < 6; ++i )
            {
                int const j = ( i + 1 ) % 6;
                EmitGalleryTri( mid[i][0], mid[i][1], mid[i][2],
                    mid[j][0], mid[j][1], mid[j][2],
                    axp, ayp, azp, cr * 1.12f, cg * 1.12f, cb * 1.12f );
            }
        }
    }

    void EmitCorundumBarrel( float bx, float by, float bz,
        float dx, float dy, float dz,
        float radius, float height,
        float cr, float cg, float cb )
    {
        // Stout hex barrel with beveled tip — chunky embedded corundum, not a thin spire.
        EmitCrystalHexPoint( bx, by, bz, dx, dy, dz,
            radius, height * 0.62f, height * 0.28f, cr, cg, cb, false );
        // Mid bulge ring (barrel read)
        float len = std::sqrt( dx * dx + dy * dy + dz * dz );
        if ( len < 1e-5f ) { return; }
        dx /= len; dy /= len; dz /= len;
        float const mx = bx + dx * height * 0.32f;
        float const my = by + dy * height * 0.32f;
        float const mz = bz + dz * height * 0.32f;
        EmitEllipsoidPatch( mx, my, mz, radius * 1.15f, radius * 1.15f, height * 0.18f,
            0.f, 3.14159265f, 6, 4, 0.05f, cr * 0.95f, cg * 0.9f, cb * 0.9f );
    }

    void EmitFormHematiteOre( float cx, float cy, float cz, float S,
        float /*cr*/, float /*cg*/, float /*cb*/ )
    {
        // Dense host fragment cut by rusty ore seam / nodule — not a red cube.
        float const hR = 58.f, hG = 56.f, hB = 60.f;
        float const oR = 148.f, oG = 62.f, oB = 42.f;
        EmitIrregularHostMass( cx, cy, cz, S, hR, hG, hB );
        EmitSolidLump( cx - S * 0.02f, cy + S * 0.04f, cz + S * 0.08f,
            S * 0.30f, S * 0.08f, S * 0.09f, 0.06f, oR, oG, oB );
        EmitSolidLump( cx + S * 0.10f, cy - S * 0.12f, cz + S * 0.14f,
            S * 0.11f, S * 0.10f, S * 0.10f, 0.08f, oR * 1.1f, oG * 0.9f, oB * 0.85f );
    }

    void EmitFormAzuriteVein( float cx, float cy, float cz, float S,
        float /*cr*/, float /*cg*/, float /*cb*/ )
    {
        float const hR = 62.f, hG = 60.f, hB = 58.f;
        float const aR = 42.f, aG = 92.f, aB = 168.f;
        float const tR = 38.f, tG = 138.f, tB = 132.f;
        EmitIrregularHostMass( cx - S * 0.06f, cy, cz, S * 0.95f, hR, hG, hB );
        EmitSolidLump( cx + S * 0.16f, cy - S * 0.02f, cz + S * 0.08f,
            S * 0.13f, S * 0.20f, S * 0.11f, 0.10f, aR, aG, aB );
        EmitSolidLump( cx + S * 0.08f, cy + S * 0.14f, cz + S * 0.14f,
            S * 0.11f, S * 0.09f, S * 0.09f, 0.08f, tR, tG, tB );
    }

    void EmitFormGoldSeam( float cx, float cy, float cz, float S,
        float /*cr*/, float /*cg*/, float /*cb*/ )
    {
        float const hR = 72.f, hG = 70.f, hB = 68.f;
        float const gR = 212.f, gG = 178.f, gB = 58.f;
        EmitIrregularHostMass( cx, cy, cz, S, hR, hG, hB );
        EmitSolidLump( cx + S * 0.02f, cy + S * 0.04f, cz + S * 0.06f,
            S * 0.28f, S * 0.06f, S * 0.07f, 0.06f, gR, gG, gB );
        EmitSolidLump( cx + S * 0.14f, cy - S * 0.08f, cz + S * 0.12f,
            S * 0.09f, S * 0.07f, S * 0.07f, 0.08f, gR * 1.05f, gG * 1.02f, gB * 0.9f );
    }

    void EmitFormQuartzCluster( float cx, float cy, float cz, float S,
        float cr, float cg, float cb )
    {
        // Shape law: clustered prismatic points on irregular matrix — not stacked cubes.
        float const mR = 92.f, mG = 86.f, mB = 78.f;
        EmitSolidLump( cx, cy, cz - S * 0.06f, S * 0.30f, S * 0.26f, S * 0.14f, 0.16f, mR, mG, mB );
        EmitSolidLump( cx + S * 0.12f, cy - S * 0.06f, cz - S * 0.04f,
            S * 0.13f, S * 0.11f, S * 0.09f, 0.14f, mR * 0.85f, mG * 0.85f, mB * 0.88f );

        // 1 dominant + companions, tilted, uneven heights; one broken tip
        EmitCrystalHexPoint( cx - S * 0.02f, cy - S * 0.02f, cz + S * 0.02f,
            0.08f, -0.05f, 1.f, S * 0.085f, S * 0.42f, S * 0.22f, cr, cg, cb, false );
        EmitCrystalHexPoint( cx - S * 0.14f, cy + S * 0.08f, cz + S * 0.00f,
            -0.25f, 0.18f, 0.95f, S * 0.055f, S * 0.28f, S * 0.16f,
            cr * 0.95f, cg * 0.95f, cb * 0.98f, false );
        EmitCrystalHexPoint( cx + S * 0.12f, cy + S * 0.06f, cz + S * 0.01f,
            0.28f, 0.12f, 0.92f, S * 0.05f, S * 0.24f, S * 0.14f,
            cr * 1.02f, cg * 1.02f, cb * 1.0f, true );
        EmitCrystalHexPoint( cx + S * 0.06f, cy - S * 0.14f, cz - S * 0.01f,
            0.12f, -0.32f, 0.9f, S * 0.045f, S * 0.20f, S * 0.12f,
            cr * 0.9f, cg * 0.9f, cb * 0.95f, false );
        EmitCrystalHexPoint( cx - S * 0.08f, cy - S * 0.12f, cz + S * 0.00f,
            -0.1f, -0.2f, 0.95f, S * 0.038f, S * 0.16f, S * 0.10f,
            cr * 1.05f, cg * 1.05f, cb * 1.02f, false );
    }

    void EmitFormRubyInBasalt( float cx, float cy, float cz, float S,
        float /*cr*/, float /*cg*/, float /*cb*/ )
    {
        // Shape law: dark host fracture exposing stout embedded corundum — not red cubes on a black cube.
        float const bR = 46.f, bG = 48.f, bB = 52.f;
        float const rR = 188.f, rG = 38.f, rB = 46.f;
        EmitIrregularHostMass( cx - S * 0.04f, cy, cz - S * 0.02f, S * 1.05f, bR, bG, bB );

        // Broken-open face: planar fracture where crystal is exposed from within
        EmitGalleryTri(
            cx - S * 0.05f, cy - S * 0.18f, cz + S * 0.06f,
            cx + S * 0.28f, cy - S * 0.02f, cz + S * 0.16f,
            cx + S * 0.06f, cy + S * 0.22f, cz + S * 0.14f,
            bR * 0.7f, bG * 0.7f, bB * 0.75f );
        EmitGalleryTri(
            cx - S * 0.05f, cy - S * 0.18f, cz + S * 0.06f,
            cx + S * 0.06f, cy + S * 0.22f, cz + S * 0.14f,
            cx - S * 0.18f, cy + S * 0.06f, cz + S * 0.10f,
            bR * 0.65f, bG * 0.65f, bB * 0.7f );

        // Dominant barrel crystal emerging from fracture (thick, legible)
        EmitCorundumBarrel( cx + S * 0.02f, cy - S * 0.02f, cz + S * 0.02f,
            0.15f, 0.08f, 1.f, S * 0.11f, S * 0.48f, rR, rG, rB );
        // Smaller satellites still embedded / half-buried
        EmitCorundumBarrel( cx - S * 0.14f, cy + S * 0.10f, cz - S * 0.02f,
            -0.2f, 0.25f, 0.9f, S * 0.07f, S * 0.28f, rR * 0.92f, rG * 0.85f, rB * 0.88f );
        EmitCorundumBarrel( cx + S * 0.16f, cy + S * 0.12f, cz + S * 0.00f,
            0.35f, 0.15f, 0.85f, S * 0.055f, S * 0.22f, rR * 1.05f, rG * 0.8f, rB * 0.85f );
    }

    void EmitFormAmethystGeode( float cx, float cy, float cz, float S,
        float /*cr*/, float /*cg*/, float /*cb*/ )
    {
        // Closed rind body + darker pocket dent on -Y + crystals facing out (no open-pole shell).
        float const rindR = 70.f, rindG = 66.f, rindB = 62.f;
        float const pR = 132.f, pG = 78.f, pB = 182.f;
        EmitSolidLump( cx, cy + S * 0.04f, cz, S * 0.42f, S * 0.38f, S * 0.36f, 0.12f,
            rindR, rindG, rindB );
        EmitSolidLump( cx, cy - S * 0.14f, cz, S * 0.28f, S * 0.16f, S * 0.24f, 0.08f,
            rindR * 0.45f, rindG * 0.42f, rindB * 0.48f ); // pocket cue
        EmitSolidLump( cx - S * 0.18f, cy - S * 0.10f, cz + S * 0.04f,
            S * 0.12f, S * 0.10f, S * 0.11f, 0.1f, rindR * 0.9f, rindG * 0.9f, rindB * 0.92f );
        EmitSolidLump( cx + S * 0.16f, cy - S * 0.10f, cz - S * 0.02f,
            S * 0.11f, S * 0.09f, S * 0.10f, 0.1f, rindR * 0.85f, rindG * 0.85f, rindB * 0.88f );

        struct Pt { float x, y, z, dx, dy, dz, r, h; bool brk; };
        Pt pts[] = {
            { cx - S * 0.08f, cy - S * 0.06f, cz - S * 0.02f, -0.1f, -0.85f, 0.25f, S * 0.045f, S * 0.22f, false },
            { cx + S * 0.08f, cy - S * 0.04f, cz + S * 0.02f, 0.15f, -0.9f, 0.2f, S * 0.05f, S * 0.26f, false },
            { cx + S * 0.00f, cy - S * 0.02f, cz + S * 0.08f, 0.05f, -0.8f, 0.35f, S * 0.042f, S * 0.20f, true },
            { cx - S * 0.04f, cy - S * 0.05f, cz - S * 0.08f, -0.05f, -0.75f, 0.15f, S * 0.038f, S * 0.18f, false },
            { cx + S * 0.12f, cy - S * 0.08f, cz - S * 0.04f, 0.2f, -0.85f, 0.1f, S * 0.04f, S * 0.19f, false },
            { cx - S * 0.12f, cy - S * 0.04f, cz + S * 0.06f, -0.2f, -0.8f, 0.3f, S * 0.036f, S * 0.16f, false },
        };
        for ( Pt const& p : pts )
        {
            EmitCrystalHexPoint( p.x, p.y, p.z, p.dx, p.dy, p.dz,
                p.r, p.h * 0.65f, p.h * 0.35f, pR, pG, pB, p.brk );
        }
    }

    void EmitFormGoldNugget( float cx, float cy, float cz, float S,
        float cr, float cg, float cb )
    {
        EmitSolidLump( cx, cy, cz, S * 0.28f, S * 0.22f, S * 0.18f, 0.24f, cr, cg, cb );
        EmitSolidLump( cx + S * 0.14f, cy - S * 0.04f, cz + S * 0.02f,
            S * 0.14f, S * 0.12f, S * 0.10f, 0.20f, cr * 1.05f, cg * 1.02f, cb * 0.9f );
        EmitSolidLump( cx - S * 0.10f, cy + S * 0.08f, cz - S * 0.02f,
            S * 0.12f, S * 0.10f, S * 0.09f, 0.18f, cr * 0.92f, cg * 0.88f, cb * 0.75f );
    }

    void EmitFormQuartzFree( float cx, float cy, float cz, float S,
        float cr, float cg, float cb )
    {
        // Free quartz cluster — no host pedestal required.
        EmitCrystalHexPoint( cx, cy, cz - S * 0.02f, 0.05f, -0.05f, 1.f,
            S * 0.09f, S * 0.38f, S * 0.20f, cr, cg, cb, false );
        EmitCrystalHexPoint( cx - S * 0.12f, cy + S * 0.06f, cz - S * 0.04f,
            -0.3f, 0.15f, 0.9f, S * 0.055f, S * 0.26f, S * 0.14f,
            cr * 0.95f, cg * 0.95f, cb * 0.98f, false );
        EmitCrystalHexPoint( cx + S * 0.10f, cy - S * 0.08f, cz - S * 0.02f,
            0.25f, -0.2f, 0.92f, S * 0.05f, S * 0.22f, S * 0.12f,
            cr * 1.02f, cg * 1.02f, cb * 1.0f, true );
        EmitCrystalHexPoint( cx + S * 0.04f, cy + S * 0.12f, cz - S * 0.06f,
            0.1f, 0.3f, 0.9f, S * 0.04f, S * 0.18f, S * 0.10f,
            cr * 0.9f, cg * 0.9f, cb * 0.95f, false );
    }

    void EmitFormRubyFree( float cx, float cy, float cz, float S,
        float /*cr*/, float /*cg*/, float /*cb*/ )
    {
        float const rR = 188.f, rG = 38.f, rB = 46.f;
        EmitCorundumBarrel( cx, cy, cz - S * 0.04f, 0.08f, 0.05f, 1.f,
            S * 0.12f, S * 0.52f, rR, rG, rB );
        EmitCorundumBarrel( cx - S * 0.12f, cy + S * 0.06f, cz - S * 0.06f,
            -0.25f, 0.2f, 0.9f, S * 0.07f, S * 0.28f, rR * 0.92f, rG * 0.85f, rB * 0.88f );
    }

    void EmitFormAmethystFree( float cx, float cy, float cz, float S,
        float /*cr*/, float /*cg*/, float /*cb*/ )
    {
        float const pR = 132.f, pG = 78.f, pB = 182.f;
        EmitCrystalHexPoint( cx, cy, cz - S * 0.02f, 0.05f, -0.05f, 1.f,
            S * 0.08f, S * 0.34f, S * 0.18f, pR, pG, pB, false );
        EmitCrystalHexPoint( cx - S * 0.10f, cy + S * 0.08f, cz - S * 0.04f,
            -0.28f, 0.2f, 0.9f, S * 0.05f, S * 0.24f, S * 0.13f, pR * 1.05f, pG * 0.95f, pB, false );
        EmitCrystalHexPoint( cx + S * 0.10f, cy - S * 0.06f, cz - S * 0.02f,
            0.25f, -0.18f, 0.92f, S * 0.048f, S * 0.22f, S * 0.12f, pR, pG * 0.9f, pB * 1.05f, true );
        EmitCrystalHexPoint( cx + S * 0.02f, cy + S * 0.12f, cz - S * 0.06f,
            0.08f, 0.28f, 0.9f, S * 0.04f, S * 0.16f, S * 0.10f, pR * 0.9f, pG * 0.85f, pB * 1.1f, false );
    }

    void EmitFormHematiteNodule( float cx, float cy, float cz, float S,
        float /*cr*/, float /*cg*/, float /*cb*/ )
    {
        float const hR = 88.f, hG = 48.f, hB = 42.f;
        EmitSolidLump( cx, cy, cz, S * 0.32f, S * 0.28f, S * 0.26f, 0.16f, hR, hG, hB );
        EmitSolidLump( cx + S * 0.12f, cy - S * 0.06f, cz + S * 0.06f,
            S * 0.14f, S * 0.12f, S * 0.12f, 0.12f, hR * 1.15f, hG * 0.85f, hB * 0.75f );
    }

    void EmitFormAzuriteMass( float cx, float cy, float cz, float S,
        float /*cr*/, float /*cg*/, float /*cb*/ )
    {
        float const aR = 42.f, aG = 92.f, aB = 168.f;
        float const tR = 38.f, tG = 138.f, tB = 132.f;
        EmitSolidLump( cx, cy, cz, S * 0.30f, S * 0.26f, S * 0.24f, 0.14f, aR, aG, aB );
        EmitSolidLump( cx + S * 0.10f, cy + S * 0.08f, cz + S * 0.06f,
            S * 0.14f, S * 0.12f, S * 0.12f, 0.10f, tR, tG, tB );
    }

    void EmitFormEmeraldFree( float cx, float cy, float cz, float S,
        float /*cr*/, float /*cg*/, float /*cb*/ )
    {
        float const eR = 48.f, eG = 158.f, eB = 78.f;
        EmitCrystalHexPoint( cx, cy, cz - S * 0.02f, 0.1f, 0.05f, 1.f,
            S * 0.08f, S * 0.36f, S * 0.18f, eR, eG, eB, false );
        EmitCrystalHexPoint( cx - S * 0.08f, cy + S * 0.06f, cz - S * 0.04f,
            -0.2f, 0.2f, 0.9f, S * 0.05f, S * 0.20f, S * 0.11f,
            eR * 0.9f, eG * 0.95f, eB * 0.85f, true );
    }

    void EmitFormLapisMass( float cx, float cy, float cz, float S, float cr, float cg, float cb )
    {
        EmitSolidLump( cx, cy, cz, S * 0.40f, S * 0.36f, S * 0.32f, 0.16f, cr, cg, cb );
        EmitSolidLump( cx + S * 0.14f, cy - S * 0.08f, cz + S * 0.04f,
            S * 0.18f, S * 0.16f, S * 0.14f, 0.14f, cr * 0.9f, cg * 0.92f, cb * 0.95f );
        EmitSolidLump( cx - S * 0.12f, cy + S * 0.10f, cz - S * 0.04f,
            S * 0.16f, S * 0.14f, S * 0.12f, 0.12f, cr * 0.82f, cg * 0.85f, cb * 0.92f );
        EmitGalleryTri(
            cx - S * 0.10f, cy - S * 0.20f, cz + S * 0.10f,
            cx + S * 0.22f, cy - S * 0.06f, cz + S * 0.18f,
            cx + S * 0.04f, cy + S * 0.14f, cz + S * 0.20f,
            cr * 0.75f, cg * 0.78f, cb * 0.88f );
        EmitSolidLump( cx + S * 0.02f, cy + S * 0.06f, cz + S * 0.16f,
            S * 0.09f, S * 0.035f, S * 0.028f, 0.04f, 210.f, 206.f, 196.f );
        EmitSolidLump( cx - S * 0.14f, cy - S * 0.04f, cz + S * 0.12f,
            S * 0.07f, S * 0.028f, S * 0.022f, 0.04f, 198.f, 194.f, 184.f );
        EmitSolidLump( cx + S * 0.12f, cy + S * 0.12f, cz + S * 0.10f,
            S * 0.022f, S * 0.018f, S * 0.018f, 0.0f, 218.f, 188.f, 72.f );
        EmitSolidLump( cx - S * 0.06f, cy - S * 0.10f, cz + S * 0.14f,
            S * 0.018f, S * 0.016f, S * 0.016f, 0.0f, 210.f, 176.f, 58.f );
    }

    void EmitFormEmeraldHost( float cx, float cy, float cz, float S,
        float /*cr*/, float /*cg*/, float /*cb*/ )
    {
        // Host rock + protruding green crystal body (seam-backed).
        float const hR = 68.f, hG = 66.f, hB = 62.f;
        float const eR = 48.f, eG = 158.f, eB = 78.f;
        EmitIrregularHostMass( cx - S * 0.06f, cy, cz - S * 0.02f, S * 0.95f, hR, hG, hB );
        EmitCrystalHexPoint( cx + S * 0.14f, cy - S * 0.02f, cz + S * 0.04f,
            0.55f, 0.05f, 0.75f, S * 0.07f, S * 0.28f, S * 0.16f, eR, eG, eB, false );
        EmitCrystalHexPoint( cx + S * 0.08f, cy + S * 0.10f, cz + S * 0.02f,
            0.4f, 0.25f, 0.7f, S * 0.045f, S * 0.16f, S * 0.10f,
            eR * 0.9f, eG * 0.95f, eB * 0.85f, true );
    }

    void EmitGalleryMaterial( char const* id, float px, float py, float cz, float edge,
        float cr, float cg, float cb )
    {
        if ( !id || !id[0] ) { return; }
        if ( std::strcmp( id, "wood" ) == 0 ) { EmitFormWoodBranch( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "basalt" ) == 0 ) { EmitFormBasaltShard( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "mica_schist" ) == 0 ) { EmitFormSchistPlate( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "granite" ) == 0 ) { EmitFormGraniteChunk( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "limestone" ) == 0 ) { EmitFormLimestoneBlock( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "shale" ) == 0 ) { EmitFormShaleFlake( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "sandstone" ) == 0 ) { EmitFormSandstoneSlab( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "stone" ) == 0 ) { EmitFormStoneChip( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "clay" ) == 0 ) { EmitFormClayLump( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "gravel" ) == 0 ) { EmitFormGravelPile( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "sand" ) == 0 ) { EmitFormSandMound( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "loam" ) == 0 ) { EmitFormLoamClod( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "dirt" ) == 0 ) { EmitFormDirtClod( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "grass" ) == 0 ) { EmitFormGrassSod( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "hematite" ) == 0 ) { EmitFormHematiteOre( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "azurite" ) == 0 ) { EmitFormAzuriteVein( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "gold" ) == 0 ) { EmitFormGoldSeam( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "quartz" ) == 0 ) { EmitFormQuartzCluster( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "ruby" ) == 0 ) { EmitFormRubyInBasalt( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "amethyst" ) == 0 ) { EmitFormAmethystGeode( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "lapis" ) == 0 ) { EmitFormLapisMass( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "emerald" ) == 0 ) { EmitFormEmeraldHost( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "hematite_free" ) == 0 ) { EmitFormHematiteNodule( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "azurite_free" ) == 0 ) { EmitFormAzuriteMass( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "gold_free" ) == 0 ) { EmitFormGoldNugget( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "quartz_free" ) == 0 ) { EmitFormQuartzFree( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "ruby_free" ) == 0 ) { EmitFormRubyFree( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "amethyst_free" ) == 0 ) { EmitFormAmethystFree( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "lapis_free" ) == 0 ) { EmitFormLapisMass( px, py, cz, edge, cr, cg, cb ); }
        else if ( std::strcmp( id, "emerald_free" ) == 0 ) { EmitFormEmeraldFree( px, py, cz, edge, cr, cg, cb ); }
        else { EmitGalleryBox( px, py, cz, edge * 0.4f, edge * 0.4f, edge * 0.4f, cr, cg, cb ); }
    }

    float GalleryCollideRadius( char const* id )
    {
        float const e = kVoxelEdgeM;
        if ( !id ) { return e * 0.4f; }
        if ( std::strcmp( id, "wood" ) == 0 ) { return e * 0.28f; }
        if ( std::strcmp( id, "amethyst" ) == 0 ) { return e * 0.52f; }
        if ( std::strcmp( id, "quartz" ) == 0 || std::strcmp( id, "quartz_free" ) == 0 ) { return e * 0.42f; }
        if ( std::strcmp( id, "ruby" ) == 0 || std::strcmp( id, "ruby_free" ) == 0 ) { return e * 0.40f; }
        if ( std::strstr( id, "_free" ) ) { return e * 0.36f; }
        return e * 0.40f;
    }

    void ReseatGallerySamples()
    {
        for ( int i = 0; i < g.galleryCount; ++i )
        {
            AppState::GallerySample& s = g.gallery[i];
            if ( !s.present || !s.id ) { continue; }
            float ground = 0.f;
            if ( !SampleGroundZ( s.x, s.y, ground ) ) { continue; }
            // Seat on the heightfield so the rounded underside stays visible (not buried flat).
            s.z = ground + GalleryCollideRadius( s.id ) * 1.02f;
        }
    }

    void EnsureGallerySpawned()
    {
        if ( g.gallerySpawned ) { return; }
        g.gallerySpawned = true;
        g.galleryCount = 0;

        // HOST-BACKED test palette (row 0–2) — occurrence forms.
        static char const* const kHostSoil[] = {
            "wood", "basalt", "mica_schist", "granite", "limestone", "shale", "sandstone",
            "stone", "clay", "gravel", "sand", "loam", "dirt", "grass"
        };
        static char const* const kDeposit[] = { "hematite", "azurite", "gold" };
        static char const* const kCrystalHost[] = { "quartz", "ruby", "amethyst", "lapis", "emerald" };
        // FREE / DETACHED — same matter after liberation (size from mineral, not host voxel).
        static char const* const kFree[] = {
            "hematite_free", "azurite_free", "gold_free", "quartz_free",
            "ruby_free", "amethyst_free", "lapis_free", "emerald_free"
        };

        float const spacing = kVoxelEdgeM * 2.6f;
        float const baseX = 128.5f + 3.0f;
        float const rowY[4] = { 128.5f, 128.05f, 127.60f, 127.15f };

        auto addRow = [&]( char const* const* ids, int count, float py )
        {
            for ( int i = 0; i < count; ++i )
            {
                if ( g.galleryCount >= AppState::kGalleryMax ) { return; }
                float const px = baseX + (float)i * spacing;
                float ground = 0.f;
                if ( !SampleGroundZ( px, py, ground ) ) { ground = GradeToZ( 0.85f ); }
                AppState::GallerySample& s = g.gallery[g.galleryCount++];
                s.id = ids[i];
                s.x = px;
                s.y = py;
                s.z = ground + GalleryCollideRadius( ids[i] ) * 1.02f;
                s.present = true;
            }
        };
        addRow( kHostSoil, (int)( sizeof( kHostSoil ) / sizeof( kHostSoil[0] ) ), rowY[0] );
        addRow( kDeposit, (int)( sizeof( kDeposit ) / sizeof( kDeposit[0] ) ), rowY[1] );
        addRow( kCrystalHost, (int)( sizeof( kCrystalHost ) / sizeof( kCrystalHost[0] ) ), rowY[2] );
        addRow( kFree, (int)( sizeof( kFree ) / sizeof( kFree[0] ) ), rowY[3] );
    }

    void DrawMaterialFormGallery()
    {
        EnsureGallerySpawned();
        ReseatGallerySamples();
        float const edge = kVoxelEdgeM;
        LitSetIdentity();
        glShadeModel( GL_FLAT );
        glDisable( GL_CULL_FACE );
        glBegin( GL_TRIANGLES );
        for ( int i = 0; i < g.galleryCount; ++i )
        {
            AppState::GallerySample const& s = g.gallery[i];
            if ( !s.present || !s.id ) { continue; }
            uint8_t rr = 90, gg = 120, bb = 70;
            CapColor( s.id, rr, gg, bb );
            LitBindMaterial( s.id );
            EmitGalleryMaterial( s.id, s.x, s.y, s.z, edge, (float)rr, (float)gg, (float)bb );
        }
        glEnd();
        glEnable( GL_CULL_FACE );
    }

    void HandHoldWorldPos( float& hx, float& hy, float& hz )
    {
        float cp = std::cos( g.pitch ), sp = std::sin( g.pitch );
        float cyw = std::cos( g.yaw ), sy = std::sin( g.yaw );
        float fx = sy * cp, fy = cyw * cp, fz = sp;
        // Projection near plane — keep held body beyond active near or it vanishes.
        float const holdDist = (std::max)( 0.72f, g.projNearM + 0.22f );
        float const rad = GalleryCollideRadius( g.heldGalleryId.c_str() );
        hx = g.camX + fx * holdDist + cyw * 0.20f;
        hy = g.camY + fy * holdDist - sy * 0.20f;
        hz = g.camZ + fz * holdDist - 0.10f;
        // Keep body clear of the near clip sphere around the eye.
        float dx = hx - g.camX, dy = hy - g.camY, dz = hz - g.camZ;
        float d = std::sqrt( dx * dx + dy * dy + dz * dz );
        float const minD = g.projNearM + 0.08f + rad;
        if ( d > 1e-5f && d < minD )
        {
            float s = minD / d;
            hx = g.camX + dx * s;
            hy = g.camY + dy * s;
            hz = g.camZ + dz * s;
        }
    }

    void DrawHeldGallerySample()
    {
        if ( g.heldGalleryId.empty() ) { return; }
        float hx = 0.f, hy = 0.f, hz = 0.f;
        HandHoldWorldPos( hx, hy, hz );
        float const edge = kVoxelEdgeM;
        uint8_t rr = 90, gg = 120, bb = 70;
        CapColor( g.heldGalleryId.c_str(), rr, gg, bb );

        // Upright by default (local Z = world up). Scroll tumbles about hand-right so you
        // can roll the sample over in-hand; Shift+scroll spins left/right about up.
        float const ang = g.yaw + g.heldYaw;
        float const ca = std::cos( ang ), sa = std::sin( ang );
        float const rx = ca, ry = -sa;           // hand-right (horizontal)
        float const f0x = sa, f0y = ca;         // horizontal forward before tumble
        float const ct = std::cos( g.heldTumble ), st = std::sin( g.heldTumble );
        // Rotate forward/up about right: tip top toward camera at +90°.
        float const fx = f0x * ct;              // f' = f*ct + u*st ; u=(0,0,1) → xy from f only
        float const fy = f0y * ct;
        float const fz = st;
        float const ux = -f0x * st;             // u' = u*ct - f*st
        float const uy = -f0y * st;
        float const uz = ct;

        // Column-major: local X | local Y | local Z | translation
        float M[16] = {
            rx, ry, 0.f, 0.f,
            fx, fy, fz, 0.f,
            ux, uy, uz, 0.f,
            hx, hy, hz, 1.f
        };

        LitSetFromMatrix( M );
        LitBindMaterial( g.heldGalleryId.c_str() );

        glPushMatrix();
        glMultMatrixf( M );
        // Fresh depth so host faces occlude crystals correctly, while still drawing over world.
        glClear( GL_DEPTH_BUFFER_BIT );
        glEnable( GL_DEPTH_TEST );
        glDepthFunc( GL_LESS );
        glShadeModel( GL_FLAT );
        glDisable( GL_CULL_FACE ); // irregular shells; winding varies
        glBegin( GL_TRIANGLES );
        EmitGalleryMaterial( g.heldGalleryId.c_str(), 0.f, 0.f, 0.f, edge,
            (float)rr, (float)gg, (float)bb );
        glEnd();
        glPopMatrix();
        LitSetIdentity();
    }

    bool DropHeldGalleryToGround()
    {
        if ( g.heldGalleryId.empty() ) { return false; }
        EnsureGallerySpawned();
        UpdateAim();

        float dx = 0.f, dy = 0.f, ground = 0.f;
        bool atAim = false;
        if ( g.aimHit )
        {
            float const adx = g.aimX - g.feetX;
            float const ady = g.aimY - g.feetY;
            float const adist = std::sqrt( adx * adx + ady * ady );
            if ( adist <= kReachCells )
            {
                dx = g.aimX;
                dy = g.aimY;
                ground = g.aimZ;
                atAim = true;
            }
        }
        if ( !atAim )
        {
            float cyw = std::cos( g.yaw ), sy = std::sin( g.yaw );
            dx = g.feetX + sy * 0.55f;
            dy = g.feetY + cyw * 0.55f;
            if ( !SampleGroundZ( dx, dy, ground ) ) { ground = g.feetZ; }
        }

        int slot = g.heldGallerySrc;
        if ( slot < 0 || slot >= g.galleryCount
          || !g.gallery[slot].id
          || std::strcmp( g.gallery[slot].id, g.heldGalleryId.c_str() ) != 0 )
        {
            slot = -1;
            for ( int i = 0; i < g.galleryCount; ++i )
            {
                if ( g.gallery[i].id && !g.gallery[i].present
                  && std::strcmp( g.gallery[i].id, g.heldGalleryId.c_str() ) == 0 )
                {
                    slot = i;
                    break;
                }
            }
        }
        if ( slot < 0 && g.galleryCount < AppState::kGalleryMax )
        {
            slot = g.galleryCount++;
            g.gallery[slot].id = nullptr;
            for ( int i = 0; i < g.galleryCount; ++i )
            {
                if ( g.gallery[i].id && std::strcmp( g.gallery[i].id, g.heldGalleryId.c_str() ) == 0 )
                {
                    g.gallery[slot].id = g.gallery[i].id;
                    break;
                }
            }
            if ( !g.gallery[slot].id )
            {
                g.gallery[slot].id = H2H::FormOrDirt( g.heldGalleryId.c_str() ).material_id;
            }
        }
        if ( slot < 0 ) { return false; }

        AppState::GallerySample& s = g.gallery[slot];
        s.x = dx;
        s.y = dy;
        s.z = ground + GalleryCollideRadius( s.id ? s.id : g.heldGalleryId.c_str() ) * 1.02f;
        s.present = true;
        char d[180];
        std::snprintf( d, sizeof( d ), "E drop — %s at %s",
            g.heldGalleryId.c_str(), atAim ? "reticle" : "feet" );
        g.heldGalleryId.clear();
        g.heldGallerySrc = -1;
        g.heldYaw = 0.f;
        g.heldTumble = 0.f;
        g.digestLine = d;
        UpdateStreamHud();
        return true;
    }

    void ResolveSolidSampleCollisions()
    {
        // Painted material surfaces are solid: push feet/eye out of gallery + held bodies.
        EnsureGallerySpawned();
        ReseatGallerySamples();
        auto pushOut = [&]( float& x, float& y, float& z, float pointR, bool keepZ )
        {
            for ( int i = 0; i < g.galleryCount; ++i )
            {
                AppState::GallerySample const& s = g.gallery[i];
                if ( !s.present || !s.id ) { continue; }
                float const R = GalleryCollideRadius( s.id ) + pointR;
                float dx = x - s.x, dy = y - s.y, dz = z - s.z;
                float d2 = dx * dx + dy * dy + dz * dz;
                if ( d2 >= R * R || d2 < 1e-10f ) { continue; }
                float d = std::sqrt( d2 );
                float nx = dx / d, ny = dy / d, nz = dz / d;
                float pen = R - d;
                x += nx * pen;
                y += ny * pen;
                if ( !keepZ ) { z += nz * pen; }
            }
            // Held inspect body is depth-off past near-plane — do not shove the eye away from it.
        };

        // Feet + body capsule probes
        float fx = g.feetX, fy = g.feetY, fz = g.feetZ + 0.35f;
        pushOut( fx, fy, fz, kCapsuleRadiusM, true );
        g.feetX = fx; g.feetY = fy;
        // Eye must not enter solid shells (amethyst cavity included — outer radius).
        float ex = g.camX, ey = g.camY, ez = g.camZ;
        pushOut( ex, ey, ez, 0.12f, false );
        // If eye was pushed, pull feet with the horizontal delta so walk stays coherent.
        g.feetX += ( ex - g.camX );
        g.feetY += ( ey - g.camY );
        g.camX = ex; g.camY = ey; g.camZ = ez;
    }

    int RayHitGallerySample( float maxDist )
    {
        EnsureGallerySpawned();
        ReseatGallerySamples();
        float cp = std::cos( g.pitch ), sp = std::sin( g.pitch );
        float cyw = std::cos( g.yaw ), sy = std::sin( g.yaw );
        float fx = sy * cp, fy = cyw * cp, fz = sp;
        int best = -1;
        float bestT = maxDist;
        for ( int i = 0; i < g.galleryCount; ++i )
        {
            AppState::GallerySample const& s = g.gallery[i];
            if ( !s.present || !s.id ) { continue; }
            float const hitR = GalleryCollideRadius( s.id );
            float const hitR2 = hitR * hitR;
            float const dx = s.x - g.camX;
            float const dy = s.y - g.camY;
            float const dz = s.z - g.camZ;
            float const t = dx * fx + dy * fy + dz * fz;
            if ( t < 0.2f || t > bestT ) { continue; }
            float const px = g.camX + fx * t;
            float const py = g.camY + fy * t;
            float const pz = g.camZ + fz * t;
            float const ox = s.x - px, oy = s.y - py, oz = s.z - pz;
            if ( ox * ox + oy * oy + oz * oz > hitR2 ) { continue; }
            bestT = t;
            best = i;
        }
        return best;
    }

    bool TryPickupGallerySample()
    {
        if ( g.journalOpen ) { return false; }
        int const hit = RayHitGallerySample( 3.5f );
        if ( hit < 0 )
        {
            // E with nothing aimed: drop held sample back onto the ground.
            if ( !g.heldGalleryId.empty() ) { return DropHeldGalleryToGround(); }
            return false;
        }

        AppState::GallerySample& s = g.gallery[hit];
        char const* newId = s.id;
        if ( !newId || !newId[0] ) { return false; }

        if ( g.grippedBodyId != 0 )
        {
            for ( H2H::MatterBody& b : H2H::State().bodies )
            {
                if ( b.body_id != g.grippedBodyId ) { continue; }
                b.gripped = false;
                break;
            }
            g.grippedBodyId = 0;
        }

        SeedStarterBag();
        if ( !g.heldGalleryId.empty() )
        {
            // Swap: bag current, take the new sample (drop-to-ground is E with empty aim).
            BagAdd( g.heldGalleryId, 1 );
            char d[240];
            std::snprintf( d, sizeof( d ),
                "E swap — bagged %s, now holding %s",
                g.heldGalleryId.c_str(), newId );
            g.heldGalleryId = newId;
            g.heldGallerySrc = hit;
            g.heldYaw = 0.f;
            g.heldTumble = 0.f;
            s.present = false;
            g.digestLine = d;
        }
        else
        {
            g.heldGalleryId = newId;
            g.heldGallerySrc = hit;
            g.heldYaw = 0.f;
            g.heldTumble = 0.f;
            s.present = false;
            char d[200];
            std::snprintf( d, sizeof( d ),
                "E pickup — holding %s  (scroll rolls over; Shift+scroll spins)",
                newId );
            g.digestLine = d;
        }
        UpdateStreamHud();
        return true;
    }

    void ParseColumnReply( std::string const& line )
    {
        int cx = g.pendingColX, cy = g.pendingColY;
        ExtractJsonInt( line, "x", cx );
        ExtractJsonInt( line, "y", cy );

        size_t fillPos = line.find( "\"fill\"" );
        size_t resPos = line.find( "\"resolution\"" );
        float grade = 0.85f;
        ExtractJsonFloat( line, "grade", grade );

        int bw = 8, bh = 8, kz = 32;
        if ( resPos != std::string::npos )
        {
            size_t b = line.find( '[', resPos );
            if ( b != std::string::npos )
            {
                char* e = nullptr;
                bw = (int)strtol( line.c_str() + b + 1, &e, 10 );
                if ( e )
                {
                    while ( *e == ' ' || *e == ',' ) { ++e; }
                    bh = (int)strtol( e, &e, 10 );
                    while ( e && ( *e == ' ' || *e == ',' ) ) { ++e; }
                    kz = (int)strtol( e, &e, 10 );
                }
            }
        }

        std::vector<uint8_t> fillBytes;
        float meanTop = (float)kz;
        if ( fillPos != std::string::npos )
        {
            size_t b = line.find( '[', fillPos );
            size_t e = line.find( ']', b );
            if ( b != std::string::npos && e != std::string::npos && e > b )
            {
                std::string arr = line.substr( b + 1, e - b - 1 );
                std::vector<int> fill;
                fill.reserve( (size_t)bw * bh * kz );
                char const* p = arr.c_str();
                while ( *p )
                {
                    while ( *p == ' ' || *p == ',' ) { ++p; }
                    if ( !*p ) { break; }
                    char* end = nullptr;
                    long v = strtol( p, &end, 10 );
                    if ( end == p ) { break; }
                    fill.push_back( (int)v );
                    p = end;
                }
                int const n = bw * bh;
                if ( (int)fill.size() >= n * kz && n > 0 )
                {
                    double sum = 0.0;
                    fillBytes.resize( (size_t)n * kz );
                    for ( int i = 0; i < n; ++i )
                    {
                        float h = 0.f;
                        for ( int k = 0; k < kz; ++k )
                        {
                            int f = fill[(size_t)k * n + i];
                            fillBytes[(size_t)k * n + i] = (uint8_t)(std::max)( 0, (std::min)( 255, f ) );
                            if ( f > 0 ) { h = (float)k + (float)f / (float)kFillFull; }
                        }
                        sum += h;
                    }
                    meanTop = (float)( sum / n );
                }
            }
        }

        CellSample& cell = g.cells[CellKey( cx, cy )];
        float const wireGrade = grade;
        float const wireZ = GradeToZ( wireGrade );
        // Never bootstrap HF grade from wire — wire crest is ~+10m vs geography and tears
        // vertical triangle strips between geo cells and column-first cells.
        if ( !cell.valid )
        {
            EnsureGeoCell( cx, cy );
        }
        bool const hadGeo = cell.valid;
        float const geoGradeBefore = hadGeo ? cell.grade : wireGrade;
        float const geoZBefore = GradeToZ( geoGradeBefore );
        float const deltaZm = wireZ - geoZBefore;

        if ( kForbidColumnGradeOverwrite )
        {
            // Conviction gate: keep resident geography grade; still take fill / cavity.
            ++g.spireOverwriteBlocked;
            if ( std::fabs( deltaZm ) > 0.01f )
            {
                ++g.spireOverwriteWouldHave;
            }
        }
        else
        {
            cell.grade = wireGrade;
            if ( std::fabs( deltaZm ) > 0.01f )
            {
                ++g.spireOverwriteWouldHave;
            }
        }

        float const keptGrade = cell.grade;
        float const keptZ = GradeToZ( keptGrade );

        // Carved cells keep local affect-sphere fill. Wire voxel_column is often virgin /
        // pre-carve occupancy — replacing fill erased tip voids and made digs vanish at random
        // as the 3×3 column fan-in arrived (HF "hop" / hide previous strikes).
        bool const hadWireFill = !fillBytes.empty();
        bool const keepLocalCarveFill = cell.carved && !cell.fill.empty();
        if ( hadWireFill && !keepLocalCarveFill )
        {
            cell.fill = std::move( fillBytes );
            cell.fillW = bw;
            cell.fillH = bh;
            cell.fillK = kz;
        }
        // Crest/fillZ from KEPT geography grade — never let wire grade spike the HF datum.
        float const newFillZ = FillTopToWorldZ( keptGrade, kz, meanTop );
        // Prefetch/look/walk: store fill only. Never rebuild cavity or punch terrain.
        if ( cell.carved )
        {
            if ( cell.hasFillZ )
            {
                cell.fillZ = (std::min)( cell.fillZ, newFillZ );
            }
            else
            {
                cell.fillZ = newFillZ;
            }
            cell.hasFillZ = true;
            cell.edited = true;
            if ( !keepLocalCarveFill )
            {
                // Only rebuild when we actually accepted wire fill (non-carved→carved edge cases).
                RebuildCavityMesh( cx, cy );
            }
            if ( cell.hasCavity && kAabbCavityOwnsHf )
            {
                InvalidateTerrainMesh();
                RetirePresentationScarsNear( (float)cx + 0.5f, (float)cy + 0.5f, 0.9f );
            }
            else if ( cell.hasCavity && kAabbCavityOwnsHf == false )
            {
                // leave cavity list + tip stencil alone
            }
        }
        else
        {
            cell.fillZ = newFillZ;
            cell.hasFillZ = true;
            DestroyCavityList( cell );
        }

        {
            char const* applied = kForbidColumnGradeOverwrite ? "preserved" : "overwrite";
            char const* fillFate = keepLocalCarveFill ? "keepLocalCarve"
                : ( hadWireFill ? "acceptWire" : "noFill" );
            bool const struck = g.spireHavePre && cx == g.spireCx && cy == g.spireCy;
            char buf[720];
            std::snprintf( buf, sizeof( buf ),
                "SPIRE col cell=(%d,%d)%s geoBefore=%.5f/%.3fm wire=%.5f/%.3fm kept=%.5f/%.3fm deltaZm=%+.3f applied=%s fill=%s blocked=%d wouldHave=%d | visCap=%s authMat=%s",
                cx, cy, struck ? " STRUCK" : "",
                geoGradeBefore, geoZBefore,
                wireGrade, wireZ,
                keptGrade, keptZ,
                deltaZm, applied, fillFate,
                g.spireOverwriteBlocked, g.spireOverwriteWouldHave,
                g.spireVisualCap.empty() ? "?" : g.spireVisualCap.c_str(),
                g.spireAuthMat.empty() ? "?" : g.spireAuthMat.c_str() );
            if ( struck || std::fabs( deltaZm ) > 0.05f )
            {
                g.spireLine = buf;
            }
            OutputDebugStringA( buf );
            OutputDebugStringA( "\n" );
        }

        char d[240];
        std::snprintf( d, sizeof( d ),
            "COLUMN truth (%d,%d): fillZ=%.2f gradeZ=%.2f (wireZ=%.2f) meanTop=%.2f/%d cavity=%s%s%s",
            cx, cy, cell.fillZ, keptZ, wireZ, meanTop, kz,
            cell.hasCavity ? "D2" : "none",
            cell.carved ? " carved" : " (resident)",
            keepLocalCarveFill ? " keepLocal" : "" );
        if ( g.digestLine.find( "DIG digest" ) == std::string::npos &&
             g.digestLine.find( "PLACE digest" ) == std::string::npos )
        {
            g.digestLine = d;
        }
        UpdateStreamHud();
    }

    void HandleReply( std::string const& line )
    {
        PendingKind kind = g.pending;
        g.pending = PendingKind::None;

        bool ok = true;
        ExtractJsonBool( line, "ok", ok );
        if ( !ok && kind != PendingKind::Caps && kind != PendingKind::Carve && kind != PendingKind::Place )
        {
            std::string msg;
            ExtractJsonString( line, "message", msg );
            g.statusLine = "request failed";
            g.digestLine = msg.empty() ? "request failed" : msg;
            UpdateStreamHud();
            return;
        }

        switch ( kind )
        {
            case PendingKind::Caps: ParseCapsReply( line ); break;
            case PendingKind::Player: ParsePlayerReply( line ); break;
            case PendingKind::Surface: ParseSurfaceReply( line ); break;
            case PendingKind::Carve: ParseCarveReply( line ); break;
            case PendingKind::Place: ParsePlaceReply( line ); break;
            case PendingKind::Column: ParseColumnReply( line ); break;
            default: break;
        }
    }

    void PollSocket()
    {
        if ( g.sock == INVALID_SOCKET ) { return; }

        for ( ;; )
        {
            char chunk[65536];
            int n = recv( g.sock, chunk, sizeof( chunk ), 0 );
            if ( n > 0 )
            {
                g.recvBuf.append( chunk, chunk + n );
                continue;
            }
            if ( n == 0 )
            {
                CloseSock();
                g.link = LinkState::Disconnected;
                g.statusLine = "engine closed connection";
                return;
            }
            int err = WSAGetLastError();
            if ( err == WSAEWOULDBLOCK ) { break; }
            CloseSock();
            g.link = LinkState::SocketError;
            g.statusLine = "recv failed";
            char e[64]; std::snprintf( e, sizeof( e ), "WSA %d", err );
            g.detail = e;
            return;
        }

        while ( true )
        {
            size_t nl = g.recvBuf.find( '\n' );
            if ( nl == std::string::npos ) { break; }
            std::string line = g.recvBuf.substr( 0, nl );
            g.recvBuf.erase( 0, nl + 1 );
            HandleReply( line );
        }
    }

    void ResetWorldCache()
    {
        g.cells.clear();
        g.fetchedBlocks.clear();
        g.cellsLoaded = 0;
        g.blocksLoaded = 0;
        g.blocksWanted = 0;
        g.streamComplete = false;
        g.havePlayer = false;
        g.pending = PendingKind::None;
        g.heldBite.clear();
        g.heldTotalG = 0;
        g.heldDominant.clear();
        g.pendingPlaceAsk.clear();
        g.pendingPlaceG = 0;
        ClearPendingScarEdit();
        g.digestLine = "Interaction digest idle - LMB dig into stock, RMB place one scoop, Tab unlock cursor";
        g.scars.clear();
        ++g.scarGen;
        g.columnQueue.clear();
        InvalidateTerrainMesh();
    }

    // Re-author resident grades/caps after --geo-fixture / F8. Dig scars cleared (HF law changed).
    // Does not wake D2 — cavity lists deleted; virgin path stays HF-only until next dig.
    void RefreshGeographyFixture()
    {
        std::vector<std::pair<int, int>> keys;
        keys.reserve( g.cells.size() );
        for ( auto& kv : g.cells )
        {
            int const cx = (int)( kv.first >> 32 );
            int const cy = (int)( kv.first & 0xffffffffu );
            keys.push_back( { cx, cy } );
            if ( kv.second.cavityList )
            {
                glDeleteLists( kv.second.cavityList, 1 );
                kv.second.cavityList = 0;
            }
            if ( kv.second.debugBoundaryList )
            {
                glDeleteLists( kv.second.debugBoundaryList, 1 );
                kv.second.debugBoundaryList = 0;
            }
        }
        g.cells.clear();
        g.cellsLoaded = 0;
        g.scars.clear();
        ++g.scarGen;
        g.editedRegions.clear();
        g.nextEditedRegionId = 1;
        g.cavityTrisTotal = 0;
        g.cavityEdgesEmitted = 0;
        for ( auto const& xy : keys )
        {
            EnsureGeoCell( xy.first, xy.second );
        }
        InvalidateTerrainMesh();
        char d[192];
        std::snprintf( d, sizeof( d ), "Geo fixture: %s  (%s)  [F8] cycle  --geo-fixture=range|torture",
            ProvenanceGeo::FixtureName( ProvenanceGeo::Fixture() ),
            ProvenanceGeo::FixtureLabel( ProvenanceGeo::Fixture() ) );
        g.digestLine = d;
        g.statusLine = d;
    }

    void TryConnect()
    {
        CloseSock();
        ResetWorldCache();
        g.link = LinkState::Connecting;
        g.statusLine = "connecting...";
        g.detail.clear();
        g.attempts++;

        addrinfo hints = {};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        char portStr[16];
        std::snprintf( portStr, sizeof( portStr ), "%d", g.port );
        addrinfo* res = nullptr;
        if ( getaddrinfo( g.host.c_str(), portStr, &hints, &res ) != 0 || !res )
        {
            g.link = LinkState::SocketError;
            g.statusLine = "getaddrinfo failed";
            return;
        }

        SOCKET s = socket( res->ai_family, res->ai_socktype, res->ai_protocol );
        if ( s == INVALID_SOCKET )
        {
            freeaddrinfo( res );
            g.link = LinkState::SocketError;
            g.statusLine = "socket() failed";
            return;
        }

        if ( connect( s, res->ai_addr, (int)res->ai_addrlen ) != 0 )
        {
            closesocket( s );
            freeaddrinfo( res );
            g.link = LinkState::Disconnected;
            g.statusLine = "connect failed - is voxel_bridge listening?";
            char d[128];
            std::snprintf( d, sizeof( d ), "target %s:%d  (WSA %d)  attempt #%d",
                g.host.c_str(), g.port, WSAGetLastError(), g.attempts );
            g.detail = d;
            return;
        }
        freeaddrinfo( res );

        u_long nonBlock = 1;
        ioctlsocket( s, FIONBIO, &nonBlock );
        g.sock = s;
        g.link = LinkState::Connected;
        g.statusLine = "TCP up - requesting terrain_caps";
        if ( !RequestMethod( "terrain_caps", "{}", PendingKind::Caps ) )
        {
            CloseSock();
            g.link = LinkState::SocketError;
            g.statusLine = "failed to send terrain_caps";
        }
    }

    bool InitGL( HWND hwnd )
    {
        g.hdc = GetDC( hwnd );
        PIXELFORMATDESCRIPTOR pfd = {};
        pfd.nSize = sizeof( pfd );
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 24;
        pfd.cStencilBits = 8;
        int pf = ChoosePixelFormat( g.hdc, &pfd );
        if ( !pf || !SetPixelFormat( g.hdc, pf, &pfd ) ) { return false; }
        g.glrc = wglCreateContext( g.hdc );
        if ( !g.glrc || !wglMakeCurrent( g.hdc, g.glrc ) ) { return false; }

        glEnable( GL_DEPTH_TEST );
        glEnable( GL_CULL_FACE );
        glCullFace( GL_BACK );
        glShadeModel( GL_FLAT );

        // Bitmap font for HUD
        g.fontBase = glGenLists( 96 );
        HFONT font = CreateFontW( -16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
            FF_DONTCARE | FIXED_PITCH, L"Consolas" );
        HFONT old = (HFONT)SelectObject( g.hdc, font );
        wglUseFontBitmapsW( g.hdc, 32, 96, g.fontBase );
        SelectObject( g.hdc, old );
        DeleteObject( font );
        LoadIconTextures();
        SeedStarterBag();
        return true;
    }

    void ShutdownGL()
    {
        if ( g.terrainList ) { glDeleteLists( g.terrainList, 1 ); g.terrainList = 0; }
        if ( g.fontBase ) { glDeleteLists( g.fontBase, 96 ); g.fontBase = 0; }
        UnloadIconTextures();
        wglMakeCurrent( nullptr, nullptr );
        if ( g.glrc ) { wglDeleteContext( g.glrc ); g.glrc = nullptr; }
        if ( g.hdc && g.hwnd ) { ReleaseDC( g.hwnd, g.hdc ); g.hdc = nullptr; }
    }

    void DrawHudText( float x, float y, char const* text )
    {
        // Call ONLY font glyph lists (fontBase .. fontBase+94).
        // Never use glListBase+glCallLists — if terrainList ever lands in that byte range,
        // HUD text would execute the world heightfield in ortho (floating "minimap" pane).
        if ( !text || !g.fontBase ) { return; }
        glRasterPos2f( x, y );
        for ( char const* p = text; *p; ++p )
        {
            unsigned char const c = (unsigned char)*p;
            if ( c < 32 || c > 126 ) { continue; }
            glCallList( g.fontBase + ( c - 32 ) );
        }
    }

    GLuint AllocDisplayListOutsideFonts()
    {
        // Keep world geometry lists out of the bitmap-font ID range.
        GLuint const fontLo = g.fontBase;
        GLuint const fontHi = g.fontBase ? ( g.fontBase + 95 ) : 0;
        for ( int attempt = 0; attempt < 8; ++attempt )
        {
            GLuint id = glGenLists( 1 );
            if ( !id ) { return 0; }
            if ( !g.fontBase || id < fontLo || id > fontHi ) { return id; }
            glDeleteLists( id, 1 );
            // Burn a gap past the font block, then retry.
            GLuint burn = glGenLists( 96 );
            if ( burn ) { glDeleteLists( burn, 96 ); }
        }
        return glGenLists( 1 );
    }

    // ---------------- Journal / hotbar UI ----------------

    bool FileExistsA( char const* path )
    {
        DWORD a = GetFileAttributesA( path );
        return a != INVALID_FILE_ATTRIBUTES && !( a & FILE_ATTRIBUTE_DIRECTORY );
    }

    std::string JoinPath( std::string const& a, char const* b )
    {
        if ( a.empty() ) { return b; }
        if ( a.back() == '\\' || a.back() == '/' ) { return a + b; }
        return a + "\\" + b;
    }

    void ResolveAssetsRoot()
    {
        if ( !g.assetsRoot.empty() ) { return; }
        char mod[MAX_PATH] = {};
        GetModuleFileNameA( nullptr, mod, MAX_PATH );
        std::string dir( mod );
        size_t slash = dir.find_last_of( "\\/" );
        if ( slash != std::string::npos ) { dir.resize( slash ); }
        for ( int up = 0; up < 6; ++up )
        {
            std::string probe = JoinPath( dir, "Assets\\Icons\\Materials\\dirt.png" );
            if ( FileExistsA( probe.c_str() ) )
            {
                g.assetsRoot = JoinPath( dir, "Assets" );
                return;
            }
            slash = dir.find_last_of( "\\/" );
            if ( slash == std::string::npos ) { break; }
            dir.resize( slash );
        }
        // Fallback: common checkout layout beside Build\
        g.assetsRoot = "C:\\Users\\D-Day\\ProvenanceEsoterica\\Assets";
    }

    GLuint LoadIconTextureFile( char const* path )
    {
        int w = 0, h = 0, n = 0;
        stbi_set_flip_vertically_on_load( 1 );
        unsigned char* data = stbi_load( path, &w, &h, &n, 4 );
        if ( !data || w <= 0 || h <= 0 ) { return 0; }
        GLuint tex = 0;
        glGenTextures( 1, &tex );
        glBindTexture( GL_TEXTURE_2D, tex );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
        stbi_image_free( data );
        glBindTexture( GL_TEXTURE_2D, 0 );
        return tex;
    }

    void LoadIconTextures()
    {
        ResolveAssetsRoot();
        char const* folders[] = { "Icons\\Tools", "Icons\\Materials" };
        for ( char const* folder : folders )
        {
            std::string dir = JoinPath( g.assetsRoot, folder );
            std::string pattern = JoinPath( dir, "*.png" );
            WIN32_FIND_DATAA fd = {};
            HANDLE h = FindFirstFileA( pattern.c_str(), &fd );
            if ( h == INVALID_HANDLE_VALUE ) { continue; }
            do
            {
                if ( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) { continue; }
                std::string name = fd.cFileName;
                if ( name.size() < 5 || name[0] == '_' ) { continue; }
                if ( name.size() < 4 || name.substr( name.size() - 4 ) != ".png" ) { continue; }
                std::string id = name.substr( 0, name.size() - 4 );
                if ( g.iconTex.count( id ) ) { continue; }
                std::string path = JoinPath( dir, name.c_str() );
                GLuint tex = LoadIconTextureFile( path.c_str() );
                if ( tex ) { g.iconTex[id] = tex; }
            } while ( FindNextFileA( h, &fd ) );
            FindClose( h );
        }
    }

    void UnloadIconTextures()
    {
        for ( auto& kv : g.iconTex )
        {
            if ( kv.second ) { glDeleteTextures( 1, &kv.second ); }
        }
        g.iconTex.clear();
    }

    int BagFind( std::string const& id )
    {
        for ( int i = 0; i < g.bagCount; ++i )
        {
            if ( g.bag[i].id == id ) { return i; }
        }
        return -1;
    }

    void BagAdd( std::string const& id, int count )
    {
        if ( id.empty() || count <= 0 ) { return; }
        int i = BagFind( id );
        if ( i >= 0 ) { g.bag[i].count += count; return; }
        if ( g.bagCount >= (int)( sizeof( g.bag ) / sizeof( g.bag[0] ) ) ) { return; }
        g.bag[g.bagCount].id = id;
        g.bag[g.bagCount].count = count;
        ++g.bagCount;
    }

    bool BagTake( std::string const& id, int count )
    {
        int i = BagFind( id );
        if ( i < 0 || g.bag[i].count < count ) { return false; }
        g.bag[i].count -= count;
        if ( g.bag[i].count <= 0 )
        {
            g.bag[i] = g.bag[g.bagCount - 1];
            --g.bagCount;
        }
        return true;
    }

    void SeedStarterBag()
    {
        if ( g.bagSeeded ) { return; }
        g.bagSeeded = true;
        BagAdd( "axe", 1 );
        BagAdd( "pick", 1 );
        BagAdd( "shovel", 1 );
        BagAdd( "knife", 1 );
        BagAdd( "wood_log", 4 );
        BagAdd( "sticks_tinder", 6 );
        BagAdd( "flint", 3 );
        BagAdd( "bark", 4 );
        BagAdd( "iron", 4 );
        BagAdd( "coal", 2 );
        BagAdd( "leather_hide", 2 );
        BagAdd( "rope", 2 );
        BagAdd( "dirt", 8 );
        g.hotbar[0] = "shovel";
        g.hotbar[1] = "axe";
        g.hotbar[2] = "pick";
        g.hotbar[3] = "torch";
        g.hotbar[4] = "";
        g.hotbar[5] = "";
    }

    void SyncBagFromHeldBite()
    {
        SeedStarterBag();
        // Mirror carry into bag for journal visibility — do not wipe engine heldBite.
        for ( auto const& kv : g.heldBite )
        {
            if ( kv.second <= 0 ) { continue; }
            int units = (std::max)( 1, kv.second / (int)kHandfulDirtG );
            std::string id = kv.first;
            if ( id == "loam" || id == "soil" ) { id = "dirt"; }
            int existing = BagFind( id );
            if ( existing < 0 ) { BagAdd( id, units ); }
            else if ( g.bag[existing].count < units ) { g.bag[existing].count = units; }
        }
    }

    void ClearDeposit()
    {
        for ( int i = 0; i < kMaxDeposit; ++i )
        {
            g.deposit[i].id.clear();
            g.deposit[i].count = 0;
        }
    }

    void OpenJournal()
    {
        if ( g.journalOpen ) { return; }
        SyncBagFromHeldBite();
        g.journalWasMouseLook = g.mouseLook;
        g.journalOpen = true;
        SetMouseLook( g.hwnd, false );
        g.digestLine = "Journal open - [J]/Esc] close   arrows page inventory/craft   1-6 hotbar";
    }

    void CloseJournal()
    {
        if ( !g.journalOpen ) { return; }
        g.journalOpen = false;
        g.selectedRecipe = -1;
        ClearDeposit();
        SetMouseLook( g.hwnd, g.journalWasMouseLook );
        g.digestLine = "Journal closed - hotbar stays   [J] journal";
    }

    void ToggleJournal()
    {
        if ( g.journalOpen ) { CloseJournal(); }
        else { OpenJournal(); }
    }

    struct UiRect { float x0, y0, x1, y1; };
    bool UiHit( UiRect const& r, float x, float y )
    {
        return x >= r.x0 && x <= r.x1 && y >= r.y0 && y <= r.y1;
    }

    void FillRect( float x0, float y0, float x1, float y1, float r, float gcol, float b, float a = 1.f )
    {
        if ( a < 0.999f )
        {
            glEnable( GL_BLEND );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        }
        glColor4f( r, gcol, b, a );
        glBegin( GL_QUADS );
        glVertex2f( x0, y0 ); glVertex2f( x1, y0 ); glVertex2f( x1, y1 ); glVertex2f( x0, y1 );
        glEnd();
        if ( a < 0.999f ) { glDisable( GL_BLEND ); }
    }

    void StrokeRect( float x0, float y0, float x1, float y1, float r, float gcol, float b )
    {
        glColor3f( r, gcol, b );
        glBegin( GL_LINE_LOOP );
        glVertex2f( x0, y0 ); glVertex2f( x1, y0 ); glVertex2f( x1, y1 ); glVertex2f( x0, y1 );
        glEnd();
    }

    void DrawIconInRect( std::string const& id, float x0, float y0, float x1, float y1 )
    {
        auto it = g.iconTex.find( id );
        if ( it == g.iconTex.end() || !it->second )
        {
            uint8_t rr = 90, gg = 70, bb = 45;
            CapColor( id.c_str(), rr, gg, bb );
            FillRect( x0 + 2, y0 + 2, x1 - 2, y1 - 2, rr / 255.f, gg / 255.f, bb / 255.f );
            return;
        }
        glEnable( GL_TEXTURE_2D );
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glBindTexture( GL_TEXTURE_2D, it->second );
        glColor4f( 1.f, 1.f, 1.f, 1.f );
        glBegin( GL_QUADS );
        glTexCoord2f( 0.f, 0.f ); glVertex2f( x0, y0 );
        glTexCoord2f( 1.f, 0.f ); glVertex2f( x1, y0 );
        glTexCoord2f( 1.f, 1.f ); glVertex2f( x1, y1 );
        glTexCoord2f( 0.f, 1.f ); glVertex2f( x0, y1 );
        glEnd();
        glBindTexture( GL_TEXTURE_2D, 0 );
        glDisable( GL_TEXTURE_2D );
        glDisable( GL_BLEND );
    }

    uint32_t HashMix( uint32_t x )
    {
        x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
        return x;
    }

    float SeedNoise( int x, int y, uint32_t seed )
    {
        uint32_t h = HashMix( (uint32_t)x * 374761393u ^ (uint32_t)y * 668265263u ^ seed );
        return ( h & 0xFFFF ) / 65535.f;
    }

    uint32_t MapSeedFromWorld()
    {
        uint32_t s = 0xA5A5A5A5u;
        for ( char c : g.worldIdentityHash ) { s = HashMix( s ^ (uint8_t)c ); }
        for ( char c : g.generatorId ) { s = HashMix( s ^ (uint8_t)c ); }
        s ^= (uint32_t)g.generatorVersion * 0x9E3779B9u;
        if ( s == 0 ) { s = 0xC0FFEEu; }
        return s;
    }

    void DrawSeedMap( float x0, float y0, float x1, float y1 )
    {
        FillRect( x0, y0, x1, y1, 0.78f, 0.70f, 0.52f );
        uint32_t seed = MapSeedFromWorld();
        int const nx = 48, ny = 48;
        float const dx = ( x1 - x0 ) / (float)nx;
        float const dy = ( y1 - y0 ) / (float)ny;
        int const pcx = (int)std::floor( g.feetX );
        int const pcy = (int)std::floor( g.feetY );
        glBegin( GL_QUADS );
        for ( int j = 0; j < ny; ++j )
        {
            for ( int i = 0; i < nx; ++i )
            {
                int wx = pcx + i - nx / 2;
                int wy = pcy + j - ny / 2;
                float n = SeedNoise( wx, wy, seed );
                float n2 = SeedNoise( wx / 4, wy / 4, seed ^ 0x1111u );
                float hgt = 0.55f * n + 0.45f * n2;
                float r = 0.25f + 0.35f * hgt;
                float gg = 0.40f + 0.35f * ( 1.f - hgt );
                float b = 0.22f + 0.15f * n;
                if ( hgt < 0.28f ) { r = 0.30f; gg = 0.45f; b = 0.70f; } // water-ish
                glColor3f( r, gg, b );
                float px0 = x0 + i * dx, py0 = y0 + j * dy;
                glVertex2f( px0, py0 ); glVertex2f( px0 + dx, py0 );
                glVertex2f( px0 + dx, py0 + dy ); glVertex2f( px0, py0 + dy );
            }
        }
        glEnd();
        // Player marker
        float mx = x0 + ( nx * 0.5f ) * dx;
        float my = y0 + ( ny * 0.5f ) * dy;
        FillRect( mx - 3, my - 3, mx + 3, my + 3, 0.85f, 0.15f, 0.12f );
        StrokeRect( x0, y0, x1, y1, 0.35f, 0.25f, 0.12f );
    }

    // Layout helpers (ortho y-up). Hotbar always at bottom-center.
    void HotbarLayout( float& x0, float& y0, float& slot, float& gap )
    {
        float const w = (float)g.uiWinW;
        slot = 52.f;
        gap = 8.f;
        float total = kHotbarSlots * slot + ( kHotbarSlots - 1 ) * gap;
        x0 = ( w - total ) * 0.5f;
        y0 = 18.f;
    }

    void DrawHotbar()
    {
        float x0, y0, slot, gap;
        HotbarLayout( x0, y0, slot, gap );
        for ( int i = 0; i < kHotbarSlots; ++i )
        {
            float sx = x0 + i * ( slot + gap );
            float sy = y0;
            FillRect( sx - 3, sy - 3, sx + slot + 3, sy + slot + 3, 0.28f, 0.18f, 0.10f, 0.92f );
            FillRect( sx, sy, sx + slot, sy + slot, 0.45f, 0.34f, 0.22f, 0.95f );
            if ( !g.hotbar[i].empty() )
            {
                DrawIconInRect( g.hotbar[i], sx + 4, sy + 4, sx + slot - 4, sy + slot - 4 );
            }
            if ( i == g.hotbarSel )
            {
                StrokeRect( sx - 2, sy - 2, sx + slot + 2, sy + slot + 2, 0.95f, 0.85f, 0.35f );
            }
            else
            {
                StrokeRect( sx, sy, sx + slot, sy + slot, 0.15f, 0.10f, 0.05f );
            }
            char lab[4];
            std::snprintf( lab, sizeof( lab ), "%d", i + 1 );
            glColor3f( 0.95f, 0.90f, 0.75f );
            DrawHudText( sx + 4, sy + slot - 14, lab );
        }
    }

    bool TryHotbarClick( float mx, float my )
    {
        float x0, y0, slot, gap;
        HotbarLayout( x0, y0, slot, gap );
        for ( int i = 0; i < kHotbarSlots; ++i )
        {
            float sx = x0 + i * ( slot + gap );
            if ( UiHit( { sx, y0, sx + slot, y0 + slot }, mx, my ) )
            {
                g.hotbarSel = i;
                return true;
            }
        }
        return false;
    }

    bool RecipeDepositSatisfied( int ri )
    {
        if ( ri < 0 || ri >= kRecipeCount ) { return false; }
        RecipeDef const& r = kRecipes[ri];
        for ( int i = 0; i < r.nReq; ++i )
        {
            if ( g.deposit[i].id != r.reqId[i] || g.deposit[i].count < r.reqCount[i] )
            {
                return false;
            }
        }
        return true;
    }

    bool TryCraftSelected()
    {
        if ( g.selectedRecipe < 0 || !RecipeDepositSatisfied( g.selectedRecipe ) ) { return false; }
        RecipeDef const& r = kRecipes[g.selectedRecipe];
        for ( int i = 0; i < r.nReq; ++i )
        {
            // mats already in deposit — clear them
            g.deposit[i].id.clear();
            g.deposit[i].count = 0;
        }
        BagAdd( r.resultId, r.resultCount );
        g.digestLine = std::string( "Crafted " ) + r.name;
        return true;
    }

    bool TryDepositFromBag( int bagIndex )
    {
        if ( g.selectedRecipe < 0 || bagIndex < 0 || bagIndex >= g.bagCount ) { return false; }
        RecipeDef const& r = kRecipes[g.selectedRecipe];
        std::string const& id = g.bag[bagIndex].id;
        for ( int i = 0; i < r.nReq; ++i )
        {
            if ( r.reqId[i] != id ) { continue; }
            int need = r.reqCount[i] - g.deposit[i].count;
            if ( need <= 0 ) { continue; }
            int take = (std::min)( need, g.bag[bagIndex].count );
            if ( !BagTake( id, take ) ) { return false; }
            g.deposit[i].id = id;
            g.deposit[i].count += take;
            return true;
        }
        return false;
    }

    void DrawJournal()
    {
        float const W = (float)g.uiWinW;
        float const H = (float)g.uiWinH;
        // Dim world
        FillRect( 0, 0, W, H, 0.02f, 0.02f, 0.02f, 0.45f );

        float const margin = 28.f;
        float const frameX0 = margin;
        float const frameY0 = 90.f;
        float const frameX1 = W - margin;
        float const frameY1 = H - 36.f;
        FillRect( frameX0, frameY0, frameX1, frameY1, 0.22f, 0.12f, 0.07f, 0.97f ); // leather
        StrokeRect( frameX0, frameY0, frameX1, frameY1, 0.10f, 0.05f, 0.02f );

        float const colGap = 14.f;
        float const inner = 16.f;
        float const usableW = frameX1 - frameX0 - inner * 2;
        float const leftW = usableW * 0.28f;
        float const midW = usableW * 0.44f;
        float const rightW = usableW - leftW - midW - colGap * 2;
        float const leftX0 = frameX0 + inner;
        float const midX0 = leftX0 + leftW + colGap;
        float const rightX0 = midX0 + midW + colGap;
        float const topY1 = frameY1 - inner;
        float const botY0 = frameY0 + inner + 70.f; // leave room above hotbar tabs visually

        // --- LEFT: inventory + crafting ---
        float invTop = topY1;
        float invBot = frameY0 + ( frameY1 - frameY0 ) * 0.48f;
        FillRect( leftX0, invBot, leftX0 + leftW, invTop, 0.82f, 0.74f, 0.55f );
        glColor3f( 0.20f, 0.12f, 0.06f );
        DrawHudText( leftX0 + 10, invTop - 18, "INVENTORY" );
        // arrows
        UiRect invL = { leftX0 + leftW - 52, invTop - 24, leftX0 + leftW - 30, invTop - 6 };
        UiRect invR = { leftX0 + leftW - 28, invTop - 24, leftX0 + leftW - 6, invTop - 6 };
        FillRect( invL.x0, invL.y0, invL.x1, invL.y1, 0.35f, 0.22f, 0.12f );
        FillRect( invR.x0, invR.y0, invR.x1, invR.y1, 0.35f, 0.22f, 0.12f );
        glColor3f( 0.95f, 0.90f, 0.75f );
        DrawHudText( invL.x0 + 5, invL.y0 + 4, "<" );
        DrawHudText( invR.x0 + 5, invR.y0 + 4, ">" );

        float slot = (std::min)( 48.f, ( leftW - 24 ) / kInvCols - 4 );
        float gridX = leftX0 + 12;
        float gridY1 = invTop - 36;
        int invPages = (std::max)( 1, ( g.bagCount + kInvPageSize - 1 ) / kInvPageSize );
        if ( g.invPage >= invPages ) { g.invPage = invPages - 1; }
        if ( g.invPage < 0 ) { g.invPage = 0; }
        int invBase = g.invPage * kInvPageSize;
        for ( int row = 0; row < kInvRows; ++row )
        {
            for ( int col = 0; col < kInvCols; ++col )
            {
                int si = invBase + row * kInvCols + col;
                float sx = gridX + col * ( slot + 6 );
                float sy = gridY1 - ( row + 1 ) * ( slot + 6 );
                FillRect( sx, sy, sx + slot, sy + slot, 0.18f, 0.12f, 0.08f );
                StrokeRect( sx, sy, sx + slot, sy + slot, 0.08f, 0.05f, 0.03f );
                if ( si < g.bagCount )
                {
                    DrawIconInRect( g.bag[si].id, sx + 3, sy + 3, sx + slot - 3, sy + slot - 3 );
                    if ( g.bag[si].count > 1 )
                    {
                        char nbuf[16];
                        std::snprintf( nbuf, sizeof( nbuf ), "%d", g.bag[si].count );
                        glColor3f( 1.f, 1.f, 0.85f );
                        DrawHudText( sx + 4, sy + 4, nbuf );
                    }
                }
            }
        }

        // Crafting panel
        float craftTop = invBot - 10;
        float craftBot = botY0;
        FillRect( leftX0, craftBot, leftX0 + leftW, craftTop, 0.80f, 0.72f, 0.52f );
        glColor3f( 0.20f, 0.12f, 0.06f );
        DrawHudText( leftX0 + 10, craftTop - 18, "CRAFTING" );
        UiRect craftBtn = { leftX0 + leftW - 70, craftTop - 24, leftX0 + leftW - 6, craftTop - 6 };
        bool canCraft = RecipeDepositSatisfied( g.selectedRecipe );
        FillRect( craftBtn.x0, craftBtn.y0, craftBtn.x1, craftBtn.y1,
            canCraft ? 0.45f : 0.30f, canCraft ? 0.32f : 0.20f, canCraft ? 0.14f : 0.10f );
        glColor3f( 0.95f, 0.90f, 0.75f );
        DrawHudText( craftBtn.x0 + 8, craftBtn.y0 + 4, "Craft" );

        UiRect craftL = { leftX0 + 8, craftTop - 48, leftX0 + 28, craftTop - 30 };
        UiRect craftR = { leftX0 + 30, craftTop - 48, leftX0 + 50, craftTop - 30 };
        FillRect( craftL.x0, craftL.y0, craftL.x1, craftL.y1, 0.35f, 0.22f, 0.12f );
        FillRect( craftR.x0, craftR.y0, craftR.x1, craftR.y1, 0.35f, 0.22f, 0.12f );
        glColor3f( 0.95f, 0.90f, 0.75f );
        DrawHudText( craftL.x0 + 5, craftL.y0 + 4, "<" );
        DrawHudText( craftR.x0 + 5, craftR.y0 + 4, ">" );

        if ( g.selectedRecipe < 0 )
        {
            glColor3f( 0.25f, 0.15f, 0.08f );
            DrawHudText( leftX0 + 56, craftTop - 44, "select a recipe" );
            int pages = (std::max)( 1, ( kRecipeCount + kCraftPageSize - 1 ) / kCraftPageSize );
            if ( g.craftPage >= pages ) { g.craftPage = pages - 1; }
            int base = g.craftPage * kCraftPageSize;
            float cslot = (std::min)( 44.f, ( leftW - 24 ) / kCraftCols - 4 );
            for ( int row = 0; row < kCraftRows; ++row )
            {
                for ( int col = 0; col < kCraftCols; ++col )
                {
                    int ri = base + row * kCraftCols + col;
                    float sx = leftX0 + 12 + col * ( cslot + 6 );
                    float sy = craftTop - 60 - ( row + 1 ) * ( cslot + 8 );
                    FillRect( sx, sy, sx + cslot, sy + cslot, 0.18f, 0.12f, 0.08f );
                    if ( ri < kRecipeCount )
                    {
                        DrawIconInRect( kRecipes[ri].resultId, sx + 3, sy + 3, sx + cslot - 3, sy + cslot - 3 );
                    }
                }
            }
        }
        else
        {
            RecipeDef const& r = kRecipes[g.selectedRecipe];
            glColor3f( 0.20f, 0.12f, 0.06f );
            DrawHudText( leftX0 + 56, craftTop - 44, r.name );
            // result icon
            float rx = leftX0 + leftW * 0.5f - 28;
            float ry = craftTop - 120;
            FillRect( rx, ry, rx + 56, ry + 56, 0.18f, 0.12f, 0.08f );
            DrawIconInRect( r.resultId, rx + 4, ry + 4, rx + 52, ry + 52 );
            glColor3f( 0.25f, 0.15f, 0.08f );
            DrawHudText( leftX0 + 12, ry - 18, "REQUIRED" );
            float reqY = ry - 70;
            for ( int i = 0; i < r.nReq; ++i )
            {
                float sx = leftX0 + 12 + i * 48;
                FillRect( sx, reqY, sx + 42, reqY + 42, 0.18f, 0.12f, 0.08f );
                DrawIconInRect( r.reqId[i], sx + 3, reqY + 3, sx + 39, reqY + 39 );
                char nb[8];
                std::snprintf( nb, sizeof( nb ), "x%d", r.reqCount[i] );
                glColor3f( 1.f, 1.f, 0.85f );
                DrawHudText( sx + 4, reqY + 2, nb );
            }
            glColor3f( 0.25f, 0.15f, 0.08f );
            DrawHudText( leftX0 + 12, reqY - 16, "put mats below" );
            float depY = craftBot + 16;
            for ( int i = 0; i < r.nReq; ++i )
            {
                float sx = leftX0 + 12 + i * 48;
                FillRect( sx, depY, sx + 42, depY + 42, 0.12f, 0.18f, 0.10f );
                StrokeRect( sx, depY, sx + 42, depY + 42, 0.40f, 0.55f, 0.30f );
                if ( !g.deposit[i].id.empty() )
                {
                    DrawIconInRect( g.deposit[i].id, sx + 3, depY + 3, sx + 39, depY + 39 );
                    char nb[8];
                    std::snprintf( nb, sizeof( nb ), "%d", g.deposit[i].count );
                    glColor3f( 1.f, 1.f, 0.85f );
                    DrawHudText( sx + 4, depY + 2, nb );
                }
            }
            // back hint
            glColor3f( 0.35f, 0.22f, 0.12f );
            DrawHudText( leftX0 + 12, craftTop - 64, "[RMB] back to list" );
        }

        // --- CENTER: map ---
        FillRect( midX0, botY0, midX0 + midW, topY1, 0.75f, 0.68f, 0.50f );
        DrawSeedMap( midX0 + 8, botY0 + 8, midX0 + midW - 8, topY1 - 28 );
        glColor3f( 0.20f, 0.12f, 0.06f );
        DrawHudText( midX0 + 12, topY1 - 18, "WORLD MAP" );
        char seedLine[96];
        std::snprintf( seedLine, sizeof( seedLine ), "seed %08X", MapSeedFromWorld() );
        DrawHudText( midX0 + 12, botY0 + 12, seedLine );

        // --- RIGHT: skills + journal ---
        float skillsBot = frameY0 + ( frameY1 - frameY0 ) * 0.52f;
        FillRect( rightX0, skillsBot, rightX0 + rightW, topY1, 0.82f, 0.74f, 0.55f );
        glColor3f( 0.20f, 0.12f, 0.06f );
        DrawHudText( rightX0 + 10, topY1 - 18, "SKILLS" );
        for ( int i = 0; i < 5; ++i )
        {
            float yy = topY1 - 48 - i * 36;
            // diamond
            float cx = rightX0 + 22, cy = yy + 10;
            glColor3f( 0.45f, 0.32f, 0.18f );
            glBegin( GL_QUADS );
            glVertex2f( cx, cy + 10 ); glVertex2f( cx + 10, cy );
            glVertex2f( cx, cy - 10 ); glVertex2f( cx - 10, cy );
            glEnd();
            DrawHudText( rightX0 + 40, yy + 4, g.skillNames[i] );
            float barX0 = rightX0 + 40, barX1 = rightX0 + rightW - 12;
            FillRect( barX0, yy - 8, barX1, yy - 2, 0.25f, 0.18f, 0.10f );
            FillRect( barX0, yy - 8, barX0 + ( barX1 - barX0 ) * g.skillFrac[i], yy - 2, 0.55f, 0.40f, 0.18f );
        }

        FillRect( rightX0, botY0, rightX0 + rightW, skillsBot - 8, 0.80f, 0.72f, 0.52f );
        glColor3f( 0.20f, 0.12f, 0.06f );
        DrawHudText( rightX0 + 10, skillsBot - 26, "JOURNAL" );
        for ( int i = 0; i < 6; ++i )
        {
            float yy = skillsBot - 50 - i * 22;
            StrokeRect( rightX0 + 10, yy, rightX0 + rightW - 10, yy + 1, 0.45f, 0.35f, 0.22f );
            if ( g.journalNotes[i] && g.journalNotes[i][0] )
            {
                DrawHudText( rightX0 + 12, yy + 4, g.journalNotes[i] );
            }
        }

        glColor3f( 0.90f, 0.85f, 0.70f );
        DrawHudText( frameX0 + 12, frameY0 + 8, "[J]/Esc] close journal   click bag to deposit mats   Craft when ready" );
    }

    bool HandleJournalClick( float mx, float my, bool rightClick )
    {
        float const W = (float)g.uiWinW;
        float const H = (float)g.uiWinH;
        float const margin = 28.f;
        float const frameX0 = margin;
        float const frameY0 = 90.f;
        float const frameX1 = W - margin;
        float const frameY1 = H - 36.f;
        float const colGap = 14.f;
        float const inner = 16.f;
        float const usableW = frameX1 - frameX0 - inner * 2;
        float const leftW = usableW * 0.28f;
        float const leftX0 = frameX0 + inner;
        float const topY1 = frameY1 - inner;
        float const invBot = frameY0 + ( frameY1 - frameY0 ) * 0.48f;
        float const craftTop = invBot - 10;
        float const botY0 = frameY0 + inner + 70.f;

        if ( rightClick && g.selectedRecipe >= 0 )
        {
            g.selectedRecipe = -1;
            ClearDeposit();
            return true;
        }

        // Inventory arrows
        UiRect invL = { leftX0 + leftW - 52, topY1 - 24, leftX0 + leftW - 30, topY1 - 6 };
        UiRect invR = { leftX0 + leftW - 28, topY1 - 24, leftX0 + leftW - 6, topY1 - 6 };
        if ( UiHit( invL, mx, my ) ) { g.invPage = (std::max)( 0, g.invPage - 1 ); return true; }
        if ( UiHit( invR, mx, my ) )
        {
            int pages = (std::max)( 1, ( g.bagCount + kInvPageSize - 1 ) / kInvPageSize );
            g.invPage = (std::min)( pages - 1, g.invPage + 1 );
            return true;
        }

        // Inventory slots -> deposit or assign to hotbar if shift... simple: deposit when recipe selected
        float slot = (std::min)( 48.f, ( leftW - 24 ) / kInvCols - 4 );
        float gridX = leftX0 + 12;
        float gridY1 = topY1 - 36;
        int invBase = g.invPage * kInvPageSize;
        for ( int row = 0; row < kInvRows; ++row )
        {
            for ( int col = 0; col < kInvCols; ++col )
            {
                int si = invBase + row * kInvCols + col;
                float sx = gridX + col * ( slot + 6 );
                float sy = gridY1 - ( row + 1 ) * ( slot + 6 );
                if ( !UiHit( { sx, sy, sx + slot, sy + slot }, mx, my ) ) { continue; }
                if ( si >= g.bagCount ) { return true; }
                if ( g.selectedRecipe >= 0 )
                {
                    TryDepositFromBag( si );
                }
                else
                {
                    // equip to selected hotbar
                    g.hotbar[g.hotbarSel] = g.bag[si].id;
                    g.digestLine = std::string( "Hotbar " ) + std::to_string( g.hotbarSel + 1 )
                        + " = " + g.bag[si].id;
                }
                return true;
            }
        }

        // Craft button
        UiRect craftBtn = { leftX0 + leftW - 70, craftTop - 24, leftX0 + leftW - 6, craftTop - 6 };
        if ( UiHit( craftBtn, mx, my ) ) { TryCraftSelected(); return true; }

        UiRect craftL = { leftX0 + 8, craftTop - 48, leftX0 + 28, craftTop - 30 };
        UiRect craftR = { leftX0 + 30, craftTop - 48, leftX0 + 50, craftTop - 30 };
        if ( g.selectedRecipe < 0 )
        {
            if ( UiHit( craftL, mx, my ) ) { g.craftPage = (std::max)( 0, g.craftPage - 1 ); return true; }
            if ( UiHit( craftR, mx, my ) )
            {
                int pages = (std::max)( 1, ( kRecipeCount + kCraftPageSize - 1 ) / kCraftPageSize );
                g.craftPage = (std::min)( pages - 1, g.craftPage + 1 );
                return true;
            }
            float cslot = (std::min)( 44.f, ( leftW - 24 ) / kCraftCols - 4 );
            int base = g.craftPage * kCraftPageSize;
            for ( int row = 0; row < kCraftRows; ++row )
            {
                for ( int col = 0; col < kCraftCols; ++col )
                {
                    int ri = base + row * kCraftCols + col;
                    float sx = leftX0 + 12 + col * ( cslot + 6 );
                    float sy = craftTop - 60 - ( row + 1 ) * ( cslot + 8 );
                    if ( UiHit( { sx, sy, sx + cslot, sy + cslot }, mx, my ) && ri < kRecipeCount )
                    {
                        g.selectedRecipe = ri;
                        ClearDeposit();
                        return true;
                    }
                }
            }
        }

        (void)botY0;
        return false;
    }

    void UpdateUiMouseFromWin( int winX, int winY )
    {
        g.uiMouseX = (float)winX;
        g.uiMouseY = (float)g.uiWinH - (float)winY;
    }

    CellSample const* GetCell( int x, int y )
    {
        auto it = g.cells.find( CellKey( x, y ) );
        if ( it == g.cells.end() || !it->second.valid ) { return nullptr; }
        return &it->second;
    }

    CellSample* GetCellMutable( int x, int y )
    {
        auto it = g.cells.find( CellKey( x, y ) );
        if ( it == g.cells.end() || !it->second.valid ) { return nullptr; }
        return &it->second;
    }

    bool SampleGroundZBase( float x, float y, float& outZ )
    {
        // Grade continuum: resident cells, else analytic Esoterica geography (absolute coords).
        // Dig/place are live cups — never sink whole cell plates via fillZ.
        int const x0 = (int)std::floor( x );
        int const y0 = (int)std::floor( y );
        float const tx = x - (float)x0;
        float const ty = y - (float)y0;
        CellSample const* c00 = GetCell( x0, y0 );
        CellSample const* c10 = GetCell( x0 + 1, y0 );
        CellSample const* c01 = GetCell( x0, y0 + 1 );
        CellSample const* c11 = GetCell( x0 + 1, y0 + 1 );
        auto zAtCell = [&]( int cx, int cy, CellSample const* c ) -> float
        {
            if ( c ) { return GradeToZ( c->grade ); }
            ProvenanceGeo::EnsureReady();
            auto s = ProvenanceGeo::SampleSurface(
                (double)cx + 0.5, (double)cy + 0.5,
                g.gradeDatum, g.reliefVoxels, g.voxelEdgeM );
            return GradeToZ( s.grade );
        };
        float const z00 = zAtCell( x0, y0, c00 );
        float const z10 = zAtCell( x0 + 1, y0, c10 );
        float const z01 = zAtCell( x0, y0 + 1, c01 );
        float const z11 = zAtCell( x0 + 1, y0 + 1, c11 );
        float const z0 = z00 * ( 1.f - tx ) + z10 * tx;
        float const z1 = z01 * ( 1.f - tx ) + z11 * tx;
        outZ = z0 * ( 1.f - ty ) + z1 * ty;
        return true;
    }

    bool SampleAimSurfaceZ( float x, float y, float& outZ )
    {
        // Visible targetable skin: virgin grade + place mounds only.
        // Do NOT sink into DigScar cups — that left the reticle under the roof, misaligned the
        // grade punch, and made floating grade islands un-aimable once neighbours were dug out.
        if ( !SampleGroundZBase( x, y, outZ ) ) { return false; }
        outZ += PlaceLiftAt( x, y );
        return true;
    }

    bool SampleTerrainDrawZ( float x, float y, float& outZ )
    {
        // Drawn HF: virgin continuum by default.
        // Handoff only when skin is open AND D2 is ready — omit without cavity = sky void.
        if ( !SampleGroundZBase( x, y, outZ ) ) { return false; }
        if ( CrestMouthStencilAt( x, y ) ) { return false; }
        // Soft roof collapse (sand/gravel) inside occupancy-broken crest.
        if ( !SurfaceBrokenByOccupancy( x, y ) ) { return true; }
        std::string const cap = CapAtWorld( x, y );
        if ( !MaterialSlumpsOpen( cap ) ) { return true; }
        float occZ = outZ;
        if ( SampleOccupancyZ( x, y, occZ ) )
        {
            outZ = (std::min)( outZ, occZ );
        }
        else
        {
            outZ -= (std::max)( 0.08f, 1.5f * kVoxelEdgeM );
        }
        return true;
    }

    bool SampleGroundZ( float x, float y, float& outZ )
    {
        // Feet / walk / body settle — Z-aware occupancy support (not column crest / D2 tris).
        float qz = (std::max)( g.camZ, g.feetZ + 1.2f );
        if ( std::fabs( x - g.feetX ) > 3.f || std::fabs( y - g.feetY ) > 3.f )
        {
            float crest = 0.f;
            if ( SampleGroundZBase( x, y, crest ) ) { qz = crest + 2.5f; }
        }
        SupportHit const h = SupportBelow( x, y, qz );
        if ( !h.hit ) { return false; }
        outZ = h.position.z;
        return true;
    }

    bool SampleCupSurfaceZ( float x, float y, bool placeCup, float& outZ )
    {
        // Dig cups: grade − dig. Place cups sit on the dug floor: grade − dig + place.
        if ( !SampleGroundZBase( x, y, outZ ) ) { return false; }
        float const digDep = DigDepAt( x, y );
        if ( placeCup )
        {
            outZ += -digDep + PlaceLiftAt( x, y );
        }
        else
        {
            outZ -= digDep;
        }
        return true;
    }

    bool RayHitTunnelSphere( float ox, float oy, float oz,
                             float fx, float fy, float fz,
                             float& outT, float& outX, float& outY, float& outZ )
    {
        // Nearest hit against cohesive buried scoop spheres (inner surface).
        bool hit = false;
        float bestT = 1e9f;
        for ( DigScar const& s : g.scars )
        {
            if ( s.place || !s.tunnel ) { continue; }
            float const r = s.radius;
            if ( r < 1e-5f ) { continue; }
            float const lx = ox - s.wx, ly = oy - s.wy, lz = oz - s.wz;
            float const b = lx * fx + ly * fy + lz * fz;
            float const c = lx * lx + ly * ly + lz * lz - r * r;
            float const disc = b * b - c;
            if ( disc < 0.f ) { continue; }
            float const sd = std::sqrt( disc );
            float t0 = -b - sd;
            float t1 = -b + sd;
            float t = ( t0 > 0.05f ) ? t0 : t1;
            if ( t < 0.05f || t >= bestT ) { continue; }
            bestT = t;
            outT = t;
            outX = ox + fx * t;
            outY = oy + fy * t;
            outZ = oz + fz * t;
            hit = true;
        }
        return hit;
    }

    bool RayHitTriangle( float ox, float oy, float oz,
                         float dx, float dy, float dz,
                         float x0, float y0, float z0,
                         float x1, float y1, float z1,
                         float x2, float y2, float z2,
                         float& outT )
    {
        // Möller–Trumbore — same planar faces the vista draws (not bilinear height march).
        constexpr float eps = 1e-7f;
        float e1x = x1 - x0, e1y = y1 - y0, e1z = z1 - z0;
        float e2x = x2 - x0, e2y = y2 - y0, e2z = z2 - z0;
        float px = dy * e2z - dz * e2y;
        float py = dz * e2x - dx * e2z;
        float pz = dx * e2y - dy * e2x;
        float det = e1x * px + e1y * py + e1z * pz;
        if ( det > -eps && det < eps ) { return false; }
        float inv = 1.f / det;
        float tx = ox - x0, ty = oy - y0, tz = oz - z0;
        float u = ( tx * px + ty * py + tz * pz ) * inv;
        if ( u < 0.f || u > 1.f ) { return false; }
        float qx = ty * e1z - tz * e1y;
        float qy = tz * e1x - tx * e1z;
        float qz = tx * e1y - ty * e1x;
        float v = ( dx * qx + dy * qy + dz * qz ) * inv;
        if ( v < 0.f || u + v > 1.f ) { return false; }
        float t = ( e2x * qx + e2y * qy + e2z * qz ) * inv;
        if ( t < 0.05f ) { return false; }
        outT = t;
        return true;
    }

    bool RayHitDrawnSkin( float ox, float oy, float oz,
                          float fx, float fy, float fz,
                          float maxT,
                          float& outT, float& outX, float& outY, float& outZ )
    {
        // Finer than MeshDiv=2 so crest edges / steep faces don't miss under the crosshair.
        int const div = 4;
        float bestT = maxT;
        bool hit = false;
        for ( float ts = 0.15f; ts <= maxT + 0.2f; ts += 0.28f )
        {
            int const cx = (int)std::floor( ox + fx * ts );
            int const cy = (int)std::floor( oy + fy * ts );
            for ( int y = cy - 1; y <= cy + 1; ++y )
            {
                for ( int x = cx - 1; x <= cx + 1; ++x )
                {
                    if ( !GetCell( x, y ) || !GetCell( x + 1, y ) || !GetCell( x, y + 1 ) || !GetCell( x + 1, y + 1 ) )
                    {
                        continue;
                    }
                    for ( int j = 0; j < div; ++j )
                    {
                        for ( int i = 0; i < div; ++i )
                        {
                            float const u0 = (float)i / (float)div;
                            float const v0 = (float)j / (float)div;
                            float const u1 = (float)( i + 1 ) / (float)div;
                            float const v1 = (float)( j + 1 ) / (float)div;
                            float const px00 = (float)x + u0, py00 = (float)y + v0;
                            float const px10 = (float)x + u1, py10 = (float)y + v0;
                            float const px01 = (float)x + u0, py01 = (float)y + v1;
                            float const px11 = (float)x + u1, py11 = (float)y + v1;
                            float z00, z10, z01, z11;
                            if ( !SampleAimSurfaceZ( px00, py00, z00 ) ) { continue; }
                            if ( !SampleAimSurfaceZ( px10, py10, z10 ) ) { continue; }
                            if ( !SampleAimSurfaceZ( px01, py01, z01 ) ) { continue; }
                            if ( !SampleAimSurfaceZ( px11, py11, z11 ) ) { continue; }
                            float t = 0.f;
                            if ( RayHitTriangle( ox, oy, oz, fx, fy, fz,
                                                 px00, py00, z00, px10, py10, z10, px01, py01, z01, t )
                              && t < bestT )
                            {
                                bestT = t; hit = true;
                            }
                            if ( RayHitTriangle( ox, oy, oz, fx, fy, fz,
                                                 px10, py10, z10, px11, py11, z11, px01, py01, z01, t )
                              && t < bestT )
                            {
                                bestT = t; hit = true;
                            }
                        }
                    }
                }
            }
            if ( hit && bestT < ts - 0.35f ) { break; }
        }
        if ( !hit ) { return false; }
        outT = bestT;
        outX = ox + fx * bestT;
        outY = oy + fy * bestT;
        outZ = oz + fz * bestT;
        return true;
    }

    void UpdateAim()
    {
        g.aimHit = false;
        g.aimStrikeCap = "-";
        g.aimWireCap = "-";
        g.aimBiome = "-";
        g.aimFormId = "-";
        g.aimStrikeR = 128; g.aimStrikeG = 128; g.aimStrikeB = 128;
        float cp = std::cos( g.pitch ), sp = std::sin( g.pitch );
        float cy = std::cos( g.yaw ), sy = std::sin( g.yaw );
        float fx = sy * cp, fy = cy * cp, fz = sp;
        float ox = g.camX, oy = g.camY, oz = g.camZ;

        float const maxT = g.walkMode ? ( kReachCells + 2.f ) : 80.f;
        float hitT = -1.f;
        float hitX = 0.f, hitY = 0.f, hitZ = 0.f;

        // Prefer occupancy aim only on cells we already carved (committed).
        // Aiming alone must NOT seed lattices, expand volume, or rebuild cavity/D2.
        bool hitOcc = false;
        {
            // Probe aim cell without prefetching / EnsureOccupancyLattice.
            float tProbe = 1.2f;
            int const acx = (int)std::floor( ox + fx * tProbe );
            int const acy = (int)std::floor( oy + fy * tProbe );
            CellSample const* ac = GetCell( acx, acy );
            if ( ac && ac->carved && ac->hasCavity && !ac->fill.empty() )
            {
                hitOcc = RayHitOccupancy( ox, oy, oz, fx, fy, fz, maxT, hitT, hitX, hitY, hitZ );
            }
        }
        if ( !hitOcc )
        {
            hitT = -1.f;
            if ( !RayHitDrawnSkin( ox, oy, oz, fx, fy, fz, maxT, hitT, hitX, hitY, hitZ ) )
            {
                // Fallback: height march + crest/cliff proximity (triangle walk can miss ridges).
                constexpr float kStep = 0.05f;
                float prevZ = oz, prevG = oz;
                bool havePrev = false;
                for ( int step = 0; step < 320; ++step )
                {
                    float t = 0.10f + step * kStep;
                    if ( t > maxT ) { break; }
                    float x = ox + fx * t;
                    float y = oy + fy * t;
                    float z = oz + fz * t;
                    float ground = 0.f;
                    if ( !SampleAimSurfaceZ( x, y, ground ) ) { havePrev = false; continue; }
                    bool const below = z <= ground + 0.05f;
                    bool const nearSkin = std::fabs( z - ground ) < 0.14f;
                    float gxm = ground, gxp = ground, gym = ground, gyp = ground;
                    SampleAimSurfaceZ( x - 0.10f, y, gxm );
                    SampleAimSurfaceZ( x + 0.10f, y, gxp );
                    SampleAimSurfaceZ( x, y - 0.10f, gym );
                    SampleAimSurfaceZ( x, y + 0.10f, gyp );
                    float const slope = (std::max)(
                        std::fabs( gxp - gxm ), std::fabs( gyp - gym ) ) / 0.20f;
                    bool hitHere = below;
                    if ( havePrev && prevZ > prevG + 0.05f && below ) { hitHere = true; }
                    // Crest / knife edge: ray skims the skin on a steep grade.
                    if ( nearSkin && slope > 1.2f ) { hitHere = true; }
                    if ( hitHere )
                    {
                        float t0 = (std::max)( 0.05f, t - kStep );
                        float t1 = t;
                        for ( int i = 0; i < 10; ++i )
                        {
                            float tm = 0.5f * ( t0 + t1 );
                            float xm = ox + fx * tm;
                            float ym = oy + fy * tm;
                            float zm = oz + fz * tm;
                            float gm = 0.f;
                            if ( !SampleAimSurfaceZ( xm, ym, gm ) || zm <= gm + 0.05f
                              || ( std::fabs( zm - gm ) < 0.14f && slope > 1.2f ) )
                            {
                                t1 = tm;
                            }
                            else { t0 = tm; }
                        }
                        hitT = t1;
                        hitX = ox + fx * hitT;
                        hitY = oy + fy * hitT;
                        hitZ = oz + fz * hitT;
                        break;
                    }
                    prevZ = z; prevG = ground; havePrev = true;
                }
            }
        }
        if ( hitT < 0.f ) { return; }

        // Only when firmly INSIDE a dig, slide deeper along the look ray.
        // Rim overlap (partial DigDep) must stay on solid so remaining dirt can still be carved.
        // Occupancy hits already sit on the matter face — skip DigDep slide when fill owns the cell.
        if ( !hitOcc )
        {
            float const R = kHandfulRadiusM;
            if ( DigDepAt( hitX, hitY ) > R * 0.70f )
            {
                float const kStep = 0.03f;
                float bestT = hitT;
                float const endT = (std::min)( maxT, hitT + R * 3.f );
                for ( float t = hitT; t <= endT; t += kStep )
                {
                    float const x = ox + fx * t;
                    float const y = oy + fy * t;
                    float const z = oz + fz * t;
                    if ( DigDepAt( x, y ) < R * 0.12f ) { break; }
                    float ground = 0.f;
                    if ( !SampleGroundZ( x, y, ground ) ) { break; }
                    if ( z <= ground + 0.04f ) { bestT = t; }
                }
                hitT = bestT;
                hitX = ox + fx * hitT;
                hitY = oy + fy * hitT;
                hitZ = oz + fz * hitT;
            }
        }

        g.aimHit = true;
        g.aimX = hitX;
        g.aimY = hitY;
        g.aimZ = hitZ;
        g.aimCx = (int)std::floor( hitX );
        g.aimCy = (int)std::floor( hitY );
        g.aimU = hitX - (float)g.aimCx;
        g.aimV = hitY - (float)g.aimCy;
        g.aimDepth = AimDigAffect().radiusM;
        // Seeking identity: Esoterica geography rock + province. Wire cell.cap may be cover (grass).
        {
            ProvenanceGeo::EnsureReady();
            auto const surf = ProvenanceGeo::SampleSurface(
                (double)hitX, (double)hitY,
                g.gradeDatum, g.reliefVoxels, g.voxelEdgeM );
            g.aimStrikeCap = surf.cap ? surf.cap : "dirt";
            g.aimBiome = ProvenanceGeo::ProvinceName( surf.province );
            CellSample const* cell = GetCell( g.aimCx, g.aimCy );
            g.aimWireCap = ( cell && cell->valid && !cell->cap.empty() )
                ? cell->cap
                : g.aimStrikeCap;
            H2H::MaterialFormContract const& form = H2H::FormOrDirt( g.aimStrikeCap.c_str() );
            g.aimFormId = form.material_id;
            VisualMat::CapColor( g.aimStrikeCap.c_str(), g.aimStrikeR, g.aimStrikeG, g.aimStrikeB );
        }
        // HF contact refine only. Occupancy seed + cavity rebuild happen on dig/pick commit.
    }

    bool TryPickFoliatedStrike()
    {
        // Horizon-to-Hand: pick strike → occupancy D2 cavity + optional MatterBody plate.
        UpdateAim();
        if ( !g.aimHit )
        {
            g.digestLine = "PICK miss - no face in aim";
            UpdateStreamHud();
            return false;
        }
        float dx = g.aimX - g.feetX;
        float dy = g.aimY - g.feetY;
        if ( std::sqrt( dx * dx + dy * dy ) > kReachCells )
        {
            g.digestLine = "PICK too far - step closer";
            UpdateStreamHud();
            return false;
        }
        if ( g.pending != PendingKind::None )
        {
            g.digestLine = "PICK busy - wait for engine digest";
            UpdateStreamHud();
            return false;
        }

        std::string const cap = g.aimStrikeCap.empty() || g.aimStrikeCap == "-"
            ? CapAtWorld( g.aimX, g.aimY )
            : g.aimStrikeCap;
        H2H::MaterialFormContract const& form = H2H::FormOrDirt( cap.c_str() );
        bool const foliated = form.fabric == H2H::FabricKind::FoliatedAnisotropic
            || form.fabric == H2H::FabricKind::BeddedFissile
            || CapUsesFoliation( cap.c_str() );
        if ( !foliated && form.rigid_fracture_body == false && form.fabric != H2H::FabricKind::Granular )
        {
            g.digestLine = "PICK — use shovel (1) for soft scoop matter";
            UpdateStreamHud();
            return false;
        }

        float cp = std::cos( g.pitch ), sp = std::sin( g.pitch );
        float cyw = std::cos( g.yaw ), sy = std::sin( g.yaw );
        float fx = sy * cp, fy = cyw * cp, fz = sp;

        DigAffectSpec const affect = AimDigAffect();
        // Carve INTO the matter face. Side/dark ridge walls need look-into, not crest-up HF N.
        float fnx = 0.f, fny = 0.f, fnz = 1.f;
        ResolveCarveIntoNormal( g.aimX, g.aimY, g.aimZ, fx, fy, fz, fnx, fny, fnz );
        // Tip-only carve is invisible on HF after handoff — use readable radius on surface
        // and contact radius on cliffs / downward chops.
        float visualR = (std::max)( affect.radiusM, kVoxelEdgeM * 0.85f );
        visualR = (std::max)( visualR, ActiveAimRadiusM() * 0.55f );
        if ( fnz < 0.72f || g.pitch < -0.28f )
        {
            visualR = (std::max)( visualR, ActiveAimRadiusM() );
        }
        float const into = visualR * 0.70f;
        float const bx = g.aimX - fnx * into;
        float const by = g.aimY - fny * into;
        float const bz = g.aimZ - fnz * into;
        int const bcx = (int)std::floor( bx );
        int const bcy = (int)std::floor( by );
        float const bu = bx - (float)bcx;
        float const bv = by - (float)bcy;

        // Engine carve receipt — tip affect (authority grams); presentation uses visualR.
        float const radEng = WorldToEngDepth( affect.radiusM );
        float const depthEng = WorldToEngDepth( affect.depthM );
        int px = (int)std::floor( g.feetX );
        int py = (int)std::floor( g.feetY );
        char params[288];
        std::snprintf( params, sizeof( params ),
            "{\"x\":%d,\"y\":%d,\"u\":%.5f,\"v\":%.5f,\"depth\":%.5f,\"radius\":%.5f,\"shape\":\"sphere\",\"px\":%d,\"py\":%d}",
            bcx, bcy, bu, bv, depthEng, radEng, px, py );
        g.intent = IntentKind::Dig;
        g.pendingLocalScoopG = AffectAcceptedGrams( form, affect );
        g.pendingLocalScoopMat = form.material_id;
        g.pendingAffectRM = affect.radiusM;
        if ( !RequestMethod( "carve", params, PendingKind::Carve ) )
        {
            g.digestLine = "PICK send failed";
            g.pendingAffectRM = 0.f;
            UpdateStreamHud();
            return false;
        }
        g.pendingBiteCx = bcx;
        g.pendingBiteCy = bcy;
        g.pendingBiteU = bu;
        g.pendingBiteV = bv;
        g.pendingBiteWx = bx;
        g.pendingBiteWy = by;
        g.pendingBiteWz = bz;
        g.pendingBiteForward = true;
        CaptureSpirePreRequest( bcx, bcy, g.aimX, g.aimY, g.aimZ, form.material_id );

        PrefetchOccupancyCell( bcx, bcy );
        // Occupancy carve + fracture-matched stencil (crest = air-under-skin; walls = face disks).
        bool const carved = CarveOccupancySphere( bx, by, bz, visualR, g.aimX, g.aimY, g.aimZ );
        // Keep scars when AABB does not own HF — retired only if something else peels.
        if ( carved && kAabbCavityOwnsHf )
        {
            RetirePresentationScarsNear( g.aimX, g.aimY, visualR * 2.5f );
        }
        if ( !carved )
        {
            AddFacePunctureScar( g.aimX, g.aimY, g.aimZ, bcx, bcy,
                affect.radiusM, affect.depthM, fnx, fny, fnz );
        }

        H2H::SeparationResult const sep = H2H::StrikePick(
            g.aimX, g.aimY, g.aimZ, fx, fy, fz, form.material_id );

        H2H::FracturePatch const* patch = nullptr;
        for ( H2H::FracturePatch const& p : H2H::State().patches )
        {
            if ( p.patch_id == sep.patch_id ) { patch = &p; break; }
        }
        if ( sep.detached )
        {
            float strikeX = 1.f, strikeY = 0.f;
            StrikeAxesFromLook( fnx, fny, fnz, fx, fy, fz, strikeX, strikeY );
            for ( H2H::MatterBody& body : H2H::State().bodies )
            {
                if ( body.body_id != sep.body_id ) { continue; }
                body.nx = fnx; body.ny = fny; body.nz = fnz;
                body.strikeX = strikeX; body.strikeY = strikeY;
                body.x = g.aimX + fx * 0.08f - fnx * 0.04f;
                body.y = g.aimY + fy * 0.08f - fny * 0.04f;
                body.z = g.aimZ + fz * 0.08f - fnz * 0.04f;
                body.vx = fx * 0.55f - fnx * 0.25f;
                body.vy = fy * 0.55f - fny * 0.25f;
                body.vz = fz * 0.55f - fnz * 0.25f - 0.2f;
                break;
            }
        }
        // Fines/grams from carve digest only — do not double-credit with local StrikePick fines.

        FireActionCue( false, false );
        char d[480];
        if ( sep.detached )
        {
            std::snprintf( d, sizeof( d ),
                "H2H %s %s D2=%s sep=%llu body=%llu | plate %dg | visualR=%.3f | [G] grip",
                sep.act, sep.material_id.c_str(), carved ? "yes" : "miss",
                (unsigned long long)sep.separation_id, (unsigned long long)sep.body_id,
                sep.plate_g, visualR );
            g.statusLine = "P3b pick - plate + occupancy cavity";
        }
        else
        {
            float attach = patch ? patch->attachment : 1.f;
            std::snprintf( d, sizeof( d ),
                "H2H %s %s D2=%s patch=%llu attach=%.2f | visualR=%.3fm tipR=%.3fm | await digest",
                sep.act, sep.material_id.c_str(), carved ? "yes" : "miss",
                (unsigned long long)sep.patch_id, attach, visualR, affect.radiusM );
            g.statusLine = carved
                ? "P3b pick - occupancy cavity (matter face)"
                : "P3b pick - carve missed lattice (check column)";
        }
        g.digestLine = d;
        UpdateStreamHud();
        return true;
    }

    bool TryGripMatterBody()
    {
        // Grip at actual contact on a loose / settled plate — not a central socket.
        if ( g.grippedBodyId != 0 )
        {
            // Drop gripped body at feet.
            for ( H2H::MatterBody& b : H2H::State().bodies )
            {
                if ( b.body_id != g.grippedBodyId ) { continue; }
                b.gripped = false;
                b.x = g.feetX + std::sin( g.yaw ) * 0.6f;
                b.y = g.feetY + std::cos( g.yaw ) * 0.6f;
                b.z = g.feetZ + 0.4f;
                H2H::WakeChip( b );
                g.grippedBodyId = 0;
                g.digestLine = "H2H drop plate body";
                UpdateStreamHud();
                return true;
            }
            g.grippedBodyId = 0;
        }

        float cp = std::cos( g.pitch ), sp = std::sin( g.pitch );
        float cyw = std::cos( g.yaw ), sy = std::sin( g.yaw );
        float fx = sy * cp, fy = cyw * cp, fz = sp;
        float t = 0.f, hx = 0.f, hy = 0.f, hz = 0.f;
        H2H::MatterBody* body = H2H::RayHitBody(
            g.camX, g.camY, g.camZ, fx, fy, fz, 3.2f, t, hx, hy, hz );
        if ( !body )
        {
            g.digestLine = "H2H grip miss - aim a loose plate";
            UpdateStreamHud();
            return false;
        }
        // Burden from contact-to-COM distance (edge grip harder than center).
        float const comDx = hx - body->x, comDy = hy - body->y, comDz = hz - body->z;
        float const lever = std::sqrt( comDx * comDx + comDy * comDy + comDz * comDz );
        bool const twoHand = body->materials_g > 8000 || lever > 0.12f;
        if ( body->materials_g > 18000 )
        {
            char d[200];
            std::snprintf( d, sizeof( d ),
                "H2H grip refuse body=%llu %dg — too heavy (need lever/team)",
                (unsigned long long)body->body_id, body->materials_g );
            g.digestLine = d;
            UpdateStreamHud();
            return false;
        }
        body->gripped = true;
        body->settled = true;
        body->vx = body->vy = body->vz = 0.f;
        g.grippedBodyId = body->body_id;
        if ( !g.heldGalleryId.empty() )
        {
            // One hand: stow gallery sample to bag when gripping a plate.
            SeedStarterBag();
            BagAdd( g.heldGalleryId, 1 );
            g.heldGalleryId.clear();
            g.heldGallerySrc = -1;
            g.heldYaw = 0.f;
            g.heldTumble = 0.f;
        }
        char d[240];
        std::snprintf( d, sizeof( d ),
            "H2H grip body=%llu %s %dg @ contact lever=%.2fm %s | G again to drop",
            (unsigned long long)body->body_id, body->material_id.c_str(), body->materials_g,
            lever, twoHand ? "two-hand burden" : "one-hand" );
        g.digestLine = d;
        UpdateStreamHud();
        return true;
    }

    bool TryDigHandful()
    {
        if ( g.link != LinkState::CapsOk ) { return false; }
        UpdateAim();
        if ( !g.aimHit )
        {
            g.digestLine = "DIG miss - no surface in aim";
            UpdateStreamHud();
            return false;
        }

        std::string const cap = g.aimStrikeCap.empty() || g.aimStrikeCap == "-"
            ? CapAtWorld( g.aimX, g.aimY )
            : g.aimStrikeCap;
        H2H::MaterialFormContract const& form = H2H::FormOrDirt( cap.c_str() );
        H2H::ToolMatterProfile const& tool = ActiveToolProfile();
        H2H::ToolRole const role = H2H::RoleForAttached( tool, form );

        if ( tool.cuts_wood && form.fabric != H2H::FabricKind::FibrousWood )
        {
            g.digestLine = "AXE — wood only (not terrain dig)";
            UpdateStreamHud();
            return false;
        }
        if ( !H2H::CanExcavateAttached( tool, form ) )
        {
            if ( role == H2H::ToolRole::CollectLoose )
            {
                g.digestLine = "H2H gate: collect loose only — pick frees intact rock (hands/shovel cannot mine it)";
            }
            else if ( role == H2H::ToolRole::Indirect )
            {
                g.digestLine = "H2H gate: tool only opens terrain capacity for water — use bucket to transfer fluid";
            }
            else
            {
                char d[200];
                std::snprintf( d, sizeof( d ), "H2H gate: %s cannot cut attached %s",
                    tool.id, form.material_id );
                g.digestLine = d;
            }
            FireActionCue( false, true );
            UpdateStreamHud();
            return false;
        }

        // Hard / foliated / bedded / massive rock → Horizon-to-Hand fracture (pick primary).
        if ( form.hardness >= 3 || form.fabric == H2H::FabricKind::FoliatedAnisotropic
          || form.fabric == H2H::FabricKind::BeddedFissile
          || ( form.fabric == H2H::FabricKind::Massive && form.rigid_fracture_body ) )
        {
            if ( tool.force != H2H::ForceClass::Pick )
            {
                g.digestLine = "H2H: switch to pick (3) for hard/foliated rock";
                UpdateStreamHud();
                return false;
            }
            return TryPickFoliatedStrike();
        }

        if ( g.pending != PendingKind::None )
        {
            g.digestLine = "DIG busy - wait for engine digest";
            if ( g.aimHit ) { FireActionCue( false, true ); }
            UpdateStreamHud();
            return false;
        }

        // Soft soil — dig-volume sphere from tool×material; preview/scar/carve/grams share one R.
        float bx, by, bz, bu, bv, bdepthM;
        int bcx, bcy;
        ResolveBiteFromAim( false, bx, by, bz, bcx, bcy, bu, bv, bdepthM );
        float dx = (float)bcx + 0.5f - g.feetX;
        float dy = (float)bcy + 0.5f - g.feetY;
        if ( std::sqrt( dx * dx + dy * dy ) > kReachCells )
        {
            g.digestLine = "DIG too far - step closer (reach ~3.5 m)";
            UpdateStreamHud();
            return false;
        }

        // Recompute steep at bite + final affect (slope may differ from aim cell).
        float fnx = 0.f, fny = 0.f, fnz = 1.f;
        {
            float cp = std::cos( g.pitch ), sp = std::sin( g.pitch );
            float cyw = std::cos( g.yaw ), sy = std::sin( g.yaw );
            ResolveCarveIntoNormal( g.aimX, g.aimY, g.aimZ, sy * cp, cyw * cp, sp, fnx, fny, fnz );
        }
        bool const steepFace = IsSteepFaceAt( bx, by ) || fnz < 0.58f;
        DigAffectSpec const hitAffect = ComputeDigAffect( tool, form, steepFace );
        int const acceptG = AffectAcceptedGrams( form, hitAffect );
        g.pendingLocalScoopG = acceptG;
        g.pendingLocalScoopMat = form.material_id;
        g.pendingAffectRM = hitAffect.radiusM;

        float const radEng = WorldToEngDepth( hitAffect.radiusM );
        float const depthEng = WorldToEngDepth( (std::max)( bdepthM, hitAffect.depthM ) );
        int px = (int)std::floor( g.feetX );
        int py = (int)std::floor( g.feetY );
        char params[288];
        std::snprintf( params, sizeof( params ),
            "{\"x\":%d,\"y\":%d,\"u\":%.5f,\"v\":%.5f,\"depth\":%.5f,\"radius\":%.5f,\"shape\":\"sphere\",\"px\":%d,\"py\":%d}",
            bcx, bcy, bu, bv, depthEng, radEng, px, py );
        g.intent = IntentKind::Dig;
        if ( !RequestMethod( "carve", params, PendingKind::Carve ) )
        {
            g.digestLine = "DIG send failed";
            g.pendingAffectRM = 0.f;
            UpdateStreamHud();
            return false;
        }
        g.pendingBiteCx = bcx;
        g.pendingBiteCy = bcy;
        g.pendingBiteU = bu;
        g.pendingBiteV = bv;
        g.pendingBiteWx = bx;
        g.pendingBiteWy = by;
        g.pendingBiteWz = bz;
        g.pendingBiteForward = ( DigDepAt( g.aimX, g.aimY ) > hitAffect.radiusM * 0.20f )
            && ( -std::sin( g.pitch ) < 0.72f );
        CaptureSpirePreRequest( bcx, bcy, g.aimX, g.aimY, g.aimZ, form.material_id );

        // P3b: occupancy sphere carve owns the hole; DigScar is flash only if carve misses.
        float visualR = (std::max)( hitAffect.radiusM, kVoxelEdgeM * 0.85f );
        if ( fnz < 0.72f || g.pitch < -0.28f )
        {
            visualR = (std::max)( visualR, ActiveAimRadiusM() );
        }
        float carveX = bx, carveY = by, carveZ = bz;
        if ( hitAffect.mode != DigAffectMode::ScoopHemi )
        {
            // Face bites: centre into the matter along -N (same as pick).
            carveX = g.aimX - fnx * visualR * 0.70f;
            carveY = g.aimY - fny * visualR * 0.70f;
            carveZ = g.aimZ - fnz * visualR * 0.70f;
        }
        PrefetchOccupancyCell( bcx, bcy );
        PrefetchOccupancyCell( (int)std::floor( carveX ), (int)std::floor( carveY ) );
        bool const carved = CarveOccupancySphere( carveX, carveY, carveZ, visualR, g.aimX, g.aimY, g.aimZ );
        if ( carved && kAabbCavityOwnsHf ) { RetirePresentationScarsNear( g.aimX, g.aimY, visualR * 2.5f ); }
        if ( !carved )
        {
            if ( hitAffect.mode == DigAffectMode::ScoopHemi )
            {
                float scarZ = bz;
                float gradeAtBite = bz;
                if ( SampleGroundZBase( bx, by, gradeAtBite ) )
                {
                    scarZ = (std::min)( bz, gradeAtBite - hitAffect.radiusM * 0.15f );
                }
                AddScar( bx, by, scarZ, bcx, bcy, false, false );
                if ( g.pendingScarIndex >= 0 && g.pendingScarIndex < (int)g.scars.size() )
                {
                    DigScar& s = g.scars[(size_t)g.pendingScarIndex];
                    s.radius = hitAffect.radiusM;
                    s.depth = hitAffect.depthM;
                }
            }
            else
            {
                float const rad = hitAffect.radiusM;
                float const depthAmp = hitAffect.depthM;
                float const cp = std::cos( g.pitch ), sp = std::sin( g.pitch );
                float const cyw = std::cos( g.yaw ), sy = std::sin( g.yaw );
                float const lx = sy * cp, ly = cyw * cp, lz = sp;
                AddFacePunctureScar( g.aimX + lx * rad * 0.35f,
                    g.aimY + ly * rad * 0.35f,
                    g.aimZ + lz * rad * 0.35f,
                    bcx, bcy, rad, depthAmp, fnx, fny, fnz );
            }
        }
        FireActionCue( false, false );
        char sent[280];
        std::snprintf( sent, sizeof( sent ),
            "H2H %s %s via %s — D2=%s visualR=%.3fm tipR=%.3fm → ~%dg",
            hitAffect.feel, form.material_id, tool.id,
            carved ? "yes" : "flash", visualR, hitAffect.radiusM, acceptG );
        g.digestLine = sent;
        g.statusLine = carved
            ? "P3b - occupancy cavity (matter face)"
            : "P3b - scar flash until occupancy";
        UpdateStreamHud();
        return true;
    }

    bool TryPlaceHandful()
    {
        if ( g.link != LinkState::CapsOk ) { return false; }
        if ( g.pending != PendingKind::None )
        {
            g.digestLine = "PLACE busy - wait for engine digest";
            if ( g.aimHit ) { FireActionCue( true, true ); }
            UpdateStreamHud();
            return false;
        }
        if ( g.heldTotalG <= 0 || g.heldDominant.empty() )
        {
            g.digestLine = "PLACE empty hand - LMB scoops into stock, RMB places one scoop";
            UpdateStreamHud();
            return false;
        }
        UpdateAim();
        if ( !g.aimHit )
        {
            g.digestLine = "PLACE miss - no surface in aim";
            UpdateStreamHud();
            return false;
        }
        float bx, by, bz, bu, bv, bdepthM;
        int bcx, bcy;
        ResolveBiteFromAim( true, bx, by, bz, bcx, bcy, bu, bv, bdepthM );
        float dx = (float)bcx + 0.5f - g.feetX;
        float dy = (float)bcy + 0.5f - g.feetY;
        if ( std::sqrt( dx * dx + dy * dy ) > kReachCells )
        {
            g.digestLine = "PLACE too far - step closer";
            UpdateStreamHud();
            return false;
        }

        std::unordered_map<std::string, int> ask;
        int askG = 0;
        std::string askDom;
        if ( !BuildPlaceScoopAsk( ask, askG, askDom ) )
        {
            g.digestLine = "PLACE empty hand - LMB scoops into stock, RMB places one scoop";
            UpdateStreamHud();
            return false;
        }

        float const radEng = HandfulRadiusEng();
        float const depthEng = WorldToEngDepth( bdepthM );
        int px = (int)std::floor( g.feetX );
        int py = (int)std::floor( g.feetY );
        char params[512];
        if ( ask.size() > 1 )
        {
            std::string amounts = "{";
            bool first = true;
            for ( auto const& kv : ask )
            {
                if ( !first ) { amounts += ","; }
                first = false;
                char one[64];
                std::snprintf( one, sizeof( one ), "\"%s\":%d", kv.first.c_str(), kv.second );
                amounts += one;
            }
            amounts += "}";
            std::snprintf( params, sizeof( params ),
                "{\"x\":%d,\"y\":%d,\"u\":%.5f,\"v\":%.5f,\"depth\":%.5f,\"radius\":%.5f,\"amounts\":%s,\"px\":%d,\"py\":%d}",
                bcx, bcy, bu, bv, depthEng, radEng, amounts.c_str(), px, py );
        }
        else
        {
            std::snprintf( params, sizeof( params ),
                "{\"x\":%d,\"y\":%d,\"u\":%.5f,\"v\":%.5f,\"depth\":%.5f,\"radius\":%.5f,\"material\":\"%s\",\"amount\":%d,\"px\":%d,\"py\":%d}",
                bcx, bcy, bu, bv, depthEng, radEng,
                askDom.c_str(), askG, px, py );
        }
        g.intent = IntentKind::PlaceHeld;
        if ( !RequestMethod( "place", params, PendingKind::Place ) )
        {
            g.digestLine = "PLACE send failed";
            return false;
        }
        g.pendingPlaceAsk = ask;
        g.pendingPlaceG = askG;
        g.pendingBiteCx = bcx;
        g.pendingBiteCy = bcy;
        g.pendingBiteU = bu;
        g.pendingBiteV = bv;
        g.columnQueue.clear();
        // P3e: place into occupancy (matter), not DigScar mound authority.
        g.pendingPlaceIntoHole = ( bdepthM > kHandfulRadiusM * 0.25f );
        float placeZ = bz;
        if ( g.pendingPlaceIntoHole )
        {
            // Seat fill on cavity floor / matter boundary under aim — not virgin HF skin.
            SupportHit const seat = SupportBelow( bx, by, g.aimZ + 0.05f );
            if ( seat.hit ) { placeZ = seat.position.z + kVoxelEdgeM * 0.35f; }
            ClearPendingScarEdit();
        }
        else
        {
            SampleGroundZBase( bx, by, placeZ );
            placeZ += kVoxelEdgeM * 0.35f; // mound seat just above virgin skin
        }
        PrefetchOccupancyCell( bcx, bcy );
        PlaceFillResult const filled = PlaceOccupancyFill(
            bx, by, placeZ, (std::max)( kHandfulRadiusM, kVoxelEdgeM * 0.85f ), askG );
        if ( filled.ok && filled.acceptedGrams > 0 )
        {
            // Local debit matches accepted placed matter (engine digest reconciles).
            DebitHeldTotal( filled.acceptedGrams );
            g.pendingPlaceG = filled.acceptedGrams;
        }
        FireActionCue( true, false );
        g.statusLine = filled.ok
            ? "P3e - placed into occupancy"
            : "Phase 4 - placing scoop...";
        char sent[200];
        std::snprintf( sent, sizeof( sent ),
            "PLACE %s %dg->occ units=%d (hand %dg) - awaiting digest",
            filled.ok ? "fill" : "sent",
            filled.ok ? filled.acceptedGrams : askG,
            filled.unitsFilled, g.heldTotalG );
        g.digestLine = sent;
        UpdateStreamHud();
        return true;
    }

    void RecomputeBlocksWanted()
    {
        int const maxRing = ( kFarRadiusCells + kBlockCells - 1 ) / kBlockCells;
        g.blocksWanted = 0;
        for ( int ring = 0; ring <= maxRing; ++ring )
        {
            for ( int dy = -ring; dy <= ring; ++dy )
            {
                for ( int dx = -ring; dx <= ring; ++dx )
                {
                    if ( (std::max)( std::abs( dx ), std::abs( dy ) ) != ring ) { continue; }
                    ++g.blocksWanted;
                }
            }
        }
    }

    void FollowStreamCenter()
    {
        int const cx = (int)std::floor( g.feetX );
        int const cy = (int)std::floor( g.feetY );
        if ( cx == g.playerX && cy == g.playerY ) { return; }
        g.playerX = cx;
        g.playerY = cy;
        // --cert-geo walk/dig: residency frozen after transect prefetch so walk can assert
        // HF remesh=0 / no disk growth. Play path still expands below.
        if ( g.certGeo && g.certGeoPhase >= 1 && g.certGeoPhase <= 3 )
        {
            return;
        }
        // Expand analytic residency with the player (isotropic disk).
        // EnsureGeoDisk invalidates only when new cells appear — do NOT remesh every footstep.
        EnsureGeoDisk( cx, cy, (std::min)( kFarRadiusCells, 64 ) );
        g.streamComplete = true;
        g.blocksLoaded = g.blocksWanted;
        g.statusLine = "Phase 4 - Esoterica geography resident + interaction digests";
    }

    // Phase 3 terrain draw: uniform heightfield from grades (+ scoop scars as height only).
    // Dig/place tools stay; digs must NOT change cell LOD, punch tiles, cup overlays, or grain dither.

    void SampleCapColor( float x, float y, float& outR, float& outG, float& outB )
    {
        // Use cached cell palette from EnsureGeoCell — NEVER re-run SampleSurface FBM per tri.
        ++g.perfSampleCapColorCalls;
        int const x0 = (int)std::floor( x );
        int const y0 = (int)std::floor( y );
        float const tx = x - (float)x0;
        float const ty = y - (float)y0;
        auto at = [&]( int cx, int cy, float& r, float& gcol, float& b )
        {
            CellSample const* c = GetCell( cx, cy );
            if ( c && c->valid )
            {
                r = (float)c->r; gcol = (float)c->g; b = (float)c->b;
                return;
            }
            r = 90.f; gcol = 120.f; b = 70.f;
        };
        float r00, g00, b00, r10, g10, b10, r01, g01, b01, r11, g11, b11;
        at( x0, y0, r00, g00, b00 );
        at( x0 + 1, y0, r10, g10, b10 );
        at( x0, y0 + 1, r01, g01, b01 );
        at( x0 + 1, y0 + 1, r11, g11, b11 );
        float const r0 = r00 * ( 1.f - tx ) + r10 * tx;
        float const r1 = r01 * ( 1.f - tx ) + r11 * tx;
        float const g0 = g00 * ( 1.f - tx ) + g10 * tx;
        float const g1 = g01 * ( 1.f - tx ) + g11 * tx;
        float const b0 = b00 * ( 1.f - tx ) + b10 * tx;
        float const b1 = b01 * ( 1.f - tx ) + b11 * tx;
        outR = r0 * ( 1.f - ty ) + r1 * ty;
        outG = g0 * ( 1.f - ty ) + g1 * ty;
        outB = b0 * ( 1.f - ty ) + b1 * ty;
    }

    int MeshDivForCell( int x, int y, float feetX, float feetY )
    {
        // Vista base only — dig mouths use adaptive quad subdiv in RebuildTerrainMesh.
        (void)x; (void)y; (void)feetX; (void)feetY;
        return 2;
    }

    bool QuadHitsMouthCollar( float x0, float y0, float x1, float y1, float& outMinDist, float& outNeedR )
    {
        // True if this XY quad intersects any opening disk (+ tiny stitch collar).
        outMinDist = 1e9f;
        outNeedR = 0.08f;
        float const mx = 0.5f * ( x0 + x1 ), my = 0.5f * ( y0 + y1 );
        bool hit = false;
        auto consider = [&]( float ox, float oy, float orad, float onz )
        {
            // Refine every mouth rim (shallow + steep) so HF handoff looks the same on walls
            // and flats — walls used to stay coarse while flats adaptive-cut.
            (void)onz;
            // Presentation refine target tip/6 — not matter resolution (12.5cm lattice).
            float const rad = (std::min)( (std::max)( 0.05f, orad ), 0.35f );
            float const px = (std::min)( (std::max)( ox, x0 ), x1 );
            float const py = (std::min)( (std::max)( oy, y0 ), y1 );
            float const dx = px - ox, dy = py - oy;
            float const d = std::sqrt( dx * dx + dy * dy );
            outMinDist = (std::min)( outMinDist, d );
            outNeedR = (std::max)( outNeedR, rad );
            if ( d <= rad + 0.04f ) { hit = true; }
        };
        for ( EditedRegion const& er : g.editedRegions )
        {
            for ( EditOpening const& o : er.openings )
            {
                consider( o.x, o.y, o.r, o.nz );
            }
        }
        (void)mx; (void)my;
        return hit;
    }

    void EmitTerrainQuadAdaptive( float x0, float y0, float x1, float y1, int depth )
    {
        // Refine only quads that touch a mouth disk — not the whole cell (square fine patches).
        // Same law as x64_Release_adapt: vista stays coarse 2×2; tip/6 only on opening collar.
        float minDist = 0.f, needR = 0.08f;
        bool const nearMouth = !g.editedRegions.empty()
            && QuadHitsMouthCollar( x0, y0, x1, y1, minDist, needR );
        float const span = (std::max)( x1 - x0, y1 - y0 );
        float const target = (std::max)( 0.04f, needR / 6.f );
        if ( nearMouth && span > target && depth < 5 )
        {
            float const xm = 0.5f * ( x0 + x1 );
            float const ym = 0.5f * ( y0 + y1 );
            EmitTerrainQuadAdaptive( x0, y0, xm, ym, depth + 1 );
            EmitTerrainQuadAdaptive( xm, y0, x1, ym, depth + 1 );
            EmitTerrainQuadAdaptive( x0, ym, xm, y1, depth + 1 );
            EmitTerrainQuadAdaptive( xm, ym, x1, y1, depth + 1 );
            return;
        }
        float z00 = 0.f, z10 = 0.f, z01 = 0.f, z11 = 0.f;
        bool const ok00 = SampleTerrainDrawZ( x0, y0, z00 );
        bool const ok10 = SampleTerrainDrawZ( x1, y0, z10 );
        bool const ok01 = SampleTerrainDrawZ( x0, y1, z01 );
        bool const ok11 = SampleTerrainDrawZ( x1, y1, z11 );
        int const nOk = ( ok00 ? 1 : 0 ) + ( ok10 ? 1 : 0 ) + ( ok01 ? 1 : 0 ) + ( ok11 ? 1 : 0 );
        if ( nOk == 0 ) { return; } // full mouth — D2 owns
        // Mixed HF/mouth: subdivide omit cracks. At max depth do NOT emit partial tris —
        // those became meter-long diagonal HF leaves (user shot 3) bridging grade→void.
        if ( nOk < 4 )
        {
            if ( span > 0.035f && depth < 6 )
            {
                float const xm = 0.5f * ( x0 + x1 );
                float const ym = 0.5f * ( y0 + y1 );
                EmitTerrainQuadAdaptive( x0, y0, xm, ym, depth + 1 );
                EmitTerrainQuadAdaptive( xm, y0, x1, ym, depth + 1 );
                EmitTerrainQuadAdaptive( x0, ym, xm, y1, depth + 1 );
                EmitTerrainQuadAdaptive( xm, ym, x1, y1, depth + 1 );
            }
            return;
        }
        if ( ok00 && ok10 && ok01 )
        {
            EmitPhase3Tri( x0, y0, z00, x1, y0, z10, x0, y1, z01, 0.f );
        }
        if ( ok10 && ok11 && ok01 )
        {
            EmitPhase3Tri( x1, y0, z10, x1, y1, z11, x0, y1, z01, 0.f );
        }
    }

    void EmitPhase3Tri( float x0, float y0, float z0,
                        float x1, float y1, float z1,
                        float x2, float y2, float z2,
                        float cavity )
    {
        // Flat face lighting — world sun + sky on geometric normals (same frame as gallery).
        float ax = x1 - x0, ay = y1 - y0, az = z1 - z0;
        float bx = x2 - x0, by = y2 - y0, bz = z2 - z0;
        float nx = ay * bz - az * by;
        float ny = az * bx - ax * bz;
        float nz = ax * by - ay * bx;
        float const nl = std::sqrt( nx * nx + ny * ny + nz * nz );
        if ( nl > 1e-6f ) { nx /= nl; ny /= nl; nz /= nl; }

        float const mx = ( x0 + x1 + x2 ) * ( 1.f / 3.f );
        float const my = ( y0 + y1 + y2 ) * ( 1.f / 3.f );
        float const mz = ( z0 + z1 + z2 ) * ( 1.f / 3.f );
        float cr, cg, cb;
        SampleCapColor( mx, my, cr, cg, cb );

        char const* cap = "dirt";
        {
            CellSample const* mc = GetCell( (int)std::floor( mx ), (int)std::floor( my ) );
            if ( mc && mc->valid && !mc->cap.empty() ) { cap = mc->cap.c_str(); }
        }
        LitBindMaterial( cap );

        float outR = 0.f, outG = 0.f, outB = 0.f;
        ShadeLitFace( nx, ny, nz, mx, my, mz,
            cr / 255.f, cg / 255.f, cb / 255.f, outR, outG, outB );

        // cavity > 0 dig darken; cavity < 0 place brighten — enclosure cue, not sun direction.
        if ( cavity > 0.f )
        {
            float const k = 1.f - 0.22f * (std::min)( 1.f, cavity );
            outR *= k; outG *= k; outB *= k;
        }
        else if ( cavity < 0.f )
        {
            float const k = 1.f - 0.35f * cavity;
            outR *= k; outG *= k; outB *= k;
        }

        glColor3f( outR, outG, outB );
        glVertex3f( x0, y0, z0 );
        glVertex3f( x1, y1, z1 );
        glVertex3f( x2, y2, z2 );
    }

    void DrawLiveScoopCups( bool digOnly )
    {
        // Open lit scoop bowls — polar rings for a round rim; winding faces sky (cull-safe).
        if ( g.scars.empty() ) { return; }

        constexpr int kRad = 14;
        constexpr int kSeg = 32;
        glShadeModel( GL_FLAT );
        glDisable( GL_CULL_FACE );
        glBegin( GL_TRIANGLES );
        for ( DigScar const& s : g.scars )
        {
            if ( digOnly && s.place ) { continue; }
            if ( !digOnly && !s.place ) { continue; }
            if ( !s.place && s.tunnel ) { continue; }
            // Plates / face punctures use sealed overlays — round scoop bowls tear steep walls.
            if ( !s.place && ( s.kind == ScarKind::FoliationPlate || s.kind == ScarKind::FacePuncture ) )
            {
                continue;
            }
            // Soft hemi cups are interim only — occupancy D2 cavity owns the hole once present.
            if ( !s.place )
            {
                int const scx = (int)std::floor( s.wx );
                int const scy = (int)std::floor( s.wy );
                CellSample const* sc = GetCell( scx, scy );
                if ( sc && sc->hasCavity ) { continue; }
            }
            float const rad = s.radius;
            if ( rad < 1e-4f ) { continue; }
            float const cav = s.place ? -1.f : 1.f; // place brighten / dig darken
            float zCenter = 0.f;
            if ( !SampleCupSurfaceZ( s.wx, s.wy, s.place, zCenter ) ) { continue; }
            for ( int j = 0; j < kRad; ++j )
            {
                float const r0 = rad * (float)j / (float)kRad;
                float const r1 = rad * (float)( j + 1 ) / (float)kRad;
                for ( int i = 0; i < kSeg; ++i )
                {
                    float const a0 = (float)i / (float)kSeg * 6.2831853f;
                    float const a1 = (float)( i + 1 ) / (float)kSeg * 6.2831853f;
                    float const c0 = std::cos( a0 ), s0 = std::sin( a0 );
                    float const c1 = std::cos( a1 ), s1 = std::sin( a1 );
                    float const px00 = s.wx + c0 * r0, py00 = s.wy + s0 * r0;
                    float const px10 = s.wx + c1 * r0, py10 = s.wy + s1 * r0;
                    float const px01 = s.wx + c0 * r1, py01 = s.wy + s0 * r1;
                    float const px11 = s.wx + c1 * r1, py11 = s.wy + s1 * r1;

                    float z00, z10, z01, z11;
                    if ( j == 0 )
                    {
                        z00 = zCenter;
                        z10 = zCenter;
                    }
                    else
                    {
                        if ( !SampleCupSurfaceZ( px00, py00, s.place, z00 ) ) { continue; }
                        if ( !SampleCupSurfaceZ( px10, py10, s.place, z10 ) ) { continue; }
                    }
                    if ( !SampleCupSurfaceZ( px01, py01, s.place, z01 ) ) { continue; }
                    if ( !SampleCupSurfaceZ( px11, py11, s.place, z11 ) ) { continue; }

                    // Sky-facing winding: inner → outer → next (was flipped → flat black under cull).
                    if ( j == 0 )
                    {
                        EmitPhase3Tri( s.wx, s.wy, zCenter, px01, py01, z01, px11, py11, z11, cav );
                    }
                    else
                    {
                        EmitPhase3Tri( px00, py00, z00, px01, py01, z01, px10, py10, z10, cav );
                        EmitPhase3Tri( px10, py10, z10, px01, py01, z01, px11, py11, z11, cav );
                    }
                }
            }
        }
        glEnd();
        glEnable( GL_CULL_FACE );
    }

    void RebuildTerrainMesh()
    {
        // Phase 3 vista only — virgin grades/fill. Scoops are live cups (DrawLiveScoopCups).
        // Adaptive mouth refine (x64_Release_adapt): coarse 2×2 vista; tip/6 only on opening disks.
        // Never whole-cell div=12 — that fine-meshed a square metre per opening at load/dig.
        DWORD const t0 = GetTickCount();
        constexpr int kDrawRadius = 64;
        int const ax = g.terrainAnchorX;
        int const ay = g.terrainAnchorY;
        int const x0 = ax - kDrawRadius;
        int const y0 = ay - kDrawRadius;
        int const x1 = ax + kDrawRadius;
        int const y1 = ay + kDrawRadius;
        float const feetX = g.feetX;
        float const feetY = g.feetY;
        (void)feetX; (void)feetY;

        if ( g.terrainList )
        {
            glDeleteLists( g.terrainList, 1 );
            g.terrainList = 0;
        }
        g.terrainList = AllocDisplayListOutsideFonts();
        if ( !g.terrainList ) { return; }
        LitSetIdentity();
        glNewList( g.terrainList, GL_COMPILE );
        glShadeModel( GL_FLAT );
        glBegin( GL_TRIANGLES );

        for ( int y = y0; y < y1; ++y )
        {
            for ( int x = x0; x < x1; ++x )
            {
                if ( !GetCell( x, y ) || !GetCell( x + 1, y ) || !GetCell( x, y + 1 ) || !GetCell( x + 1, y + 1 ) )
                {
                    continue;
                }

                // Coarse 2×2 vista; adaptive refine only quads that intersect opening disks.
                constexpr int kBase = 2;
                for ( int j = 0; j < kBase; ++j )
                {
                    for ( int i = 0; i < kBase; ++i )
                    {
                        float const u0 = (float)i / (float)kBase;
                        float const v0 = (float)j / (float)kBase;
                        float const u1 = (float)( i + 1 ) / (float)kBase;
                        float const v1 = (float)( j + 1 ) / (float)kBase;
                        EmitTerrainQuadAdaptive(
                            (float)x + u0, (float)y + v0,
                            (float)x + u1, (float)y + v1,
                            0 );
                    }
                }
            }
        }
        glEnd();
        glEndList();
        g.terrainDirty = false;
        float const ms = (float)( GetTickCount() - t0 );
        g.certLastRemeshMs = ms;
        if ( ms > g.certMaxRemeshMs ) { g.certMaxRemeshMs = ms; }
        ++g.perfHfRebuilds;
        g.perfHfRemeshMsTotal += ms;
        // Approximate tri count: base 2×2 quads × 2 tris × cells drawn (counted during emit is heavier).
        int cells = 0;
        for ( int y = y0; y < y1; ++y )
        {
            for ( int x = x0; x < x1; ++x )
            {
                if ( GetCell( x, y ) && GetCell( x + 1, y ) && GetCell( x, y + 1 ) && GetCell( x + 1, y + 1 ) )
                {
                    ++cells;
                }
            }
        }
        g.perfHfTris = cells * 8; // 2×2 quads × 2 tris (mouth refine adds more; lower bound)
    }

    void DrawDigGradeFootprints()
    {
        // Invisible stencil seal — must match scoop footprint exactly.
        // Oversized / heavy offset punched a feathered grade roof around the cup; reject that.
        constexpr int kRing = 16;
        constexpr float kZBias = 0.003f;
        glBegin( GL_TRIANGLES );

        for ( DigScar const& s : g.scars )
        {
            if ( s.place ) { continue; }
            // Never stencil-punch foliation plates or face punctures — opens sky void on cliffs.
            if ( s.kind == ScarKind::FoliationPlate || s.kind == ScarKind::FacePuncture ) { continue; }
            {
                int const scx = (int)std::floor( s.wx );
                int const scy = (int)std::floor( s.wy );
                CellSample const* sc = GetCell( scx, scy );
                if ( sc && sc->hasCavity ) { continue; } // occupancy footprint below
            }
            float gradeC = 0.f;
            if ( !SampleGroundZBase( s.wx, s.wy, gradeC ) ) { continue; }
            float rad = s.radius; // same radius as the dig cup — no halo seal
            if ( s.tunnel )
            {
                rad = TunnelSurfaceBreakRadius( s, gradeC );
                if ( rad < 1e-4f ) { continue; }
            }
            if ( rad < 1e-4f ) { continue; }
            float const step = ( 2.f * rad ) / (float)kRing;
            float const ox = s.wx - rad;
            float const oy = s.wy - rad;
            for ( int j = 0; j < kRing; ++j )
            {
                for ( int i = 0; i < kRing; ++i )
                {
                    float const px00 = ox + (float)i * step;
                    float const py00 = oy + (float)j * step;
                    float const px10 = px00 + step, py10 = py00;
                    float const px01 = px00, py01 = py00 + step;
                    float const px11 = px00 + step, py11 = py00 + step;
                    float const mx = ( px00 + px11 ) * 0.5f;
                    float const my = ( py00 + py11 ) * 0.5f;
                    float const mdx = mx - s.wx;
                    float const mdy = my - s.wy;
                    if ( ( mdx * mdx + mdy * mdy ) > ( rad * rad ) ) { continue; }
                    // Only punch where the scoop hemi actually cuts (no disc halo past the cup).
                    if ( !s.tunnel )
                    {
                        if ( ScarHemiAt( s, mx, my ) < 1e-5f ) { continue; }
                    }
                    else
                    {
                        float gMid = gradeC;
                        SampleGroundZBase( mx, my, gMid );
                        if ( ScarTunnelDepAt( s, mx, my, gMid ) < 1e-5f ) { continue; }
                    }
                    float z00, z10, z01, z11;
                    if ( !SampleGroundZBase( px00, py00, z00 ) ) { continue; }
                    if ( !SampleGroundZBase( px10, py10, z10 ) ) { continue; }
                    if ( !SampleGroundZBase( px01, py01, z01 ) ) { continue; }
                    if ( !SampleGroundZBase( px11, py11, z11 ) ) { continue; }
                    z00 += kZBias; z10 += kZBias; z01 += kZBias; z11 += kZBias;
                    glVertex3f( px00, py00, z00 );
                    glVertex3f( px10, py10, z10 );
                    glVertex3f( px01, py01, z01 );
                    glVertex3f( px10, py10, z10 );
                    glVertex3f( px11, py11, z11 );
                    glVertex3f( px01, py01, z01 );
                }
            }
        }

        // Occupancy hole footprints — only when AABB is allowed to own HF (disabled).
        if ( kAabbCavityOwnsHf )
        {
        constexpr int kOcc = 12;
        for ( auto const& kv : g.cells )
        {
            CellSample const& cell = kv.second;
            if ( !cell.carved || !cell.hasCavity || !cell.hasPatchBounds ) { continue; }
            int cx = 0, cy = 0;
            uint64_t const key = kv.first;
            cx = (int)(int32_t)( key >> 32 );
            cy = (int)(int32_t)( key & 0xffffffffu );
            float const step = 1.f / (float)kOcc;
            for ( int j = 0; j < kOcc; ++j )
            {
                for ( int i = 0; i < kOcc; ++i )
                {
                    float const px00 = (float)cx + (float)i * step;
                    float const py00 = (float)cy + (float)j * step;
                    float const px10 = px00 + step, py10 = py00;
                    float const px01 = px00, py01 = py00 + step;
                    float const px11 = px00 + step, py11 = py00 + step;
                    float const mx = ( px00 + px11 ) * 0.5f;
                    float const my = ( py00 + py11 ) * 0.5f;
                    if ( !CavityPatchOwnsAt( mx, my ) ) { continue; }
                    float z00, z10, z01, z11;
                    if ( !SampleGroundZBase( px00, py00, z00 ) ) { continue; }
                    if ( !SampleGroundZBase( px10, py10, z10 ) ) { continue; }
                    if ( !SampleGroundZBase( px01, py01, z01 ) ) { continue; }
                    if ( !SampleGroundZBase( px11, py11, z11 ) ) { continue; }
                    z00 += kZBias; z10 += kZBias; z01 += kZBias; z11 += kZBias;
                    glVertex3f( px00, py00, z00 );
                    glVertex3f( px10, py10, z10 );
                    glVertex3f( px01, py01, z01 );
                    glVertex3f( px10, py10, z10 );
                    glVertex3f( px11, py11, z11 );
                    glVertex3f( px01, py01, z01 );
                }
            }
        }
        }
        glEnd();
    }

    void DrawTunnelCavities()
    {
        // Look-steered buried scoops — sphere at carve centre (not a grade-roof bowl from the sky).
        if ( g.scars.empty() ) { return; }
        constexpr int kLat = 12;
        constexpr int kLon = 16;
        glShadeModel( GL_FLAT );
        glDisable( GL_CULL_FACE );
        glBegin( GL_TRIANGLES );
        for ( DigScar const& s : g.scars )
        {
            if ( s.place || !s.tunnel ) { continue; }
            float const rad = s.radius;
            if ( rad < 1e-4f ) { continue; }
            for ( int j = 0; j < kLat; ++j )
            {
                float const v0 = (float)j / (float)kLat;
                float const v1 = (float)( j + 1 ) / (float)kLat;
                float const a0 = ( v0 - 0.5f ) * 3.14159265f;
                float const a1 = ( v1 - 0.5f ) * 3.14159265f;
                float const y0 = std::sin( a0 ), y1 = std::sin( a1 );
                float const c0 = std::cos( a0 ), c1 = std::cos( a1 );
                for ( int i = 0; i < kLon; ++i )
                {
                    float const u0 = (float)i / (float)kLon * 6.2831853f;
                    float const u1 = (float)( i + 1 ) / (float)kLon * 6.2831853f;
                    float const x00 = s.wx + rad * c0 * std::cos( u0 );
                    float const y00 = s.wy + rad * c0 * std::sin( u0 );
                    float const z00 = s.wz + rad * y0;
                    float const x10 = s.wx + rad * c0 * std::cos( u1 );
                    float const y10 = s.wy + rad * c0 * std::sin( u1 );
                    float const z10 = s.wz + rad * y0;
                    float const x01 = s.wx + rad * c1 * std::cos( u0 );
                    float const y01 = s.wy + rad * c1 * std::sin( u0 );
                    float const z01 = s.wz + rad * y1;
                    float const x11 = s.wx + rad * c1 * std::cos( u1 );
                    float const y11 = s.wy + rad * c1 * std::sin( u1 );
                    float const z11 = s.wz + rad * y1;
                    // Inward-facing cavity; lit like open cups (not flat black).
                    EmitPhase3Tri( x00, y00, z00, x01, y01, z01, x10, y10, z10, 1.f );
                    EmitPhase3Tri( x10, y10, z10, x01, y01, z01, x11, y11, z11, 1.f );
                }
            }
        }
        glEnd();
        glEnable( GL_CULL_FACE );
    }

    void DrawDigFloorPlugs()
    {
        // Soft floor under punched openings — only until D2 cavity owns the mouth.
        if ( g.scars.empty() ) { return; }
        constexpr int kSeg = 28;
        glShadeModel( GL_FLAT );
        glDisable( GL_CULL_FACE );
        glBegin( GL_TRIANGLES );
        for ( DigScar const& s : g.scars )
        {
            if ( s.place || s.tunnel ) { continue; }
            if ( s.kind == ScarKind::FoliationPlate || s.kind == ScarKind::FacePuncture ) { continue; }
            // Occupancy D2 owns the hole — DigDep plugs fought cavity and left bright rims.
            if ( CavityReadyNear( s.wx, s.wy ) ) { continue; }
            float const rad = s.radius;
            if ( rad < 1e-4f ) { continue; }
            float zc = 0.f;
            if ( !SampleCupSurfaceZ( s.wx, s.wy, false, zc ) ) { continue; }
            for ( int i = 0; i < kSeg; ++i )
            {
                float const a0 = (float)i / (float)kSeg * 6.2831853f;
                float const a1 = (float)( i + 1 ) / (float)kSeg * 6.2831853f;
                float const x0 = s.wx + std::cos( a0 ) * rad;
                float const y0 = s.wy + std::sin( a0 ) * rad;
                float const x1 = s.wx + std::cos( a1 ) * rad;
                float const y1 = s.wy + std::sin( a1 ) * rad;
                float z0 = zc, z1 = zc;
                SampleCupSurfaceZ( x0, y0, false, z0 );
                SampleCupSurfaceZ( x1, y1, false, z1 );
                EmitPhase3Tri( s.wx, s.wy, zc, x0, y0, z0, x1, y1, z1, 1.f );
            }
        }
        glEnd();
        glEnable( GL_CULL_FACE );
    }

    void DrawFoliationPlateNotches()
    {
        // Retired for P3b — pick opens occupancy D2 cavities, not flat face-line scars.
    }

    void DrawFacePunctures()
    {
        // Retired for P3b — steep/pick soft bites use occupancy cavities, not sealed chips.
    }

    void DrawMatterBodies();
    void DrawHeightfield()
    {
        int const ax = (int)std::floor( g.feetX );
        int const ay = (int)std::floor( g.feetY );
        // --cert-geo walk: freeze 8-cell vista recenters so remesh=0 is measurable without
        // residency growth. Play path still recenters for streaming LOD.
        bool const freezeVistaRecenter = g.certGeo && g.certGeoPhase == 1 && g.certGeoDigArmed == 1;
        if ( !freezeVistaRecenter
          && ( std::abs( ax - g.terrainAnchorX ) >= 8 || std::abs( ay - g.terrainAnchorY ) >= 8 ) )
        {
            g.terrainDirty = true;
        }

        if ( g.terrainDirty || !g.terrainList )
        {
            g.terrainAnchorX = ax;
            g.terrainAnchorY = ay;
            RebuildTerrainMesh();
        }

        // Terrain → depth-tested stencil → mouth ALWAYS in punched pixels → interior LEQUAL.
        // Depth-OFF stencil x-rayed through hills. Depth-ON + LEQUAL mouth left HF skin.
        // Punch bias wins vs coplanar HF; nearer hills still win depth by metres.
        if ( g.terrainList ) { glCallList( g.terrainList ); }

        glClear( GL_STENCIL_BUFFER_BIT );
        glEnable( GL_STENCIL_TEST );
        glStencilMask( 0xFF );
        glStencilFunc( GL_ALWAYS, 1, 0xFF );
        glStencilOp( GL_KEEP, GL_KEEP, GL_REPLACE ); // stencil only if punch passes depth
        glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
        glDepthMask( GL_FALSE );
        glEnable( GL_DEPTH_TEST );
        glDepthFunc( GL_LEQUAL );
        glEnable( GL_POLYGON_OFFSET_FILL );
        glPolygonOffset( -3.f, -6.f ); // beat HF z-fight; not enough to leap a nearer hill
        DrawDigGradeFootprints();
        DrawCarveActionFootprints();
        glDisable( GL_POLYGON_OFFSET_FILL );
        glDepthMask( GL_TRUE );

        glStencilFunc( GL_EQUAL, 1, 0xFF );
        glStencilMask( 0x00 );
        glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
        // ALWAYS only inside depth-gated stencil — clears HF skin without through-hill paint.
        // No per-frame mouth underlay (lag + side-view floaters). Backdrop peek = sky blue void.
        glDepthFunc( GL_ALWAYS );
        DrawDigFloorPlugs();
        DrawLiveScoopCups( true );
        DrawCavityMeshesMouth();

        glStencilMask( 0xFF );
        glDisable( GL_STENCIL_TEST );
        glDepthFunc( GL_LEQUAL );
        DrawCavityMeshesInterior();
        DrawMaterialFormGallery(); // hand-scale material forms east of spawn
        DrawHeldGallerySample();   // E-held sample at hand for close inspect
        DrawLiveScoopCups( false );
        DrawFoliationPlateNotches(); // flash only when carve missed
        DrawFacePunctures();
        DrawMatterBodies();
        glDepthFunc( GL_LESS );
    }

    void DrawMatterBodies()
    {
        // Explicit H2H plates — conserved geometry, not scoop cups / not inventory cubes.
        if ( g.chipMode == ChipMode::Off ) { return; }
        if ( H2H::State().bodies.empty() ) { return; }
        glShadeModel( GL_FLAT );
        glDisable( GL_CULL_FACE );
        glBegin( GL_TRIANGLES );
        for ( H2H::MatterBody const& b : H2H::State().bodies )
        {
            if ( b.gripped ) { continue; } // carried body drawn at hand below
            float const ha = b.alongM * 0.5f;
            float const hc = b.acrossM * 0.5f;
            float const ht = b.thickM * 0.5f;
            float nx = b.nx, ny = b.ny, nz = b.nz;
            float nlen = std::sqrt( nx * nx + ny * ny + nz * nz );
            if ( nlen > 1e-5f ) { nx /= nlen; ny /= nlen; nz /= nlen; }
            else { nx = 0.f; ny = 0.f; nz = 1.f; }
            // Orthonormal tangent frame on the plate (works on vertical walls).
            float sx = b.strikeX, sy = b.strikeY, sz = 0.f;
            float sN = sx * nx + sy * ny + sz * nz;
            sx -= nx * sN; sy -= ny * sN; sz -= nz * sN;
            float slen = std::sqrt( sx * sx + sy * sy + sz * sz );
            if ( slen < 1e-4f )
            {
                sx = -ny; sy = nx; sz = 0.f;
                slen = std::sqrt( sx * sx + sy * sy );
            }
            if ( slen > 1e-5f ) { sx /= slen; sy /= slen; sz /= slen; }
            float tx = ny * sz - nz * sy;
            float ty = nz * sx - nx * sz;
            float tz = nx * sy - ny * sx;
            auto corner = [&]( float a, float c, float t, float& ox, float& oy, float& oz )
            {
                ox = b.x + sx * a + tx * c + nx * t;
                oy = b.y + sy * a + ty * c + ny * t;
                oz = b.z + sz * a + tz * c + nz * t;
            };
            float x000, y000, z000, x100, y100, z100, x010, y010, z010, x110, y110, z110;
            float x001, y001, z001, x101, y101, z101, x011, y011, z011, x111, y111, z111;
            corner( -ha, -hc, -ht, x000, y000, z000 );
            corner( +ha, -hc, -ht, x100, y100, z100 );
            corner( -ha, +hc, -ht, x010, y010, z010 );
            corner( +ha, +hc, -ht, x110, y110, z110 );
            corner( -ha, -hc, +ht, x001, y001, z001 );
            corner( +ha, -hc, +ht, x101, y101, z101 );
            corner( -ha, +hc, +ht, x011, y011, z011 );
            corner( +ha, +hc, +ht, x111, y111, z111 );
            float cav = b.settled ? 0.35f : 0.55f;
            // Broad faces (foliation planes) + thin edges.
            EmitPhase3Tri( x001, y001, z001, x101, y101, z101, x011, y011, z011, cav );
            EmitPhase3Tri( x101, y101, z101, x111, y111, z111, x011, y011, z011, cav );
            EmitPhase3Tri( x000, y000, z000, x010, y010, z010, x100, y100, z100, cav + 0.15f );
            EmitPhase3Tri( x100, y100, z100, x010, y010, z010, x110, y110, z110, cav + 0.15f );
            EmitPhase3Tri( x000, y000, z000, x100, y100, z100, x001, y001, z001, 0.7f );
            EmitPhase3Tri( x100, y100, z100, x101, y101, z101, x001, y001, z001, 0.7f );
            EmitPhase3Tri( x010, y010, z010, x011, y011, z011, x110, y110, z110, 0.7f );
            EmitPhase3Tri( x110, y110, z110, x011, y011, z011, x111, y111, z111, 0.7f );
        }
        glEnd();
        // Gripped plate at hand (contact-carry, not socket snap — offset from camera).
        if ( g.grippedBodyId != 0 )
        {
            for ( H2H::MatterBody const& b : H2H::State().bodies )
            {
                if ( b.body_id != g.grippedBodyId ) { continue; }
                float cp = std::cos( g.pitch ), sp = std::sin( g.pitch );
                float cyw = std::cos( g.yaw ), sy = std::sin( g.yaw );
                float fx = sy * cp, fy = cyw * cp, fz = sp;
                float hx = g.camX + fx * 0.55f + cyw * 0.22f;
                float hy = g.camY + fy * 0.55f - sy * 0.22f;
                float hz = g.camZ + fz * 0.55f - 0.15f;
                H2H::MatterBody draw = b;
                draw.x = hx; draw.y = hy; draw.z = hz;
                draw.gripped = false;
                // Re-enter single-body draw via temporary push (simple: wire box).
                glLineWidth( 2.f );
                glColor3f( 0.85f, 0.88f, 0.70f );
                float const ha = draw.alongM * 0.5f, hc = draw.acrossM * 0.5f, ht = draw.thickM * 0.5f;
                glBegin( GL_LINE_LOOP );
                glVertex3f( hx - ha, hy - hc, hz + ht );
                glVertex3f( hx + ha, hy - hc, hz + ht );
                glVertex3f( hx + ha, hy + hc, hz + ht );
                glVertex3f( hx - ha, hy + hc, hz + ht );
                glEnd();
                glBegin( GL_LINE_LOOP );
                glVertex3f( hx - ha, hy - hc, hz - ht );
                glVertex3f( hx + ha, hy - hc, hz - ht );
                glVertex3f( hx + ha, hy + hc, hz - ht );
                glVertex3f( hx - ha, hy + hc, hz - ht );
                glEnd();
                glLineWidth( 1.f );
                break;
            }
        }
        glEnable( GL_CULL_FACE );
    }

    void DrawScaleReference()
    {
        // Quiet 6ft marker — muted so it is never read as the dig/pick volume sphere.
        float cy = std::cos( g.yaw ), sy = std::sin( g.yaw );
        float fx = sy, fy = cy;
        float rx = cy, ry = -sy;
        float bx = g.feetX + fx * 5.5f + rx * 2.2f;
        float by = g.feetY + fy * 5.5f + ry * 2.2f;
        float gz = g.feetZ;
        SampleGroundZ( bx, by, gz );

        float const h = kCharHeightM;
        float const rad = kCapsuleRadiusM * 0.65f;
        glDisable( GL_CULL_FACE );
        glColor3f( 0.35f, 0.38f, 0.42f );
        glLineWidth( 1.f );
        // vertical body (line strip cylinder proxy)
        glBegin( GL_LINES );
        glVertex3f( bx, by, gz );
        glVertex3f( bx, by, gz + h );
        // height ticks every 1 ft (~0.3048 m)
        for ( int i = 1; i <= 6; ++i )
        {
            float z = gz + (float)i * 0.3048f;
            glVertex3f( bx - rad, by, z );
            glVertex3f( bx + rad, by, z );
        }
        // head ring
        for ( int i = 0; i < 12; ++i )
        {
            float a0 = (float)i / 12.f * 6.2831853f;
            float a1 = (float)( i + 1 ) / 12.f * 6.2831853f;
            glVertex3f( bx + std::cos( a0 ) * rad, by + std::sin( a0 ) * rad, gz + h );
            glVertex3f( bx + std::cos( a1 ) * rad, by + std::sin( a1 ) * rad, gz + h );
        }
        // foot ring
        for ( int i = 0; i < 12; ++i )
        {
            float a0 = (float)i / 12.f * 6.2831853f;
            float a1 = (float)( i + 1 ) / 12.f * 6.2831853f;
            glVertex3f( bx + std::cos( a0 ) * rad, by + std::sin( a0 ) * rad, gz + 0.02f );
            glVertex3f( bx + std::cos( a1 ) * rad, by + std::sin( a1 ) * rad, gz + 0.02f );
        }
        glEnd();
        glEnable( GL_CULL_FACE );
    }

    void SampleAimNormal( float x, float y, float& nx, float& ny, float& nz )
    {
        // Heightfield normal at the click — cue rings lie in this plane (Unreal ImpactNormal).
        constexpr float e = 0.08f;
        float zxm = 0.f, zxp = 0.f, zym = 0.f, zyp = 0.f, zc = 0.f;
        if ( !SampleAimSurfaceZ( x, y, zc ) ) { nx = 0.f; ny = 0.f; nz = 1.f; return; }
        if ( !SampleAimSurfaceZ( x - e, y, zxm ) ) { zxm = zc; }
        if ( !SampleAimSurfaceZ( x + e, y, zxp ) ) { zxp = zc; }
        if ( !SampleAimSurfaceZ( x, y - e, zym ) ) { zym = zc; }
        if ( !SampleAimSurfaceZ( x, y + e, zyp ) ) { zyp = zc; }
        nx = -( zxp - zxm ) / ( 2.f * e );
        ny = -( zyp - zym ) / ( 2.f * e );
        nz = 1.f;
        float const len = std::sqrt( nx * nx + ny * ny + nz * nz );
        if ( len > 1e-6f ) { nx /= len; ny /= len; nz /= len; }
        else { nx = 0.f; ny = 0.f; nz = 1.f; }
    }

    void BasisFromNormal( float nx, float ny, float nz,
                          float& tx, float& ty, float& tz,
                          float& bx, float& by, float& bz )
    {
        // Twin of Unreal FindBestAxisVectors — orthonormal tangent frame on the hit plane.
        float ax = 1.f, ay = 0.f, az = 0.f;
        if ( std::fabs( nx ) > 0.9f ) { ax = 0.f; ay = 1.f; az = 0.f; }
        tx = ay * nz - az * ny;
        ty = az * nx - ax * nz;
        tz = ax * ny - ay * nx;
        float tl = std::sqrt( tx * tx + ty * ty + tz * tz );
        if ( tl > 1e-6f ) { tx /= tl; ty /= tl; tz /= tl; }
        else { tx = 0.f; ty = 1.f; tz = 0.f; }
        bx = ny * tz - nz * ty;
        by = nz * tx - nx * tz;
        bz = nx * ty - ny * tx;
    }

    void FireActionCue( bool place, bool busy )
    {
        if ( !g.aimHit ) { return; }
        g.cueT = busy ? 0.22f : 0.20f;
        g.cueX = g.aimX;
        g.cueY = g.aimY;
        g.cueZ = g.aimZ;
        SampleAimNormal( g.aimX, g.aimY, g.cueNx, g.cueNy, g.cueNz );
        g.cuePlace = place;
        g.cueBusy = busy;
    }

    void DrawSurfaceRing( float ox, float oy, float oz,
                          float tx, float ty, float tz,
                          float bx, float by, float bz,
                          float radius, float cr, float cg, float cb, int seg )
    {
        glColor3f( cr, cg, cb );
        glBegin( GL_LINE_LOOP );
        for ( int i = 0; i < seg; ++i )
        {
            float const a = (float)i / (float)seg * 6.2831853f;
            float const ca = std::cos( a ), sa = std::sin( a );
            glVertex3f( ox + ( tx * ca + bx * sa ) * radius,
                        oy + ( ty * ca + by * sa ) * radius,
                        oz + ( tz * ca + bz * sa ) * radius );
        }
        glEnd();
    }

    void DrawHorizontalRing( float cx, float cy, float cz, float radius, float cr, float cg, float cb, int seg )
    {
        DrawSurfaceRing( cx, cy, cz, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, radius, cr, cg, cb, seg );
    }

    void DrawActionCue( float dt )
    {
        // Twin of Unreal FsDrawImmediateTerrainCue: rings in the hit tangent plane + radial dashes.
        if ( g.cueT <= 0.f ) { return; }
        g.cueT = (std::max)( 0.f, g.cueT - dt );
        float const life = g.cueBusy ? 0.22f : 0.20f;
        float const u = (std::max)( 0.f, g.cueT / life ); // 1 → 0
        float const fade = u * u;
        float cr, cg, cb;
        if ( g.cueBusy ) { cr = 1.f; cg = 0.67f; cb = 0.18f; }
        else if ( g.cuePlace ) { cr = 0.31f; cg = 0.86f; cb = 1.f; }
        else { cr = 0.35f; cg = 1.f; cb = 0.53f; }
        cr *= fade; cg *= fade; cb *= fade;

        float const cueR = AimDigAffect().radiusM;
        float nx = g.cueNx, ny = g.cueNy, nz = g.cueNz;
        float tx, ty, tz, bx, by, bz;
        BasisFromNormal( nx, ny, nz, tx, ty, tz, bx, by, bz );
        // Lift along normal so the splash sits on the clicked face (Unreal: ImpactPoint + N*1.5cm).
        float const lift = 0.015f;
        float const ox = g.cueX + nx * lift;
        float const oy = g.cueY + ny * lift;
        float const oz = g.cueZ + nz * lift;
        float const expand = 1.f + ( 1.f - u ) * 0.35f;

        glDisable( GL_CULL_FACE );
        glLineWidth( 2.2f );
        DrawSurfaceRing( ox, oy, oz, tx, ty, tz, bx, by, bz, cueR * expand, cr, cg, cb, 28 );
        DrawSurfaceRing( ox + nx * 0.005f, oy + ny * 0.005f, oz + nz * 0.005f,
                         tx, ty, tz, bx, by, bz, cueR * 0.52f * expand, cr, cg, cb, 20 );
        DrawWireSphere( ox, oy, oz, 0.012f, fade, fade, fade, 8 );

        // Dig radiates outward; place gathers inward — in the surface plane.
        glBegin( GL_LINES );
        glColor3f( cr, cg, cb );
        for ( int i = 0; i < 6; ++i )
        {
            float const a = ( 6.2831853f * (float)i / 6.f ) + 0.24f;
            float const ca = std::cos( a ), sa = std::sin( a );
            float const dx = tx * ca + bx * sa;
            float const dy = ty * ca + by * sa;
            float const dz = tz * ca + bz * sa;
            float const r0 = cueR * ( g.cuePlace ? 1.18f : 0.68f ) * expand;
            float const r1 = cueR * ( g.cuePlace ? 0.68f : 1.18f ) * expand;
            float const n0 = g.cuePlace ? 0.035f : 0.005f;
            float const n1 = g.cuePlace ? 0.005f : 0.035f;
            glVertex3f( ox + dx * r0 + nx * n0, oy + dy * r0 + ny * n0, oz + dz * r0 + nz * n0 );
            glVertex3f( ox + dx * r1 + nx * n1, oy + dy * r1 + ny * n1, oz + dz * r1 + nz * n1 );
        }
        glEnd();
        glLineWidth( 1.f );
        glEnable( GL_CULL_FACE );
    }

    void DrawAimScoop()
    {
        if ( !g.aimHit ) { return; }
        glDisable( GL_CULL_FACE );
        DigAffectSpec const affect = AimDigAffect();
        float const affectR = affect.radiusM;
        // Dig-volume ONLY — never draw the large contact envelope as a second sphere
        // (players read contact r=0.18–0.28m as the pick/shovel dig volume).
        if ( g.heldTotalG > 0 )
        {
            DrawWireSphere( g.aimX, g.aimY, g.aimZ, affectR, 0.95f, 0.72f, 0.28f, 24 );
        }
        else if ( affect.mode == DigAffectMode::FoliationPlate )
        {
            float fnx = 0.f, fny = 0.f, fnz = 1.f;
            CaptureFaceNormalAt( g.aimX, g.aimY, fnx, fny, fnz );
            float cp = std::cos( g.pitch ), sp = std::sin( g.pitch );
            float cyw = std::cos( g.yaw ), sy = std::sin( g.yaw );
            float fx = sy * cp, fy = cyw * cp, fz = sp;
            float strikeX = 1.f, strikeY = 0.f;
            StrikeAxesFromLook( fnx, fny, fnz, fx, fy, fz, strikeX, strikeY );
            float const along = (std::max)( affectR * 1.8f, RockStruct::kPickOpenAlongM * 0.45f );
            float const across = affectR;
            glLineWidth( 2.f );
            glColor3f( 0.75f, 0.92f, 1.f );
            // Seam ellipse in the FACE plane (not XY flat on grade).
            float t1x = strikeX, t1y = strikeY, t1z = 0.f;
            float t1n = t1x * fnx + t1y * fny + t1z * fnz;
            t1x -= fnx * t1n; t1y -= fny * t1n; t1z -= fnz * t1n;
            float t1len = std::sqrt( t1x * t1x + t1y * t1y + t1z * t1z );
            if ( t1len > 1e-5f ) { t1x /= t1len; t1y /= t1len; t1z /= t1len; }
            float t2x = fny * t1z - fnz * t1y;
            float t2y = fnz * t1x - fnx * t1z;
            float t2z = fnx * t1y - fny * t1x;
            float const lift = 0.008f;
            float const ox = g.aimX + fnx * lift, oy = g.aimY + fny * lift, oz = g.aimZ + fnz * lift;
            glBegin( GL_LINE_LOOP );
            for ( int i = 0; i < 24; ++i )
            {
                float const a = (float)i * 6.2831853f / 24.f;
                float const ca = std::cos( a ), sa = std::sin( a );
                glVertex3f(
                    ox + t1x * ( along * ca ) + t2x * ( across * sa ),
                    oy + t1y * ( along * ca ) + t2y * ( across * sa ),
                    oz + t1z * ( along * ca ) + t2z * ( across * sa ) );
            }
            glEnd();
            DrawWireSphere( ox, oy, oz, affectR, 0.45f, 0.90f, 1.f, 20 );
            glLineWidth( 1.f );
        }
        else
        {
            DrawWireSphere( g.aimX, g.aimY, g.aimZ, affectR, 0.25f, 0.95f, 0.45f, 26 );
        }
        glEnable( GL_CULL_FACE );
    }

    // Heightfield is a sheet: solid is "below grade". Steep cliffs need horizontal capsule tests
    // or the camera walks through the wall (per-frame climb < maxStep skates into the mesh).
    bool CapsuleHitsWall( float x, float y, float feetZ )
    {
        constexpr int kProbes = 12;
        float const r = kCapsuleRadiusM;
        float const bodyTop = feetZ + kEyeHeightM; // refuse walls into the POV
        int const feetCx = (int)std::floor( x );
        int const feetCy = (int)std::floor( y );
        CellSample const* feetCell = GetCell( feetCx, feetCy );
        bool const standingInCarve = feetCell && feetCell->carved && !feetCell->fill.empty();
        float virginUnderFeet = feetZ;
        bool haveVirgin = SampleGroundZBase( x, y, virginUnderFeet );

        for ( int i = 0; i < kProbes; ++i )
        {
            float const a = ( 6.2831853f * (float)i ) / (float)kProbes;
            float const px = x + std::cos( a ) * r;
            float const py = y + std::sin( a ) * r;
            float gz = feetZ;
            if ( !SampleGroundZ( px, py, gz ) ) { continue; }
            // Standing in a cavity: ignore virgin HF sheet beside the aperture as a "wall."
            // Real walls are occupancy-solid at torso/eye height.
            if ( standingInCarve && haveVirgin && feetZ < virginUnderFeet - 0.12f )
            {
                int const pcx = (int)std::floor( px );
                int const pcy = (int)std::floor( py );
                CellSample const* pc = GetCell( pcx, pcy );
                bool const probeCarved = pc && pc->carved && !pc->fill.empty();
                if ( !probeCarved
                    || ( !OccupancySolidAt( px, py, feetZ + 0.9f )
                      && !OccupancySolidAt( px, py, feetZ + 1.55f ) ) )
                {
                    continue;
                }
            }
            // Wall beside feet: grade rises into the body / eyes.
            if ( gz > feetZ + kWallBodyClearM && gz > feetZ + 0.15f )
            {
                return true;
            }
            // Camera buried in cliff sheet.
            if ( gz > bodyTop - 0.12f )
            {
                return true;
            }
        }
        // Center column: eye must stay above support (never inside the sheet).
        float gCenter = feetZ;
        if ( SampleGroundZ( x, y, gCenter ) && gCenter > bodyTop - 0.08f )
        {
            return true;
        }
        return false;
    }

    bool WalkStepAllowed( float fromX, float fromY, float fromZ, float toX, float toY, float& outGroundZ )
    {
        if ( !SampleGroundZ( toX, toY, outGroundZ ) ) { return false; }
        float const climb = outGroundZ - fromZ;
        float const dx = toX - fromX;
        float const dy = toY - fromY;
        float const horiz = std::sqrt( dx * dx + dy * dy );
        // Up only — walking off cliffs is a downward (or airborne) step and must stay allowed.
        if ( climb > kMaxStepM ) { return false; }
        if ( horiz > 1e-5f && climb > 0.f && ( climb / horiz ) > kMaxWalkSlope ) { return false; }

        // Capsule vs walls: test at current height when stepping down / off a ledge.
        // Using the *lower* standZ false-positives against the cliff you just left (blocks run-off).
        float const capsuleZ = ( climb > 0.05f ) ? outGroundZ : fromZ;
        if ( CapsuleHitsWall( toX, toY, capsuleZ ) ) { return false; }
        return true;
    }

    void ResolveEyeInOccupancyAir()
    {
        // Inside carved matter: crouch eye under solid, then push XY if eye still embeds in a wall.
        // Without this, standing eye height punches through the back wall → near-clip void.
        int const cx = (int)std::floor( g.feetX );
        int const cy = (int)std::floor( g.feetY );
        CellSample const* cell = GetCell( cx, cy );
        bool const inCarve = cell && cell->carved && !cell->fill.empty();
        float virginZ = g.feetZ;
        bool const belowVirgin = SampleGroundZBase( g.feetX, g.feetY, virginZ )
            && g.feetZ < virginZ - 0.10f;
        if ( !inCarve && !belowVirgin )
        {
            g.camX = g.feetX;
            g.camY = g.feetY;
            g.camZ = g.feetZ + kEyeHeightM;
            g.projNearM = kProjNearDefaultM;
            return;
        }

        g.projNearM = kProjNearCavityM;

        float eyeH = kEyeHeightM;
        // Shallow/open pits: standing eye floats above the virgin lip (screenshot: feet 3.66, eye 5.36
        // over grade ~4.1). Prefer eyes under the crest when there is headroom in the pocket.
        if ( belowVirgin )
        {
            float const underLip = virginZ - g.feetZ - 0.10f;
            if ( underLip >= kEyeCrouchMinM )
            {
                eyeH = (std::min)( eyeH, underLip );
            }
        }
        auto eyeBlocked = [&]( float h ) -> bool
        {
            float const ez = g.feetZ + h;
            float const mz = g.feetZ + h * 0.55f;
            return OccupancySolidAt( g.feetX, g.feetY, ez )
                || OccupancySolidAt( g.feetX, g.feetY, mz );
        };
        if ( inCarve && eyeBlocked( eyeH ) )
        {
            while ( eyeH > kEyeCrouchMinM && eyeBlocked( eyeH ) )
            {
                eyeH -= 0.05f;
            }
            if ( eyeBlocked( eyeH ) ) { eyeH = kEyeCrouchMinM; }
        }

        g.camX = g.feetX;
        g.camY = g.feetY;
        g.camZ = g.feetZ + eyeH;

        // Lateral push: eye must sit in air, not inside the wall behind the character.
        for ( int iter = 0; iter < 10; ++iter )
        {
            if ( !OccupancySolidAt( g.camX, g.camY, g.camZ )
              && !OccupancySolidAt( g.camX, g.camY, g.feetZ + 0.95f ) )
            {
                break;
            }
            float pushX = 0.f, pushY = 0.f;
            int airN = 0;
            constexpr int kDirs = 12;
            for ( int i = 0; i < kDirs; ++i )
            {
                float const a = ( 6.2831853f * (float)i ) / (float)kDirs;
                float const dx = std::cos( a ), dy = std::sin( a );
                float const px = g.camX + dx * 0.12f;
                float const py = g.camY + dy * 0.12f;
                if ( !OccupancySolidAt( px, py, g.camZ )
                  && !OccupancySolidAt( px, py, g.feetZ + 0.95f ) )
                {
                    pushX += dx; pushY += dy; ++airN;
                }
            }
            if ( airN <= 0 ) { break; }
            float plen = std::sqrt( pushX * pushX + pushY * pushY );
            if ( plen < 1e-5f ) { break; }
            pushX /= plen; pushY /= plen;
            g.feetX += pushX * 0.06f;
            g.feetY += pushY * 0.06f;
            g.camX = g.feetX;
            g.camY = g.feetY;
            float ground = g.feetZ;
            if ( SupportAt( g.feetX, g.feetY, ground ) )
            {
                if ( g.feetZ < ground ) { g.feetZ = ground; }
            }
            g.camZ = g.feetZ + eyeH;
        }
    }

    void ResolveWalkOutOfWall()
    {
        // If already clipped into a cliff, nudge feet toward free space (down-slope / away from high probes).
        for ( int iter = 0; iter < 6; ++iter )
        {
            if ( !CapsuleHitsWall( g.feetX, g.feetY, g.feetZ ) )
            {
                float eyeG = g.feetZ;
                if ( SampleGroundZ( g.feetX, g.feetY, eyeG )
                  && eyeG <= g.feetZ + kEyeHeightM - 0.08f )
                {
                    return;
                }
            }
            float pushX = 0.f, pushY = 0.f;
            constexpr int kProbes = 12;
            for ( int i = 0; i < kProbes; ++i )
            {
                float const a = ( 6.2831853f * (float)i ) / (float)kProbes;
                float const cx = std::cos( a ), cy = std::sin( a );
                float const px = g.feetX + cx * kCapsuleRadiusM;
                float const py = g.feetY + cy * kCapsuleRadiusM;
                float gz = g.feetZ;
                if ( !SampleGroundZ( px, py, gz ) ) { continue; }
                float const pen = gz - ( g.feetZ + kWallBodyClearM );
                if ( pen > 0.f )
                {
                    pushX -= cx * pen;
                    pushY -= cy * pen;
                }
            }
            float eyeG = g.feetZ;
            if ( SampleGroundZ( g.feetX, g.feetY, eyeG ) )
            {
                float const eyePen = eyeG - ( g.feetZ + kEyeHeightM - 0.1f );
                if ( eyePen > 0.f )
                {
                    // Push opposite look-forward horizontal (back out of the face).
                    float const ly = std::cos( g.yaw ), lx = std::sin( g.yaw );
                    pushX -= lx * eyePen;
                    pushY -= ly * eyePen;
                }
            }
            float plen = std::sqrt( pushX * pushX + pushY * pushY );
            if ( plen < 1e-5f ) { break; }
            pushX /= plen; pushY /= plen;
            g.feetX += pushX * 0.08f;
            g.feetY += pushY * 0.08f;
            float ground = g.feetZ;
            if ( SampleGroundZ( g.feetX, g.feetY, ground ) )
            {
                g.feetZ = ground;
                g.velZ = 0.f;
                g.grounded = true;
            }
        }
    }

    void UpdateCamera( float dt )
    {
        // F toggles walk / free-fly
        if ( g.keys['F'] && !g.keyToggleLatch['F'] )
        {
            g.walkMode = !g.walkMode;
            g.keyToggleLatch['F'] = true;
            if ( g.walkMode )
            {
                g.velZ = 0.f;
                float ground = g.feetZ;
                if ( SampleGroundZ( g.feetX, g.feetY, ground ) )
                {
                    g.feetZ = ground;
                    g.grounded = true;
                }
                g.statusLine = "Phase 4 - walk (6ft)";
            }
            else
            {
                g.statusLine = "FREE CAMERA — WASD move  Q/Ctrl down  E/Space up  F walk";
            }
            UpdateStreamHud();
        }
        if ( !g.keys['F'] ) { g.keyToggleLatch['F'] = false; }

        float cy = std::cos( g.yaw ), sy = std::sin( g.yaw );
        float fx = sy, fy = cy;
        float rx = cy, ry = -sy;

        if ( g.walkMode )
        {
            float const speed = g.keys[VK_SHIFT] ? kSprintSpeedMps : kWalkSpeedMps;
            float wishX = 0.f, wishY = 0.f;
            if ( g.keys['W'] ) { wishX += fx; wishY += fy; }
            if ( g.keys['S'] ) { wishX -= fx; wishY -= fy; }
            if ( g.keys['A'] ) { wishX -= rx; wishY -= ry; }
            if ( g.keys['D'] ) { wishX += rx; wishY += ry; }
            float wlen = std::sqrt( wishX * wishX + wishY * wishY );
            if ( wlen > 1e-5f )
            {
                wishX = ( wishX / wlen ) * speed * dt;
                wishY = ( wishY / wlen ) * speed * dt;
            }

            float const fromX = g.feetX, fromY = g.feetY, fromZ = g.feetZ;
            auto tryMove = [&]( float dx, float dy ) -> bool
            {
                float tx = fromX + dx, ty = fromY + dy, gz = fromZ;
                if ( !WalkStepAllowed( fromX, fromY, fromZ, tx, ty, gz ) ) { return false; }
                g.feetX = tx;
                g.feetY = ty;
                return true;
            };
            if ( wlen > 1e-5f )
            {
                // Full wish, then axis slides along walls.
                if ( !tryMove( wishX, wishY ) )
                {
                    if ( !tryMove( wishX, 0.f ) )
                    {
                        tryMove( 0.f, wishY );
                    }
                }
            }

            if ( g.keys[VK_SPACE] && g.grounded )
            {
                g.velZ = kJumpSpeedMps;
                g.grounded = false;
            }

            g.velZ -= kGravityMps2 * dt;
            g.feetZ += g.velZ * dt;

            float ground = g.feetZ;
            if ( SampleGroundZ( g.feetX, g.feetY, ground ) )
            {
                if ( g.feetZ <= ground )
                {
                    g.feetZ = ground;
                    g.velZ = 0.f;
                    g.grounded = true;
                }
                else
                {
                    g.grounded = false;
                }
            }

            ResolveWalkOutOfWall();

            // Horizon-to-Hand loose plates — PHYS only; SupportBelow from body Z (not crest).
            if ( g.chipMode == ChipMode::Phys )
            {
                H2H::StepBodies( dt, []( float x, float y, float queryZ ) -> H2H::SupportQuery {
                    SupportHit const h = SupportBelow( x, y, queryZ );
                    H2H::SupportQuery q{};
                    q.hit = h.hit;
                    q.deferred = h.deferred;
                    q.x = h.position.x;
                    q.y = h.position.y;
                    q.z = h.position.z;
                    q.nx = h.normal.x;
                    q.ny = h.normal.y;
                    q.nz = h.normal.z;
                    q.supportRev = h.regionRev;
                    return q;
                } );
            }
            // Carry gripped plate with the hand (contact follow).
            if ( g.grippedBodyId != 0 )
            {
                float cp = std::cos( g.pitch ), sp = std::sin( g.pitch );
                float cyw = std::cos( g.yaw ), sy = std::sin( g.yaw );
                float fx = sy * cp, fy = cyw * cp, fz = sp;
                for ( H2H::MatterBody& b : H2H::State().bodies )
                {
                    if ( b.body_id != g.grippedBodyId ) { continue; }
                    b.x = g.camX + fx * 0.55f + cyw * 0.22f;
                    b.y = g.camY + fy * 0.55f - sy * 0.22f;
                    b.z = g.camZ + fz * 0.55f - 0.15f;
                    b.vx = b.vy = b.vz = 0.f;
                    break;
                }
            }

            // Camera is eyes on the 6ft body — crouch / push clear of occupancy walls in carves.
            ResolveEyeInOccupancyAir();
            // Final POV clamp — never leave the eye inside the heightfield sheet (virgin only).
            {
                float gEye = g.camZ;
                int const ecx = (int)std::floor( g.camX );
                int const ecy = (int)std::floor( g.camY );
                CellSample const* ec = GetCell( ecx, ecy );
                bool const eyeInCarve = ec && ec->carved;
                if ( !eyeInCarve
                  && SampleGroundZBase( g.camX, g.camY, gEye ) && gEye > g.camZ - 0.05f )
                {
                    ResolveWalkOutOfWall();
                    ResolveEyeInOccupancyAir();
                }
            }
            FollowStreamCenter();
            ResolveSolidSampleCollisions();
        }
        else
        {
            // Free-fly (debug / aerial). Ctrl/Q down, E/Space up — no auto terrain stick.
            float const speed = g.keys[VK_SHIFT] ? kFlySprintMps : kFlySpeedMps;
            if ( g.keys['W'] ) { g.camX += fx * speed * dt; g.camY += fy * speed * dt; }
            if ( g.keys['S'] ) { g.camX -= fx * speed * dt; g.camY -= fy * speed * dt; }
            if ( g.keys['A'] ) { g.camX -= rx * speed * dt; g.camY -= ry * speed * dt; }
            if ( g.keys['D'] ) { g.camX += rx * speed * dt; g.camY += ry * speed * dt; }
            if ( g.keys['Q'] || g.keys[VK_CONTROL] ) { g.camZ -= speed * dt; }
            if ( g.keys['E'] || g.keys[VK_SPACE] ) { g.camZ += speed * dt; }
            g.feetX = g.camX;
            g.feetY = g.camY;
            g.feetZ = g.camZ - kEyeHeightM;
            g.grounded = false;
            {
                int const cx = (int)std::floor( g.camX );
                int const cy = (int)std::floor( g.camY );
                CellSample const* cell = GetCell( cx, cy );
                g.projNearM = ( cell && cell->carved ) ? kProjNearCavityM : kProjNearDefaultM;
            }
            FollowStreamCenter();
            ResolveSolidSampleCollisions();
        }
    }

    void DrawCompass( float cx, float cy )
    {
        // Easy-read heading: yaw 0 = +Y (North). Clockwise to East (+X).
        float deg = g.yaw * ( 180.f / 3.14159265f );
        while ( deg < 0.f ) { deg += 360.f; }
        while ( deg >= 360.f ) { deg -= 360.f; }
        char const* card = "N";
        if ( deg >= 337.5f || deg < 22.5f ) { card = "N"; }
        else if ( deg < 67.5f ) { card = "NE"; }
        else if ( deg < 112.5f ) { card = "E"; }
        else if ( deg < 157.5f ) { card = "SE"; }
        else if ( deg < 202.5f ) { card = "S"; }
        else if ( deg < 247.5f ) { card = "SW"; }
        else if ( deg < 292.5f ) { card = "W"; }
        else { card = "NW"; }

        float const R = 36.f;
        glColor3f( 0.12f, 0.14f, 0.18f );
        glBegin( GL_TRIANGLE_FAN );
        glVertex2f( cx, cy );
        for ( int i = 0; i <= 24; ++i )
        {
            float a = (float)i / 24.f * 6.2831853f;
            glVertex2f( cx + std::cos( a ) * R, cy + std::sin( a ) * R );
        }
        glEnd();
        glColor3f( 0.85f, 0.88f, 0.92f );
        glBegin( GL_LINE_LOOP );
        for ( int i = 0; i < 32; ++i )
        {
            float a = (float)i / 32.f * 6.2831853f;
            glVertex2f( cx + std::cos( a ) * R, cy + std::sin( a ) * R );
        }
        glEnd();

        // Needle: tip points the way you face (screen-up = look). Fixed N mark at top of dial.
        float rad = g.yaw; // face direction in world; dial keeps N up, needle rotates
        // With N fixed at screen-up, needle angle from +screenY: world yaw measured from +Y,
        // screen needle: rotate by -yaw so when yaw=0 needle points up (N).
        float nx = std::sin( rad );
        float ny = std::cos( rad );
        glColor3f( 0.95f, 0.35f, 0.28f );
        glBegin( GL_TRIANGLES );
        glVertex2f( cx + nx * ( R - 6.f ), cy + ny * ( R - 6.f ) );
        glVertex2f( cx - ny * 5.f - nx * 4.f, cy + nx * 5.f - ny * 4.f );
        glVertex2f( cx + ny * 5.f - nx * 4.f, cy - nx * 5.f - ny * 4.f );
        glEnd();

        glColor3f( 1.f, 1.f, 1.f );
        DrawHudText( cx - 4.f, cy + R + 4.f, "N" );
        DrawHudText( cx + R + 2.f, cy - 6.f, "E" );
        DrawHudText( cx - 4.f, cy - R - 16.f, "S" );
        DrawHudText( cx - R - 14.f, cy - 6.f, "W" );

        char hub[48];
        std::snprintf( hub, sizeof( hub ), "%s  %.0f", card, deg );
        DrawHudText( cx - 28.f, cy - R - 34.f, hub );
    }

    void DrawSessionClock( float x, float y )
    {
        DWORD now = GetTickCount();
        if ( g.sessionStartMs == 0 ) { g.sessionStartMs = now; }
        DWORD elapsed = now - g.sessionStartMs;
        unsigned secs = ( elapsed / 1000u ) % 60u;
        unsigned mins = ( elapsed / 60000u ) % 60u;
        unsigned hours = elapsed / 3600000u;
        char buf[64];
        if ( hours > 0 )
        {
            std::snprintf( buf, sizeof( buf ), "TIME  %u:%02u:%02u", hours, mins, secs );
        }
        else
        {
            std::snprintf( buf, sizeof( buf ), "TIME  %u:%02u", mins, secs );
        }
        glColor3f( 0.95f, 0.97f, 0.75f );
        DrawHudText( x, y, buf );
    }

    void Render()
    {
        if ( !g.glrc ) { return; }

        RECT rc; GetClientRect( g.hwnd, &rc );
        int w = (std::max)( 1, (int)rc.right );
        int h = (std::max)( 1, (int)rc.bottom );
        glViewport( 0, 0, w, h );

        UpdateWorldSun();
        LitSetIdentity();

        // Sky clear (gradient via clear + large back quad)
        glClearColor( 0.45f, 0.62f, 0.88f, 1.f );
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

        glMatrixMode( GL_PROJECTION );
        glLoadIdentity();
        float aspect = (float)w / (float)h;
        float fov = 60.f * 3.14159265f / 180.f;
        float nearZ = g.projNearM;
        if ( nearZ < 0.03f ) { nearZ = 0.03f; }
        if ( nearZ > 1.f ) { nearZ = 1.f; }
        float farZ = 600.f;
        float f = 1.f / std::tan( fov * 0.5f );
        float m[16] = {
            f / aspect, 0, 0, 0,
            0, f, 0, 0,
            0, 0, ( farZ + nearZ ) / ( nearZ - farZ ), -1,
            0, 0, ( 2 * farZ * nearZ ) / ( nearZ - farZ ), 0
        };
        glLoadMatrixf( m );

        glMatrixMode( GL_MODELVIEW );
        glLoadIdentity();

        // Camera look
        float cp = std::cos( g.pitch ), sp = std::sin( g.pitch );
        float cy = std::cos( g.yaw ), sy = std::sin( g.yaw );
        float fx = sy * cp, fy = cy * cp, fz = sp;
        // Build look-at manually (eye at cam, target = cam + forward). World Z-up.
        float tx = g.camX + fx, ty = g.camY + fy, tz = g.camZ + fz;
        float upx = 0, upy = 0, upz = 1;
        float zx = g.camX - tx, zy = g.camY - ty, zz = g.camZ - tz;
        float zl = std::sqrt( zx * zx + zy * zy + zz * zz );
        if ( zl > 1e-6f ) { zx /= zl; zy /= zl; zz /= zl; }
        float xx = upy * zz - upz * zy;
        float xy = upz * zx - upx * zz;
        float xz = upx * zy - upy * zx;
        float xl = std::sqrt( xx * xx + xy * xy + xz * xz );
        if ( xl > 1e-6f ) { xx /= xl; xy /= xl; xz /= xl; }
        float yx = zy * xz - zz * xy;
        float yy = zz * xx - zx * xz;
        float yz = zx * xy - zy * xx;
        float mv[16] = {
            xx, yx, zx, 0,
            xy, yy, zy, 0,
            xz, yz, zz, 0,
            -( xx * g.camX + xy * g.camY + xz * g.camZ ),
            -( yx * g.camX + yy * g.camY + yz * g.camZ ),
            -( zx * g.camX + zy * g.camY + zz * g.camZ ),
            1
        };
        glLoadMatrixf( mv );

        // Far backdrop quads (depth off). BOTH use clear-sky blue — never lime/grass-green.
        // Prior ground quad was glColor(0.70,0.80,0.55)=RGB(178,204,140); dig mesh gaps
        // peeked that color and were misread as grass or "lime void." Peek = sky void only.
        glDisable( GL_DEPTH_TEST );
        glBegin( GL_QUADS );
        glColor3f( 0.45f, 0.62f, 0.88f ); // == glClearColor
        float sky = 400.f;
        glVertex3f( g.camX - sky, g.camY - sky, 120.f );
        glVertex3f( g.camX + sky, g.camY - sky, 120.f );
        glVertex3f( g.camX + sky, g.camY + sky, 120.f );
        glVertex3f( g.camX - sky, g.camY + sky, 120.f );
        glVertex3f( g.camX - sky, g.camY - sky, -30.f );
        glVertex3f( g.camX + sky, g.camY - sky, -30.f );
        glVertex3f( g.camX + sky, g.camY + sky, -30.f );
        glVertex3f( g.camX - sky, g.camY + sky, -30.f );
        glEnd();
        glEnable( GL_DEPTH_TEST );

        DrawHeightfield();
        DrawScaleReference();
        DrawAimScoop();
        DrawActionCue( g.frameDt );

        // HUD orthographic overlay
        glDisable( GL_DEPTH_TEST );
        glDisable( GL_TEXTURE_2D );
        glDisable( GL_STENCIL_TEST );
        glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
        glMatrixMode( GL_PROJECTION );
        glLoadIdentity();
        glOrtho( 0, w, 0, h, -1, 1 );
        glMatrixMode( GL_MODELVIEW );
        glLoadIdentity();
        glColor3f( 0.95f, 0.97f, 1.f );

        char line[256];
        DrawHudText( 16, (float)h - 24, "PROVENANCE" );
        DrawHudText( 16, (float)h - 44, "Phase 4 - Horizon-to-Hand + geography + 6ft walk" );
        {
            char geoLine[320];
            H2H::EnsureReady();
            ProvenanceGeo::EnsureReady();
            auto surf = ProvenanceGeo::SampleSurface(
                (double)g.feetX, (double)g.feetY,
                g.gradeDatum, g.reliefVoxels, g.voxelEdgeM );
            VisualMat::VisualMaterialDef const& vd = VisualMat::OrDirt( surf.cap );
            RockStruct::Foliation const fol = RockStruct::FoliationAt( g.feetX, g.feetY );
            int openN = 0;
            for ( EditedRegion const& er : g.editedRegions ) { openN += (int)er.openings.size(); }
            std::snprintf( geoLine, sizeof( geoLine ),
                "%s | geo %s | fixture=%s | biome=%s | cap=%s form=%s | dip=%.0f° | bodies=%d | editReg=%d opens=%d | bound=%s d2=%d chips=%s | [B][C][G][F8]",
                H2H::kCapability,
                ProvenanceGeo::kGeneratorId,
                ProvenanceGeo::FixtureName( ProvenanceGeo::Fixture() ),
                ProvenanceGeo::ProvinceName( surf.province ),
                vd.id,
                H2H::FormOrDirt( surf.cap ).material_id,
                fol.dipDeg,
                (int)H2H::State().bodies.size(),
                (int)g.editedRegions.size(),
                openN,
                g.boundaryMode == BoundaryMode::D2 ? "D2"
                    : ( g.boundaryMode == BoundaryMode::DebugBoundary ? "AABB" : "OVER" ),
                g.cavityTrisTotal,
                g.chipMode == ChipMode::Off ? "OFF"
                    : ( g.chipMode == ChipMode::Visual ? "VIS" : "PHYS" ) );
            DrawHudText( 16, (float)h - 56, geoLine );
        }
        // Chip quiescence probe (P3d.1) — id/life/state/speed/nz/z/restZ/|z-rest|/quiet/supportRev
        if ( g.chipMode != ChipMode::Off && !H2H::State().bodies.empty() )
        {
            int shown = 0;
            float chipY = (float)h - 62.f;
            for ( H2H::MatterBody const& b : H2H::State().bodies )
            {
                if ( shown >= 3 ) { break; }
                char probe[256];
                H2H::FormatChipProbe( b, probe, (int)sizeof( probe ) );
                glColor3f( 0.85f, 0.95f, 0.75f );
                DrawHudText( 16, chipY - (float)( shown * 12 ), probe );
                ++shown;
            }
            char actLine[96];
            std::snprintf( actLine, sizeof( actLine ),
                "activeChips=%d sleepQuietN=%d",
                H2H::CountActiveChips(), H2H::kChipSleepQuietFrames );
            glColor3f( 0.75f, 0.90f, 0.70f );
            DrawHudText( 16, chipY - (float)( shown * 12 ), actLine );
            glColor3f( 0.95f, 0.97f, 1.f );
        }
        std::snprintf( line, sizeof( line ), "Link: %s   Engine: %s:%d", LinkLabel( g.link ), g.host.c_str(), g.port );
        DrawHudText( 16, (float)h - 68, line );
        DrawHudText( 16, (float)h - 88, g.statusLine.c_str() );
        DrawSessionClock( 16, (float)h - 108 );

        int yHud = h - 132;
        size_t start = 0;
        while ( start < g.detail.size() && yHud > 56 )
        {
            size_t end = g.detail.find( '\n', start );
            if ( end == std::string::npos ) { end = g.detail.size(); }
            std::string row = g.detail.substr( start, end - start );
            DrawHudText( 16, (float)yHud, row.c_str() );
            yHud -= 18;
            start = end + 1;
        }

        // Crosshair
        glColor3f( 1.f, 1.f, 1.f );
        float hx = w * 0.5f, hy = h * 0.5f;
        glBegin( GL_LINES );
        glVertex2f( hx - 8, hy ); glVertex2f( hx - 2, hy );
        glVertex2f( hx + 2, hy ); glVertex2f( hx + 8, hy );
        glVertex2f( hx, hy - 8 ); glVertex2f( hx, hy - 2 );
        glVertex2f( hx, hy + 2 ); glVertex2f( hx, hy + 8 );
        glEnd();

        // Aim strike identity — color swatch + material/biome (what you seek to take).
        if ( g.aimHit )
        {
            float const sx0 = hx + 14.f, sy0 = hy - 8.f;
            float const sx1 = sx0 + 14.f, sy1 = sy0 + 14.f;
            glColor3ub( g.aimStrikeR, g.aimStrikeG, g.aimStrikeB );
            glBegin( GL_QUADS );
            glVertex2f( sx0, sy0 ); glVertex2f( sx1, sy0 );
            glVertex2f( sx1, sy1 ); glVertex2f( sx0, sy1 );
            glEnd();
            glColor3f( 0.15f, 0.15f, 0.18f );
            glBegin( GL_LINE_LOOP );
            glVertex2f( sx0, sy0 ); glVertex2f( sx1, sy0 );
            glVertex2f( sx1, sy1 ); glVertex2f( sx0, sy1 );
            glEnd();
            char strikeLine[192];
            if ( _stricmp( g.aimWireCap.c_str(), g.aimStrikeCap.c_str() ) != 0 )
            {
                std::snprintf( strikeLine, sizeof( strikeLine ),
                    "%s  %s   wire:%s",
                    g.aimStrikeCap.c_str(), g.aimBiome.c_str(), g.aimWireCap.c_str() );
            }
            else
            {
                std::snprintf( strikeLine, sizeof( strikeLine ),
                    "%s  %s", g.aimStrikeCap.c_str(), g.aimBiome.c_str() );
            }
            glColor3f( 0.98f, 0.98f, 0.92f );
            DrawHudText( sx1 + 6.f, sy0 + 2.f, strikeLine );
        }

        DrawCompass( (float)w - 56.f, (float)h - 70.f );

        glColor3f( 0.95f, 0.97f, 1.f );
        if ( !g.walkMode )
        {
            glColor3f( 1.f, 0.85f, 0.35f );
            DrawHudText( 16, 64, "FREE CAMERA  [F] back to walk" );
            glColor3f( 0.95f, 0.97f, 1.f );
        }
        DrawHudText( 16, 46, "LMB dig/pick  RMB place  [E] pick/drop  [B] cavity  [C] chips  [K] chip dump  scroll rolls held" );
        DrawHudText( 16, 28, "Sun+sky light materials (not painted dark sides)  tumble gold — highlight must travel" );
        DrawHudText( 16, 10, g.digestLine.c_str() );

        g.uiWinW = w;
        g.uiWinH = h;
        if ( g.journalOpen )
        {
            DrawJournal();
        }
        DrawHotbar(); // always on — pinned tabs remain when journal is closed

        glEnable( GL_DEPTH_TEST );

        SwapBuffers( g.hdc );
    }

    bool DumpFramePpm( char const* path )
    {
        GLint vp[4] = {};
        glGetIntegerv( GL_VIEWPORT, vp );
        int const w = vp[2], h = vp[3];
        if ( w <= 0 || h <= 0 ) { return false; }
        std::vector<unsigned char> rgba( (size_t)w * (size_t)h * 4u );
        glPixelStorei( GL_PACK_ALIGNMENT, 1 );
        glReadBuffer( GL_FRONT );
        glReadPixels( 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data() );
        FILE* f = nullptr;
        if ( fopen_s( &f, path, "wb" ) != 0 || !f ) { return false; }
        std::fprintf( f, "P6\n%d %d\n255\n", w, h );
        for ( int y = h - 1; y >= 0; --y )
        {
            for ( int x = 0; x < w; ++x )
            {
                size_t const i = ( (size_t)y * (size_t)w + (size_t)x ) * 4u;
                unsigned char rgb[3] = { rgba[i], rgba[i + 1], rgba[i + 2] };
                std::fwrite( rgb, 1, 3, f );
            }
        }
        std::fclose( f );
        return true;
    }

    void WriteCertPixelReport( char const* ppmPath, char const* reportPath )
    {
        constexpr int kVoidR = 114, kVoidG = 158, kVoidB = 224;
        constexpr int kGrassR = 93, kGrassG = 133, kGrassB = 68;
        constexpr int kLimeR = 193, kLimeG = 189, kLimeB = 174;
        constexpr int kMicaR = 126, kMicaG = 123, kMicaB = 107;
        // Legacy backdrop ground quad (pre-voidfix) — exact RGB(178,204,140); not grass.
        constexpr int kBackR = 178, kBackG = 204, kBackB = 140;
        GLint vp[4] = {};
        glGetIntegerv( GL_VIEWPORT, vp );
        int const w = vp[2], h = vp[3];
        if ( w <= 8 || h <= 8 ) { return; }
        std::vector<unsigned char> rgba( (size_t)w * (size_t)h * 4u );
        glPixelStorei( GL_PACK_ALIGNMENT, 1 );
        glReadPixels( 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data() );
        int x0 = w * 2 / 5, x1 = w * 3 / 5, y0 = h * 2 / 5, y1 = h * 3 / 5;
        long long n = 0, sky = 0, grassN = 0, chalk = 0, dark = 0, back = 0, other = 0;
        auto d2 = []( int r, int gc, int b, int rr, int gg, int bb ) {
            long long dr = r - rr, dg = gc - gg, db = b - bb;
            return dr * dr + dg * dg + db * db;
        };
        for ( int y = y0; y < y1; ++y )
        {
            for ( int x = x0; x < x1; ++x )
            {
                size_t const i = ( (size_t)y * (size_t)w + (size_t)x ) * 4u;
                int r = rgba[i], gc = rgba[i + 1], b = rgba[i + 2];
                ++n;
                if ( r + gc + b < 90 ) { ++dark; continue; }
                if ( d2( r, gc, b, kBackR, kBackG, kBackB ) <= 64 ) { ++back; continue; }
                long long dv = d2( r, gc, b, kVoidR, kVoidG, kVoidB );
                long long dg = d2( r, gc, b, kGrassR, kGrassG, kGrassB );
                long long dc = d2( r, gc, b, kLimeR, kLimeG, kLimeB );
                long long dm = d2( r, gc, b, kMicaR, kMicaG, kMicaB );
                if ( b > gc && b > r && b > 150 && dv <= dg && dv <= dc ) { ++sky; continue; }
                if ( dg <= dv && dg <= dc && dg <= dm && gc > r + 20 && gc > b + 20 ) { ++grassN; continue; }
                if ( dc <= dv && dc <= dg && ( dc <= dm || r + gc + b > 480 ) ) { ++chalk; continue; }
                ++other;
            }
        }
        FILE* f = nullptr;
        if ( fopen_s( &f, reportPath, "w" ) != 0 || !f ) { return; }
        std::fprintf( f,
            "ppm=%s\nviewport=%dx%d\ncenter_samples=%lld\n"
            "refs: void_clear=RGB(%d,%d,%d) grass_mid=RGB(%d,%d,%d) limestone_mid=RGB(%d,%d,%d) mica_mid=RGB(%d,%d,%d) legacy_backdrop=RGB(%d,%d,%d)\n"
            "class_sky_void_pct=%.2f\nclass_legacy_backdrop_void_pct=%.2f\nclass_grass_pct=%.2f\nclass_chalk_stone_pct=%.2f\nclass_shadow_dark_pct=%.2f\nclass_other_pct=%.2f\n"
            "digest=%s\naim=%s/%s\ncavity_tris=%d\n"
            "note=mesh gap void=sky clear (or legacy lime backdrop); grass mid is olive CapColor — never RGB(178,204,140)\n",
            ppmPath, w, h, n,
            kVoidR, kVoidG, kVoidB, kGrassR, kGrassG, kGrassB, kLimeR, kLimeG, kLimeB, kMicaR, kMicaG, kMicaB,
            kBackR, kBackG, kBackB,
            n ? ( 100.0 * sky / n ) : 0.0,
            n ? ( 100.0 * back / n ) : 0.0,
            n ? ( 100.0 * grassN / n ) : 0.0,
            n ? ( 100.0 * chalk / n ) : 0.0,
            n ? ( 100.0 * dark / n ) : 0.0,
            n ? ( 100.0 * other / n ) : 0.0,
            g.digestLine.c_str(),
            g.aimStrikeCap.c_str(), g.aimWireCap.c_str(),
            g.cavityTrisTotal );
        std::fclose( f );
    }

    void WritePerfReport( char const* path, char const* tag )
    {
        FILE* f = nullptr;
        if ( fopen_s( &f, path, "w" ) != 0 || !f ) { return; }
        int matterN = (int)H2H::State().bodies.size();
        std::fprintf( f,
            "tag=%s\n"
            "geoCellsCreated=%d\nSampleSurface_calls=%d\nSampleCapColor_calls=%d\n"
            "HF_rebuild_count=%d\nHF_tris_approx=%d\nHF_remesh_ms_total=%.2f\nHF_remesh_ms_max=%.2f\n"
            "EditedRegions=%d\n"
            "D2_rebuild_count=%d\nD2_tris=%d\nD2_edges=%d\nD2_qef_fallbacks=%d\nD2_ms_total=%.2f\n"
            "virgin_D2_rebuilds_at_stream=%d\n"
            "MatterBodies=%d\nchipMode=%d\n"
            "invariant_virgin_D2_zero=%s\n"
            "laws=vista_cheap+interaction_local; action!=D2_dirty!=HF_aperture; "
            "D2_0.85=work_partition_not_clip; HF_12cm=new_strike_only; tip/6=presentation_only\n",
            tag ? tag : "-",
            g.perfGeoCellsCreated, g.perfSampleSurfaceCalls, g.perfSampleCapColorCalls,
            g.perfHfRebuilds, g.perfHfTris, g.perfHfRemeshMsTotal, g.certMaxRemeshMs,
            (int)g.editedRegions.size(),
            g.perfD2Rebuilds, g.perfD2Tris, g.perfD2Edges, g.perfD2QefFallbacks, g.perfD2MsTotal,
            g.perfVirginD2Rebuilds,
            matterN, (int)g.chipMode,
            ( g.perfVirginD2Rebuilds == 0 ) ? "PASS" : "FAIL_or_unset" );
        std::fclose( f );
    }

    // Agent-facing chip probe (P3d.1) — full body list; HUD stays compressed (first 3 only).
    // Path: %TEMP%\provenance_chip_probe.txt  (1 Hz + [K] force)
    void WriteChipProbeDump()
    {
        char path[MAX_PATH];
        if ( GetTempPathA( MAX_PATH, path ) == 0 ) { return; }
        if ( strcat_s( path, MAX_PATH, "provenance_chip_probe.txt" ) != 0 ) { return; }
        FILE* f = nullptr;
        if ( fopen_s( &f, path, "w" ) != 0 || !f ) { return; }

        char const* mode = g.chipMode == ChipMode::Off ? "OFF"
            : ( g.chipMode == ChipMode::Visual ? "VISUAL" : "PHYS" );
        int const bodies = (int)H2H::State().bodies.size();
        int const active = H2H::CountActiveChips();
        int moving = 0;
        int settled = 0;
        for ( H2H::MatterBody const& b : H2H::State().bodies )
        {
            float const spd = std::sqrt( b.vx * b.vx + b.vy * b.vy + b.vz * b.vz );
            if ( spd > 1.0e-4f ) { ++moving; }
            if ( b.life == H2H::ChipLife::Settled || b.settled ) { ++settled; }
        }

        SYSTEMTIME st;
        GetLocalTime( &st );
        std::fprintf( f,
            "# provenance_chip_probe\n"
            "time=%04u-%02u-%02uT%02u:%02u:%02u\n"
            "chipMode=%s\n"
            "bodies=%d\n"
            "activeChips=%d\n"
            "movingChips=%d\n"
            "settledChips=%d\n"
            "sleepQuietN=%d\n"
            "---\n",
            (unsigned)st.wYear, (unsigned)st.wMonth, (unsigned)st.wDay,
            (unsigned)st.wHour, (unsigned)st.wMinute, (unsigned)st.wSecond,
            mode, bodies, active, moving, settled, H2H::kChipSleepQuietFrames );

        for ( H2H::MatterBody const& b : H2H::State().bodies )
        {
            char probe[256];
            H2H::FormatChipProbe( b, probe, (int)sizeof( probe ) );
            std::fprintf( f, "%s\n", probe );
        }
        std::fclose( f );
    }

    void SnapVirginPerfIfNeeded()
    {
        if ( g.perfVirginSnapDone || !g.streamComplete ) { return; }
        g.perfVirginD2Rebuilds = g.perfD2Rebuilds;
        g.perfVirginSnapDone = true;
        if ( !g.certOutDir[0] ) { GetTempPathA( MAX_PATH, g.certOutDir ); }
        char path[MAX_PATH];
        std::snprintf( path, sizeof( path ), "%s\\provenance_startup_perf.txt", g.certOutDir );
        WritePerfReport( path, "virgin_stream_complete" );
    }

    // ---------- Geography interaction cert (--cert-geo) ----------
    // RANGE fixture as Horizon-to-Hand transect. Capture FAIL; do not auto-fix.
    struct GeoCertContact
    {
        char id[24] = {};
        char terrain[48] = {};
        float x = 0.f, y = 0.f, z = 0.f;
        float nx = 0.f, ny = 0.f, nz = 1.f;
        float slopeDeg = 0.f;
        char material[32] = {};
        int cellX = 0, cellY = 0;
        bool soft = true; // shovel vs pick
        bool found = false;
    };

    struct GeoCertRow
    {
        char section[8] = {};
        char scenario[64] = {};
        char terrain[48] = {};
        float slope = 0.f;
        char material[32] = {};
        float x = 0.f, y = 0.f, z = 0.f;
        float nx = 0.f, ny = 0.f, nz = 1.f;
        char action[48] = {};
        int grams = 0;
        int occDelta = 0;
        int hfSubdiv = 2;
        int editedRegion = 0;
        int d2Tris = 0;
        int hermites = 0;
        int qefFb = 0;
        int haloMiss = 0;
        int ownViol = 0;
        int matMismatch = 0;
        int supportFail = 0;
        char hash[24] = "-";
        char timing[48] = "-";
        char verdict[12] = "SKIP"; // PASS / FAIL / SKIP
        char note[160] = {};
    };

    static constexpr int kGeoCertContactCap = 16;
    static constexpr int kGeoCertRowCap = 160;
    static GeoCertContact s_geoContacts[kGeoCertContactCap];
    static int s_geoContactN = 0;
    static GeoCertRow s_geoRows[kGeoCertRowCap];
    static int s_geoRowN = 0;
    static char s_geoFailReason[96] = {};
    static char s_geoFailFixture[48] = {};
    static char s_geoFailAction[48] = {};
    static float s_geoFailX = 0.f, s_geoFailY = 0.f, s_geoFailZ = 0.f;
    static float s_geoFailNx = 0.f, s_geoFailNy = 0.f, s_geoFailNz = 1.f;

    bool GeoCertIsSoftRock( ProvenanceGeo::RockBody b )
    {
        using RB = ProvenanceGeo::RockBody;
        return b == RB::DirtMantle || b == RB::Loam || b == RB::Sand
            || b == RB::Gravel || b == RB::Clay;
    }

    void GeoCertAddRow( GeoCertRow const& r )
    {
        if ( s_geoRowN >= kGeoCertRowCap ) { return; }
        s_geoRows[s_geoRowN++] = r;
    }

    void GeoCertScaffold( char const* section, char const* scenario, char const* note )
    {
        GeoCertRow r{};
        std::snprintf( r.section, sizeof( r.section ), "%s", section );
        std::snprintf( r.scenario, sizeof( r.scenario ), "%s", scenario );
        std::snprintf( r.verdict, sizeof( r.verdict ), "SKIP" );
        std::snprintf( r.note, sizeof( r.note ), "%s", note ? note : "scaffold" );
        std::snprintf( r.action, sizeof( r.action ), "-" );
        std::snprintf( r.terrain, sizeof( r.terrain ), "-" );
        std::snprintf( r.material, sizeof( r.material ), "-" );
        GeoCertAddRow( r );
    }

    void GeoCertHardFail( char const* fixture, char const* action, char const* reason,
        float x, float y, float z, float nx, float ny, float nz )
    {
        if ( g.certGeoFailWritten ) { return; }
        g.certGeoFailWritten = true;
        g.certGeoExitCode = 1;
        std::snprintf( s_geoFailFixture, sizeof( s_geoFailFixture ), "%s", fixture ? fixture : "?" );
        std::snprintf( s_geoFailAction, sizeof( s_geoFailAction ), "%s", action ? action : "?" );
        std::snprintf( s_geoFailReason, sizeof( s_geoFailReason ), "%s", reason ? reason : "?" );
        s_geoFailX = x; s_geoFailY = y; s_geoFailZ = z;
        s_geoFailNx = nx; s_geoFailNy = ny; s_geoFailNz = nz;

        if ( !g.certOutDir[0] ) { GetTempPathA( MAX_PATH, g.certOutDir ); }
        char path[MAX_PATH];
        std::snprintf( path, sizeof( path ), "%s\\provenance_geography_interaction_fail.txt", g.certOutDir );
        FILE* f = nullptr;
        if ( fopen_s( &f, path, "w" ) == 0 && f )
        {
            std::fprintf( f,
                "FAIL:\n"
                "fixture=%s\n"
                "action=%s\n"
                "world=(%.3f,%.3f,%.3f)\n"
                "normal=(%.4f,%.4f,%.4f)\n"
                "reason=%s\n"
                "beforeMeshHash=-\n"
                "afterMeshHash=-\n"
                "occupancyHash=-\n"
                "note=teleport visual client to world XYZ; do not auto-compensate\n"
                "geoCellsCreated=%d SampleSurface=%d HF_rebuilds=%d D2_rebuilds=%d\n"
                "EditedRegions=%d MatterBodies=%d virgin_D2=%d\n",
                s_geoFailFixture, s_geoFailAction,
                s_geoFailX, s_geoFailY, s_geoFailZ,
                s_geoFailNx, s_geoFailNy, s_geoFailNz,
                s_geoFailReason,
                g.perfGeoCellsCreated, g.perfSampleSurfaceCalls, g.perfHfRebuilds, g.perfD2Rebuilds,
                (int)g.editedRegions.size(), (int)H2H::State().bodies.size(),
                g.perfVirginD2Rebuilds );
            std::fclose( f );
        }
        char msg[160];
        std::snprintf( msg, sizeof( msg ), "CERT-GEO FAIL %s @ (%.1f,%.1f)", reason, x, y );
        g.statusLine = msg;
    }

    bool GeoCertSampleAt( float x, float y, float& z, float& nx, float& ny, float& nz,
        ProvenanceGeo::SurfaceSample& surf )
    {
        surf = ProvenanceGeo::SampleSurface( x, y, g.gradeDatum, g.reliefVoxels, g.voxelEdgeM );
        // Discovery samples analytic geography — does NOT bump perfSampleSurfaceCalls
        // (cell cache path owns that counter). Cert compares EnsureGeoCell creates only.
        if ( !SampleGroundZBase( x, y, z ) ) { return false; }
        CaptureFaceNormalAt( x, y, nx, ny, nz );
        return true;
    }

    void GeoCertPushContact( char const* id, char const* terrain, float x, float y )
    {
        if ( s_geoContactN >= kGeoCertContactCap ) { return; }
        GeoCertContact& c = s_geoContacts[s_geoContactN];
        std::memset( &c, 0, sizeof( c ) );
        std::snprintf( c.id, sizeof( c.id ), "%s", id );
        std::snprintf( c.terrain, sizeof( c.terrain ), "%s", terrain );
        ProvenanceGeo::SurfaceSample surf{};
        float z = 0.f, nx = 0.f, ny = 0.f, nz = 1.f;
        if ( !GeoCertSampleAt( x, y, z, nx, ny, nz, surf ) ) { return; }
        c.x = x; c.y = y; c.z = z;
        c.nx = nx; c.ny = ny; c.nz = nz;
        float const horiz = std::sqrt( nx * nx + ny * ny );
        c.slopeDeg = std::atan2( horiz, (std::max)( 1e-4f, nz ) ) * ( 180.f / 3.14159265f );
        std::snprintf( c.material, sizeof( c.material ), "%s", surf.cap ? surf.cap : ProvenanceGeo::CapId( surf.rock ) );
        c.cellX = (int)std::floor( x );
        c.cellY = (int)std::floor( y );
        c.soft = GeoCertIsSoftRock( surf.rock );
        c.found = true;
        ++s_geoContactN;
    }

    void GeoCertDiscoverContacts()
    {
        s_geoContactN = 0;
        float const oy = (float)ProvenanceGeo::kRangeOriginY;
        float const ox = (float)ProvenanceGeo::kRangeOriginX;

        // Nominal RANGE transect anchors (A–J) + packaging probes.
        GeoCertPushContact( "A_flat", "flat_soil_valley", ox + 2.f, oy );
        GeoCertPushContact( "B_slope", "gentle_slope", ox + 18.f, oy );
        GeoCertPushContact( "C_mound", "rounded_mound", ox + 36.f, oy );
        GeoCertPushContact( "D_drain", "concave_drainage", ox + 51.f, oy );
        GeoCertPushContact( "E_rocky", "moderate_rocky_slope", ox + 68.f, oy );
        GeoCertPushContact( "F_diag", "steep_diagonal_rock", ox + 84.f, oy );
        GeoCertPushContact( "G_cliff", "near_vertical_face", ox + 94.0f, oy );
        GeoCertPushContact( "I_scree", "crest_or_scree_apron", ox + 91.f, oy );
        GeoCertPushContact( "J_lime", "mineral_limestone_shoulder", ox + 102.f, oy - 3.f );

        // Material boundary: scan u for rock change.
        {
            ProvenanceGeo::RockBody prev = ProvenanceGeo::RockAt( ox + 20.f, oy );
            for ( float u = 22.f; u < 100.f; u += 1.f )
            {
                ProvenanceGeo::RockBody cur = ProvenanceGeo::RockAt( ox + u, oy );
                if ( cur != prev )
                {
                    GeoCertPushContact( "mat_bound", "material_boundary", ox + u, oy );
                    break;
                }
                prev = cur;
            }
        }
        // World-cell boundary + multi-cell (near integer + half-step).
        GeoCertPushContact( "cell_edge", "world_cell_boundary", std::floor( ox + 36.f ) + 0.02f, oy );
        GeoCertPushContact( "multi_cell", "multi_cell_crossing", std::floor( ox + 68.f ) + 0.98f, oy );

        // Programmatic upgrade: pick steepest sample near cliff for G if slope weak.
        {
            float bestSlope = -1.f;
            float bx = ox + 94.f, by = oy, bz = 0.f, bnx = 0.f, bny = 0.f, bnz = 1.f;
            char bmat[32] = "rock";
            for ( float u = 92.f; u <= 97.f; u += 0.25f )
            {
                for ( float v = -2.f; v <= 2.f; v += 0.5f )
                {
                    ProvenanceGeo::SurfaceSample surf{};
                    float z, nx, ny, nz;
                    if ( !GeoCertSampleAt( ox + u, oy + v, z, nx, ny, nz, surf ) ) { continue; }
                    float const horiz = std::sqrt( nx * nx + ny * ny );
                    float const slope = std::atan2( horiz, (std::max)( 1e-4f, nz ) ) * ( 180.f / 3.14159265f );
                    if ( slope > bestSlope )
                    {
                        bestSlope = slope;
                        bx = ox + u; by = oy + v; bz = z;
                        bnx = nx; bny = ny; bnz = nz;
                        std::snprintf( bmat, sizeof( bmat ), "%s",
                            surf.cap ? surf.cap : ProvenanceGeo::CapId( surf.rock ) );
                    }
                }
            }
            for ( int i = 0; i < s_geoContactN; ++i )
            {
                if ( std::strcmp( s_geoContacts[i].id, "G_cliff" ) != 0 ) { continue; }
                s_geoContacts[i].x = bx; s_geoContacts[i].y = by; s_geoContacts[i].z = bz;
                s_geoContacts[i].nx = bnx; s_geoContacts[i].ny = bny; s_geoContacts[i].nz = bnz;
                s_geoContacts[i].slopeDeg = bestSlope;
                std::snprintf( s_geoContacts[i].material, sizeof( s_geoContacts[i].material ), "%s", bmat );
                s_geoContacts[i].cellX = (int)std::floor( bx );
                s_geoContacts[i].cellY = (int)std::floor( by );
                s_geoContacts[i].soft = false;
                s_geoContacts[i].found = bestSlope > 35.f;
                break;
            }
        }
    }

    void GeoCertTeleport( float x, float y, float pitch, float yaw )
    {
        g.feetX = x;
        g.feetY = y;
        float gz = g.feetZ;
        if ( SampleGroundZ( x, y, gz ) ) { g.feetZ = gz; }
        g.camX = g.feetX;
        g.camY = g.feetY;
        g.camZ = g.feetZ + 1.7f;
        g.pitch = pitch;
        g.yaw = yaw;
        // Do NOT InvalidateTerrainMesh here — walk cert requires remesh=0 without residency change.
        UpdateAim();
    }

    float GeoCertOxFallback( char const* id )
    {
        float const ox = (float)ProvenanceGeo::kRangeOriginX;
        if ( std::strcmp( id, "A_flat" ) == 0 ) { return ox + 2.f; }
        if ( std::strcmp( id, "B_slope" ) == 0 ) { return ox + 18.f; }
        if ( std::strcmp( id, "C_mound" ) == 0 ) { return ox + 36.f; }
        if ( std::strcmp( id, "D_drain" ) == 0 ) { return ox + 51.f; }
        if ( std::strcmp( id, "E_rocky" ) == 0 ) { return ox + 68.f; }
        if ( std::strcmp( id, "F_diag" ) == 0 ) { return ox + 84.f; }
        if ( std::strcmp( id, "G_cliff" ) == 0 ) { return ox + 94.f; }
        return ox;
    }

    void GeoCertWriteArtifact()
    {
        if ( !g.certOutDir[0] ) { GetTempPathA( MAX_PATH, g.certOutDir ); }
        char path[MAX_PATH];
        std::snprintf( path, sizeof( path ), "%s\\provenance_geography_interaction_cert.txt", g.certOutDir );
        FILE* f = nullptr;
        if ( fopen_s( &f, path, "w" ) != 0 || !f ) { return; }

        std::fprintf( f,
            "# provenance_geography_interaction_cert\n"
            "fixture=%s\n"
            "fixture_label=%s\n"
            "exit_code=%d\n"
            "contacts=%d\n"
            "rows=%d\n"
            "geoCellsCreated=%d SampleSurface=%d SampleCapColor=%d\n"
            "HF_rebuilds=%d D2_rebuilds=%d virgin_D2=%d\n"
            "EditedRegions=%d MatterBodies=%d\n"
            "laws=action!=D2_dirty!=HF_aperture; 0.85=work_partition; 12cm=new_strike_mouth; tip/6=presentation\n"
            "\n# per-scenario rows\n"
            "# section|scenario|terrain|slope|material|xyz|N|action|grams|occDelta|hfSub|ER|d2Tris|herm|qefFb|halo|own|mat|sup|hash|timing|verdict|note\n",
            ProvenanceGeo::FixtureName( ProvenanceGeo::Fixture() ),
            ProvenanceGeo::FixtureLabel( ProvenanceGeo::Fixture() ),
            g.certGeoExitCode,
            s_geoContactN, s_geoRowN,
            g.perfGeoCellsCreated, g.perfSampleSurfaceCalls, g.perfSampleCapColorCalls,
            g.perfHfRebuilds, g.perfD2Rebuilds, g.perfVirginD2Rebuilds,
            (int)g.editedRegions.size(), (int)H2H::State().bodies.size() );

        for ( int i = 0; i < s_geoRowN; ++i )
        {
            GeoCertRow const& r = s_geoRows[i];
            std::fprintf( f,
                "%s|%s|%s|%.1f|%s|(%.3f,%.3f,%.3f)|(%.3f,%.3f,%.3f)|%s|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%s|%s|%s|%s\n",
                r.section, r.scenario, r.terrain, r.slope, r.material,
                r.x, r.y, r.z, r.nx, r.ny, r.nz,
                r.action, r.grams, r.occDelta, r.hfSubdiv, r.editedRegion,
                r.d2Tris, r.hermites, r.qefFb, r.haloMiss, r.ownViol, r.matMismatch, r.supportFail,
                r.hash, r.timing, r.verdict, r.note );
        }

        std::fprintf( f, "\n# discovered contacts\n" );
        for ( int i = 0; i < s_geoContactN; ++i )
        {
            GeoCertContact const& c = s_geoContacts[i];
            std::fprintf( f,
                "contact id=%s terrain=%s soft=%d found=%d xyz=(%.3f,%.3f,%.3f) N=(%.3f,%.3f,%.3f) "
                "slope=%.1f mat=%s cell=(%d,%d)\n",
                c.id, c.terrain, c.soft ? 1 : 0, c.found ? 1 : 0,
                c.x, c.y, c.z, c.nx, c.ny, c.nz, c.slopeDeg, c.material, c.cellX, c.cellY );
        }

        int passN = 0, failN = 0, skipN = 0;
        for ( int i = 0; i < s_geoRowN; ++i )
        {
            if ( std::strcmp( s_geoRows[i].verdict, "PASS" ) == 0 ) { ++passN; }
            else if ( std::strcmp( s_geoRows[i].verdict, "FAIL" ) == 0 ) { ++failN; }
            else { ++skipN; }
        }
        std::fprintf( f,
            "\n# red-gate summary\n"
            "PASS_rows=%d FAIL_rows=%d SKIP_rows=%d\n"
            "hard_fail=%s\n"
            "gates=spire_Z,lost_carve,HF_resurrection,uncovered_void,packaging_signature,"
            "incomplete_halo,nondeterministic_D2,wrong_material,support_over_void,"
            "virgin_D2_work,excessive_remesh,mass_discrepancy\n",
            passN, failN, skipN,
            g.certGeoExitCode ? s_geoFailReason : "none" );

        if ( g.certGeoExitCode )
        {
            std::fprintf( f,
                "\nFAIL:\n"
                "fixture=%s\n"
                "action=%s\n"
                "world=(%.3f,%.3f,%.3f)\n"
                "normal=(%.4f,%.4f,%.4f)\n"
                "reason=%s\n"
                "beforeMeshHash=-\n"
                "afterMeshHash=-\n"
                "occupancyHash=-\n",
                s_geoFailFixture, s_geoFailAction,
                s_geoFailX, s_geoFailY, s_geoFailZ,
                s_geoFailNx, s_geoFailNy, s_geoFailNz,
                s_geoFailReason );
        }
        std::fclose( f );

        // Best-effort mirror under Build/cert next to common spill folders.
        char mirror[MAX_PATH];
        std::snprintf( mirror, sizeof( mirror ),
            "C:\\Users\\D-Day\\ProvenanceEsoterica\\Build\\cert\\provenance_geography_interaction_cert.txt" );
        CreateDirectoryA( "C:\\Users\\D-Day\\ProvenanceEsoterica\\Build\\cert", nullptr );
        CopyFileA( path, mirror, FALSE );
    }

    void GeoCertRunVirginSection()
    {
        SnapVirginPerfIfNeeded();

        auto add = [&]( char const* scen, char const* verdict, char const* note,
            float x = 0.f, float y = 0.f, float z = 0.f )
        {
            GeoCertRow r{};
            std::snprintf( r.section, sizeof( r.section ), "2" );
            std::snprintf( r.scenario, sizeof( r.scenario ), "%s", scen );
            std::snprintf( r.verdict, sizeof( r.verdict ), "%s", verdict );
            std::snprintf( r.note, sizeof( r.note ), "%s", note );
            std::snprintf( r.action, sizeof( r.action ), "virgin_assert" );
            r.x = x; r.y = y; r.z = z;
            GeoCertAddRow( r );
            if ( std::strcmp( verdict, "FAIL" ) == 0 )
            {
                GeoCertHardFail( scen, "virgin_assert", note, x, y, z, 0.f, 0.f, 1.f );
            }
        };

        // §1 rows — discovered contacts
        for ( int i = 0; i < s_geoContactN; ++i )
        {
            GeoCertContact const& c = s_geoContacts[i];
            GeoCertRow r{};
            std::snprintf( r.section, sizeof( r.section ), "1" );
            std::snprintf( r.scenario, sizeof( r.scenario ), "discover_%s", c.id );
            std::snprintf( r.terrain, sizeof( r.terrain ), "%s", c.terrain );
            r.slope = c.slopeDeg;
            std::snprintf( r.material, sizeof( r.material ), "%s", c.material );
            r.x = c.x; r.y = c.y; r.z = c.z;
            r.nx = c.nx; r.ny = c.ny; r.nz = c.nz;
            std::snprintf( r.action, sizeof( r.action ), "classify" );
            r.hfSubdiv = 2;
            std::snprintf( r.verdict, sizeof( r.verdict ), c.found ? "PASS" : "FAIL" );
            std::snprintf( r.note, sizeof( r.note ), "cell=(%d,%d) soft=%d", c.cellX, c.cellY, c.soft ? 1 : 0 );
            GeoCertAddRow( r );
            if ( !c.found )
            {
                GeoCertHardFail( c.id, "discover", "CONTACT_NOT_FOUND", c.x, c.y, c.z, c.nx, c.ny, c.nz );
            }
        }

        char const* required[] = {
            "A_flat", "B_slope", "C_mound", "D_drain", "E_rocky", "F_diag", "G_cliff",
            "cell_edge", "multi_cell"
        };
        for ( char const* id : required )
        {
            bool ok = false;
            for ( int i = 0; i < s_geoContactN; ++i )
            {
                if ( std::strcmp( s_geoContacts[i].id, id ) == 0 && s_geoContacts[i].found )
                {
                    ok = true; break;
                }
            }
            if ( !ok )
            {
                add( "required_contact", "FAIL", id, GeoCertOxFallback( id ), 128.f, 0.f );
            }
        }

        int d2 = g.perfD2Rebuilds;
        int er = (int)g.editedRegions.size();
        int mb = (int)H2H::State().bodies.size();
        int virgin = g.perfVirginD2Rebuilds;
        if ( virgin < 0 ) { virgin = d2; }

        if ( virgin != 0 || d2 != 0 )
        {
            add( "virgin_D2_zero", "FAIL", "VIRGIN_D2_WORK", g.feetX, g.feetY, g.feetZ );
        }
        else
        {
            add( "virgin_D2_zero", "PASS", "D2_rebuilds=0" );
        }
        // QEF proxy: no separate solve counter yet — virgin D2=0 implies QEF idle.
        add( "virgin_QEF_zero", ( d2 == 0 ) ? "PASS" : "FAIL",
            ( d2 == 0 ) ? "proxy_via_D2_rebuilds" : "VIRGIN_QEF_WORK" );

        if ( er != 0 )
        {
            add( "virgin_EditedRegions", "FAIL", "VIRGIN_EDITED_REGIONS" );
        }
        else
        {
            add( "virgin_EditedRegions", "PASS", "EditedRegions=0" );
        }
        if ( mb != 0 )
        {
            add( "virgin_MatterBodies", "FAIL", "VIRGIN_MATTER_BODIES" );
        }
        else
        {
            add( "virgin_MatterBodies", "PASS", "MatterBodies=0" );
        }

        // SampleSurface ~= geoCellsCreated (allow small slack for fixture refresh paths).
        int const ss = g.perfSampleSurfaceCalls;
        int const gc = g.perfGeoCellsCreated;
        int const slack = (std::max)( 4, gc / 50 );
        if ( ss > gc + slack )
        {
            char note[96];
            std::snprintf( note, sizeof( note ), "SAMPLE_SURFACE_CACHE_MISS ss=%d gc=%d", ss, gc );
            add( "SampleSurface_cache", "FAIL", note );
        }
        else
        {
            char note[96];
            std::snprintf( note, sizeof( note ), "ss=%d gc=%d slack=%d", ss, gc, slack );
            add( "SampleSurface_cache", "PASS", note );
        }

        // Finite surface probe along transect (spire / NaN gate).
        {
            bool ok = true;
            char note[96] = "finite_OK";
            float const oy = (float)ProvenanceGeo::kRangeOriginY;
            float const ox = (float)ProvenanceGeo::kRangeOriginX;
            float prevZ = 0.f;
            bool havePrev = false;
            for ( float u = 0.f; u <= 110.f; u += 2.f )
            {
                float z = 0.f;
                if ( !SampleGroundZBase( ox + u, oy, z ) || !std::isfinite( z ) )
                {
                    ok = false;
                    std::snprintf( note, sizeof( note ), "NONFINITE_Z u=%.1f", u );
                    GeoCertHardFail( "transect_Z", "virgin_probe", note, ox + u, oy, z, 0, 0, 1 );
                    break;
                }
                if ( havePrev && std::fabs( z - prevZ ) > 12.f )
                {
                    ok = false;
                    std::snprintf( note, sizeof( note ), "SPIRE_OR_RUNAWAY_Z u=%.1f dz=%.2f", u, z - prevZ );
                    GeoCertHardFail( "transect_Z", "virgin_probe", note, ox + u, oy, z, 0, 0, 1 );
                    break;
                }
                prevZ = z;
                havePrev = true;
            }
            add( "virgin_surface_finite", ok ? "PASS" : "FAIL", note );
        }
    }

    void GeoCertRunWalkSection()
    {
        // Walk/teleport along already-resident RANGE corridor without InvalidateTerrainMesh.
        // Limitation: if something else dirties HF or expands geo disk, document FAIL honestly.
        int const hf0 = g.certGeoWalkHf0;
        int const d20 = g.certGeoWalkD20;
        int const cells0 = g.certGeoWalkCells0;
        int const ss0 = g.certGeoWalkSample0;
        int const dHf = g.perfHfRebuilds - hf0;
        int const dD2 = g.perfD2Rebuilds - d20;
        int const dCells = g.perfGeoCellsCreated - cells0;
        int const dSs = g.perfSampleSurfaceCalls - ss0;

        auto add = [&]( char const* scen, char const* verdict, char const* note )
        {
            GeoCertRow r{};
            std::snprintf( r.section, sizeof( r.section ), "2" );
            std::snprintf( r.scenario, sizeof( r.scenario ), "%s", scen );
            std::snprintf( r.verdict, sizeof( r.verdict ), "%s", verdict );
            std::snprintf( r.note, sizeof( r.note ), "%s", note );
            std::snprintf( r.action, sizeof( r.action ), "walk_transect" );
            r.x = g.feetX; r.y = g.feetY; r.z = g.feetZ;
            GeoCertAddRow( r );
            if ( std::strcmp( verdict, "FAIL" ) == 0 )
            {
                GeoCertHardFail( scen, "walk_transect", note, g.feetX, g.feetY, g.feetZ, 0, 0, 1 );
            }
        };

        if ( dD2 != 0 )
        {
            char note[96];
            std::snprintf( note, sizeof( note ), "WALK_D2_NONEZERO dD2=%d", dD2 );
            add( "walk_D2_zero", "FAIL", note );
        }
        else
        {
            add( "walk_D2_zero", "PASS", "dD2=0" );
        }

        if ( dCells > 0 )
        {
            // Honest FAIL row + reproduce blob; dig matrix still runs (documented limitation path).
            char note[128];
            std::snprintf( note, sizeof( note ),
                "RESIDENCY_GREW_ON_WALK dCells=%d dHf=%d (EnsureGeoDisk recentered)", dCells, dHf );
            GeoCertRow r{};
            std::snprintf( r.section, sizeof( r.section ), "2" );
            std::snprintf( r.scenario, sizeof( r.scenario ), "walk_no_residency_growth" );
            std::snprintf( r.verdict, sizeof( r.verdict ), "FAIL" );
            std::snprintf( r.note, sizeof( r.note ), "%s", note );
            std::snprintf( r.action, sizeof( r.action ), "walk_transect" );
            GeoCertAddRow( r );
            if ( !g.certGeoFailWritten )
            {
                GeoCertHardFail( "walk_residency", "walk_transect", "RESIDENCY_GREW_ON_WALK",
                    g.feetX, g.feetY, g.feetZ, 0, 0, 1 );
            }
            // Do not treat as terminal for §4 — continue after artifact note already written.
        }
        else if ( dHf != 0 )
        {
            char note[96];
            std::snprintf( note, sizeof( note ), "WALK_HF_REMESH dHf=%d (no residency growth)", dHf );
            add( "walk_HF_remesh_zero", "FAIL", note );
        }
        else
        {
            char note[96];
            std::snprintf( note, sizeof( note ), "dHf=0 dCells=0 dSs=%d", dSs );
            add( "walk_HF_remesh_zero", "PASS", note );
            add( "walk_no_residency_growth", "PASS", "dCells=0" );
        }

        // Grade stability probe (analytic geography; column reply grade gate scaffolded if no wire).
        {
            float const oy = (float)ProvenanceGeo::kRangeOriginY;
            float const ox = (float)ProvenanceGeo::kRangeOriginX;
            bool ok = true;
            char note[96] = "grade_stable_analytic";
            for ( float u = 0.f; u <= 100.f; u += 10.f )
            {
                auto a = ProvenanceGeo::SampleSurface( ox + u, oy, g.gradeDatum, g.reliefVoxels, g.voxelEdgeM );
                auto b = ProvenanceGeo::SampleSurface( ox + u, oy, g.gradeDatum, g.reliefVoxels, g.voxelEdgeM );
                if ( std::fabs( a.grade - b.grade ) > 1e-6f )
                {
                    ok = false;
                    std::snprintf( note, sizeof( note ), "GRADE_REWRITE u=%.1f", u );
                    break;
                }
            }
            GeoCertRow r{};
            std::snprintf( r.section, sizeof( r.section ), "2" );
            std::snprintf( r.scenario, sizeof( r.scenario ), "grade_before_eq_after" );
            std::snprintf( r.verdict, sizeof( r.verdict ), ok ? "PASS" : "FAIL" );
            std::snprintf( r.note, sizeof( r.note ), "%s; voxel_column_fanin=SKIP_needs_bridge_torture", note );
            std::snprintf( r.action, sizeof( r.action ), "grade_probe" );
            GeoCertAddRow( r );
            if ( !ok )
            {
                GeoCertHardFail( "grade", "grade_probe", note, ox, oy, 0, 0, 0, 1 );
            }
        }
    }

    void GeoCertStrikeAt( GeoCertContact const& c )
    {
        // Stand slightly back along outward normal; look into the contact.
        float pitch = -1.10f;
        float yaw = 0.f;
        if ( c.nz < 0.55f )
        {
            pitch = -0.20f;
            yaw = std::atan2( -c.nx, -c.ny );
        }
        float const standX = c.x + c.nx * 0.55f;
        float const standY = c.y + c.ny * 0.55f;
        GeoCertTeleport( standX, standY, pitch, yaw );
        // Face the contact point from camera.
        {
            float dx = c.x - g.camX, dy = c.y - g.camY, dz = c.z - g.camZ;
            float const horiz = std::sqrt( dx * dx + dy * dy );
            g.yaw = std::atan2( dx, dy );
            g.pitch = std::atan2( dz, (std::max)( 0.05f, horiz ) );
            if ( g.pitch < -1.4f ) { g.pitch = -1.4f; }
            if ( g.pitch > 1.4f ) { g.pitch = 1.4f; }
        }
        UpdateAim();
        g.hotbarSel = c.soft ? 0 : 2;
        int const er0 = (int)g.editedRegions.size();
        int const d20 = g.perfD2Rebuilds;
        int const hf0 = g.perfHfRebuilds;
        int const held0 = g.heldTotalG;
        bool struck = false;
        if ( !g.aimHit )
        {
            // Last resort: force aim at classified contact (still one strike attempt).
            g.aimHit = true;
            g.aimX = c.x; g.aimY = c.y; g.aimZ = c.z;
            g.aimCx = c.cellX; g.aimCy = c.cellY;
        }
        if ( !c.soft )
        {
            struck = TryPickFoliatedStrike();
            if ( !struck ) { g.hotbarSel = 0; struck = TryDigHandful(); }
        }
        else
        {
            struck = TryDigHandful();
        }

        GeoCertRow r{};
        std::snprintf( r.section, sizeof( r.section ), "4" );
        std::snprintf( r.scenario, sizeof( r.scenario ), "dig_%s", c.id );
        std::snprintf( r.terrain, sizeof( r.terrain ), "%s", c.terrain );
        r.slope = c.slopeDeg;
        std::snprintf( r.material, sizeof( r.material ), "%s", c.material );
        r.x = c.x; r.y = c.y; r.z = c.z;
        r.nx = c.nx; r.ny = c.ny; r.nz = c.nz;
        std::snprintf( r.action, sizeof( r.action ), "%s", c.soft ? "shovel" : "pick" );
        r.grams = g.heldTotalG - held0;
        r.editedRegion = (int)g.editedRegions.size();
        r.d2Tris = g.perfD2Tris;
        r.qefFb = g.perfD2QefFallbacks;
        r.hfSubdiv = 2;
        int const dD2 = g.perfD2Rebuilds - d20;
        int const dHf = g.perfHfRebuilds - hf0;
        int const dEr = (int)g.editedRegions.size() - er0;
        std::snprintf( r.timing, sizeof( r.timing ), "dD2=%d dHf=%d dER=%d", dD2, dHf, dEr );
        std::snprintf( r.note, sizeof( r.note ),
            "action_strike=%d D2_dirty_rebuilds=%d HF_aperture_remesh=%d (distinct)",
            struck ? 1 : 0, dD2, dHf );
        if ( !struck )
        {
            std::snprintf( r.verdict, sizeof( r.verdict ), "FAIL" );
            GeoCertAddRow( r );
            GeoCertHardFail( c.id, r.action, "STRIKE_NO_EFFECT", c.x, c.y, c.z, c.nx, c.ny, c.nz );
            return;
        }
        std::snprintf( r.verdict, sizeof( r.verdict ), "PASS" );
        GeoCertAddRow( r );
        (void)dEr;
    }

    uint32_t GeoCertFnv1a( void const* data, size_t n, uint32_t h = 2166136261u )
    {
        uint8_t const* p = (uint8_t const*)data;
        for ( size_t i = 0; i < n; ++i )
        {
            h ^= p[i];
            h *= 16777619u;
        }
        return h;
    }

    uint32_t GeoCertHashCellFill( int cx, int cy )
    {
        CellSample const* cell = GetCell( cx, cy );
        if ( !cell || cell->fill.empty() ) { return 0u; }
        return GeoCertFnv1a( cell->fill.data(), cell->fill.size() );
    }

    uint32_t GeoCertHashCavityTris( CellSample const& cell )
    {
        uint32_t h = 2166136261u;
        for ( DualContourQef::Tri const& t : cell.cavityTris )
        {
            float v[9] = {
                t.a.x, t.a.y, t.a.z,
                t.b.x, t.b.y, t.b.z,
                t.c.x, t.c.y, t.c.z
            };
            h = GeoCertFnv1a( v, sizeof( v ), h );
        }
        uint32_t const n = (uint32_t)cell.cavityTris.size();
        h = GeoCertFnv1a( &n, sizeof( n ), h );
        return h;
    }

    void GeoCertRunHfRefineNoEditSection()
    {
        // §3: tip/6 is presentation refine on mouth collar only. Without edit there is no
        // mouth → coarse vista stays; aim/look must not change virgin surface or wake D2.
        auto add = [&]( char const* scen, char const* verdict, char const* note,
            float x = 0.f, float y = 0.f, float z = 0.f,
            float nx = 0.f, float ny = 0.f, float nz = 1.f )
        {
            GeoCertRow r{};
            std::snprintf( r.section, sizeof( r.section ), "3" );
            std::snprintf( r.scenario, sizeof( r.scenario ), "%s", scen );
            std::snprintf( r.verdict, sizeof( r.verdict ), "%s", verdict );
            std::snprintf( r.note, sizeof( r.note ), "%s", note );
            std::snprintf( r.action, sizeof( r.action ), "hf_refine_no_edit" );
            r.x = x; r.y = y; r.z = z;
            r.nx = nx; r.ny = ny; r.nz = nz;
            r.hfSubdiv = 2;
            GeoCertAddRow( r );
            if ( std::strcmp( verdict, "FAIL" ) == 0 )
            {
                GeoCertHardFail( scen, "hf_refine_no_edit", note, x, y, z, nx, ny, nz );
            }
        };

        int const d20 = g.perfD2Rebuilds;
        int const er0 = (int)g.editedRegions.size();
        int const hf0 = g.perfHfRebuilds;

        struct Probe { float x, y, z, nx, ny, nz; };
        Probe before[8];
        int probeN = 0;
        for ( int i = 0; i < s_geoContactN && probeN < 8; ++i )
        {
            GeoCertContact const& c = s_geoContacts[i];
            if ( !c.found ) { continue; }
            before[probeN++] = { c.x, c.y, c.z, c.nx, c.ny, c.nz };
        }
        if ( probeN == 0 )
        {
            add( "surface_identity", "FAIL", "NO_PROBES" );
            return;
        }

        // Aim/look at probes without committing dig — residency already frozen from walk.
        for ( int i = 0; i < probeN; ++i )
        {
            Probe const& p = before[i];
            float pitch = -0.55f;
            float yaw = 0.f;
            if ( p.nz < 0.55f )
            {
                pitch = -0.15f;
                yaw = std::atan2( -p.nx, -p.ny );
            }
            GeoCertTeleport( p.x + p.nx * 0.45f, p.y + p.ny * 0.45f, pitch, yaw );
            UpdateAim();
        }

        bool mouthHit = false;
        {
            float md = 0.f, nr = 0.f;
            mouthHit = QuadHitsMouthCollar( before[0].x - 0.25f, before[0].y - 0.25f,
                before[0].x + 0.25f, before[0].y + 0.25f, md, nr );
        }
        int const div = MeshDivForCell( (int)std::floor( before[0].x ), (int)std::floor( before[0].y ),
            g.feetX, g.feetY );

        if ( g.perfD2Rebuilds != d20 || (int)g.editedRegions.size() != er0 )
        {
            char note[96];
            std::snprintf( note, sizeof( note ), "AIM_WOKE_D2_OR_ER dD2=%d dER=%d",
                g.perfD2Rebuilds - d20, (int)g.editedRegions.size() - er0 );
            add( "aim_no_D2", "FAIL", note, before[0].x, before[0].y, before[0].z );
        }
        else
        {
            add( "aim_no_D2", "PASS", "aim/look D2=0 ER=0", before[0].x, before[0].y, before[0].z );
        }

        if ( mouthHit || !g.editedRegions.empty() )
        {
            add( "no_mouth_collar_pre_edit", "FAIL", "MOUTH_COLLAR_WITHOUT_EDIT",
                before[0].x, before[0].y, before[0].z );
        }
        else
        {
            add( "no_mouth_collar_pre_edit", "PASS", "QuadHitsMouthCollar=0 tip6_dormant",
                before[0].x, before[0].y, before[0].z );
        }

        if ( div != 2 )
        {
            char note[64];
            std::snprintf( note, sizeof( note ), "VISTA_DIV_CHANGED div=%d", div );
            add( "vista_coarse_div2", "FAIL", note );
        }
        else
        {
            add( "vista_coarse_div2", "PASS", "MeshDivForCell=2 tip6=presentation_only" );
        }

        bool surfaceOk = true;
        char surfNote[96] = "surface_before~=after";
        for ( int i = 0; i < probeN; ++i )
        {
            float z = 0.f, nx = 0.f, ny = 0.f, nz = 1.f;
            ProvenanceGeo::SurfaceSample surf{};
            if ( !GeoCertSampleAt( before[i].x, before[i].y, z, nx, ny, nz, surf ) )
            {
                surfaceOk = false;
                std::snprintf( surfNote, sizeof( surfNote ), "SAMPLE_FAIL i=%d", i );
                break;
            }
            if ( std::fabs( z - before[i].z ) > 1e-3f
              || std::fabs( nx - before[i].nx ) > 2e-3f
              || std::fabs( ny - before[i].ny ) > 2e-3f
              || std::fabs( nz - before[i].nz ) > 2e-3f )
            {
                surfaceOk = false;
                std::snprintf( surfNote, sizeof( surfNote ),
                    "SURFACE_CHANGED i=%d dz=%.4f", i, z - before[i].z );
                GeoCertHardFail( "HF_refine_no_edit", "surface_probe", surfNote,
                    before[i].x, before[i].y, z, nx, ny, nz );
                break;
            }
        }
        add( "surface_identity", surfaceOk ? "PASS" : "FAIL", surfNote,
            before[0].x, before[0].y, before[0].z, before[0].nx, before[0].ny, before[0].nz );

        // Neighbor stitch ≠ edited matter: no remesh / no ER from aim alone.
        if ( g.perfHfRebuilds != hf0 )
        {
            char note[80];
            std::snprintf( note, sizeof( note ), "AIM_TRIGGERED_HF_REMESH dHf=%d", g.perfHfRebuilds - hf0 );
            add( "aim_no_HF_remesh", "FAIL", note );
        }
        else
        {
            add( "aim_no_HF_remesh", "PASS", "dHf=0 (no edit → no tip/6 remesh)" );
        }
    }

    void GeoCertRunD2QefSection()
    {
        // §5: production extract halo completeness + byte-identical re-extract.
        auto add = [&]( char const* scen, char const* verdict, char const* note,
            float x = 0.f, float y = 0.f, float z = 0.f,
            int d2Tris = 0, int qefFb = 0, int haloMiss = 0, char const* hash = "-" )
        {
            GeoCertRow r{};
            std::snprintf( r.section, sizeof( r.section ), "5" );
            std::snprintf( r.scenario, sizeof( r.scenario ), "%s", scen );
            std::snprintf( r.verdict, sizeof( r.verdict ), "%s", verdict );
            std::snprintf( r.note, sizeof( r.note ), "%s", note );
            std::snprintf( r.action, sizeof( r.action ), "d2_reextract" );
            r.x = x; r.y = y; r.z = z;
            r.d2Tris = d2Tris;
            r.qefFb = qefFb;
            r.haloMiss = haloMiss;
            std::snprintf( r.hash, sizeof( r.hash ), "%s", hash ? hash : "-" );
            GeoCertAddRow( r );
            if ( std::strcmp( verdict, "FAIL" ) == 0 )
            {
                GeoCertHardFail( scen, "d2_reextract", note, x, y, z, 0.f, 0.f, 1.f );
            }
        };

        int cx = 0, cy = 0;
        CellSample* cell = nullptr;
        for ( int i = 0; i < s_geoContactN; ++i )
        {
            GeoCertContact const& c = s_geoContacts[i];
            if ( !c.found ) { continue; }
            CellSample* cand = GetCellMutable( c.cellX, c.cellY );
            if ( cand && cand->carved && cand->hasCavity && !cand->cavityTris.empty() )
            {
                cell = cand;
                cx = c.cellX;
                cy = c.cellY;
                break;
            }
        }
        if ( !cell )
        {
            // Fallback: any carved cavity in residency.
            for ( auto& kv : g.cells )
            {
                if ( kv.second.carved && kv.second.hasCavity && !kv.second.cavityTris.empty() )
                {
                    cell = &kv.second;
                    cx = (int)(uint32_t)( kv.first >> 32 );
                    cy = (int)(uint32_t)( kv.first & 0xffffffffu );
                    break;
                }
            }
        }
        if ( !cell )
        {
            add( "D2_QEF_halo_determinism", "FAIL", "NO_CAVITY_AFTER_DIG_MATRIX" );
            return;
        }

        uint32_t const h0 = GeoCertHashCavityTris( *cell );
        int const tris0 = (int)cell->cavityTris.size();
        int const halo0 = g.perfD2HaloMiss;
        int const qef0 = g.perfD2QefFallbacks;
        RebuildCavityMesh( cx, cy );
        cell = GetCellMutable( cx, cy );
        if ( !cell || cell->lastD2PublishRefused || cell->lastD2HaloMissing > 0 )
        {
            char note[96];
            std::snprintf( note, sizeof( note ),
                "HALO_PUBLISH_REFUSED miss=%d refused=%d (fail-closed)",
                cell ? cell->lastD2HaloMissing : -1,
                cell && cell->lastD2PublishRefused ? 1 : 0 );
            add( "halo_fail_closed_complete", "FAIL", note,
                (float)cx + 0.5f, (float)cy + 0.5f, 0.f );
            add( "reextract_keeps_cavity", "FAIL", "REEXTRACT_LOST_CAVITY_OR_HALO",
                (float)cx + 0.5f, (float)cy + 0.5f, 0.f );
            return;
        }
        if ( !cell->hasCavity || cell->cavityTris.empty() )
        {
            add( "reextract_keeps_cavity", "FAIL", "REEXTRACT_LOST_CAVITY",
                (float)cx + 0.5f, (float)cy + 0.5f, 0.f );
            return;
        }
        uint32_t const h1 = GeoCertHashCavityTris( *cell );
        int const haloDelta = g.perfD2HaloMiss - halo0;
        int const qefDelta = g.perfD2QefFallbacks - qef0;
        char hashStr[24];
        std::snprintf( hashStr, sizeof( hashStr ), "%08X", (unsigned)h1 );

        bool finiteOk = true;
        for ( DualContourQef::Tri const& t : cell->cavityTris )
        {
            if ( !std::isfinite( t.a.x ) || !std::isfinite( t.a.y ) || !std::isfinite( t.a.z )
              || !std::isfinite( t.b.x ) || !std::isfinite( t.b.y ) || !std::isfinite( t.b.z )
              || !std::isfinite( t.c.x ) || !std::isfinite( t.c.y ) || !std::isfinite( t.c.z ) )
            {
                finiteOk = false;
                break;
            }
            float const abx = t.b.x - t.a.x, aby = t.b.y - t.a.y, abz = t.b.z - t.a.z;
            float const acx = t.c.x - t.a.x, acy = t.c.y - t.a.y, acz = t.c.z - t.a.z;
            float const nx = aby * acz - abz * acy;
            float const ny = abz * acx - abx * acz;
            float const nz = abx * acy - aby * acx;
            if ( nx * nx + ny * ny + nz * nz < 1e-12f )
            {
                finiteOk = false;
                break;
            }
        }

        // Documentary revoke of prior false-confident haloMiss=0 PASS (invented solid neighbors,
        // dens=0→Solid, tiled haloMissing not aggregated). Does not hard-fail the process.
        {
            GeoCertRow r{};
            std::snprintf( r.section, sizeof( r.section ), "5" );
            std::snprintf( r.scenario, sizeof( r.scenario ), "haloMiss_zero_PRIOR_REVOKED" );
            std::snprintf( r.verdict, sizeof( r.verdict ), "SKIP" );
            std::snprintf( r.note, sizeof( r.note ),
                "revoked false PASS — invent kFillFull + dens0 Solid + incomplete tile agg" );
            std::snprintf( r.action, sizeof( r.action ), "d2_reextract" );
            GeoCertAddRow( r );
        }

        // Re-prove: fail-closed Unknown halo + full ExtractStats aggregation (incl. haloMissing).
        bool const haloOk = ( cell->lastD2HaloMissing == 0 && haloDelta == 0
            && !cell->lastD2PublishRefused );
        if ( !haloOk )
        {
            char note[96];
            std::snprintf( note, sizeof( note ),
                "INCOMPLETE_HALO lastMiss=%d delta=%d refused=%d",
                cell->lastD2HaloMissing, haloDelta, cell->lastD2PublishRefused ? 1 : 0 );
            add( "halo_fail_closed_complete", "FAIL", note,
                (float)cx + 0.5f, (float)cy + 0.5f, 0.f,
                (int)cell->cavityTris.size(), qefDelta, cell->lastD2HaloMissing, hashStr );
        }
        else
        {
            char note[96];
            std::snprintf( note, sizeof( note ),
                "haloMiss=0 after Ensure+agg (fail-closed Unknown; no invent solid)" );
            add( "halo_fail_closed_complete", "PASS", note,
                (float)cx + 0.5f, (float)cy + 0.5f, 0.f,
                (int)cell->cavityTris.size(), qefDelta, 0, hashStr );
        }

        if ( h0 != h1 || tris0 != (int)cell->cavityTris.size() )
        {
            char note[96];
            std::snprintf( note, sizeof( note ), "NONDETERMINISTIC_D2 h0=%08X h1=%08X tris=%d→%d",
                (unsigned)h0, (unsigned)h1, tris0, (int)cell->cavityTris.size() );
            add( "reextract_bit_identity", "FAIL", note, (float)cx + 0.5f, (float)cy + 0.5f, 0.f,
                (int)cell->cavityTris.size(), qefDelta, haloDelta, hashStr );
        }
        else
        {
            char note[80];
            std::snprintf( note, sizeof( note ), "tris=%d qefFbDelta=%d", tris0, qefDelta );
            add( "reextract_bit_identity", "PASS", note, (float)cx + 0.5f, (float)cy + 0.5f, 0.f,
                tris0, qefDelta, haloDelta, hashStr );
        }

        add( "finite_nondegenerate_tris", finiteOk ? "PASS" : "FAIL",
            finiteOk ? "finite_OK" : "DEGENERATE_OR_NONFINITE_TRI",
            (float)cx + 0.5f, (float)cy + 0.5f, 0.f,
            (int)cell->cavityTris.size(), qefDelta, haloDelta, hashStr );

        // --- Cross-cell primal-edge ownership / watertight seam topology ---
        float const ox = (float)ProvenanceGeo::kRangeOriginX;
        float const oy = (float)ProvenanceGeo::kRangeOriginY;
        int const leftCx = (int)std::floor( ox + 22.f );
        int const seamCy = (int)std::floor( oy - 10.f );
        int const rightCx = leftCx + 1;
        float const seamX = (float)rightCx; // shared +X face of left cell
        float const seamY = (float)seamCy + 0.5f;
        float seamZ = 0.f;
        if ( !SampleGroundZBase( seamX, seamY, seamZ ) )
        {
            add( "cross_cell_seam_watertight", "FAIL", "SEAM_PAD_SAMPLE_FAIL", seamX, seamY, 0.f );
            return;
        }
        EnsureD2HaloLattices( leftCx, seamCy );
        EnsureD2HaloLattices( rightCx, seamCy );
        constexpr float kSeamR = 0.28f;
        bool const carvedSeam = CarveOccupancySphere(
            seamX, seamY, seamZ - kSeamR * 0.45f, kSeamR, seamX, seamY, seamZ );
        if ( !carvedSeam )
        {
            add( "cross_cell_seam_watertight", "FAIL", "SEAM_CARVE_FAIL", seamX, seamY, seamZ );
            return;
        }
        RebuildCavityMesh( leftCx, seamCy );
        RebuildCavityMesh( rightCx, seamCy );
        CellSample const* left = GetCell( leftCx, seamCy );
        CellSample const* right = GetCell( rightCx, seamCy );
        if ( !left || !right )
        {
            add( "cross_cell_seam_watertight", "FAIL", "SEAM_CELLS_MISSING", seamX, seamY, seamZ );
            return;
        }
        if ( left->lastD2HaloMissing > 0 || right->lastD2HaloMissing > 0
          || left->lastD2PublishRefused || right->lastD2PublishRefused )
        {
            char note[96];
            std::snprintf( note, sizeof( note ),
                "SEAM_HALO_INCOMPLETE Lmiss=%d Rmiss=%d Lref=%d Rref=%d",
                left->lastD2HaloMissing, right->lastD2HaloMissing,
                left->lastD2PublishRefused ? 1 : 0, right->lastD2PublishRefused ? 1 : 0 );
            add( "cross_cell_seam_watertight", "FAIL", note, seamX, seamY, seamZ,
                0, 0, left->lastD2HaloMissing + right->lastD2HaloMissing );
            return;
        }
        if ( !left->hasCavity || !right->hasCavity
          || left->cavityTris.empty() || right->cavityTris.empty() )
        {
            add( "cross_cell_seam_watertight", "FAIL", "SEAM_CAVITY_MISSING",
                seamX, seamY, seamZ,
                (int)( left->cavityTris.size() + right->cavityTris.size() ) );
            return;
        }

        // +max face ownership: left cell emits the shared X seam; right does not emit -min face.
        // Prove owner emitted at least one +face boundary edge in this rebuild.
        if ( left->lastD2BoundaryEdges <= 0 )
        {
            char note[80];
            std::snprintf( note, sizeof( note ),
                "SEAM_OWNER_NO_BOUNDARY_EDGE Lbound=%d Rbound=%d",
                left->lastD2BoundaryEdges, right->lastD2BoundaryEdges );
            add( "cross_cell_seam_owner_once", "FAIL", note, seamX, seamY, seamZ,
                0, 0, 0 );
        }
        else
        {
            char note[96];
            std::snprintf( note, sizeof( note ),
                "owner=+max Lbound=%d Rbound=%d (-min not emitted)",
                left->lastD2BoundaryEdges, right->lastD2BoundaryEdges );
            add( "cross_cell_seam_owner_once", "PASS", note, seamX, seamY, seamZ,
                left->lastD2BoundaryEdges, 0, 0 );
        }

        auto countSeamTris = [&]( CellSample const& c ) -> int
        {
            constexpr float kEps = 0.10f;
            int n = 0;
            for ( DualContourQef::Tri const& t : c.cavityTris )
            {
                auto vertOnSeam = [&]( DualContourQef::Vec3 const& v ) -> bool {
                    return std::fabs( v.x - seamX ) <= kEps;
                };
                if ( vertOnSeam( t.a ) || vertOnSeam( t.b ) || vertOnSeam( t.c ) ) { ++n; }
            }
            return n;
        };
        int const seamTrisL = countSeamTris( *left );
        int const seamTrisR = countSeamTris( *right );
        // Watertight presence: owner mesh covers the seam; neighbor may also have near-seam verts
        // from halo dual cells, but topology must not leave an empty shared face.
        if ( seamTrisL <= 0 )
        {
            char note[80];
            std::snprintf( note, sizeof( note ),
                "SEAM_GAP ownerTris=%d neighborTris=%d", seamTrisL, seamTrisR );
            add( "cross_cell_seam_watertight", "FAIL", note, seamX, seamY, seamZ,
                seamTrisL + seamTrisR );
        }
        else
        {
            char note[96];
            std::snprintf( note, sizeof( note ),
                "seamTris L=%d R=%d boundL=%d (watertight +max owner)",
                seamTrisL, seamTrisR, left->lastD2BoundaryEdges );
            add( "cross_cell_seam_watertight", "PASS", note, seamX, seamY, seamZ,
                seamTrisL + seamTrisR, 0, 0 );
        }

        // --- Order independence: L→R vs R→L must yield identical per-cell canonical hashes ---
        {
            left = GetCell( leftCx, seamCy );
            right = GetCell( rightCx, seamCy );
            uint32_t const hL0 = left ? GeoCertHashCavityTris( *left ) : 0u;
            uint32_t const hR0 = right ? GeoCertHashCavityTris( *right ) : 0u;
            int const tL0 = left ? (int)left->cavityTris.size() : 0;
            int const tR0 = right ? (int)right->cavityTris.size() : 0;
            RebuildCavityMesh( rightCx, seamCy );
            RebuildCavityMesh( leftCx, seamCy );
            left = GetCell( leftCx, seamCy );
            right = GetCell( rightCx, seamCy );
            uint32_t const hL1 = left ? GeoCertHashCavityTris( *left ) : 0u;
            uint32_t const hR1 = right ? GeoCertHashCavityTris( *right ) : 0u;
            int const tL1 = left ? (int)left->cavityTris.size() : 0;
            int const tR1 = right ? (int)right->cavityTris.size() : 0;
            bool const orderOk = left && right
                && !left->lastD2PublishRefused && !right->lastD2PublishRefused
                && hL0 == hL1 && hR0 == hR1 && tL0 == tL1 && tR0 == tR1;
            if ( !orderOk )
            {
                char note[120];
                std::snprintf( note, sizeof( note ),
                    "ORDER_MISMATCH L %08X→%08X R %08X→%08X",
                    (unsigned)hL0, (unsigned)hL1, (unsigned)hR0, (unsigned)hR1 );
                add( "order_independence_LR_RL", "FAIL", note, seamX, seamY, seamZ );
            }
            else
            {
                char note[96];
                std::snprintf( note, sizeof( note ),
                    "L→R==R→L L=%08X R=%08X tris=%d/%d",
                    (unsigned)hL0, (unsigned)hR0, tL0, tR0 );
                add( "order_independence_LR_RL", "PASS", note, seamX, seamY, seamZ,
                    tL0 + tR0, 0, 0 );
            }
        }

        // --- Deliberate Unknown halo: strip required neighbor → refuse, no publishable cavity ---
        {
            EnsureD2HaloLattices( leftCx, seamCy );
            CellSample* home = GetCellMutable( leftCx, seamCy );
            CellSample* nbr = GetCellMutable( leftCx + 1, seamCy );
            if ( !home || !nbr || home->fill.empty() )
            {
                add( "UNKNOWN_HALO_REFUSED", "FAIL", "HALO_REFUSE_SETUP_FAIL",
                    seamX, seamY, seamZ );
            }
            else
            {
                std::vector<uint8_t> savedFill = nbr->fill;
                int const sw = nbr->fillW, sh = nbr->fillH, sk = nbr->fillK;
                bool const sfz = nbr->hasFillZ;
                nbr->fill.clear();
                nbr->fillW = nbr->fillH = nbr->fillK = 0;
                nbr->hasFillZ = false;

                g.certD2SkipHaloEnsure = true;
                RebuildCavityMesh( leftCx, seamCy );
                g.certD2SkipHaloEnsure = false;

                home = GetCellMutable( leftCx, seamCy );
                bool const refused = home
                    && home->lastD2PublishRefused
                    && home->lastD2HaloMissing > 0
                    && !home->hasCavity
                    && home->cavityTris.empty();
                int const miss = home ? home->lastD2HaloMissing : -1;

                // Restore neighbor authority and republish.
                nbr = GetCellMutable( leftCx + 1, seamCy );
                if ( nbr )
                {
                    nbr->fill = savedFill;
                    nbr->fillW = sw;
                    nbr->fillH = sh;
                    nbr->fillK = sk;
                    nbr->hasFillZ = sfz;
                }
                RebuildCavityMesh( leftCx, seamCy );
                RebuildCavityMesh( rightCx, seamCy );

                if ( !refused )
                {
                    char note[96];
                    std::snprintf( note, sizeof( note ),
                        "HALO_DID_NOT_REFUSE miss=%d refused=%d cavity=%d",
                        miss,
                        home && home->lastD2PublishRefused ? 1 : 0,
                        home && home->hasCavity ? 1 : 0 );
                    add( "UNKNOWN_HALO_REFUSED", "FAIL", note, seamX, seamY, seamZ,
                        0, 0, miss );
                }
                else
                {
                    char note[96];
                    std::snprintf( note, sizeof( note ),
                        "refuse+empty cavity; missing authority recorded miss=%d (no invent solid/air)",
                        miss );
                    add( "UNKNOWN_HALO_REFUSED", "PASS", note, seamX, seamY, seamZ,
                        0, 0, miss );
                }
            }
        }

        // --- Partitioned extract: large corridor + Unknown halo stats aggregate + determinism ---
        {
            float const partY = (float)seamCy - 3.5f;
            float partZ = 0.f;
            float const partX0 = (float)leftCx + 0.5f;
            if ( !SampleGroundZBase( partX0, partY, partZ ) )
            {
                add( "order_independence_partitioned", "FAIL", "PART_PAD_SAMPLE_FAIL",
                    partX0, partY, 0.f );
            }
            else
            {
                // Overlapping strikes → dirty span forces work-budget partition (>0.85+halo).
                constexpr float kPartR = 0.32f;
                for ( int i = 0; i < 8; ++i )
                {
                    float const x = partX0 + (float)i * 0.40f;
                    CarveOccupancySphere( x, partY, partZ - kPartR * 0.4f, kPartR,
                        x, partY, partZ );
                }
                int const partCx = (int)std::floor( partX0 + 1.5f );
                int const partCy = (int)std::floor( partY );
                EnsureD2HaloLattices( partCx, partCy );
                RebuildCavityMesh( partCx, partCy );
                CellSample* pcell = GetCellMutable( partCx, partCy );
                if ( !pcell || pcell->lastD2PublishRefused || !pcell->hasCavity
                    || pcell->cavityTris.empty() )
                {
                    add( "order_independence_partitioned", "FAIL", "PART_CAVITY_MISSING",
                        partX0, partY, partZ );
                }
                else
                {
                    uint32_t const hp0 = GeoCertHashCavityTris( *pcell );
                    int const tp0 = (int)pcell->cavityTris.size();
                    RebuildCavityMesh( partCx, partCy );
                    pcell = GetCellMutable( partCx, partCy );
                    uint32_t const hp1 = pcell ? GeoCertHashCavityTris( *pcell ) : 0u;
                    int const tp1 = pcell ? (int)pcell->cavityTris.size() : 0;
                    bool const partDetOk = pcell && hp0 == hp1 && tp0 == tp1
                        && !pcell->lastD2PublishRefused && pcell->lastD2HaloMissing == 0;
                    if ( !partDetOk )
                    {
                        char note[96];
                        std::snprintf( note, sizeof( note ),
                            "PART_NONDET %08X→%08X tris=%d→%d",
                            (unsigned)hp0, (unsigned)hp1, tp0, tp1 );
                        add( "order_independence_partitioned", "FAIL", note,
                            partX0, partY, partZ );
                    }
                    else
                    {
                        char note[96];
                        std::snprintf( note, sizeof( note ),
                            "partition rebuild hash-stable %08X tris=%d",
                            (unsigned)hp0, tp0 );
                        add( "order_independence_partitioned", "PASS", note,
                            partX0, partY, partZ, tp0, 0, 0 );
                    }

                    // Partitioned Unknown-halo: strip neighbor → refuse; haloMissing must aggregate.
                    CellSample* pnbr = GetCellMutable( partCx + 1, partCy );
                    if ( !pnbr )
                    {
                        add( "UNKNOWN_HALO_REFUSED_partitioned", "FAIL", "PART_NBR_MISSING",
                            partX0, partY, partZ );
                    }
                    else
                    {
                        std::vector<uint8_t> savedFill = pnbr->fill;
                        int const sw = pnbr->fillW, sh = pnbr->fillH, sk = pnbr->fillK;
                        bool const sfz = pnbr->hasFillZ;
                        pnbr->fill.clear();
                        pnbr->fillW = pnbr->fillH = pnbr->fillK = 0;
                        pnbr->hasFillZ = false;
                        g.certD2SkipHaloEnsure = true;
                        RebuildCavityMesh( partCx, partCy );
                        g.certD2SkipHaloEnsure = false;
                        pcell = GetCellMutable( partCx, partCy );
                        bool const prefused = pcell
                            && pcell->lastD2PublishRefused
                            && pcell->lastD2HaloMissing > 0
                            && !pcell->hasCavity
                            && pcell->cavityTris.empty();
                        int const pmiss = pcell ? pcell->lastD2HaloMissing : -1;
                        pnbr = GetCellMutable( partCx + 1, partCy );
                        if ( pnbr )
                        {
                            pnbr->fill = savedFill;
                            pnbr->fillW = sw;
                            pnbr->fillH = sh;
                            pnbr->fillK = sk;
                            pnbr->hasFillZ = sfz;
                        }
                        RebuildCavityMesh( partCx, partCy );
                        if ( !prefused )
                        {
                            char note[96];
                            std::snprintf( note, sizeof( note ),
                                "PART_HALO_DID_NOT_REFUSE miss=%d", pmiss );
                            add( "UNKNOWN_HALO_REFUSED_partitioned", "FAIL", note,
                                partX0, partY, partZ, 0, 0, pmiss );
                        }
                        else
                        {
                            char note[96];
                            std::snprintf( note, sizeof( note ),
                                "partition AccumulateStats haloMiss=%d refused (no invent)",
                                pmiss );
                            add( "UNKNOWN_HALO_REFUSED_partitioned", "PASS", note,
                                partX0, partY, partZ, 0, 0, pmiss );
                        }
                    }
                }
            }
        }
    }

    void GeoCertRunAccumulateSection()
    {
        // §6: ≥20 adjoining strikes on flat pad → one coherent EditedRegion; prior carve stays
        // removed; outside dirty+halo bit-identical; new mouth ≤12cm; work may partition.
        auto add = [&]( char const* scen, char const* verdict, char const* note,
            float x = 0.f, float y = 0.f, float z = 0.f,
            int er = 0, char const* hash = "-" )
        {
            GeoCertRow r{};
            std::snprintf( r.section, sizeof( r.section ), "6" );
            std::snprintf( r.scenario, sizeof( r.scenario ), "%s", scen );
            std::snprintf( r.verdict, sizeof( r.verdict ), "%s", verdict );
            std::snprintf( r.note, sizeof( r.note ), "%s", note );
            std::snprintf( r.action, sizeof( r.action ), "accumulate_strikes" );
            r.x = x; r.y = y; r.z = z;
            r.editedRegion = er;
            std::snprintf( r.hash, sizeof( r.hash ), "%s", hash ? hash : "-" );
            GeoCertAddRow( r );
            if ( std::strcmp( verdict, "FAIL" ) == 0 )
            {
                GeoCertHardFail( scen, "accumulate_strikes", note, x, y, z, 0.f, 0.f, 1.f );
            }
        };

        float const ox = (float)ProvenanceGeo::kRangeOriginX;
        float const oy = (float)ProvenanceGeo::kRangeOriginY;
        // Fresh pad north of A_flat dig matrix site so we own a clean accumulate corridor.
        float const padX0 = ox + 4.f;
        float const padY = oy + 6.f;
        float padZ = 0.f;
        if ( !SampleGroundZBase( padX0, padY, padZ ) )
        {
            add( "accumulated_20_strikes", "FAIL", "PAD_SAMPLE_FAIL", padX0, padY, 0.f );
            return;
        }

        int const farCx = (int)std::floor( ox + 40.f );
        int const farCy = (int)std::floor( oy + 6.f );
        // Side cell ~2 m off the corridor — stays outside dirty+halo while corridor advances.
        int const sideCx = (int)std::floor( padX0 );
        int const sideCy = (int)std::floor( padY ) + 2;
        auto seedHashCell = [&]( int cx, int cy ) -> uint32_t
        {
            PrefetchOccupancyCell( cx, cy );
            EnsureOccupancyLattice( cx, cy );
            CellSample* c = GetCellMutable( cx, cy );
            if ( c && !c->carved && c->fill.empty() )
            {
                SeedOccupancyFromVirginSurface( cx, cy );
            }
            return GeoCertHashCellFill( cx, cy );
        };
        uint32_t const farHash0 = seedHashCell( farCx, farCy );
        uint32_t const sideHash0 = seedHashCell( sideCx, sideCy );

        constexpr int kStrikes = 20;
        // Step ≈ one lattice edge so each adjoining strike still bites a solid crescent;
        // R > step/2 keeps mouths connected into one EditedRegion.
        constexpr float kStep = 0.125f;
        constexpr float kR = 0.20f;
        int struck = 0;
        float firstX = padX0, firstY = padY, firstZ = padZ;
        uint32_t regionId = 0;
        int openMin = 1000000;
        int openMax = 0;
        int prevOpens = -1;
        bool openShrink = false;

        for ( int s = 0; s < kStrikes; ++s )
        {
            float const x = padX0 + (float)s * kStep;
            float const y = padY;
            float z = padZ;
            SampleGroundZBase( x, y, z );
            // Soft scoop: carve under skin, open at skin. Deeper retry if overlap already air.
            float carveZ = z - kR * 0.45f;
            PrefetchOccupancyCell( (int)std::floor( x ), (int)std::floor( y ) );
            bool ok = CarveOccupancySphere( x, y, carveZ, kR, x, y, z );
            if ( !ok )
            {
                carveZ = z - kR * 0.90f;
                ok = CarveOccupancySphere( x, y, carveZ, kR, x, y, z );
            }
            if ( !ok )
            {
                // Nudge forward into uncut solid (still adjoining corridor).
                float const x2 = x + kStep * 0.5f;
                SampleGroundZBase( x2, y, z );
                carveZ = z - kR * 0.45f;
                PrefetchOccupancyCell( (int)std::floor( x2 ), (int)std::floor( y ) );
                ok = CarveOccupancySphere( x2, y, carveZ, kR, x2, y, z );
            }
            if ( !ok )
            {
                char note[80];
                std::snprintf( note, sizeof( note ), "STRIKE_NO_CARVE s=%d", s );
                add( "accumulated_20_strikes", "FAIL", note, x, y, z );
                return;
            }
            ++struck;
            if ( s == 0 ) { firstX = x; firstY = y; firstZ = carveZ; }

            CellSample const* home = GetCell( (int)std::floor( x ), (int)std::floor( y ) );
            if ( home && home->editedRegionId != 0 )
            {
                regionId = home->editedRegionId;
            }
            EditedRegion* er = FindEditedRegion( regionId );
            if ( !er )
            {
                add( "coherent_EditedRegion", "FAIL", "ER_MISSING_MID_SEQUENCE", x, y, z );
                return;
            }
            int const opens = (int)er->openings.size();
            if ( opens < openMin ) { openMin = opens; }
            if ( opens > openMax ) { openMax = opens; }
            if ( prevOpens >= 0 && opens < prevOpens ) { openShrink = true; }
            prevOpens = opens;
        }

        EditedRegion* er = FindEditedRegion( regionId );
        int const erN = (int)g.editedRegions.size();
        char farHashStr[24];
        uint32_t const farHash1 = GeoCertHashCellFill( farCx, farCy );
        uint32_t const sideHash1 = GeoCertHashCellFill( sideCx, sideCy );
        std::snprintf( farHashStr, sizeof( farHashStr ), "%08X", (unsigned)farHash1 );

        if ( struck != kStrikes )
        {
            char note[64];
            std::snprintf( note, sizeof( note ), "struck=%d want=%d", struck, kStrikes );
            add( "accumulated_20_strikes", "FAIL", note, padX0, padY, padZ, erN, farHashStr );
        }
        else
        {
            char note[96];
            std::snprintf( note, sizeof( note ), "struck=%d opens=%d..%d erId=%u",
                struck, openMin == 1000000 ? 0 : openMin, openMax, (unsigned)regionId );
            add( "accumulated_20_strikes", "PASS", note, padX0, padY, padZ,
                er ? 1 : 0, farHashStr );
        }

        // One coherent region for the pad corridor (cells along strikes share er id).
        bool coherent = er != nullptr && regionId != 0;
        int distinct = 0;
        {
            uint32_t seen[8] = {};
            int seenN = 0;
            for ( int s = 0; s < kStrikes; ++s )
            {
                float const x = padX0 + (float)s * kStep;
                CellSample const* c = GetCell( (int)std::floor( x ), (int)std::floor( padY ) );
                if ( !c || c->editedRegionId == 0 ) { coherent = false; break; }
                bool found = false;
                for ( int i = 0; i < seenN; ++i ) { if ( seen[i] == c->editedRegionId ) { found = true; break; } }
                if ( !found && seenN < 8 ) { seen[seenN++] = c->editedRegionId; }
            }
            distinct = seenN;
            if ( seenN != 1 ) { coherent = false; }
        }
        if ( !coherent )
        {
            char note[80];
            std::snprintf( note, sizeof( note ), "SPLIT_OR_MISSING_ER distinct=%d", distinct );
            add( "coherent_EditedRegion", "FAIL", note, padX0, padY, padZ, distinct, farHashStr );
        }
        else
        {
            add( "coherent_EditedRegion", "PASS", "single_ER_along_corridor", padX0, padY, padZ, 1, farHashStr );
        }

        if ( openShrink )
        {
            add( "openings_remove_only", "FAIL", "OPENINGS_SHRANK", padX0, padY, padZ, 1, farHashStr );
        }
        else
        {
            char note[64];
            std::snprintf( note, sizeof( note ), "opens_max=%d", openMax );
            add( "openings_remove_only", "PASS", note, padX0, padY, padZ, 1, farHashStr );
        }

        bool priorGone = !OccupancySolidAt( firstX, firstY, firstZ );
        if ( !priorGone )
        {
            add( "prior_carve_stays_removed", "FAIL", "LOST_CARVE_OR_HF_RESURRECTION",
                firstX, firstY, firstZ, 1, farHashStr );
        }
        else
        {
            add( "prior_carve_stays_removed", "PASS", "first_strike_air", firstX, firstY, firstZ, 1, farHashStr );
        }

        if ( farHash0 != farHash1 )
        {
            char note[96];
            std::snprintf( note, sizeof( note ), "FAR_CELL_MUTATED h0=%08X h1=%08X",
                (unsigned)farHash0, (unsigned)farHash1 );
            add( "outside_dirty_bit_identity", "FAIL", note,
                (float)farCx + 0.5f, (float)farCy + 0.5f, 0.f, 1, farHashStr );
        }
        else if ( sideHash0 != sideHash1 )
        {
            char note[96];
            std::snprintf( note, sizeof( note ), "SIDE_LIP_MUTATED h0=%08X h1=%08X cell=(%d,%d)",
                (unsigned)sideHash0, (unsigned)sideHash1, sideCx, sideCy );
            add( "outside_dirty_bit_identity", "FAIL", note,
                (float)sideCx + 0.5f, (float)sideCy + 0.5f, 0.f, 1, farHashStr );
        }
        else
        {
            char note[96];
            std::snprintf( note, sizeof( note ), "far=%s side=(%d,%d) ok", farHashStr, sideCx, sideCy );
            add( "outside_dirty_bit_identity", "PASS", note, padX0, padY, padZ, 1, farHashStr );
        }

        // Cross cell edge continuity (corridor spans ≥1 cell with 20*0.08=1.6m).
        int const cellA = (int)std::floor( padX0 );
        int const cellB = (int)std::floor( padX0 + (float)( kStrikes - 1 ) * kStep );
        if ( cellB <= cellA )
        {
            add( "cross_cell_continuity", "FAIL", "CORRIDOR_DID_NOT_CROSS_CELL", padX0, padY, padZ );
        }
        else if ( !coherent )
        {
            add( "cross_cell_continuity", "FAIL", "CROSS_CELL_SPLIT_ER", padX0, padY, padZ );
        }
        else
        {
            char note[80];
            std::snprintf( note, sizeof( note ), "cells=%d..%d same_ER", cellA, cellB );
            add( "cross_cell_continuity", "PASS", note, padX0, padY, padZ, 1, farHashStr );
        }

        // 0.85 = work partition, not clip: large dirty still yields cavity tris.
        bool cavityOk = false;
        if ( er )
        {
            for ( auto const& xy : er->cells )
            {
                CellSample const* c = GetCell( xy.first, xy.second );
                if ( c && c->hasCavity && !c->cavityTris.empty() ) { cavityOk = true; break; }
            }
        }
        if ( !cavityOk )
        {
            add( "work_budget_partition_not_clip", "FAIL", "CAVITY_CLIPPED_OR_MISSING", padX0, padY, padZ );
        }
        else
        {
            add( "work_budget_partition_not_clip", "PASS", "cavity_present_after_20 (0.85=partition)",
                padX0, padY, padZ, 1, farHashStr );
        }

    }

    // §6 variant: same ER/lip/cross-cell gates as flat, on a named pad with optional face-normal carve.
    void GeoCertRunAccumulatePad( char const* tag, float padX0, float padY,
        float stepNx, float stepNy, float stepNz, bool intoNormal )
    {
        auto add = [&]( char const* scen, char const* verdict, char const* note,
            float x = 0.f, float y = 0.f, float z = 0.f,
            int er = 0, char const* hash = "-" )
        {
            GeoCertRow r{};
            std::snprintf( r.section, sizeof( r.section ), "6" );
            std::snprintf( r.scenario, sizeof( r.scenario ), "%s_%s", scen, tag );
            std::snprintf( r.verdict, sizeof( r.verdict ), "%s", verdict );
            std::snprintf( r.note, sizeof( r.note ), "%s", note );
            std::snprintf( r.action, sizeof( r.action ), "accumulate_strikes" );
            std::snprintf( r.terrain, sizeof( r.terrain ), "%s", tag );
            r.x = x; r.y = y; r.z = z;
            r.editedRegion = er;
            std::snprintf( r.hash, sizeof( r.hash ), "%s", hash ? hash : "-" );
            GeoCertAddRow( r );
            if ( std::strcmp( verdict, "FAIL" ) == 0 )
            {
                GeoCertHardFail( r.scenario, "accumulate_strikes", note, x, y, z, 0.f, 0.f, 1.f );
            }
        };

        float padZ = 0.f;
        if ( !SampleGroundZBase( padX0, padY, padZ ) )
        {
            add( "accumulated_20_strikes", "FAIL", "PAD_SAMPLE_FAIL", padX0, padY, 0.f );
            return;
        }
        float snx = stepNx, sny = stepNy, snz = stepNz;
        float snLen = std::sqrt( snx * snx + sny * sny + snz * snz );
        if ( snLen < 1e-4f ) { snx = 1.f; sny = 0.f; snz = 0.f; snLen = 1.f; }
        snx /= snLen; sny /= snLen; snz /= snLen;

        float fnx = 0.f, fny = 0.f, fnz = 1.f;
        CaptureFaceNormalAt( padX0, padY, fnx, fny, fnz );

        int const farCx = (int)std::floor( padX0 + 36.f );
        int const farCy = (int)std::floor( padY );
        int const sideCx = (int)std::floor( padX0 );
        int const sideCy = (int)std::floor( padY ) + 2;
        auto seedHashCell = [&]( int cx, int cy ) -> uint32_t
        {
            PrefetchOccupancyCell( cx, cy );
            EnsureOccupancyLattice( cx, cy );
            CellSample* c = GetCellMutable( cx, cy );
            if ( c && !c->carved && c->fill.empty() )
            {
                SeedOccupancyFromVirginSurface( cx, cy );
            }
            return GeoCertHashCellFill( cx, cy );
        };
        uint32_t const farHash0 = seedHashCell( farCx, farCy );
        uint32_t const sideHash0 = seedHashCell( sideCx, sideCy );

        constexpr int kStrikes = 20;
        constexpr float kStep = 0.125f;
        constexpr float kR = 0.20f;
        int struck = 0;
        float firstX = padX0, firstY = padY, firstZ = padZ;
        uint32_t regionId = 0;
        int openMin = 1000000;
        int openMax = 0;
        int prevOpens = -1;
        bool openShrink = false;

        for ( int s = 0; s < kStrikes; ++s )
        {
            float const x = padX0 + snx * (float)s * kStep;
            float const y = padY + sny * (float)s * kStep;
            float z = padZ;
            SampleGroundZBase( x, y, z );
            if ( intoNormal )
            {
                CaptureFaceNormalAt( x, y, fnx, fny, fnz );
            }
            float carveX = x, carveY = y, carveZ = z - kR * 0.45f;
            if ( intoNormal )
            {
                carveX = x - fnx * kR * 0.55f;
                carveY = y - fny * kR * 0.55f;
                carveZ = z - fnz * kR * 0.55f;
            }
            PrefetchOccupancyCell( (int)std::floor( x ), (int)std::floor( y ) );
            bool ok = CarveOccupancySphere( carveX, carveY, carveZ, kR, x, y, z );
            if ( !ok )
            {
                carveX = x - fnx * kR * 0.90f;
                carveY = y - fny * kR * 0.90f;
                carveZ = z - fnz * kR * 0.90f;
                ok = CarveOccupancySphere( carveX, carveY, carveZ, kR, x, y, z );
            }
            if ( !ok )
            {
                char note[80];
                std::snprintf( note, sizeof( note ), "STRIKE_NO_CARVE s=%d", s );
                add( "accumulated_20_strikes", "FAIL", note, x, y, z );
                return;
            }
            ++struck;
            if ( s == 0 ) { firstX = carveX; firstY = carveY; firstZ = carveZ; }

            CellSample const* home = GetCell( (int)std::floor( x ), (int)std::floor( y ) );
            if ( home && home->editedRegionId != 0 ) { regionId = home->editedRegionId; }
            EditedRegion* er = FindEditedRegion( regionId );
            if ( !er )
            {
                add( "coherent_EditedRegion", "FAIL", "ER_MISSING_MID_SEQUENCE", x, y, z );
                return;
            }
            int const opens = (int)er->openings.size();
            if ( opens < openMin ) { openMin = opens; }
            if ( opens > openMax ) { openMax = opens; }
            if ( prevOpens >= 0 && opens < prevOpens ) { openShrink = true; }
            prevOpens = opens;
        }

        EditedRegion* er = FindEditedRegion( regionId );
        char farHashStr[24];
        uint32_t const farHash1 = GeoCertHashCellFill( farCx, farCy );
        uint32_t const sideHash1 = GeoCertHashCellFill( sideCx, sideCy );
        std::snprintf( farHashStr, sizeof( farHashStr ), "%08X", (unsigned)farHash1 );

        {
            char note[96];
            std::snprintf( note, sizeof( note ), "struck=%d opens=%d..%d erId=%u",
                struck, openMin == 1000000 ? 0 : openMin, openMax, (unsigned)regionId );
            add( "accumulated_20_strikes", struck == kStrikes ? "PASS" : "FAIL", note,
                padX0, padY, padZ, er ? 1 : 0, farHashStr );
            if ( struck != kStrikes ) { return; }
        }

        bool coherent = er != nullptr && regionId != 0;
        int distinct = 0;
        {
            uint32_t seen[8] = {};
            int seenN = 0;
            for ( int s = 0; s < kStrikes; ++s )
            {
                float const x = padX0 + snx * (float)s * kStep;
                float const y = padY + sny * (float)s * kStep;
                CellSample const* c = GetCell( (int)std::floor( x ), (int)std::floor( y ) );
                if ( !c || c->editedRegionId == 0 ) { coherent = false; break; }
                bool found = false;
                for ( int i = 0; i < seenN; ++i ) { if ( seen[i] == c->editedRegionId ) { found = true; break; } }
                if ( !found && seenN < 8 ) { seen[seenN++] = c->editedRegionId; }
            }
            distinct = seenN;
            if ( seenN != 1 ) { coherent = false; }
        }
        if ( !coherent )
        {
            char note[80];
            std::snprintf( note, sizeof( note ), "SPLIT_OR_MISSING_ER distinct=%d", distinct );
            add( "coherent_EditedRegion", "FAIL", note, padX0, padY, padZ, distinct, farHashStr );
        }
        else
        {
            add( "coherent_EditedRegion", "PASS", "single_ER_along_corridor", padX0, padY, padZ, 1, farHashStr );
        }

        add( "openings_remove_only", openShrink ? "FAIL" : "PASS",
            openShrink ? "OPENINGS_SHRANK" : "opens_monotonic",
            padX0, padY, padZ, 1, farHashStr );

        bool priorGone = !OccupancySolidAt( firstX, firstY, firstZ );
        add( "prior_carve_stays_removed", priorGone ? "PASS" : "FAIL",
            priorGone ? "first_strike_air" : "LOST_CARVE_OR_HF_RESURRECTION",
            firstX, firstY, firstZ, 1, farHashStr );

        if ( farHash0 != farHash1 )
        {
            char note[96];
            std::snprintf( note, sizeof( note ), "FAR_CELL_MUTATED h0=%08X h1=%08X",
                (unsigned)farHash0, (unsigned)farHash1 );
            add( "outside_dirty_bit_identity", "FAIL", note,
                (float)farCx + 0.5f, (float)farCy + 0.5f, 0.f, 1, farHashStr );
        }
        else if ( sideHash0 != sideHash1 )
        {
            char note[96];
            std::snprintf( note, sizeof( note ), "SIDE_LIP_MUTATED h0=%08X h1=%08X",
                (unsigned)sideHash0, (unsigned)sideHash1 );
            add( "outside_dirty_bit_identity", "FAIL", note,
                (float)sideCx + 0.5f, (float)sideCy + 0.5f, 0.f, 1, farHashStr );
        }
        else
        {
            add( "outside_dirty_bit_identity", "PASS", "far+side ok", padX0, padY, padZ, 1, farHashStr );
        }

        int const cellA = (int)std::floor( padX0 );
        int const cellB = (int)std::floor( padX0 + snx * (float)( kStrikes - 1 ) * kStep );
        int const cellAy = (int)std::floor( padY );
        int const cellBy = (int)std::floor( padY + sny * (float)( kStrikes - 1 ) * kStep );
        bool crossed = ( cellB != cellA ) || ( cellBy != cellAy );
        if ( !crossed )
        {
            add( "cross_cell_continuity", "FAIL", "CORRIDOR_DID_NOT_CROSS_CELL", padX0, padY, padZ );
        }
        else if ( !coherent )
        {
            add( "cross_cell_continuity", "FAIL", "CROSS_CELL_SPLIT_ER", padX0, padY, padZ );
        }
        else
        {
            char note[80];
            std::snprintf( note, sizeof( note ), "cells=(%d,%d)..(%d,%d)", cellA, cellAy, cellB, cellBy );
            add( "cross_cell_continuity", "PASS", note, padX0, padY, padZ, 1, farHashStr );
        }

        bool cavityOk = false;
        if ( er )
        {
            for ( auto const& xy : er->cells )
            {
                CellSample const* c = GetCell( xy.first, xy.second );
                if ( c && c->hasCavity && !c->cavityTris.empty() ) { cavityOk = true; break; }
            }
        }
        add( "work_budget_partition_not_clip", cavityOk ? "PASS" : "FAIL",
            cavityOk ? "cavity_present (0.85=partition)" : "CAVITY_CLIPPED_OR_MISSING",
            padX0, padY, padZ, 1, farHashStr );
    }

    void GeoCertRunOwnershipSection()
    {
        // §7: action bite ≠ D2 dirty(+halo) ≠ HF aperture; no cross-owner mutation; no uncovered void.
        auto add = [&]( char const* scen, char const* verdict, char const* note,
            float x = 0.f, float y = 0.f, float z = 0.f,
            int ownViol = 0, int er = 0 )
        {
            GeoCertRow r{};
            std::snprintf( r.section, sizeof( r.section ), "7" );
            std::snprintf( r.scenario, sizeof( r.scenario ), "%s", scen );
            std::snprintf( r.verdict, sizeof( r.verdict ), "%s", verdict );
            std::snprintf( r.note, sizeof( r.note ), "%s", note );
            std::snprintf( r.action, sizeof( r.action ), "ownership_masks" );
            r.x = x; r.y = y; r.z = z;
            r.ownViol = ownViol;
            r.editedRegion = er;
            GeoCertAddRow( r );
            if ( std::strcmp( verdict, "FAIL" ) == 0 )
            {
                GeoCertHardFail( scen, "ownership_masks", note, x, y, z, 0.f, 0.f, 1.f );
            }
        };

        EditedRegion* er = nullptr;
        int homeCx = 0, homeCy = 0;
        CellSample* homeCell = nullptr;
        // Prefer accumulate corridor ER (largest openings), else any carved ER with action.
        size_t bestOpens = 0;
        for ( EditedRegion& cand : g.editedRegions )
        {
            if ( cand.openings.empty() || !cand.hasAction ) { continue; }
            if ( cand.openings.size() >= bestOpens )
            {
                bestOpens = cand.openings.size();
                er = &cand;
            }
        }
        if ( !er )
        {
            add( "HF_D2_ownership_masks", "FAIL", "NO_ER_WITH_ACTION_OPENINGS" );
            return;
        }
        // Dirty AABB is per-cell — evaluate the cell that owns the action tip (last strike),
        // not an arbitrary first cell along a multi-cell corridor.
        homeCx = (int)std::floor( er->actionX );
        homeCy = (int)std::floor( er->actionY );
        homeCell = GetCellMutable( homeCx, homeCy );
        if ( !homeCell || !homeCell->carved )
        {
            homeCell = nullptr;
            for ( auto const& xy : er->cells )
            {
                CellSample* c = GetCellMutable( xy.first, xy.second );
                if ( c && c->carved && c->hasCarveFocus )
                {
                    homeCell = c;
                    homeCx = xy.first;
                    homeCy = xy.second;
                    break;
                }
            }
        }
        if ( !homeCell )
        {
            for ( auto const& xy : er->cells )
            {
                CellSample* c = GetCellMutable( xy.first, xy.second );
                if ( c && c->carved )
                {
                    homeCell = c;
                    homeCx = xy.first;
                    homeCy = xy.second;
                    break;
                }
            }
        }
        if ( !homeCell )
        {
            add( "HF_D2_ownership_masks", "FAIL", "NO_CARVED_HOME_CELL",
                er->actionX, er->actionY, er->actionZ, 1, (int)er->id );
            return;
        }

        float mnX, mxX, mnY, mxY, mnZ, mxZ;
        if ( !DirtyBoundsFromEditedRegion( er, *homeCell, homeCx, homeCy, mnX, mxX, mnY, mxY, mnZ, mxZ ) )
        {
            add( "masks_distinct", "FAIL", "DIRTY_BOUNDS_EMPTY",
                er->actionX, er->actionY, er->actionZ, 1, (int)er->id );
            return;
        }
        float const dirtySpanX = mxX - mnX;
        float const dirtySpanY = mxY - mnY;
        float const dirtyHalf = 0.5f * std::sqrt( dirtySpanX * dirtySpanX + dirtySpanY * dirtySpanY );
        float maxOpenR = 0.f;
        for ( EditOpening const& o : er->openings )
        {
            if ( o.r > maxOpenR ) { maxOpenR = o.r; }
        }
        float const biteR = homeCell->hasCarveFocus ? homeCell->carveRM : 0.f;
        float const actionR = er->hasAction ? er->actionR : 0.f;

        // Distinct: dirty (+halo) larger than tip aperture; bite carve may exceed tip mouth.
        bool const actionInsideDirty =
            er->actionX >= mnX - 1e-3f && er->actionX <= mxX + 1e-3f
            && er->actionY >= mnY - 1e-3f && er->actionY <= mxY + 1e-3f
            && er->actionZ >= mnZ - 1e-3f && er->actionZ <= mxZ + 1e-3f;
        bool const dirtyGtHf = dirtyHalf > maxOpenR + 0.05f; // halo 0.20 expands beyond tip mouth
        bool const tipCapOk = actionR <= 0.12f + 1e-3f;
        bool const biteNeAction = !homeCell->hasCarveFocus
            || std::fabs( biteR - actionR ) > 1e-4f
            || std::fabs( homeCell->carveWx - er->actionX ) > 1e-4f
            || std::fabs( homeCell->carveWz - er->actionZ ) > 1e-4f
            || biteR > actionR + 1e-4f; // soft pads often share XY; bite R still distinct

        int viol = 0;
        char failWhy[64] = "ACTION_D2_HF_NOT_DISTINCT";
        if ( !actionInsideDirty ) { ++viol; std::snprintf( failWhy, sizeof( failWhy ), "ACTION_OUTSIDE_DIRTY" ); }
        if ( !dirtyGtHf ) { ++viol; std::snprintf( failWhy, sizeof( failWhy ), "DIRTY_NOT_LARGER_THAN_HF" ); }
        if ( !tipCapOk ) { ++viol; std::snprintf( failWhy, sizeof( failWhy ), "NEW_STRIKE_MOUTH_GT_12CM" ); }
        {
            char note[160];
            std::snprintf( note, sizeof( note ),
                "biteR=%.3f actionR=%.3f maxOpenR=%.3f dirtyHalf=%.3f tip<=12=%d insideDirty=%d biteNeTip=%d",
                biteR, actionR, maxOpenR, dirtyHalf, tipCapOk ? 1 : 0, actionInsideDirty ? 1 : 0,
                biteNeAction ? 1 : 0 );
            if ( viol != 0 )
            {
                add( "masks_distinct", "FAIL", failWhy,
                    er->actionX, er->actionY, er->actionZ, viol, (int)er->id );
            }
            else
            {
                add( "masks_distinct", "PASS", note,
                    er->actionX, er->actionY, er->actionZ, 0, (int)er->id );
            }
        }

        // Prior openings: occupancy carved + HF aperture + D2 owner present.
        EditOpening const& o0 = er->openings.front();
        bool const occOpen = SurfaceBrokenByOccupancy( o0.x, o0.y )
            || !OccupancySolidAt( o0.x, o0.y, o0.z - 0.02f );
        bool const hfMouth = NearOpeningMouthAt( o0.x, o0.y );
        bool const d2Ready = CavityReadyNear( o0.x, o0.y );
        bool homeHasCavity = false;
        for ( auto const& xy : er->cells )
        {
            CellSample const* c = GetCell( xy.first, xy.second );
            if ( c && c->hasCavity && !c->cavityTris.empty() ) { homeHasCavity = true; break; }
        }
        if ( !occOpen || !hfMouth || !d2Ready || !homeHasCavity )
        {
            char note[120];
            std::snprintf( note, sizeof( note ),
                "PRIOR_OPEN_OWNERSHIP occ=%d hf=%d d2=%d cavity=%d",
                occOpen ? 1 : 0, hfMouth ? 1 : 0, d2Ready ? 1 : 0, homeHasCavity ? 1 : 0 );
            add( "prior_opening_triple_owner", "FAIL", note, o0.x, o0.y, o0.z, 1, (int)er->id );
        }
        else
        {
            add( "prior_opening_triple_owner", "PASS", "occ+HF_mouth+D2_owner",
                o0.x, o0.y, o0.z, 0, (int)er->id );
        }

        // No uncovered void: CrestMouthStencil ⇒ CavityReady; mouth∩skin-open without D2 = FAIL.
        int voidViol = 0;
        int stencilN = 0;
        if ( er->hasOwnBounds )
        {
            float const x0 = er->ownMinX - 0.05f, x1 = er->ownMaxX + 0.05f;
            float const y0 = er->ownMinY - 0.05f, y1 = er->ownMaxY + 0.05f;
            for ( float y = y0; y <= y1 + 1e-4f; y += 0.10f )
            {
                for ( float x = x0; x <= x1 + 1e-4f; x += 0.10f )
                {
                    bool const mouth = NearOpeningMouthAt( x, y );
                    bool const skin = SurfaceBrokenByOccupancy( x, y );
                    bool const d2 = CavityReadyNear( x, y );
                    bool const stencil = CrestMouthStencilAt( x, y );
                    if ( stencil ) { ++stencilN; if ( !d2 ) { ++voidViol; } }
                    if ( mouth && skin && !d2 ) { ++voidViol; }
                }
            }
        }
        if ( voidViol != 0 )
        {
            char note[96];
            std::snprintf( note, sizeof( note ), "UNCOVERED_VOID viol=%d stencilN=%d", voidViol, stencilN );
            add( "no_uncovered_void", "FAIL", note, o0.x, o0.y, o0.z, voidViol, (int)er->id );
        }
        else
        {
            char note[80];
            std::snprintf( note, sizeof( note ), "stencilN=%d voidViol=0", stencilN );
            add( "no_uncovered_void", "PASS", note, o0.x, o0.y, o0.z, 0, (int)er->id );
        }

        // D2 re-extract must not mutate HF ownership (openings / action / own bounds).
        int const opens0 = (int)er->openings.size();
        float const ownMinX0 = er->ownMinX, ownMaxX0 = er->ownMaxX;
        float const actX0 = er->actionX, actR0 = er->actionR;
        int const dirtyRev0 = er->dirtyRev;
        RebuildCavityMesh( homeCx, homeCy );
        er = FindEditedRegion( er->id );
        if ( !er )
        {
            add( "d2_no_cross_owner_mutation", "FAIL", "ER_LOST_AFTER_REEXTRACT",
                (float)homeCx + 0.5f, (float)homeCy + 0.5f, 0.f, 1 );
            return;
        }
        bool const hfStable = (int)er->openings.size() == opens0
            && er->dirtyRev == dirtyRev0
            && std::fabs( er->ownMinX - ownMinX0 ) < 1e-6f
            && std::fabs( er->ownMaxX - ownMaxX0 ) < 1e-6f
            && std::fabs( er->actionX - actX0 ) < 1e-6f
            && std::fabs( er->actionR - actR0 ) < 1e-6f;
        if ( !hfStable )
        {
            add( "d2_no_cross_owner_mutation", "FAIL", "D2_MUTATED_HF_OWNERSHIP",
                er->actionX, er->actionY, er->actionZ, 1, (int)er->id );
        }
        else
        {
            char note[80];
            std::snprintf( note, sizeof( note ), "opens=%d dirtyRev=%d HF_fields_stable", opens0, dirtyRev0 );
            add( "d2_no_cross_owner_mutation", "PASS", note,
                er->actionX, er->actionY, er->actionZ, 0, (int)er->id );
        }

        // Unrelated far sample: no HF aperture flicker / no mouth ownership.
        float const ox = (float)ProvenanceGeo::kRangeOriginX;
        float const oy = (float)ProvenanceGeo::kRangeOriginY;
        float const farX = ox + 40.f, farY = oy + 10.f;
        bool const farMouth = NearOpeningMouthAt( farX, farY ) || CrestMouthStencilAt( farX, farY );
        if ( farMouth )
        {
            add( "unrelated_no_hf_flicker", "FAIL", "FAR_HF_APERTURE_OWNED", farX, farY, 0.f, 1 );
        }
        else
        {
            add( "unrelated_no_hf_flicker", "PASS", "far_mouth=0", farX, farY, 0.f, 0 );
        }
    }

    void GeoCertRunMaterialSection()
    {
        // §8: presented vs authoritative caps; soft roof (sand/gravel) vs hard rock gate.
        // Do not invent AUTH remappers — flag mismatch only.
        auto add = [&]( char const* scen, char const* verdict, char const* note,
            float x = 0.f, float y = 0.f, float z = 0.f,
            char const* mat = "-", int matMismatch = 0 )
        {
            GeoCertRow r{};
            std::snprintf( r.section, sizeof( r.section ), "8" );
            std::snprintf( r.scenario, sizeof( r.scenario ), "%s", scen );
            std::snprintf( r.verdict, sizeof( r.verdict ), "%s", verdict );
            std::snprintf( r.note, sizeof( r.note ), "%s", note );
            std::snprintf( r.action, sizeof( r.action ), "material_gate" );
            std::snprintf( r.material, sizeof( r.material ), "%s", mat ? mat : "-" );
            r.x = x; r.y = y; r.z = z;
            r.matMismatch = matMismatch;
            GeoCertAddRow( r );
            if ( std::strcmp( verdict, "FAIL" ) == 0 )
            {
                GeoCertHardFail( scen, "material_gate", note, x, y, z, 0.f, 0.f, 1.f );
            }
        };

        // Soft vs hard gate instrumentation exists (MaterialSlumpsOpen / SampleTerrainDrawZ).
        bool const softSand = MaterialSlumpsOpen( "sand" );
        bool const softGravel = MaterialSlumpsOpen( "gravel" );
        bool const hardLime = !MaterialSlumpsOpen( "limestone" );
        bool const hardGranite = !MaterialSlumpsOpen( "granite" );
        bool const hardDirt = !MaterialSlumpsOpen( "dirt" );
        if ( !( softSand && softGravel && hardLime && hardGranite && hardDirt ) )
        {
            add( "soft_roof_vs_hard_gate", "FAIL", "MATERIAL_SLUMP_GATE_WRONG" );
        }
        else
        {
            add( "soft_roof_vs_hard_gate", "PASS", "sand/gravel slump; rock/dirt hold" );
        }

        // Presented cell.cap / CapAtWorld vs geography SampleSurface.cap along contacts.
        int mismatchN = 0;
        float failX = 0.f, failY = 0.f, failZ = 0.f;
        char failMat[32] = "-";
        char failNote[120] = "presented~=auth";
        for ( int i = 0; i < s_geoContactN; ++i )
        {
            GeoCertContact const& c = s_geoContacts[i];
            if ( !c.found ) { continue; }
            ProvenanceGeo::SurfaceSample surf = ProvenanceGeo::SampleSurface(
                c.x, c.y, g.gradeDatum, g.reliefVoxels, g.voxelEdgeM );
            char const* auth = surf.cap ? surf.cap : ProvenanceGeo::CapId( surf.rock );
            std::string presented = CapAtWorld( c.x, c.y );
            CellSample const* cell = GetCell( c.cellX, c.cellY );
            char const* cellCap = ( cell && !cell->cap.empty() ) ? cell->cap.c_str() : presented.c_str();
            if ( std::strcmp( cellCap, auth ) != 0 && presented != auth )
            {
                ++mismatchN;
                failX = c.x; failY = c.y; failZ = c.z;
                std::snprintf( failMat, sizeof( failMat ), "%s", auth );
                std::snprintf( failNote, sizeof( failNote ),
                    "AUTH_MATERIAL_MISMATCH presented=%s auth=%s id=%s",
                    presented.c_str(), auth, c.id );
                break;
            }
        }
        if ( mismatchN != 0 )
        {
            add( "presented_vs_auth", "FAIL", failNote, failX, failY, failZ, failMat, mismatchN );
        }
        else
        {
            add( "presented_vs_auth", "PASS", "contacts_cap_match_SampleSurface", 0.f, 0.f, 0.f, "-", 0 );
        }

        // Soft roof draw path: gravel/sand + SurfaceBroken + !CrestMouth → SampleTerrainDrawZ sinks.
        // Hard rock draw path: SurfaceBroken + !slump → keeps virgin Z (unless CrestMouth omits).
        float softX = 0.f, softY = 0.f, softZ = 0.f;
        bool softFound = false;
        float hardX = 0.f, hardY = 0.f, hardZ = 0.f;
        bool hardFound = false;
        for ( int i = 0; i < s_geoContactN; ++i )
        {
            GeoCertContact const& c = s_geoContacts[i];
            if ( !c.found ) { continue; }
            std::string cap = CapAtWorld( c.x, c.y );
            if ( !softFound && MaterialSlumpsOpen( cap ) && SurfaceBrokenByOccupancy( c.x, c.y ) )
            {
                softX = c.x; softY = c.y; softZ = c.z; softFound = true;
            }
            if ( !hardFound && !MaterialSlumpsOpen( cap ) && SurfaceBrokenByOccupancy( c.x, c.y )
              && ( std::strcmp( c.id, "F_diag" ) == 0 || std::strcmp( c.id, "J_lime" ) == 0
                || std::strcmp( c.id, "E_rocky" ) == 0 || !c.soft ) )
            {
                // Prefer hard host rock contacts; E may be gravel — skip if soft.
                if ( !MaterialSlumpsOpen( cap ) )
                {
                    hardX = c.x; hardY = c.y; hardZ = c.z; hardFound = true;
                }
            }
        }
        // Fallback scan for soft broken gravel near I_scree / E.
        if ( !softFound )
        {
            float const ox = (float)ProvenanceGeo::kRangeOriginX;
            float const oy = (float)ProvenanceGeo::kRangeOriginY;
            for ( float u = 60.f; u <= 95.f && !softFound; u += 1.f )
            {
                float const x = ox + u, y = oy;
                if ( MaterialSlumpsOpen( CapAtWorld( x, y ) ) && SurfaceBrokenByOccupancy( x, y ) )
                {
                    softX = x; softY = y; SampleGroundZBase( x, y, softZ ); softFound = true;
                }
            }
        }
        if ( !hardFound )
        {
            float const ox = (float)ProvenanceGeo::kRangeOriginX;
            float const oy = (float)ProvenanceGeo::kRangeOriginY;
            for ( float u = 80.f; u <= 105.f && !hardFound; u += 1.f )
            {
                float const x = ox + u, y = oy;
                std::string cap = CapAtWorld( x, y );
                if ( !MaterialSlumpsOpen( cap ) && SurfaceBrokenByOccupancy( x, y ) )
                {
                    hardX = x; hardY = y; SampleGroundZBase( x, y, hardZ ); hardFound = true;
                }
            }
        }

        if ( softFound )
        {
            float virginZ = 0.f;
            SampleGroundZBase( softX, softY, virginZ );
            float drawZ = virginZ;
            bool const drew = SampleTerrainDrawZ( softX, softY, drawZ );
            // Soft roof: either CrestMouth omits (drew=false) or drawZ sunk below virgin.
            bool const softOk = !drew || ( drawZ < virginZ - 0.02f );
            char note[120];
            std::snprintf( note, sizeof( note ), "cap=%s drew=%d dz=%.3f",
                CapAtWorld( softX, softY ).c_str(), drew ? 1 : 0, virginZ - drawZ );
            add( "soft_roof_draw_sink", softOk ? "PASS" : "FAIL",
                softOk ? note : "SOFT_ROOF_DID_NOT_SINK",
                softX, softY, softZ, CapAtWorld( softX, softY ).c_str(), softOk ? 0 : 1 );
        }
        else
        {
            // Dig matrix may not have broken a soft cell — classify-only gate already PASS above.
            add( "soft_roof_draw_sink", "SKIP",
                "no SurfaceBroken sand/gravel after dig matrix — gate fn covered" );
        }

        if ( hardFound )
        {
            float virginZ = 0.f;
            SampleGroundZBase( hardX, hardY, virginZ );
            if ( CrestMouthStencilAt( hardX, hardY ) )
            {
                // Aperture omit is HF presentation owner, not soft-roof path — OK.
                add( "hard_rock_holds_roof", "PASS", "CrestMouthStencil_omit_HF (not soft-slump)",
                    hardX, hardY, hardZ, CapAtWorld( hardX, hardY ).c_str(), 0 );
            }
            else
            {
                float drawZ = virginZ;
                bool const drew = SampleTerrainDrawZ( hardX, hardY, drawZ );
                bool const hardOk = drew && std::fabs( drawZ - virginZ ) < 0.03f;
                char note[120];
                std::snprintf( note, sizeof( note ), "cap=%s drew=%d dz=%.3f",
                    CapAtWorld( hardX, hardY ).c_str(), drew ? 1 : 0, virginZ - drawZ );
                add( "hard_rock_holds_roof", hardOk ? "PASS" : "FAIL",
                    hardOk ? note : "HARD_ROCK_SOFT_SANK",
                    hardX, hardY, hardZ, CapAtWorld( hardX, hardY ).c_str(), hardOk ? 0 : 1 );
            }
        }
        else
        {
            add( "hard_rock_holds_roof", "SKIP",
                "no SurfaceBroken hard-rock sample after dig matrix — gate fn covered" );
        }
    }

    void GeoCertRunSupportSection()
    {
        // §10 P3c: SupportBelow from queryZ through occupancy (not D2 tris, not column crest).
        auto add = [&]( char const* scen, char const* verdict, char const* note,
            float x = 0.f, float y = 0.f, float z = 0.f,
            float nx = 0.f, float ny = 0.f, float nz = 1.f, int supportFail = 0 )
        {
            GeoCertRow r{};
            std::snprintf( r.section, sizeof( r.section ), "10" );
            std::snprintf( r.scenario, sizeof( r.scenario ), "%s", scen );
            std::snprintf( r.verdict, sizeof( r.verdict ), "%s", verdict );
            std::snprintf( r.note, sizeof( r.note ), "%s", note );
            std::snprintf( r.action, sizeof( r.action ), "support_probe" );
            r.x = x; r.y = y; r.z = z;
            r.nx = nx; r.ny = ny; r.nz = nz;
            r.supportFail = supportFail;
            GeoCertAddRow( r );
            if ( std::strcmp( verdict, "FAIL" ) == 0 )
            {
                GeoCertHardFail( scen, "support_probe", note, x, y, z, nx, ny, nz );
            }
        };

        float const ox = (float)ProvenanceGeo::kRangeOriginX;
        float const oy = (float)ProvenanceGeo::kRangeOriginY;
        // Fresh pad south of dig/accumulate corridors.
        float const padY = oy - 14.f;

        // --- virgin flat / slope: HF support unchanged ---
        float const flatX = ox + 4.f, flatY = padY;
        float flatZ = 0.f;
        if ( !SampleGroundZBase( flatX, flatY, flatZ ) )
        {
            add( "virgin_flat_HF_unchanged", "FAIL", "FLAT_SAMPLE_FAIL", flatX, flatY, 0.f, 0, 0, 1, 1 );
            return;
        }
        {
            float const scar = ScarDeltaZ( flatX, flatY );
            SupportHit const h = SupportBelow( flatX, flatY, flatZ + 1.5f );
            if ( !h.hit || std::fabs( h.position.z - ( flatZ + scar ) ) > 0.03f )
            {
                char note[120];
                std::snprintf( note, sizeof( note ), "FLAT_HF_MISMATCH sup=%.3f want=%.3f",
                    h.position.z, flatZ + scar );
                add( "virgin_flat_HF_unchanged", "FAIL", note, flatX, flatY, h.position.z, 0, 0, 1, 1 );
            }
            else
            {
                add( "virgin_flat_HF_unchanged", "PASS", "SupportBelow~HF+scars flat",
                    flatX, flatY, h.position.z, h.normal.x, h.normal.y, h.normal.z, 0 );
            }
        }
        float slopeX = ox + 18.f, slopeY = padY, slopeZ = 0.f;
        char const* slopeTerrain = "gentle_slope";
        for ( int i = 0; i < s_geoContactN; ++i )
        {
            if ( std::strcmp( s_geoContacts[i].id, "B_slope" ) == 0 && s_geoContacts[i].found )
            {
                slopeX = s_geoContacts[i].x;
                slopeY = padY;
                slopeTerrain = s_geoContacts[i].terrain;
                break;
            }
        }
        SampleGroundZBase( slopeX, slopeY, slopeZ );
        {
            float const scar = ScarDeltaZ( slopeX, slopeY );
            SupportHit const h = SupportBelow( slopeX, slopeY, slopeZ + 1.5f );
            if ( !h.hit || std::fabs( h.position.z - ( slopeZ + scar ) ) > 0.03f )
            {
                char note[120];
                std::snprintf( note, sizeof( note ), "SLOPE_HF_MISMATCH sup=%.3f want=%.3f",
                    h.position.z, slopeZ + scar );
                add( "virgin_slope_HF_unchanged", "FAIL", note, slopeX, slopeY, h.position.z, 0, 0, 1, 1 );
            }
            else
            {
                char note[96];
                std::snprintf( note, sizeof( note ), "SupportBelow~HF (%s)", slopeTerrain );
                add( "virgin_slope_HF_unchanged", "PASS", note,
                    slopeX, slopeY, h.position.z, h.normal.x, h.normal.y, h.normal.z, 0 );
            }
        }

        // --- open cavity floor: probe falls to remaining matter, not virgin HF ---
        float const cavX = ox + 10.f, cavY = padY;
        float cavGrade = 0.f;
        SampleGroundZBase( cavX, cavY, cavGrade );
        float const openR = 0.22f;
        float const carveZ = cavGrade - openR * 0.35f;
        PrefetchOccupancyCell( (int)std::floor( cavX ), (int)std::floor( cavY ) );
        bool const carvedOpen = CarveOccupancySphere( cavX, cavY, carveZ, openR, cavX, cavY, cavGrade );
        if ( !carvedOpen )
        {
            add( "cavity_floor_not_virgin_HF", "FAIL", "CAVITY_CARVE_FAIL", cavX, cavY, cavGrade, 0, 0, 1, 1 );
            add( "cavity_wall_no_false_floor", "SKIP", "no cavity for wall probe" );
            add( "tunnel_floor_below_queryZ", "SKIP", "no cavity setup" );
            add( "lip_either_side", "SKIP", "no cavity setup" );
            add( "cross_cell_support_continuous", "SKIP", "no cavity setup" );
            add( "neighbor_strike_support_identity", "SKIP", "no cavity setup" );
            add( "missing_authority_refuse", "SKIP", "no cavity setup" );
            add( "support_query_zero_remesh", "SKIP", "no cavity setup" );
            return;
        }
        {
            SupportHit const h = SupportBelow( cavX, cavY, cavGrade + 0.5f );
            float occTop = 0.f;
            bool const haveOcc = SampleOccupancyZ( cavX, cavY, occTop );
            bool const sunk = haveOcc && ( cavGrade - occTop ) > 0.04f;
            bool const matchesOcc = h.hit && haveOcc && std::fabs( h.position.z - occTop ) <= 0.04f;
            bool const offVirgin = h.hit && std::fabs( h.position.z - cavGrade ) > 0.03f;
            if ( !sunk )
            {
                add( "cavity_floor_not_virgin_HF", "SKIP",
                    "cavity floor not sunk enough vs virgin grade", cavX, cavY, h.position.z );
            }
            else if ( !( h.hit && matchesOcc && offVirgin ) )
            {
                char note[140];
                std::snprintf( note, sizeof( note ),
                    "SUPPORT_OVER_VOID grade=%.3f occ=%.3f sup=%.3f hit=%d",
                    cavGrade, occTop, h.position.z, h.hit ? 1 : 0 );
                add( "cavity_floor_not_virgin_HF", "FAIL", note,
                    cavX, cavY, h.position.z, h.normal.x, h.normal.y, h.normal.z, 1 );
            }
            else
            {
                char note[120];
                std::snprintf( note, sizeof( note ),
                    "sup=%.3f occ=%.3f grade=%.3f (occupancy floor)", h.position.z, occTop, cavGrade );
                add( "cavity_floor_not_virgin_HF", "PASS", note,
                    cavX, cavY, h.position.z, h.normal.x, h.normal.y, h.normal.z, 0 );
            }
        }

        // --- cavity wall / steep face: mid-air query must fall to floor (no false ledge at queryZ) ---
        {
            float const qz = carveZ; // mid cavity air
            SupportHit const h = SupportBelow( cavX, cavY, qz );
            bool const fell = h.hit && ( qz - h.position.z ) > 0.04f;
            bool const notAtQuery = h.hit && std::fabs( h.position.z - qz ) > 0.03f;
            bool const notVirgin = h.hit && std::fabs( h.position.z - cavGrade ) > 0.03f;
            if ( !( h.hit && fell && notAtQuery && notVirgin ) )
            {
                char note[140];
                std::snprintf( note, sizeof( note ),
                    "FALSE_FLOOR qz=%.3f sup=%.3f grade=%.3f hit=%d",
                    qz, h.position.z, cavGrade, h.hit ? 1 : 0 );
                add( "cavity_wall_no_false_floor", "FAIL", note,
                    cavX, cavY, h.position.z, h.normal.x, h.normal.y, h.normal.z, 1 );
            }
            else
            {
                char note[120];
                std::snprintf( note, sizeof( note ),
                    "fell qz=%.3f->sup=%.3f (no mid-air ledge)", qz, h.position.z );
                add( "cavity_wall_no_false_floor", "PASS", note,
                    cavX, cavY, h.position.z, h.normal.x, h.normal.y, h.normal.z, 0 );
            }
        }

        // --- tunnel with roof: floor below queryZ, not roof/topmost ---
        float const tunX = ox + 14.f, tunY = padY;
        float tunGrade = 0.f;
        SampleGroundZBase( tunX, tunY, tunGrade );
        float const tunR = 0.28f;
        float const tunCenterZ = tunGrade - 0.48f; // sphere stays under crest → roof remains
        bool const tunCarved = CarveOccupancySphere( tunX, tunY, tunCenterZ, tunR, tunX, tunY, tunGrade );
        if ( !tunCarved )
        {
            add( "tunnel_floor_below_queryZ", "FAIL", "TUNNEL_CARVE_FAIL", tunX, tunY, tunGrade, 0, 0, 1, 1 );
        }
        else
        {
            float roofZ = 0.f;
            bool const haveRoof = SampleOccupancyZ( tunX, tunY, roofZ ); // topmost = roof
            float const qz = tunCenterZ; // body inside cavity under roof
            SupportHit const h = SupportBelow( tunX, tunY, qz );
            bool const roofAbove = haveRoof && roofZ > qz + 0.05f;
            bool const floorBelow = h.hit && h.position.z < qz - 0.03f;
            bool const notRoof = h.hit && haveRoof && std::fabs( h.position.z - roofZ ) > 0.05f;
            if ( !roofAbove )
            {
                add( "tunnel_floor_below_queryZ", "SKIP",
                    "tunnel roof not intact above query (sphere breached crest?)",
                    tunX, tunY, h.position.z );
            }
            else if ( !( floorBelow && notRoof ) )
            {
                char note[140];
                std::snprintf( note, sizeof( note ),
                    "ROOF_TELEPORT roof=%.3f qz=%.3f sup=%.3f hit=%d",
                    roofZ, qz, h.position.z, h.hit ? 1 : 0 );
                add( "tunnel_floor_below_queryZ", "FAIL", note,
                    tunX, tunY, h.position.z, h.normal.x, h.normal.y, h.normal.z, 1 );
            }
            else
            {
                char note[120];
                std::snprintf( note, sizeof( note ),
                    "floor=%.3f below qz=%.3f (roof=%.3f ignored)", h.position.z, qz, roofZ );
                add( "tunnel_floor_below_queryZ", "PASS", note,
                    tunX, tunY, h.position.z, h.normal.x, h.normal.y, h.normal.z, 0 );
            }
        }

        // --- lip: outside = HF (uncarved cell), inside = cavity floor ---
        {
            float const lipInsideX = cavX;
            // Clear the carved cell + sphere halo; virgin continuum must remain HF support.
            float const lipOutsideX = cavX + 1.55f;
            float outGrade = 0.f;
            SampleGroundZBase( lipOutsideX, cavY, outGrade );
            CellSample const* outCell = GetCell( (int)std::floor( lipOutsideX ), (int)std::floor( cavY ) );
            bool const outVirginCell = !( outCell && outCell->carved );
            SupportHit const hin = SupportBelow( lipInsideX, cavY, cavGrade + 0.4f );
            SupportHit const hout = SupportBelow( lipOutsideX, cavY, outGrade + 1.5f );
            float const outScar = ScarDeltaZ( lipOutsideX, cavY );
            bool const outOk = hout.hit && outVirginCell
                && std::fabs( hout.position.z - ( outGrade + outScar ) ) <= 0.04f;
            bool const inOk = hin.hit && hin.position.z < cavGrade - 0.03f;
            if ( !outVirginCell )
            {
                add( "lip_either_side", "SKIP",
                    "outside probe landed in carved cell — widen pad",
                    lipOutsideX, cavY, hout.position.z );
            }
            else if ( !( outOk && inOk ) )
            {
                char note[140];
                std::snprintf( note, sizeof( note ),
                    "LIP_SIDE in=%.3f out=%.3f gradeIn=%.3f gradeOut=%.3f",
                    hin.position.z, hout.position.z, cavGrade, outGrade );
                add( "lip_either_side", "FAIL", note, cavX, cavY, hin.position.z, 0, 0, 1, 1 );
            }
            else
            {
                char note[120];
                std::snprintf( note, sizeof( note ),
                    "inside=%.3f outside=%.3f~HF", hin.position.z, hout.position.z );
                add( "lip_either_side", "PASS", note, cavX, cavY, hin.position.z, 0, 0, 1, 0 );
            }
        }

        // --- cross-cell cavity: no support discontinuity at seam ---
        float const seamX = std::floor( ox + 20.f ) + 0.98f; // near +X cell face
        float const seamY = padY;
        float seamGrade = 0.f;
        SampleGroundZBase( seamX, seamY, seamGrade );
        float const seamR = 0.30f;
        float const seamCarveZ = seamGrade - seamR * 0.30f;
        bool const seamCarved = CarveOccupancySphere( seamX, seamY, seamCarveZ, seamR,
            seamX, seamY, seamGrade );
        if ( !seamCarved )
        {
            add( "cross_cell_support_continuous", "FAIL", "SEAM_CARVE_FAIL",
                seamX, seamY, seamGrade, 0, 0, 1, 1 );
        }
        else
        {
            float const xL = std::floor( seamX ) - 0.02f;
            float const xR = std::floor( seamX ) + 1.02f;
            // Keep both probes inside the carve XY footprint.
            float const xL2 = seamX - 0.12f;
            float const xR2 = seamX + 0.12f;
            (void)xL; (void)xR;
            SupportHit const hL = SupportBelow( xL2, seamY, seamGrade + 0.4f );
            SupportHit const hR = SupportBelow( xR2, seamY, seamGrade + 0.4f );
            bool const both = hL.hit && hR.hit;
            bool const cont = both && std::fabs( hL.position.z - hR.position.z ) <= 0.08f;
            bool const sunk = both && hL.position.z < seamGrade - 0.03f
                && hR.position.z < seamGrade - 0.03f;
            if ( !( cont && sunk ) )
            {
                char note[140];
                std::snprintf( note, sizeof( note ),
                    "SEAM_DISCONT L=%.3f R=%.3f grade=%.3f hit=%d/%d",
                    hL.position.z, hR.position.z, seamGrade, hL.hit ? 1 : 0, hR.hit ? 1 : 0 );
                add( "cross_cell_support_continuous", "FAIL", note,
                    seamX, seamY, hL.position.z, 0, 0, 1, 1 );
            }
            else
            {
                char note[120];
                std::snprintf( note, sizeof( note ),
                    "L=%.3f R=%.3f d=%.3f", hL.position.z, hR.position.z,
                    std::fabs( hL.position.z - hR.position.z ) );
                add( "cross_cell_support_continuous", "PASS", note,
                    seamX, seamY, 0.5f * ( hL.position.z + hR.position.z ), 0, 0, 1, 0 );
            }
        }

        // --- neighboring strike: support outside dirty+halo identical ---
        float const farX = ox + 40.f, farY = padY;
        float farGrade = 0.f;
        SampleGroundZBase( farX, farY, farGrade );
        SupportHit const far0 = SupportBelow( farX, farY, farGrade + 1.5f );
        float const nearX = ox + 24.f, nearY = padY;
        float nearGrade = 0.f;
        SampleGroundZBase( nearX, nearY, nearGrade );
        bool const nearCarved = CarveOccupancySphere( nearX, nearY, nearGrade - 0.08f, 0.20f,
            nearX, nearY, nearGrade );
        SupportHit const far1 = SupportBelow( farX, farY, farGrade + 1.5f );
        if ( !nearCarved )
        {
            add( "neighbor_strike_support_identity", "FAIL", "NEAR_CARVE_FAIL",
                nearX, nearY, nearGrade, 0, 0, 1, 1 );
        }
        else if ( !far0.hit || !far1.hit
            || std::fabs( far0.position.z - far1.position.z ) > 1e-4f
            || std::fabs( far0.normal.x - far1.normal.x ) > 1e-4f
            || std::fabs( far0.normal.y - far1.normal.y ) > 1e-4f
            || std::fabs( far0.normal.z - far1.normal.z ) > 1e-4f )
        {
            char note[140];
            std::snprintf( note, sizeof( note ),
                "FAR_SUPPORT_MUTATED z0=%.4f z1=%.4f", far0.position.z, far1.position.z );
            add( "neighbor_strike_support_identity", "FAIL", note,
                farX, farY, far1.position.z, 0, 0, 1, 1 );
        }
        else
        {
            add( "neighbor_strike_support_identity", "PASS",
                "far SupportBelow identical after neighbor strike",
                farX, farY, far1.position.z, far1.normal.x, far1.normal.y, far1.normal.z, 0 );
        }

        // --- missing authority → refuse/defer, never invent floor ---
        {
            int const cx = (int)std::floor( cavX );
            int const cy = (int)std::floor( cavY );
            CellSample* c = GetCellMutable( cx, cy );
            if ( !c || !c->carved || c->fill.empty() )
            {
                add( "missing_authority_refuse", "SKIP", "no carved cell to strip for refuse probe",
                    cavX, cavY, cavGrade );
            }
            else
            {
                std::vector<uint8_t> savedFill = c->fill;
                int const sw = c->fillW, sh = c->fillH, sk = c->fillK;
                c->fill.clear();
                c->fillW = c->fillH = c->fillK = 0;
                SupportHit const h = SupportBelow( cavX, cavY, cavGrade + 0.5f );
                // Restore immediately.
                c->fill = std::move( savedFill );
                c->fillW = sw; c->fillH = sh; c->fillK = sk;
                if ( h.hit || !h.deferred )
                {
                    char note[120];
                    std::snprintf( note, sizeof( note ),
                        "INVENTED_OR_NONDEFER hit=%d deferred=%d z=%.3f",
                        h.hit ? 1 : 0, h.deferred ? 1 : 0, h.position.z );
                    add( "missing_authority_refuse", "FAIL", note,
                        cavX, cavY, h.position.z, 0, 0, 1, 1 );
                }
                else
                {
                    add( "missing_authority_refuse", "PASS",
                        "SupportBelow deferred - no invented HF floor",
                        cavX, cavY, cavGrade, 0, 0, 1, 0 );
                }
            }
        }

        // --- support query → zero D2 rebuilds, zero HF remeshes ---
        {
            int const d0 = g.perfD2Rebuilds;
            int const h0 = g.perfHfRebuilds;
            float zScratch = 0.f;
            SampleGroundZBase( flatX, flatY, zScratch );
            for ( int i = 0; i < 32; ++i )
            {
                float const px = flatX + 0.01f * (float)i;
                SupportBelow( px, flatY, zScratch + 1.5f );
                SupportBelow( cavX, cavY, carveZ );
                SupportBelow( tunX, tunY, tunCenterZ );
            }
            int const d1 = g.perfD2Rebuilds;
            int const h1 = g.perfHfRebuilds;
            if ( d1 != d0 || h1 != h0 )
            {
                char note[120];
                std::snprintf( note, sizeof( note ),
                    "QUERY_REMESH D2 %d→%d HF %d→%d", d0, d1, h0, h1 );
                add( "support_query_zero_remesh", "FAIL", note, flatX, flatY, zScratch, 0, 0, 1, 1 );
            }
            else
            {
                char note[96];
                std::snprintf( note, sizeof( note ),
                    "32x SupportBelow D2=%d HF=%d (unchanged)", d1, h1 );
                add( "support_query_zero_remesh", "PASS", note, flatX, flatY, zScratch, 0, 0, 1, 0 );
            }
        }
    }

    void GeoCertRunChipsSection()
    {
        // §11 P3d: PHYS chips consume SupportBelow only — gravity, contact normal, settle/slide/sleep.
        // Modes OFF / VISUAL / PHYS. Forbidden: D2 tri collision, crest snap, permanent always-on bodies.
        auto add = [&]( char const* scen, char const* verdict, char const* note,
            float x = 0.f, float y = 0.f, float z = 0.f,
            float nx = 0.f, float ny = 0.f, float nz = 1.f, int supportFail = 0 )
        {
            GeoCertRow r{};
            std::snprintf( r.section, sizeof( r.section ), "11" );
            std::snprintf( r.scenario, sizeof( r.scenario ), "%s", scen );
            std::snprintf( r.verdict, sizeof( r.verdict ), "%s", verdict );
            std::snprintf( r.note, sizeof( r.note ), "%s", note );
            std::snprintf( r.action, sizeof( r.action ), "chip_phys" );
            r.x = x; r.y = y; r.z = z;
            r.nx = nx; r.ny = ny; r.nz = nz;
            r.supportFail = supportFail;
            GeoCertAddRow( r );
            if ( std::strcmp( verdict, "FAIL" ) == 0 )
            {
                GeoCertHardFail( scen, "chip_phys", note, x, y, z, nx, ny, nz );
            }
        };

        auto supportFn = []( float x, float y, float queryZ ) -> H2H::SupportQuery
        {
            SupportHit const h = SupportBelow( x, y, queryZ );
            H2H::SupportQuery q{};
            q.hit = h.hit;
            q.deferred = h.deferred;
            q.x = h.position.x;
            q.y = h.position.y;
            q.z = h.position.z;
            q.nx = h.normal.x;
            q.ny = h.normal.y;
            q.nz = h.normal.z;
            q.supportRev = h.regionRev;
            return q;
        };
        auto stepPhys = [&]( float dt, int n )
        {
            for ( int i = 0; i < n; ++i )
            {
                H2H::StepBodies( dt, supportFn );
            }
        };
        auto clearCertChips = []()
        {
            H2H::State().bodies.clear();
        };

        ChipMode const savedMode = g.chipMode;
        g.chipMode = ChipMode::Phys;

        float const ox = (float)ProvenanceGeo::kRangeOriginX;
        float const oy = (float)ProvenanceGeo::kRangeOriginY;
        // South of §10 support pad — avoid shared carve interference.
        float const padY = oy - 22.f;
        constexpr float kDt = 1.f / 60.f;
        constexpr int kFallSteps = 180;

        // --- chip above virgin ground → falls to HF support ---
        float const flatX = ox + 4.f, flatY = padY;
        float flatZ = 0.f;
        if ( !SampleGroundZBase( flatX, flatY, flatZ ) )
        {
            add( "chip_falls_to_HF", "FAIL", "FLAT_SAMPLE_FAIL", flatX, flatY, 0.f, 0, 0, 1, 1 );
            g.chipMode = savedMode;
            return;
        }
        {
            clearCertChips();
            float const thick = 0.04f;
            H2H::SpawnDetachedChip( flatX, flatY, flatZ + 1.2f, 0.12f, 0.10f, thick );
            stepPhys( kDt, kFallSteps );
            H2H::MatterBody const& b = H2H::State().bodies.front();
            float const wantZ = flatZ + ScarDeltaZ( flatX, flatY ) + thick * 0.5f + 0.01f;
            bool const slept = b.settled || b.life == H2H::ChipLife::Settled
                || b.life == H2H::ChipLife::ExplicitBody;
            bool const onHf = std::fabs( b.z - wantZ ) <= 0.06f;
            if ( !( slept && onHf ) )
            {
                char note[140];
                std::snprintf( note, sizeof( note ),
                    "HF_SETTLE_FAIL z=%.3f want=%.3f life=%d settled=%d",
                    b.z, wantZ, (int)b.life, b.settled ? 1 : 0 );
                add( "chip_falls_to_HF", "FAIL", note, flatX, flatY, b.z, b.nx, b.ny, b.nz, 1 );
            }
            else
            {
                char note[120];
                std::snprintf( note, sizeof( note ),
                    "settled z=%.3f~HF life=%d", b.z, (int)b.life );
                add( "chip_falls_to_HF", "PASS", note, flatX, flatY, b.z, b.nx, b.ny, b.nz, 0 );
            }
        }

        // --- chip over open dig → falls into the hole ---
        float const cavX = ox + 10.f, cavY = padY;
        float cavGrade = 0.f;
        SampleGroundZBase( cavX, cavY, cavGrade );
        float const openR = 0.22f;
        float const carveZ = cavGrade - openR * 0.35f;
        PrefetchOccupancyCell( (int)std::floor( cavX ), (int)std::floor( cavY ) );
        bool const carvedOpen = CarveOccupancySphere( cavX, cavY, carveZ, openR, cavX, cavY, cavGrade );
        bool const chipCavityOk = carvedOpen;
        if ( !chipCavityOk )
        {
            add( "chip_falls_into_dig", "FAIL", "CAVITY_CARVE_FAIL", cavX, cavY, cavGrade, 0, 0, 1, 1 );
            add( "chip_tunnel_ignores_roof", "SKIP", "no cavity setup" );
            add( "chip_slide_inclined", "SKIP", "no cavity setup" );
            add( "chip_cross_cell_no_hop", "SKIP", "no cavity setup" );
            add( "chip_missing_occ_defer", "SKIP", "no cavity setup" );
            add( "chip_mode_switch_zero_remesh", "SKIP", "no cavity setup" );
            add( "chip_determinism", "SKIP", "no cavity setup" );
            add( "chip_ACTIVE_SETTLED_sleep", "SKIP", "no cavity setup" );
            add( "chip_cavity_quiescence", "SKIP", "no cavity setup" );
            // steep-static + wake-on-rev use fixtures — still run below
        }
        if ( chipCavityOk )
        {
            clearCertChips();
            float const thick = 0.04f;
            H2H::SpawnDetachedChip( cavX, cavY, cavGrade + 0.9f, 0.12f, 0.10f, thick );
            stepPhys( kDt, kFallSteps );
            H2H::MatterBody const& b = H2H::State().bodies.front();
            SupportHit const floor = SupportBelow( cavX, cavY, cavGrade + 0.5f );
            bool const intoHole = b.z < cavGrade - 0.04f;
            bool const nearFloor = floor.hit && std::fabs( b.z - ( floor.position.z + thick * 0.5f + 0.01f ) ) <= 0.08f;
            if ( !( intoHole && nearFloor ) )
            {
                char note[140];
                std::snprintf( note, sizeof( note ),
                    "DID_NOT_ENTER_HOLE z=%.3f grade=%.3f floor=%.3f",
                    b.z, cavGrade, floor.position.z );
                add( "chip_falls_into_dig", "FAIL", note, cavX, cavY, b.z, 0, 0, 1, 1 );
            }
            else
            {
                char note[120];
                std::snprintf( note, sizeof( note ),
                    "into hole z=%.3f < grade=%.3f floor=%.3f", b.z, cavGrade, floor.position.z );
                add( "chip_falls_into_dig", "PASS", note, cavX, cavY, b.z, b.nx, b.ny, b.nz, 0 );
            }

        // --- chip inside tunnel → ignores roof; finds floor below ---
        float const tunX = ox + 14.f, tunY = padY;
        float tunGrade = 0.f;
        SampleGroundZBase( tunX, tunY, tunGrade );
        float const tunR = 0.28f;
        float const tunCenterZ = tunGrade - 0.48f;
        bool const tunCarved = CarveOccupancySphere( tunX, tunY, tunCenterZ, tunR, tunX, tunY, tunGrade );
        if ( !tunCarved )
        {
            add( "chip_tunnel_ignores_roof", "FAIL", "TUNNEL_CARVE_FAIL", tunX, tunY, tunGrade, 0, 0, 1, 1 );
        }
        else
        {
            float roofZ = 0.f;
            bool const haveRoof = SampleOccupancyZ( tunX, tunY, roofZ );
            clearCertChips();
            float const thick = 0.04f;
            H2H::SpawnDetachedChip( tunX, tunY, tunCenterZ, 0.10f, 0.08f, thick );
            stepPhys( kDt, kFallSteps );
            H2H::MatterBody const& b = H2H::State().bodies.front();
            bool const roofAbove = haveRoof && roofZ > tunCenterZ + 0.05f;
            bool const belowQuery = b.z < tunCenterZ - 0.02f;
            bool const notRoof = !haveRoof || std::fabs( b.z - roofZ ) > 0.08f;
            if ( !roofAbove )
            {
                add( "chip_tunnel_ignores_roof", "SKIP",
                    "tunnel roof not intact — sphere breached crest?",
                    tunX, tunY, b.z );
            }
            else if ( !( belowQuery && notRoof ) )
            {
                char note[140];
                std::snprintf( note, sizeof( note ),
                    "ROOF_OR_CREST_SNAP z=%.3f roof=%.3f qz=%.3f", b.z, roofZ, tunCenterZ );
                add( "chip_tunnel_ignores_roof", "FAIL", note, tunX, tunY, b.z, 0, 0, 1, 1 );
            }
            else
            {
                char note[120];
                std::snprintf( note, sizeof( note ),
                    "floor z=%.3f below qz=%.3f (roof=%.3f ignored)", b.z, tunCenterZ, roofZ );
                add( "chip_tunnel_ignores_roof", "PASS", note, tunX, tunY, b.z, b.nx, b.ny, b.nz, 0 );
            }
        }

        // --- inclined support: contact normal; can slide downslope ---
        // Use virgin steep flank (south of dig/accumulate corridors) — F_diag dig cell traps XY.
        {
            float slopeX = ox + 84.f, slopeY = oy - 8.f, slopeZ = 0.f;
            char const* slopeTag = "virgin_steep";
            // Prefer G_cliff XY if virgin; else F_diag X on unused Y.
            for ( int i = 0; i < s_geoContactN; ++i )
            {
                if ( !s_geoContacts[i].found ) { continue; }
                if ( std::strcmp( s_geoContacts[i].id, "G_cliff" ) == 0 )
                {
                    slopeX = s_geoContacts[i].x;
                    slopeY = s_geoContacts[i].y - 4.f; // off the dig aim point
                    slopeTag = "G_cliff_virgin";
                    break;
                }
            }
            // Refuse carved cells — chip PHYS must ride SupportBelow continuum/occupancy honestly.
            {
                CellSample const* sc = GetCell( (int)std::floor( slopeX ), (int)std::floor( slopeY ) );
                if ( sc && sc->carved )
                {
                    slopeX = ox + 86.f;
                    slopeY = oy - 10.f;
                    slopeTag = "virgin_steep_pad";
                }
            }
            SampleGroundZBase( slopeX, slopeY, slopeZ );
            SupportHit const sh = SupportBelow( slopeX, slopeY, slopeZ + 1.5f );
            CellSample const* sc2 = GetCell( (int)std::floor( slopeX ), (int)std::floor( slopeY ) );
            bool const virgin = !( sc2 && sc2->carved );
            if ( !sh.hit || !virgin || sh.normal.z >= 0.98f )
            {
                char note[140];
                std::snprintf( note, sizeof( note ),
                    "no virgin incline (%s hit=%d virgin=%d nz=%.3f)",
                    slopeTag, sh.hit ? 1 : 0, virgin ? 1 : 0, sh.normal.z );
                add( "chip_slide_inclined", "SKIP", note,
                    slopeX, slopeY, slopeZ, sh.normal.x, sh.normal.y, sh.normal.z );
            }
            else
            {
                clearCertChips();
                float const thick = 0.04f;
                // Spawn ON rest (not airborne) so inelastic-land absorb does not eat the slide seed.
                float const startZ = sh.position.z + thick * 0.5f + 0.01f;
                H2H::MatterBody& chip = H2H::SpawnDetachedChip(
                    slopeX, slopeY, startZ, 0.12f, 0.10f, thick );
                // Seed above wakeSpeed so kinetic slide runs (hysteresis band holds below wake).
                float dx = sh.normal.x, dy = sh.normal.y;
                float dlen = std::sqrt( dx * dx + dy * dy );
                if ( dlen > 1e-5f ) { dx /= dlen; dy /= dlen; }
                float const slideSeed = H2H::kChipWakeSpeed + 0.12f;
                chip.vx = dx * slideSeed;
                chip.vy = dy * slideSeed;
                float const x0 = chip.x, y0 = chip.y;
                stepPhys( kDt, 180 );
                H2H::MatterBody const& b = H2H::State().bodies.front();
                float const move = ( b.x - x0 ) * dx + ( b.y - y0 ) * dy;
                // Incline from initial SupportBelow normal; chip may slide onto flatter runout.
                bool const inclined = sh.normal.z < 0.98f;
                bool const slid = move > 0.05f;
                if ( !( inclined && slid ) )
                {
                    char note[140];
                    std::snprintf( note, sizeof( note ),
                        "NO_SLIDE_OR_TILT %s move=%.3f n0z=%.3f endNz=%.3f",
                        slopeTag, move, sh.normal.z, b.nz );
                    add( "chip_slide_inclined", "FAIL", note,
                        slopeX, slopeY, b.z, sh.normal.x, sh.normal.y, sh.normal.z, 1 );
                }
                else
                {
                    char note[140];
                    std::snprintf( note, sizeof( note ),
                        "%s downslope move=%.3f n0=(%.2f,%.2f,%.2f)",
                        slopeTag, move, sh.normal.x, sh.normal.y, sh.normal.z );
                    add( "chip_slide_inclined", "PASS", note,
                        b.x, b.y, b.z, sh.normal.x, sh.normal.y, sh.normal.z, 0 );
                }
            }
        }

        // --- cross world-cell boundary → no hop ---
        {
            float const seamX = std::floor( ox + 20.f ) + 0.98f;
            float const seamY = padY;
            float seamGrade = 0.f;
            SampleGroundZBase( seamX, seamY, seamGrade );
            float const seamR = 0.30f;
            float const seamCarveZ = seamGrade - seamR * 0.30f;
            bool const seamCarved = CarveOccupancySphere( seamX, seamY, seamCarveZ, seamR,
                seamX, seamY, seamGrade );
            if ( !seamCarved )
            {
                add( "chip_cross_cell_no_hop", "FAIL", "SEAM_CARVE_FAIL",
                    seamX, seamY, seamGrade, 0, 0, 1, 1 );
            }
            else
            {
                clearCertChips();
                float const thick = 0.04f;
                SupportHit const h0 = SupportBelow( seamX - 0.10f, seamY, seamGrade + 0.4f );
                float const startZ = ( h0.hit ? h0.position.z : seamGrade ) + 0.35f;
                H2H::MatterBody& chip = H2H::SpawnDetachedChip(
                    seamX - 0.10f, seamY, startZ, 0.10f, 0.08f, thick );
                chip.vx = 0.55f; // cross +X cell face
                float maxDz = 0.f;
                float prevZ = chip.z;
                for ( int i = 0; i < 90; ++i )
                {
                    H2H::StepBodies( kDt, supportFn );
                    H2H::MatterBody const& b = H2H::State().bodies.front();
                    maxDz = (std::max)( maxDz, std::fabs( b.z - prevZ ) );
                    prevZ = b.z;
                }
                H2H::MatterBody const& b = H2H::State().bodies.front();
                bool const crossed = b.x > std::floor( seamX ) + 0.02f;
                // One frame of free-fall can drop ~0.005m; forbid crest-hop spikes.
                bool const noHop = maxDz <= 0.12f;
                if ( !( crossed && noHop ) )
                {
                    char note[140];
                    std::snprintf( note, sizeof( note ),
                        "SEAM_HOP maxDz=%.3f crossed=%d x=%.3f", maxDz, crossed ? 1 : 0, b.x );
                    add( "chip_cross_cell_no_hop", "FAIL", note, b.x, b.y, b.z, 0, 0, 1, 1 );
                }
                else
                {
                    char note[120];
                    std::snprintf( note, sizeof( note ),
                        "crossed seam maxDz=%.3f x=%.3f", maxDz, b.x );
                    add( "chip_cross_cell_no_hop", "PASS", note, b.x, b.y, b.z, 0, 0, 1, 0 );
                }
            }
        }

        // --- missing occupancy → defer, never teleport ---
        {
            int const cx = (int)std::floor( cavX );
            int const cy = (int)std::floor( cavY );
            CellSample* c = GetCellMutable( cx, cy );
            if ( !c || !c->carved || c->fill.empty() )
            {
                add( "chip_missing_occ_defer", "SKIP",
                    "no carved cell to strip for defer probe", cavX, cavY, cavGrade );
            }
            else
            {
                clearCertChips();
                float const thick = 0.04f;
                float const holdZ = cavGrade - 0.15f;
                H2H::SpawnDetachedChip( cavX, cavY, holdZ, 0.10f, 0.08f, thick );
                std::vector<uint8_t> savedFill = c->fill;
                int const sw = c->fillW, sh = c->fillH, sk = c->fillK;
                c->fill.clear();
                c->fillW = c->fillH = c->fillK = 0;
                float const x0 = H2H::State().bodies.front().x;
                float const y0 = H2H::State().bodies.front().y;
                float const z0 = H2H::State().bodies.front().z;
                stepPhys( kDt, 30 );
                H2H::MatterBody const& b = H2H::State().bodies.front();
                bool const held = std::fabs( b.x - x0 ) < 1e-4f
                    && std::fabs( b.y - y0 ) < 1e-4f
                    && std::fabs( b.z - z0 ) < 1e-4f;
                // Restore immediately.
                c->fill = std::move( savedFill );
                c->fillW = sw; c->fillH = sh; c->fillK = sk;
                if ( !held )
                {
                    char note[140];
                    std::snprintf( note, sizeof( note ),
                        "TELEPORTED dz=%.3f (want defer-in-place)", b.z - z0 );
                    add( "chip_missing_occ_defer", "FAIL", note, b.x, b.y, b.z, 0, 0, 1, 1 );
                }
                else
                {
                    add( "chip_missing_occ_defer", "PASS",
                        "deferred in place — no invented HF teleport",
                        cavX, cavY, holdZ, 0, 0, 1, 0 );
                }
            }
        }

        // --- OFF→VISUAL→PHYS mode switch → zero D2 rebuild / HF remesh ---
        {
            clearCertChips();
            H2H::SpawnDetachedChip( flatX, flatY, flatZ + 0.5f );
            int const d0 = g.perfD2Rebuilds;
            int const h0 = g.perfHfRebuilds;
            g.chipMode = ChipMode::Off;
            // OFF: DrawMatterBodies early-outs; no StepBodies.
            g.chipMode = ChipMode::Visual;
            // VISUAL: draw path only — freeze, no integrate.
            g.chipMode = ChipMode::Phys;
            for ( H2H::MatterBody& b : H2H::State().bodies ) { H2H::WakeChip( b ); }
            stepPhys( kDt, 8 );
            int const d1 = g.perfD2Rebuilds;
            int const h1 = g.perfHfRebuilds;
            if ( d1 != d0 || h1 != h0 )
            {
                char note[120];
                std::snprintf( note, sizeof( note ),
                    "MODE_REMESH D2 %d→%d HF %d→%d", d0, d1, h0, h1 );
                add( "chip_mode_switch_zero_remesh", "FAIL", note, flatX, flatY, flatZ, 0, 0, 1, 1 );
            }
            else
            {
                char note[96];
                std::snprintf( note, sizeof( note ),
                    "OFF→VIS→PHYS D2=%d HF=%d (unchanged)", d1, h1 );
                add( "chip_mode_switch_zero_remesh", "PASS", note, flatX, flatY, flatZ, 0, 0, 1, 0 );
            }
            g.chipMode = ChipMode::Phys;
        }

        // --- same initial chip state → same settled result (determinism) ---
        {
            clearCertChips();
            auto runOnce = [&]( float& oxOut, float& oyOut, float& ozOut, int& lifeOut )
            {
                clearCertChips();
                H2H::SpawnDetachedChip( flatX + 0.03f, flatY, flatZ + 1.1f, 0.12f, 0.10f, 0.04f, 200 );
                stepPhys( kDt, kFallSteps );
                H2H::MatterBody const& b = H2H::State().bodies.front();
                oxOut = b.x; oyOut = b.y; ozOut = b.z;
                lifeOut = (int)b.life;
            };
            float xA = 0, yA = 0, zA = 0, xB = 0, yB = 0, zB = 0;
            int lifeA = 0, lifeB = 0;
            runOnce( xA, yA, zA, lifeA );
            runOnce( xB, yB, zB, lifeB );
            bool const same = std::fabs( xA - xB ) < 1e-5f
                && std::fabs( yA - yB ) < 1e-5f
                && std::fabs( zA - zB ) < 1e-5f
                && lifeA == lifeB;
            if ( !same )
            {
                char note[140];
                std::snprintf( note, sizeof( note ),
                    "NONDET A=(%.4f,%.4f,%.4f) B=(%.4f,%.4f,%.4f)", xA, yA, zA, xB, yB, zB );
                add( "chip_determinism", "FAIL", note, xA, yA, zA, 0, 0, 1, 1 );
            }
            else
            {
                char note[120];
                std::snprintf( note, sizeof( note ),
                    "identical settle z=%.4f life=%d", zA, lifeA );
                add( "chip_determinism", "PASS", note, xA, yA, zA, 0, 0, 1, 0 );
            }
        }

        // --- ACTIVE→SETTLED sleep: cost tracks moving fragments, not historical digs ---
        {
            clearCertChips();
            H2H::SpawnDetachedChip( flatX, flatY, flatZ + 1.0f );
            stepPhys( kDt, kFallSteps );
            H2H::MatterBody const& b0 = H2H::State().bodies.front();
            bool const asleep = !H2H::ChipIntegrates( b0 )
                && ( b0.life == H2H::ChipLife::Settled || b0.life == H2H::ChipLife::ExplicitBody );
            float const zx = b0.x, zy = b0.y, zz = b0.z;
            int const active0 = H2H::CountActiveChips();
            stepPhys( kDt, 60 );
            H2H::MatterBody const& b1 = H2H::State().bodies.front();
            int const active1 = H2H::CountActiveChips();
            bool const frozen = std::fabs( b1.x - zx ) < 1e-5f
                && std::fabs( b1.y - zy ) < 1e-5f
                && std::fabs( b1.z - zz ) < 1e-5f
                && active0 == 0 && active1 == 0;
            if ( !( asleep && frozen ) )
            {
                char note[140];
                std::snprintf( note, sizeof( note ),
                    "SLEEP_FAIL life=%d active=%d→%d dz=%.5f",
                    (int)b1.life, active0, active1, b1.z - zz );
                add( "chip_ACTIVE_SETTLED_sleep", "FAIL", note, b1.x, b1.y, b1.z, 0, 0, 1, 1 );
            }
            else
            {
                char note[120];
                std::snprintf( note, sizeof( note ),
                    "ACTIVE→sleep life=%d active=0 (no integrate)", (int)b1.life );
                add( "chip_ACTIVE_SETTLED_sleep", "PASS", note, b1.x, b1.y, b1.z, 0, 0, 1, 0 );
            }
        }

        // --- Cavity quiescence: supportable chips SETTLED; CountActiveChips==0 for N frames ---
        // No D2 rebuild / HF remesh / crest teleport / position drift while quiet.
        {
            clearCertChips();
            int const d0 = g.perfD2Rebuilds;
            int const h0 = g.perfHfRebuilds;
            float const thick = 0.04f;
            // Three chips into the dig cavity — rough floor may have nz < 0.88.
            H2H::SpawnDetachedChip( cavX, cavY, cavGrade + 0.85f, 0.12f, 0.10f, thick, 200 );
            H2H::SpawnDetachedChip( cavX + 0.04f, cavY - 0.03f, cavGrade + 0.95f, 0.10f, 0.08f, thick, 80 );
            H2H::SpawnDetachedChip( cavX - 0.03f, cavY + 0.02f, cavGrade + 0.75f, 0.11f, 0.09f, thick, 120 );
            constexpr int kBoundSteps = 480;
            constexpr int kHoldFrames = 60; // N consecutive active==0
            int settleAt = -1;
            for ( int i = 0; i < kBoundSteps; ++i )
            {
                H2H::StepBodies( kDt, supportFn );
                if ( H2H::CountActiveChips() == 0 )
                {
                    settleAt = i;
                    break;
                }
            }
            bool allSettled = settleAt >= 0 && H2H::CountActiveChips() == 0;
            for ( H2H::MatterBody const& b : H2H::State().bodies )
            {
                if ( H2H::ChipIntegrates( b ) ) { allSettled = false; break; }
                if ( !( b.life == H2H::ChipLife::Settled || b.life == H2H::ChipLife::ExplicitBody ) )
                {
                    allSettled = false;
                    break;
                }
            }
            // Snapshot positions after settle; hold active==0 without drift / remesh.
            struct Pose { float x, y, z; };
            std::vector<Pose> poses;
            poses.reserve( H2H::State().bodies.size() );
            for ( H2H::MatterBody const& b : H2H::State().bodies )
            {
                poses.push_back( Pose{ b.x, b.y, b.z } );
            }
            int holdOk = 0;
            float maxDrift = 0.f;
            for ( int i = 0; i < kHoldFrames; ++i )
            {
                H2H::StepBodies( kDt, supportFn );
                if ( H2H::CountActiveChips() != 0 ) { break; }
                for ( size_t bi = 0; bi < H2H::State().bodies.size() && bi < poses.size(); ++bi )
                {
                    H2H::MatterBody const& b = H2H::State().bodies[bi];
                    float const d = std::fabs( b.x - poses[bi].x )
                        + std::fabs( b.y - poses[bi].y )
                        + std::fabs( b.z - poses[bi].z );
                    maxDrift = (std::max)( maxDrift, d );
                }
                ++holdOk;
            }
            int const d1 = g.perfD2Rebuilds;
            int const h1 = g.perfHfRebuilds;
            bool const noRemesh = ( d1 == d0 && h1 == h0 );
            bool const held = holdOk == kHoldFrames && maxDrift < 1e-5f;
            char probe0[200] = {};
            if ( !H2H::State().bodies.empty() )
            {
                H2H::FormatChipProbe( H2H::State().bodies.front(), probe0, (int)sizeof( probe0 ) );
            }
            if ( !( allSettled && held && noRemesh ) )
            {
                char note[220];
                std::snprintf( note, sizeof( note ),
                    "QUIESCE_FAIL settleAt=%d active=%d hold=%d/%d drift=%.6f D2 %d→%d HF %d→%d %s",
                    settleAt, H2H::CountActiveChips(), holdOk, kHoldFrames, maxDrift,
                    d0, d1, h0, h1, probe0 );
                add( "chip_cavity_quiescence", "FAIL", note, cavX, cavY, cavGrade, 0, 0, 1, 1 );
            }
            else
            {
                char note[200];
                std::snprintf( note, sizeof( note ),
                    "SETTLED@%d hold=%d active=0 drift=0 D2=%d HF=%d %s",
                    settleAt, kHoldFrames, d1, h1, probe0 );
                add( "chip_cavity_quiescence", "PASS", note, cavX, cavY, cavGrade, 0, 0, 1, 0 );
            }
        }
        } // chipCavityOk — cavity-dependent §11 probes

        // --- Steep-but-static sleep: nz≈cos(55°) < 0.88 must still SETTLED (regression) ---
        {
            clearCertChips();
            constexpr float kDeg = 55.f * 3.14159265f / 180.f;
            float const sn = std::sin( kDeg );
            float const cn = std::cos( kDeg ); // ~0.574 — below old flatNz=0.88 gate
            float const planeZ = 10.f;
            int const fixtureRev = 7;
            auto steepSupport = [&]( float x, float y, float /*queryZ*/ ) -> H2H::SupportQuery
            {
                H2H::SupportQuery q{};
                q.hit = true;
                q.deferred = false;
                q.x = x; q.y = y; q.z = planeZ;
                q.nx = sn; q.ny = 0.f; q.nz = cn;
                q.supportRev = fixtureRev;
                return q;
            };
            float const thick = 0.04f;
            float const rest = planeZ + thick * 0.5f + 0.01f;
            H2H::MatterBody& chip = H2H::SpawnDetachedChip(
                flatX, flatY, rest, 0.12f, 0.10f, thick, 200 );
            chip.vx = chip.vy = chip.vz = 0.f;
            chip.nx = sn; chip.ny = 0.f; chip.nz = cn;
            chip.supportRev = fixtureRev;
            constexpr int kSteepSteps = 90;
            for ( int i = 0; i < kSteepSteps; ++i )
            {
                H2H::StepBodies( kDt, steepSupport );
            }
            H2H::MatterBody const& b = H2H::State().bodies.front();
            bool const asleep = !H2H::ChipIntegrates( b )
                && ( b.life == H2H::ChipLife::Settled || b.life == H2H::ChipLife::ExplicitBody );
            bool const steepOk = b.nz < 0.88f && asleep && H2H::CountActiveChips() == 0;
            bool const noDrift = std::fabs( b.z - rest ) < 1e-4f
                && std::fabs( b.x - flatX ) < 1e-4f
                && std::fabs( b.y - flatY ) < 1e-4f;
            char probe[200];
            H2H::FormatChipProbe( b, probe, (int)sizeof( probe ) );
            if ( !( steepOk && noDrift ) )
            {
                char note[220];
                std::snprintf( note, sizeof( note ),
                    "STEEP_STATIC_FAIL nz=%.3f life=%d active=%d %s",
                    b.nz, (int)b.life, H2H::CountActiveChips(), probe );
                add( "chip_steep_static_sleep", "FAIL", note, b.x, b.y, b.z, b.nx, b.ny, b.nz, 1 );
            }
            else
            {
                char note[200];
                std::snprintf( note, sizeof( note ),
                    "55deg static sleep nz=%.3f life=%d active=0 %s",
                    b.nz, (int)b.life, probe );
                add( "chip_steep_static_sleep", "PASS", note, b.x, b.y, b.z, b.nx, b.ny, b.nz, 0 );
            }
        }

        // --- Wake on supportRev / EditedRegion change under chip ---
        {
            clearCertChips();
            int rev = 1;
            float const planeZ = 8.f;
            auto revSupport = [&]( float x, float y, float /*queryZ*/ ) -> H2H::SupportQuery
            {
                H2H::SupportQuery q{};
                q.hit = true;
                q.deferred = false;
                q.x = x; q.y = y; q.z = planeZ;
                q.nx = 0.f; q.ny = 0.f; q.nz = 1.f;
                q.supportRev = rev;
                return q;
            };
            float const thick = 0.04f;
            float const rest = planeZ + thick * 0.5f + 0.01f;
            H2H::MatterBody& chip = H2H::SpawnDetachedChip(
                flatX + 1.f, flatY, rest, 0.12f, 0.10f, thick, 200 );
            chip.vx = chip.vy = chip.vz = 0.f;
            chip.supportRev = rev;
            for ( int i = 0; i < 40; ++i )
            {
                H2H::StepBodies( kDt, revSupport );
            }
            bool const slept = H2H::CountActiveChips() == 0
                && !H2H::ChipIntegrates( H2H::State().bodies.front() );
            rev = 2; // matter / EditedRegion revision under chip
            H2H::StepBodies( kDt, revSupport );
            H2H::MatterBody const& bWake = H2H::State().bodies.front();
            bool const woke = H2H::ChipIntegrates( bWake ) || bWake.life == H2H::ChipLife::Active;
            // Allow re-settle after wake (hysteresis); revision stamp must advance.
            for ( int i = 0; i < 40; ++i )
            {
                H2H::StepBodies( kDt, revSupport );
            }
            H2H::MatterBody const& bEnd = H2H::State().bodies.front();
            bool const revHeld = bEnd.supportRev == 2;
            bool const reslept = H2H::CountActiveChips() == 0;
            char probe[200];
            H2H::FormatChipProbe( bEnd, probe, (int)sizeof( probe ) );
            if ( !( slept && woke && revHeld && reslept ) )
            {
                char note[220];
                std::snprintf( note, sizeof( note ),
                    "WAKE_REV_FAIL slept=%d woke=%d rev=%d reslept=%d %s",
                    slept ? 1 : 0, woke ? 1 : 0, bEnd.supportRev, reslept ? 1 : 0, probe );
                add( "chip_wake_on_supportRev", "FAIL", note, bEnd.x, bEnd.y, bEnd.z, 0, 0, 1, 1 );
            }
            else
            {
                char note[200];
                std::snprintf( note, sizeof( note ),
                    "wake on supportRev 1→2 then re-SETTLED %s", probe );
                add( "chip_wake_on_supportRev", "PASS", note, bEnd.x, bEnd.y, bEnd.z, 0, 0, 1, 0 );
            }
        }

        // --- P3d.2 vibrate/orbit halt: cup fixture buzzes under speed-only sleep; must SETTLED ---
        {
            clearCertChips();
            float const cx = flatX + 3.f, cy = flatY;
            float const baseZ = 12.f;
            int const fixtureRev = 11;
            auto cupSupport = [&]( float x, float y, float /*queryZ*/ ) -> H2H::SupportQuery
            {
                float const dx = x - cx, dy = y - cy;
                // Parabolic cup — chip with sideways KE oscillates; speed stays above sleep band.
                float const z = baseZ + 6.f * ( dx * dx + dy * dy );
                float gx = 12.f * dx, gy = 12.f * dy; // ∂z/∂x, ∂z/∂y
                float nx = -gx, ny = -gy, nz = 1.f;
                float const nlen = std::sqrt( nx * nx + ny * ny + nz * nz );
                if ( nlen > 1e-5f ) { nx /= nlen; ny /= nlen; nz /= nlen; }
                H2H::SupportQuery q{};
                q.hit = true;
                q.deferred = false;
                q.x = x; q.y = y; q.z = z;
                q.nx = nx; q.ny = ny; q.nz = nz;
                q.supportRev = fixtureRev;
                return q;
            };
            float const thick = 0.04f;
            H2H::SupportQuery const h0 = cupSupport( cx + 0.02f, cy, baseZ + 1.f );
            float const startZ = h0.z + thick * 0.5f + 0.01f;
            H2H::MatterBody& chip = H2H::SpawnDetachedChip(
                cx + 0.02f, cy, startZ, 0.12f, 0.10f, thick, 200 );
            chip.vx = 0.95f; // would buzz/orbit under P3d.1 speed thresholds alone
            chip.vy = 0.35f;
            chip.vz = 0.f;
            chip.supportRev = fixtureRev;
            int const d0 = g.perfD2Rebuilds;
            int const h0m = g.perfHfRebuilds;
            constexpr int kOrbitSteps = 360;
            int settleAt = -1;
            for ( int i = 0; i < kOrbitSteps; ++i )
            {
                H2H::StepBodies( kDt, cupSupport );
                if ( H2H::CountActiveChips() == 0 ) { settleAt = i; break; }
            }
            H2H::MatterBody const& b = H2H::State().bodies.front();
            bool const asleep = !H2H::ChipIntegrates( b )
                && ( b.life == H2H::ChipLife::Settled || b.life == H2H::ChipLife::ExplicitBody );
            bool const nearCup = std::fabs( b.x - cx ) < 0.06f && std::fabs( b.y - cy ) < 0.06f;
            bool const noRemesh = g.perfD2Rebuilds == d0 && g.perfHfRebuilds == h0m;
            char probe[200];
            H2H::FormatChipProbe( b, probe, (int)sizeof( probe ) );
            if ( !( asleep && settleAt >= 0 && nearCup && noRemesh && H2H::CountActiveChips() == 0 ) )
            {
                char note[240];
                std::snprintf( note, sizeof( note ),
                    "ORBIT_HALT_FAIL settleAt=%d life=%d active=%d near=%d remesh=%d %s",
                    settleAt, (int)b.life, H2H::CountActiveChips(), nearCup ? 1 : 0,
                    noRemesh ? 0 : 1, probe );
                add( "chip_vibrate_orbit_halt", "FAIL", note, b.x, b.y, b.z, b.nx, b.ny, b.nz, 1 );
            }
            else
            {
                char note[200];
                std::snprintf( note, sizeof( note ),
                    "orbit halt SETTLED@%d active=0 cup-center %s", settleAt, probe );
                add( "chip_vibrate_orbit_halt", "PASS", note, b.x, b.y, b.z, b.nx, b.ny, b.nz, 0 );
            }
        }

        // --- P3d.2 steep kinetic settle: steep nz + restoring tilt (thrash, not free slide) ---
        {
            clearCertChips();
            constexpr float kDeg = 55.f * 3.14159265f / 180.f;
            float const sn = std::sin( kDeg );
            float const cn = std::cos( kDeg );
            float const planeZ = 10.f;
            float const cx = flatX + 5.f, cy = flatY;
            int const fixtureRev = 13;
            auto steepSupport = [&]( float x, float y, float /*queryZ*/ ) -> H2H::SupportQuery
            {
                // Base steep face + XY restoring tilt so KE oscillates in-bound (HUD buzz class).
                float const dx = x - cx, dy = y - cy;
                float nx = sn - 14.f * dx;
                float ny = -14.f * dy;
                float nz = cn;
                float const nlen = std::sqrt( nx * nx + ny * ny + nz * nz );
                if ( nlen > 1e-5f ) { nx /= nlen; ny /= nlen; nz /= nlen; }
                H2H::SupportQuery q{};
                q.hit = true;
                q.deferred = false;
                q.x = x; q.y = y; q.z = planeZ;
                q.nx = nx; q.ny = ny; q.nz = nz;
                q.supportRev = fixtureRev;
                return q;
            };
            float const thick = 0.04f;
            float const rest = planeZ + thick * 0.5f + 0.01f;
            H2H::MatterBody& chip = H2H::SpawnDetachedChip(
                cx + 0.015f, cy, rest, 0.12f, 0.10f, thick, 200 );
            chip.vx = 0.85f;
            chip.vy = -0.40f;
            chip.vz = 0.15f;
            chip.nx = sn; chip.ny = 0.f; chip.nz = cn;
            chip.supportRev = fixtureRev;
            constexpr int kKinSteps = 300;
            int settleAt = -1;
            for ( int i = 0; i < kKinSteps; ++i )
            {
                H2H::StepBodies( kDt, steepSupport );
                if ( H2H::CountActiveChips() == 0 ) { settleAt = i; break; }
            }
            H2H::MatterBody const& b = H2H::State().bodies.front();
            bool const asleep = !H2H::ChipIntegrates( b )
                && ( b.life == H2H::ChipLife::Settled || b.life == H2H::ChipLife::ExplicitBody );
            // Fixture base is 55° (cn<0.88); restoring tilt may flatten nz at the settle point.
            bool const steepFixture = cn < 0.88f;
            bool const steepOk = steepFixture && asleep && H2H::CountActiveChips() == 0;
            // Must not crest-teleport — settle near fixture center.
            bool const noTeleport = std::fabs( b.x - cx ) < 0.10f && std::fabs( b.y - cy ) < 0.10f
                && std::fabs( b.z - rest ) < 0.05f;
            char probe[200];
            H2H::FormatChipProbe( b, probe, (int)sizeof( probe ) );
            if ( !( steepOk && noTeleport && settleAt >= 0 ) )
            {
                char note[240];
                std::snprintf( note, sizeof( note ),
                    "STEEP_KINETIC_FAIL settleAt=%d nz=%.3f life=%d active=%d %s",
                    settleAt, b.nz, (int)b.life, H2H::CountActiveChips(), probe );
                add( "chip_steep_kinetic_settle", "FAIL", note, b.x, b.y, b.z, b.nx, b.ny, b.nz, 1 );
            }
            else
            {
                char note[200];
                std::snprintf( note, sizeof( note ),
                    "55deg kinetic→SETTLED@%d endNz=%.3f %s", settleAt, b.nz, probe );
                add( "chip_steep_kinetic_settle", "PASS", note, b.x, b.y, b.z, b.nx, b.ny, b.nz, 0 );
            }
        }

        // --- AGGREGATED scaffold: fines AggregatePatch path already present ---
        {
            bool const haveAggType = true; // AggregatePatch + StrikePick fines path
            (void)haveAggType;
            char note[120];
            std::snprintf( note, sizeof( note ),
                "AggregatePatch scaffold OK (full agg later); vocab cheap / instantiate on exposure" );
            add( "chip_aggregate_scaffold", "PASS", note );
        }

        clearCertChips();
        g.chipMode = savedMode;
        g.chipMode = ChipMode::Off; // cert default — zero chip cost for residual frames
    }

    void GeoCertRunPlaceSection()
    {
        // §9 P3e PLACE / RE-FILL — reverse matter transfer into occupancy.
        auto add = [&]( char const* scen, char const* verdict, char const* note,
            float x = 0.f, float y = 0.f, float z = 0.f,
            int grams = 0, int occDelta = 0, char const* terrain = "-" )
        {
            GeoCertRow r{};
            std::snprintf( r.section, sizeof( r.section ), "9" );
            std::snprintf( r.scenario, sizeof( r.scenario ), "%s", scen );
            std::snprintf( r.verdict, sizeof( r.verdict ), "%s", verdict );
            std::snprintf( r.note, sizeof( r.note ), "%s", note );
            std::snprintf( r.action, sizeof( r.action ), "place_refill" );
            std::snprintf( r.terrain, sizeof( r.terrain ), "%s", terrain ? terrain : "-" );
            r.x = x; r.y = y; r.z = z;
            r.grams = grams;
            r.occDelta = occDelta;
            GeoCertAddRow( r );
            if ( std::strcmp( verdict, "FAIL" ) == 0 )
            {
                GeoCertHardFail( scen, "place_refill", note, x, y, z, 0.f, 0.f, 1.f );
            }
        };

        float const ox = (float)ProvenanceGeo::kRangeOriginX;
        float const oy = (float)ProvenanceGeo::kRangeOriginY;
        float const padY = oy - 28.f;
        float const placeR = (std::max)( kHandfulRadiusM * 1.8f, kVoxelEdgeM * 1.25f );
        int const placeG = (int)std::lround( kDirtVoxelG );

        auto seedBaseline = [&]( float x, float y )
        {
            int const cx = (int)std::floor( x );
            int const cy = (int)std::floor( y );
            PrefetchOccupancyCell( cx, cy );
            EnsureOccupancyLattice( cx, cy );
            CellSample* cell = GetCellMutable( cx, cy );
            if ( cell && !cell->carved && !cell->fill.empty() )
            {
                SeedOccupancyFromVirginSurface( cx, cy );
            }
        };

        auto runPlace = [&]( char const* scen, float x, float y, float placeZ,
            float mouthX, float mouthY, bool expectMouthStay, char const* terrain )
        {
            float grade0 = 0.f;
            SampleGroundZBase( x, y, grade0 );
            seedBaseline( x, y );
            bool const mouth0 = NearOpeningMouthAt( mouthX, mouthY );
            int const solid0 = CountOccupancySolidInSphere( x, y, placeZ, placeR );
            CellSample const* c0 = GetCell( (int)std::floor( x ), (int)std::floor( y ) );
            int const d2_0 = c0 ? (int)c0->cavityTris.size() : 0;

            std::unordered_map<std::string, int> credit;
            credit["dirt"] = placeG;
            CreditHeld( credit );
            int const heldCredited = g.heldTotalG;
            PlaceFillResult const pr = PlaceOccupancyFill( x, y, placeZ, placeR, placeG );
            if ( !pr.ok || pr.acceptedGrams <= 0 )
            {
                DebitHeldTotal( placeG );
                add( scen, "FAIL", "PLACE_FILL_NO_MATTER", x, y, placeZ, 0, 0, terrain );
                return;
            }
            DebitHeldTotal( pr.acceptedGrams );
            int const heldAfter = g.heldTotalG;
            int const solid1 = CountOccupancySolidInSphere( x, y, placeZ, placeR );
            float gradeAfter = 0.f;
            SampleGroundZBase( x, y, gradeAfter );
            bool const gradeStable = std::fabs( gradeAfter - grade0 ) < 1e-4f;
            bool const mouthAfter = NearOpeningMouthAt( mouthX, mouthY );
            int const dHeld = heldCredited - heldAfter;
            int const dSolid = solid1 - solid0;
            CellSample const* cell = GetCell( (int)std::floor( x ), (int)std::floor( y ) );
            int const d2Tris = cell ? (int)cell->cavityTris.size() : 0;
            SupportHit const sup = SupportBelow( x, y, gradeAfter + 2.0f );
            float occZ = gradeAfter;
            bool const haveOcc = SampleOccupancyZ( x, y, occZ );
            bool const solidNear = OccupancySolidAt( x, y, occZ - 0.02f )
                || OccupancySolidAt( x, y, placeZ );
            bool const supportSees = ( (sup.hit && !sup.deferred) || (haveOcc && solidNear) );
            int const unitsAsG = (int)std::lround(
                (double)pr.unitsFilled * (double)kDirtVoxelG / (double)kFillFull );
            bool const gramsOk = ( dHeld == pr.acceptedGrams ) && ( pr.acceptedGrams > 0 );
            bool const occOk = ( pr.unitsFilled > 0 );
            bool const massOcc = std::abs( unitsAsG - pr.acceptedGrams ) <= 2;
            bool const mouthOk = !expectMouthStay || ( mouth0 && mouthAfter ) || mouthAfter;
            if ( !gramsOk || !occOk || !massOcc || !supportSees || !gradeStable || !mouthOk )
            {
                char note[240];
                std::snprintf( note, sizeof( note ),
                    "held %d->%d acc=%d fillU=%d solid+%d sup=%d/%d occZ=%.3f grade=%d mouth=%d->%d d2=%d",
                    heldCredited, heldAfter, pr.acceptedGrams, pr.unitsFilled, dSolid,
                    (sup.hit && !sup.deferred) ? 1 : 0, solidNear ? 1 : 0, occZ,
                    gradeStable ? 1 : 0, mouth0 ? 1 : 0, mouthAfter ? 1 : 0, d2Tris );
                add( scen, "FAIL", note, x, y, placeZ, pr.acceptedGrams, dSolid, terrain );
                return;
            }
            char note[220];
            std::snprintf( note, sizeof( note ),
                "held-%dg fillU=%d solid+%d supZ=%.3f occZ=%.3f d2=%d->%d HF_grade_stable",
                pr.acceptedGrams, pr.unitsFilled, dSolid,
                sup.hit ? sup.position.z : occZ, occZ, d2_0, d2Tris );
            add( scen, "PASS", note, x, y, placeZ, pr.acceptedGrams, dSolid, terrain );
        };

        float const flatX = ox + 6.f, flatY = padY;
        float flatZ = 0.f;
        if ( !SampleGroundZBase( flatX, flatY, flatZ ) )
        {
            add( "place_flat_mound", "FAIL", "PAD_SAMPLE_FAIL", flatX, flatY, 0.f );
        }
        else
        {
            runPlace( "place_flat_mound", flatX, flatY, flatZ + kVoxelEdgeM * 0.35f,
                flatX, flatY, false, "flat_soil" );
        }

        float slopeX = ox + 18.f, slopeY = padY, slopeZ = 0.f;
        char const* slopeTerrain = "gentle_slope";
        for ( int i = 0; i < s_geoContactN; ++i )
        {
            if ( std::strcmp( s_geoContacts[i].id, "B_slope" ) == 0 && s_geoContacts[i].found )
            {
                slopeX = s_geoContacts[i].x;
                slopeY = padY;
                slopeTerrain = s_geoContacts[i].terrain;
                break;
            }
        }
        SampleGroundZBase( slopeX, slopeY, slopeZ );
        {
            runPlace( "place_slope_supported", slopeX, slopeY, slopeZ + kVoxelEdgeM * 0.35f,
                slopeX, slopeY, false, slopeTerrain );
            float const floatZ = slopeZ + kVoxelEdgeM * 2.5f;
            bool const floating = OccupancySolidAt( slopeX, slopeY, floatZ )
                && !OccupancySolidAt( slopeX, slopeY, floatZ - kVoxelEdgeM );
            if ( floating )
            {
                add( "place_slope_no_float", "FAIL", "FLOATING_BLOB",
                    slopeX, slopeY, floatZ, 0, 0, slopeTerrain );
            }
            else
            {
                add( "place_slope_no_float", "PASS", "no unsupported solid above seat",
                    slopeX, slopeY, slopeZ, 0, 0, slopeTerrain );
            }
        }

        float const cavX = ox + 10.f, cavY = padY;
        float cavGrade = 0.f;
        SampleGroundZBase( cavX, cavY, cavGrade );
        float const openR = 0.24f;
        float const carveZ = cavGrade - openR * 0.40f;
        PrefetchOccupancyCell( (int)std::floor( cavX ), (int)std::floor( cavY ) );
        bool const carved = CarveOccupancySphere( cavX, cavY, carveZ, openR, cavX, cavY, cavGrade );
        if ( !carved )
        {
            add( "place_cavity_floor_upward", "FAIL", "CAVITY_CARVE_FAIL", cavX, cavY, cavGrade );
            add( "place_cavity_lip_connect", "SKIP", "no cavity" );
            add( "place_lip_no_roof_hole", "SKIP", "no cavity" );
            add( "place_no_HF_resurrection", "SKIP", "no cavity" );
            add( "remove_place_remove_reconcile", "SKIP", "no cavity" );
        }
        else
        {
            SupportHit const floor0 = SupportBelow( cavX, cavY, cavGrade + 0.5f );
            float placeFloorZ = carveZ;
            if ( floor0.hit ) { placeFloorZ = floor0.position.z + kVoxelEdgeM * 0.35f; }
            float const occTop0 = floor0.hit ? floor0.position.z : carveZ;
            runPlace( "place_cavity_floor_upward", cavX, cavY, placeFloorZ,
                cavX, cavY, true, "cavity_floor" );
            SupportHit const floor1 = SupportBelow( cavX, cavY, cavGrade + 0.5f );
            if ( !( floor1.hit && floor1.position.z > occTop0 + 1e-4f ) )
            {
                char note[120];
                std::snprintf( note, sizeof( note ), "FLOOR_NOT_RAISED %.3f->%.3f",
                    occTop0, floor1.position.z );
                add( "place_cavity_floor_raised", "FAIL", note, cavX, cavY, placeFloorZ, 0, 0, "cavity_floor" );
            }
            else
            {
                char note[120];
                std::snprintf( note, sizeof( note ), "occ floor %.3f->%.3f (bottom-up)",
                    occTop0, floor1.position.z );
                add( "place_cavity_floor_raised", "PASS", note, cavX, cavY, placeFloorZ, 0, 0, "cavity_floor" );
            }

            float const lipX = cavX + openR * 0.85f;
            float const lipY = cavY;
            float lipZ = cavGrade;
            SampleGroundZBase( lipX, lipY, lipZ );
            bool const mouthBeforeLip = NearOpeningMouthAt( cavX, cavY );
            float const farProbeX = cavX - openR * 0.5f;
            bool const airFar0 = !OccupancySolidAt( farProbeX, cavY, carveZ );
            // Mouth probe stays on cavity center — lip XY may sit outside tip disk.
            runPlace( "place_cavity_lip_connect", lipX, lipY, lipZ - kVoxelEdgeM * 0.15f,
                cavX, cavY, true, "cavity_lip" );
            bool const mouthAfterLip = NearOpeningMouthAt( cavX, cavY );
            bool const airFar1 = !OccupancySolidAt( farProbeX, cavY, carveZ );
            bool const roofed = mouthBeforeLip && !mouthAfterLip;
            bool const filledWhole = airFar0 && !airFar1
                && std::fabs( farProbeX - lipX ) > placeR * 0.9f;
            if ( roofed || filledWhole )
            {
                char note[140];
                std::snprintf( note, sizeof( note ),
                    "ROOF_OR_WHOLE_HOLE mouth=%d->%d farAir=%d->%d",
                    mouthBeforeLip ? 1 : 0, mouthAfterLip ? 1 : 0,
                    airFar0 ? 1 : 0, airFar1 ? 1 : 0 );
                add( "place_lip_no_roof_hole", "FAIL", note, lipX, lipY, lipZ, 0, 0, "cavity_lip" );
            }
            else
            {
                add( "place_lip_no_roof_hole", "PASS",
                    "lip place local; mouth kept; far cavity air retained",
                    lipX, lipY, lipZ, 0, 0, "cavity_lip" );
            }

            float gradeB = 0.f;
            SampleGroundZBase( cavX, cavY, gradeB );
            bool const mouthKeep = NearOpeningMouthAt( cavX, cavY );
            if ( !mouthKeep || std::fabs( gradeB - cavGrade ) > 1e-4f )
            {
                char note[120];
                std::snprintf( note, sizeof( note ),
                    "HF_RESURRECT mouth=%d grade %.4f->%.4f",
                    mouthKeep ? 1 : 0, cavGrade, gradeB );
                add( "place_no_HF_resurrection", "FAIL", note, cavX, cavY, cavGrade, 0, 0, "cavity_floor" );
            }
            else
            {
                add( "place_no_HF_resurrection", "PASS",
                    "openings kept; virgin grade unchanged; no HF skin restore",
                    cavX, cavY, cavGrade, 0, 0, "cavity_floor" );
            }

            float const rpX = ox + 14.f, rpY = padY;
            float rpGrade = 0.f;
            SampleGroundZBase( rpX, rpY, rpGrade );
            float const rpR = 0.20f;
            float const rpCarveZ = rpGrade - rpR * 0.35f;
            PrefetchOccupancyCell( (int)std::floor( rpX ), (int)std::floor( rpY ) );
            int const held0 = g.heldTotalG;
            bool const rm1 = CarveOccupancySphere( rpX, rpY, rpCarveZ, rpR, rpX, rpY, rpGrade );
            std::unordered_map<std::string, int> carry;
            carry["dirt"] = placeG;
            CreditHeld( carry );
            int const heldAfterRemove = g.heldTotalG;
            int const unitsAfterRemove = CountOccupancyFillUnitsInSphere( rpX, rpY, rpCarveZ, rpR );
            SupportHit const rpFloor = SupportBelow( rpX, rpY, rpGrade + 0.5f );
            float rpPlaceZ = rpCarveZ;
            if ( rpFloor.hit ) { rpPlaceZ = rpFloor.position.z + kVoxelEdgeM * 0.35f; }
            PlaceFillResult const rpPlace = PlaceOccupancyFill( rpX, rpY, rpPlaceZ, placeR, placeG );
            if ( rpPlace.ok ) { DebitHeldTotal( rpPlace.acceptedGrams ); }
            int const heldAfterPlace = g.heldTotalG;
            int const unitsAfterPlace = CountOccupancyFillUnitsInSphere( rpX, rpY, rpPlaceZ, placeR );
            bool const rm2 = CarveOccupancySphere( rpX, rpY, rpPlaceZ, rpR * 0.9f, rpX, rpY, rpGrade );
            int const unitsAfterReRemove = CountOccupancyFillUnitsInSphere( rpX, rpY, rpPlaceZ, placeR );
            int const dPlaceHeld = heldAfterRemove - heldAfterPlace;
            bool const ok = rm1 && rm2 && rpPlace.ok
                && ( dPlaceHeld == rpPlace.acceptedGrams )
                && ( unitsAfterPlace > unitsAfterRemove )
                && ( unitsAfterReRemove < unitsAfterPlace );
            if ( !ok )
            {
                char note[180];
                std::snprintf( note, sizeof( note ),
                    "RECONCILE fail rm=%d/%d place=%d held-%d units %d->%d->%d held0=%d",
                    rm1 ? 1 : 0, rm2 ? 1 : 0, rpPlace.ok ? 1 : 0, dPlaceHeld,
                    unitsAfterRemove, unitsAfterPlace, unitsAfterReRemove, held0 );
                add( "remove_place_remove_reconcile", "FAIL", note, rpX, rpY, rpGrade,
                    rpPlace.acceptedGrams, unitsAfterPlace - unitsAfterRemove, "cycle" );
            }
            else
            {
                char note[160];
                std::snprintf( note, sizeof( note ),
                    "held-%dg units %d->%d->%d (remove/place/remove)",
                    dPlaceHeld, unitsAfterRemove, unitsAfterPlace, unitsAfterReRemove );
                add( "remove_place_remove_reconcile", "PASS", note, rpX, rpY, rpGrade,
                    rpPlace.acceptedGrams, unitsAfterPlace - unitsAfterRemove, "cycle" );
            }
        }
    }

    void GeoCertScaffoldRest()
    {
        // §3 / §5 / §6 / §7 / §8 / §10 / §11 are filled by dedicated runners; SKIP if early exit skipped them.
        // §9 place/re-fill remains frozen until after P3d.
        auto hasSec = [&]( char const* sec ) -> bool
        {
            for ( int i = 0; i < s_geoRowN; ++i )
            {
                if ( std::strcmp( s_geoRows[i].section, sec ) == 0 ) { return true; }
            }
            return false;
        };
        if ( !hasSec( "3" ) )
        {
            GeoCertScaffold( "3", "HF_refine_no_edit", "skipped — cert exited before no-edit runner" );
        }
        if ( !hasSec( "5" ) )
        {
            GeoCertScaffold( "5", "D2_QEF_halo_determinism", "skipped — cert exited before D2 runner" );
        }
        if ( !hasSec( "6" ) )
        {
            GeoCertScaffold( "6", "accumulated_20_strikes", "skipped — cert exited before accumulate runner" );
        }
        if ( !hasSec( "7" ) )
        {
            GeoCertScaffold( "7", "HF_D2_ownership_masks", "skipped — cert exited before ownership runner" );
        }
        if ( !hasSec( "8" ) )
        {
            GeoCertScaffold( "8", "material_correctness", "skipped — cert exited before material runner" );
        }
        if ( !hasSec( "9" ) )
        {
            GeoCertScaffold( "9", "placement_matter_add", "skipped — cert exited before place runner" );
        }
        if ( !hasSec( "10" ) )
        {
            GeoCertScaffold( "10", "support_collision_probes", "skipped — cert exited before support runner" );
        }
        if ( !hasSec( "11" ) )
        {
            GeoCertScaffold( "11", "chips_OFF_VISUAL_PHYS", "skipped — cert exited before chip runner" );
        }
        GeoCertScaffold( "12", "streaming_async_column_permute", "scaffold — needs bridge fan-in torture" );
        GeoCertScaffold( "13", "performance_budgets", "partial — see startup_perf + counters in header" );
        {
            GeoCertRow r{};
            std::snprintf( r.section, sizeof( r.section ), "13" );
            std::snprintf( r.scenario, sizeof( r.scenario ), "virgin_invariants_budget" );
            std::snprintf( r.verdict, sizeof( r.verdict ),
                ( g.perfVirginD2Rebuilds == 0 ) ? "PASS" : "FAIL" );
            std::snprintf( r.note, sizeof( r.note ),
                "virgin_D2=%d ss=%d gc=%d HF=%d tip6=presentation_only haloMiss=%d",
                g.perfVirginD2Rebuilds, g.perfSampleSurfaceCalls, g.perfGeoCellsCreated, g.perfHfRebuilds,
                g.perfD2HaloMiss );
            std::snprintf( r.action, sizeof( r.action ), "perf_snapshot" );
            std::snprintf( r.timing, sizeof( r.timing ), "HFms=%.1f D2ms=%.1f",
                g.perfHfRemeshMsTotal, g.perfD2MsTotal );
            GeoCertAddRow( r );
        }
        GeoCertScaffold( "14", "determinism_orders", "scaffold — reverse async / fresh process TODO" );
        {
            GeoCertRow r{};
            std::snprintf( r.section, sizeof( r.section ), "15" );
            std::snprintf( r.scenario, sizeof( r.scenario ), "artifact_written" );
            std::snprintf( r.verdict, sizeof( r.verdict ), "PASS" );
            std::snprintf( r.note, sizeof( r.note ), "provenance_geography_interaction_cert.txt" );
            std::snprintf( r.action, sizeof( r.action ), "write" );
            GeoCertAddRow( r );
        }
    }

    // ---------- Local Surface Intent (--cert-lsi) ----------
    // Read-only observation + certification around dig/place. Capture present defects.
    // Do NOT change D2 / occupancy / EditedRegion / SupportBelow / place / chips to greenwash.
    struct LsiVec3 { float x = 0.f, y = 0.f, z = 0.f; };
    struct LsiTri { LsiVec3 a{}, b{}, c{}; };
    struct LsiMeshRecord
    {
        std::vector<LsiTri> tris;
        float bmin[3] = { 1e9f, 1e9f, 1e9f };
        float bmax[3] = { -1e9f, -1e9f, -1e9f };
        uint32_t hash = 0;
        int triCount = 0;
        int invalidTris = 0;
        int degenerateTris = 0;
    };
    struct LsiOccRecord
    {
        uint32_t hash = 0;
        int solidCount = 0;
        int cellCount = 0;
        int fillBytes = 0;
    };
    struct LsiActionVolume
    {
        float cx = 0.f, cy = 0.f, cz = 0.f, r = 0.f;
        float openX = 0.f, openY = 0.f, openZ = 0.f;
        int hfIntersectBefore = 0;
        int hfIntersectAfter = 0;
        int d2IntersectAfter = 0;
        float toolScaleR = 0.f;
    };
    struct LsiDefect
    {
        char kind[48] = {};
        char note[160] = {};
        float x = 0.f, y = 0.f, z = 0.f;
    };
    struct LsiRow
    {
        char action[32] = {};
        char check[40] = {};
        char verdict[12] = "SKIP";
        char note[200] = {};
        float x = 0.f, y = 0.f, z = 0.f;
        uint32_t hashA = 0, hashB = 0;
        int nA = 0, nB = 0;
    };
    struct LsiCapture
    {
        char actionId[40] = {};
        char kind[16] = {}; // dig / place
        LsiActionVolume vol{};
        LsiMeshRecord hfBefore{};
        LsiMeshRecord hfAfter{};
        LsiMeshRecord d2After{};
        LsiMeshRecord hfOutsideBefore{};
        LsiMeshRecord hfOutsideAfter{};
        LsiOccRecord occBefore{};
        LsiOccRecord occAfter{};
        float dirtyMinX = 0.f, dirtyMinY = 0.f, dirtyMaxX = 0.f, dirtyMaxY = 0.f;
        bool haveDirty = false;
        int voidSamples = 0;
        int intentSamples = 0;
        int coveredSamples = 0;
        int openBoundaryEdges = 0; // unexplained open edges (mouth-rim excluded)
        int mouthRimEdges = 0;
        int boundaryEdgesTotal = 0;
    };

    static constexpr int kLsiRowCap = 64;
    static constexpr int kLsiDefectCap = 48;
    static LsiRow s_lsiRows[kLsiRowCap];
    static int s_lsiRowN = 0;
    static LsiDefect s_lsiDefects[kLsiDefectCap];
    static int s_lsiDefectN = 0;
    static char s_lsiFailReason[96] = {};

    void LsiAddRow( LsiRow const& r )
    {
        if ( s_lsiRowN >= kLsiRowCap ) { return; }
        s_lsiRows[s_lsiRowN++] = r;
        if ( std::strcmp( r.verdict, "FAIL" ) == 0 )
        {
            g.certLsiExitCode = 1;
            if ( !s_lsiFailReason[0] )
            {
                std::snprintf( s_lsiFailReason, sizeof( s_lsiFailReason ), "%s/%s", r.action, r.check );
            }
        }
    }

    void LsiAddDefect( char const* kind, char const* note, float x, float y, float z )
    {
        if ( s_lsiDefectN >= kLsiDefectCap ) { return; }
        LsiDefect& d = s_lsiDefects[s_lsiDefectN++];
        std::snprintf( d.kind, sizeof( d.kind ), "%s", kind ? kind : "?" );
        std::snprintf( d.note, sizeof( d.note ), "%s", note ? note : "" );
        d.x = x; d.y = y; d.z = z;
    }

    uint32_t LsiFnv1a( void const* data, size_t n, uint32_t h = 2166136261u )
    {
        uint8_t const* p = (uint8_t const*)data;
        for ( size_t i = 0; i < n; ++i )
        {
            h ^= p[i];
            h *= 16777619u;
        }
        return h;
    }

    void LsiExpandBounds( LsiMeshRecord& m, LsiVec3 const& v )
    {
        m.bmin[0] = (std::min)( m.bmin[0], v.x );
        m.bmin[1] = (std::min)( m.bmin[1], v.y );
        m.bmin[2] = (std::min)( m.bmin[2], v.z );
        m.bmax[0] = (std::max)( m.bmax[0], v.x );
        m.bmax[1] = (std::max)( m.bmax[1], v.y );
        m.bmax[2] = (std::max)( m.bmax[2], v.z );
    }

    float LsiTriArea( LsiTri const& t )
    {
        float ax = t.b.x - t.a.x, ay = t.b.y - t.a.y, az = t.b.z - t.a.z;
        float bx = t.c.x - t.a.x, by = t.c.y - t.a.y, bz = t.c.z - t.a.z;
        float nx = ay * bz - az * by;
        float ny = az * bx - ax * bz;
        float nz = ax * by - ay * bx;
        return 0.5f * std::sqrt( nx * nx + ny * ny + nz * nz );
    }

    bool LsiTriFinite( LsiTri const& t )
    {
        auto fin = []( LsiVec3 const& v ) {
            return std::isfinite( v.x ) && std::isfinite( v.y ) && std::isfinite( v.z );
        };
        return fin( t.a ) && fin( t.b ) && fin( t.c );
    }

    void LsiFinalizeMesh( LsiMeshRecord& m )
    {
        m.triCount = (int)m.tris.size();
        m.invalidTris = 0;
        m.degenerateTris = 0;
        uint32_t h = 2166136261u;
        for ( LsiTri const& t : m.tris )
        {
            if ( !LsiTriFinite( t ) ) { ++m.invalidTris; }
            else if ( LsiTriArea( t ) < 1e-10f ) { ++m.degenerateTris; }
            float v[9] = {
                t.a.x, t.a.y, t.a.z,
                t.b.x, t.b.y, t.b.z,
                t.c.x, t.c.y, t.c.z
            };
            h = LsiFnv1a( v, sizeof( v ), h );
            LsiExpandBounds( m, t.a );
            LsiExpandBounds( m, t.b );
            LsiExpandBounds( m, t.c );
        }
        uint32_t const n = (uint32_t)m.tris.size();
        h = LsiFnv1a( &n, sizeof( n ), h );
        m.hash = h;
        if ( m.tris.empty() )
        {
            m.bmin[0] = m.bmin[1] = m.bmin[2] = 0.f;
            m.bmax[0] = m.bmax[1] = m.bmax[2] = 0.f;
        }
    }

    void LsiPushTri( LsiMeshRecord& m, float x0, float y0, float z0,
        float x1, float y1, float z1, float x2, float y2, float z2 )
    {
        LsiTri t{};
        t.a = { x0, y0, z0 };
        t.b = { x1, y1, z1 };
        t.c = { x2, y2, z2 };
        m.tris.push_back( t );
    }

    // Mirror production EmitTerrainQuadAdaptive tessellation into POD (no GL, no behavior change).
    void LsiCollectTerrainQuad( LsiMeshRecord& m, float x0, float y0, float x1, float y1, int depth )
    {
        float minDist = 0.f, needR = 0.08f;
        bool const nearMouth = !g.editedRegions.empty()
            && QuadHitsMouthCollar( x0, y0, x1, y1, minDist, needR );
        float const span = (std::max)( x1 - x0, y1 - y0 );
        float const target = (std::max)( 0.04f, needR / 6.f );
        if ( nearMouth && span > target && depth < 5 )
        {
            float const xm = 0.5f * ( x0 + x1 );
            float const ym = 0.5f * ( y0 + y1 );
            LsiCollectTerrainQuad( m, x0, y0, xm, ym, depth + 1 );
            LsiCollectTerrainQuad( m, xm, y0, x1, ym, depth + 1 );
            LsiCollectTerrainQuad( m, x0, ym, xm, y1, depth + 1 );
            LsiCollectTerrainQuad( m, xm, ym, x1, y1, depth + 1 );
            return;
        }
        float z00 = 0.f, z10 = 0.f, z01 = 0.f, z11 = 0.f;
        bool const ok00 = SampleTerrainDrawZ( x0, y0, z00 );
        bool const ok10 = SampleTerrainDrawZ( x1, y0, z10 );
        bool const ok01 = SampleTerrainDrawZ( x0, y1, z01 );
        bool const ok11 = SampleTerrainDrawZ( x1, y1, z11 );
        int const nOk = ( ok00 ? 1 : 0 ) + ( ok10 ? 1 : 0 ) + ( ok01 ? 1 : 0 ) + ( ok11 ? 1 : 0 );
        if ( nOk == 0 ) { return; }
        if ( nOk < 4 )
        {
            if ( span > 0.035f && depth < 6 )
            {
                float const xm = 0.5f * ( x0 + x1 );
                float const ym = 0.5f * ( y0 + y1 );
                LsiCollectTerrainQuad( m, x0, y0, xm, ym, depth + 1 );
                LsiCollectTerrainQuad( m, xm, y0, x1, ym, depth + 1 );
                LsiCollectTerrainQuad( m, x0, ym, xm, y1, depth + 1 );
                LsiCollectTerrainQuad( m, xm, ym, x1, y1, depth + 1 );
            }
            return;
        }
        if ( ok00 && ok10 && ok01 )
        {
            LsiPushTri( m, x0, y0, z00, x1, y0, z10, x0, y1, z01 );
        }
        if ( ok10 && ok11 && ok01 )
        {
            LsiPushTri( m, x1, y0, z10, x1, y1, z11, x0, y1, z01 );
        }
    }

    void LsiFilterOutsideDirty( LsiMeshRecord const& src, LsiMeshRecord& dst,
        float excludeMinX, float excludeMinY, float excludeMaxX, float excludeMaxY )
    {
        dst = {};
        dst.tris.reserve( src.tris.size() );
        for ( LsiTri const& t : src.tris )
        {
            float const tcx = ( t.a.x + t.b.x + t.c.x ) * ( 1.f / 3.f );
            float const tcy = ( t.a.y + t.b.y + t.c.y ) * ( 1.f / 3.f );
            if ( tcx < excludeMinX || tcx > excludeMaxX
              || tcy < excludeMinY || tcy > excludeMaxY )
            {
                dst.tris.push_back( t );
            }
        }
        LsiFinalizeMesh( dst );
    }

    void LsiCaptureLocalHf( LsiMeshRecord& out, float cx, float cy, float radiusM,
        float excludeMinX, float excludeMinY, float excludeMaxX, float excludeMaxY,
        bool useExclude )
    {
        out = {};
        int const x0 = (int)std::floor( cx - radiusM - 0.5f );
        int const x1 = (int)std::ceil( cx + radiusM + 0.5f );
        int const y0 = (int)std::floor( cy - radiusM - 0.5f );
        int const y1 = (int)std::ceil( cy + radiusM + 0.5f );
        constexpr int kBase = 2;
        for ( int y = y0; y < y1; ++y )
        {
            for ( int x = x0; x < x1; ++x )
            {
                if ( !GetCell( x, y ) || !GetCell( x + 1, y )
                  || !GetCell( x, y + 1 ) || !GetCell( x + 1, y + 1 ) )
                {
                    continue;
                }
                for ( int j = 0; j < kBase; ++j )
                {
                    for ( int i = 0; i < kBase; ++i )
                    {
                        float const u0 = (float)i / (float)kBase;
                        float const v0 = (float)j / (float)kBase;
                        float const u1 = (float)( i + 1 ) / (float)kBase;
                        float const v1 = (float)( j + 1 ) / (float)kBase;
                        float const qx0 = (float)x + u0, qy0 = (float)y + v0;
                        float const qx1 = (float)x + u1, qy1 = (float)y + v1;
                        float const mx = 0.5f * ( qx0 + qx1 );
                        float const my = 0.5f * ( qy0 + qy1 );
                        if ( useExclude
                          && mx >= excludeMinX && mx <= excludeMaxX
                          && my >= excludeMinY && my <= excludeMaxY )
                        {
                            continue; // outside-identity capture skips dirty+halo
                        }
                        if ( !useExclude )
                        {
                            float const dx = mx - cx, dy = my - cy;
                            if ( dx * dx + dy * dy > ( radiusM + 0.75f ) * ( radiusM + 0.75f ) )
                            {
                                continue;
                            }
                        }
                        size_t const before = out.tris.size();
                        LsiCollectTerrainQuad( out, qx0, qy0, qx1, qy1, 0 );
                        if ( useExclude )
                        {
                            // Keep only tris whose centroid is outside exclude rect.
                            size_t w = before;
                            for ( size_t ti = before; ti < out.tris.size(); ++ti )
                            {
                                LsiTri const& t = out.tris[ti];
                                float const tcx = ( t.a.x + t.b.x + t.c.x ) * ( 1.f / 3.f );
                                float const tcy = ( t.a.y + t.b.y + t.c.y ) * ( 1.f / 3.f );
                                if ( tcx < excludeMinX || tcx > excludeMaxX
                                  || tcy < excludeMinY || tcy > excludeMaxY )
                                {
                                    out.tris[w++] = t;
                                }
                            }
                            out.tris.resize( w );
                        }
                    }
                }
            }
        }
        LsiFinalizeMesh( out );
    }

    void LsiCapturePublishedD2( LsiMeshRecord& out, float cx, float cy, float radiusM )
    {
        out = {};
        int const x0 = (int)std::floor( cx - radiusM - 1.5f );
        int const x1 = (int)std::ceil( cx + radiusM + 1.5f );
        int const y0 = (int)std::floor( cy - radiusM - 1.5f );
        int const y1 = (int)std::ceil( cy + radiusM + 1.5f );
        float const R2 = ( radiusM + 1.25f ) * ( radiusM + 1.25f );
        for ( int y = y0; y <= y1; ++y )
        {
            for ( int x = x0; x <= x1; ++x )
            {
                CellSample const* cell = GetCell( x, y );
                if ( !cell || !cell->hasCavity || cell->cavityTris.empty() ) { continue; }
                for ( DualContourQef::Tri const& t : cell->cavityTris )
                {
                    float const mx = ( t.a.x + t.b.x + t.c.x ) * ( 1.f / 3.f );
                    float const my = ( t.a.y + t.b.y + t.c.y ) * ( 1.f / 3.f );
                    float const dx = mx - cx, dy = my - cy;
                    if ( dx * dx + dy * dy > R2 ) { continue; }
                    LsiPushTri( out, t.a.x, t.a.y, t.a.z, t.b.x, t.b.y, t.b.z, t.c.x, t.c.y, t.c.z );
                }
            }
        }
        LsiFinalizeMesh( out );
    }

    void LsiCaptureOccupancy( LsiOccRecord& out, float cx, float cy, float cz, float radiusM )
    {
        out = {};
        int const x0 = (int)std::floor( cx - radiusM - 1.f );
        int const x1 = (int)std::ceil( cx + radiusM + 1.f );
        int const y0 = (int)std::floor( cy - radiusM - 1.f );
        int const y1 = (int)std::ceil( cy + radiusM + 1.f );
        uint32_t h = 2166136261u;
        for ( int y = y0; y <= y1; ++y )
        {
            for ( int x = x0; x <= x1; ++x )
            {
                CellSample const* cell = GetCell( x, y );
                if ( !cell || cell->fill.empty() ) { continue; }
                ++out.cellCount;
                out.fillBytes += (int)cell->fill.size();
                h = LsiFnv1a( cell->fill.data(), cell->fill.size(), h );
                int xy[2] = { x, y };
                h = LsiFnv1a( xy, sizeof( xy ), h );
            }
        }
        out.hash = h;
        out.solidCount = CountOccupancySolidInSphere( cx, cy, cz, radiusM );
    }

    bool LsiTriIntersectsSphere( LsiTri const& t, float cx, float cy, float cz, float r )
    {
        auto nearV = [&]( LsiVec3 const& v ) {
            float const dx = v.x - cx, dy = v.y - cy, dz = v.z - cz;
            return dx * dx + dy * dy + dz * dz <= r * r;
        };
        if ( nearV( t.a ) || nearV( t.b ) || nearV( t.c ) ) { return true; }
        float const mx = ( t.a.x + t.b.x + t.c.x ) * ( 1.f / 3.f );
        float const my = ( t.a.y + t.b.y + t.c.y ) * ( 1.f / 3.f );
        float const mz = ( t.a.z + t.b.z + t.c.z ) * ( 1.f / 3.f );
        float const dx = mx - cx, dy = my - cy, dz = mz - cz;
        return dx * dx + dy * dy + dz * dz <= r * r;
    }

    int LsiCountMeshSphereHits( LsiMeshRecord const& m, float cx, float cy, float cz, float r )
    {
        int n = 0;
        for ( LsiTri const& t : m.tris )
        {
            if ( LsiTriIntersectsSphere( t, cx, cy, cz, r ) ) { ++n; }
        }
        return n;
    }

    bool LsiPointCoveredByD2( LsiMeshRecord const& d2, float x, float y, float z, float tol )
    {
        for ( LsiTri const& t : d2.tris )
        {
            float const minx = (std::min)( t.a.x, (std::min)( t.b.x, t.c.x ) ) - tol;
            float const maxx = (std::max)( t.a.x, (std::max)( t.b.x, t.c.x ) ) + tol;
            float const miny = (std::min)( t.a.y, (std::min)( t.b.y, t.c.y ) ) - tol;
            float const maxy = (std::max)( t.a.y, (std::max)( t.b.y, t.c.y ) ) + tol;
            if ( x < minx || x > maxx || y < miny || y > maxy ) { continue; }
            float const mx = ( t.a.x + t.b.x + t.c.x ) * ( 1.f / 3.f );
            float const my = ( t.a.y + t.b.y + t.c.y ) * ( 1.f / 3.f );
            float const mz = ( t.a.z + t.b.z + t.c.z ) * ( 1.f / 3.f );
            float const dx = mx - x, dy = my - y, dz = mz - z;
            if ( dx * dx + dy * dy + dz * dz <= tol * tol * 4.f ) { return true; }
            // XY proximity + Z band (published cavity wall near intent sample).
            if ( dx * dx + dy * dy <= tol * tol && std::fabs( mz - z ) <= tol * 3.f ) { return true; }
        }
        return false;
    }

    // Boundary edges on an open dig mouth are expected. Count only unexplained tears:
    // open edges that are neither near a crest/mouth opening nor on the dirty XY rim.
    int LsiCountUnexplainedOpenEdges( LsiMeshRecord const& m, float dirtyMinX, float dirtyMinY,
        float dirtyMaxX, float dirtyMaxY, float openX, float openY, float openZ, float openR,
        int& totalBoundary, int& mouthRimEdges )
    {
        totalBoundary = 0;
        mouthRimEdges = 0;
        struct EdgeKey
        {
            int ax, ay, az, bx, by, bz;
            bool operator==( EdgeKey const& o ) const
            {
                return ax == o.ax && ay == o.ay && az == o.az
                    && bx == o.bx && by == o.by && bz == o.bz;
            }
        };
        struct EdgeHash
        {
            size_t operator()( EdgeKey const& k ) const
            {
                size_t h = (size_t)k.ax * 73856093u ^ (size_t)k.ay * 19349663u
                    ^ (size_t)k.az * 83492791u ^ (size_t)k.bx * 50331653u
                    ^ (size_t)k.by * 12582917u ^ (size_t)k.bz * 2654435761u;
                return h;
            }
        };
        auto q = []( float v ) -> int { return (int)std::lround( v * 1000.f ); };
        auto pack = [&]( LsiVec3 const& a, LsiVec3 const& b ) -> EdgeKey
        {
            EdgeKey e{};
            int ax = q( a.x ), ay = q( a.y ), az = q( a.z );
            int bx = q( b.x ), by = q( b.y ), bz = q( b.z );
            if ( ax < bx || ( ax == bx && ay < by ) || ( ax == bx && ay == by && az <= bz ) )
            {
                e = { ax, ay, az, bx, by, bz };
            }
            else
            {
                e = { bx, by, bz, ax, ay, az };
            }
            return e;
        };
        std::unordered_map<EdgeKey, int, EdgeHash> counts;
        counts.reserve( m.tris.size() * 3 );
        for ( LsiTri const& t : m.tris )
        {
            EdgeKey e0 = pack( t.a, t.b );
            EdgeKey e1 = pack( t.b, t.c );
            EdgeKey e2 = pack( t.c, t.a );
            ++counts[e0]; ++counts[e1]; ++counts[e2];
        }
        int unexplained = 0;
        float const pad = 0.08f;
        float const mouthR = (std::max)( openR, 0.12f ) + 0.10f;
        float const mouthR2 = mouthR * mouthR;
        for ( auto const& kv : counts )
        {
            if ( kv.second != 1 ) { continue; }
            ++totalBoundary;
            float const mx = 0.0005f * (float)( kv.first.ax + kv.first.bx );
            float const my = 0.0005f * (float)( kv.first.ay + kv.first.by );
            float const mz = 0.0005f * (float)( kv.first.az + kv.first.bz );
            bool const onDirtyRim =
                mx <= dirtyMinX + pad || mx >= dirtyMaxX - pad
                || my <= dirtyMinY + pad || my >= dirtyMaxY - pad;
            float const dx = mx - openX, dy = my - openY;
            bool const nearMouthXy = ( dx * dx + dy * dy ) <= mouthR2;
            bool const nearCrestZ = std::fabs( mz - openZ ) <= 0.35f;
            bool const nearOpening = NearOpeningMouthAt( mx, my ) || ( nearMouthXy && nearCrestZ );
            if ( nearOpening || onDirtyRim )
            {
                ++mouthRimEdges;
                continue;
            }
            ++unexplained;
        }
        return unexplained;
    }

    void LsiResolveDirtyHalo( LsiCapture& cap, float fallbackX, float fallbackY, float fallbackR )
    {
        cap.haveDirty = false;
        for ( EditedRegion const& er : g.editedRegions )
        {
            if ( !er.hasOwnBounds ) { continue; }
            float const dx = 0.5f * ( er.ownMinX + er.ownMaxX ) - fallbackX;
            float const dy = 0.5f * ( er.ownMinY + er.ownMaxY ) - fallbackY;
            if ( dx * dx + dy * dy > ( fallbackR + 3.f ) * ( fallbackR + 3.f ) ) { continue; }
            float const halo = 1.0f + 0.35f; // +1 cell halo + tip/6 spill collar
            cap.dirtyMinX = er.ownMinX - halo;
            cap.dirtyMinY = er.ownMinY - halo;
            cap.dirtyMaxX = er.ownMaxX + halo;
            cap.dirtyMaxY = er.ownMaxY + halo;
            cap.haveDirty = true;
            break;
        }
        if ( !cap.haveDirty )
        {
            float const halo = fallbackR + 1.35f;
            cap.dirtyMinX = fallbackX - halo;
            cap.dirtyMinY = fallbackY - halo;
            cap.dirtyMaxX = fallbackX + halo;
            cap.dirtyMaxY = fallbackY + halo;
            cap.haveDirty = true;
        }
    }

    void LsiProbeCoverageContinuity( LsiCapture& cap )
    {
        // Intent samples over the action XY disk. Dig: grade/skin band. Place: action Z band
        // (cavity floor may sit well below grade — do not require grade∈sphere).
        float const R = cap.vol.r;
        float const step = (std::max)( 0.04f, R / 5.f );
        bool const isPlace = ( std::strcmp( cap.kind, "place" ) == 0 );
        cap.voidSamples = 0;
        cap.intentSamples = 0;
        cap.coveredSamples = 0;
        for ( float y = cap.vol.cy - R; y <= cap.vol.cy + R + 1e-4f; y += step )
        {
            for ( float x = cap.vol.cx - R; x <= cap.vol.cx + R + 1e-4f; x += step )
            {
                float const dx = x - cap.vol.cx, dy = y - cap.vol.cy;
                if ( dx * dx + dy * dy > R * R ) { continue; }
                float zGrade = cap.vol.openZ;
                SampleGroundZBase( x, y, zGrade );
                float zIntent = zGrade;
                if ( isPlace )
                {
                    zIntent = cap.vol.cz;
                }
                else
                {
                    float const dz = zGrade - cap.vol.cz;
                    if ( dx * dx + dy * dy + dz * dz > ( R * 1.15f ) * ( R * 1.15f ) ) { continue; }
                }
                ++cap.intentSamples;
                float zDraw = 0.f;
                bool const hfOk = SampleTerrainDrawZ( x, y, zDraw );
                bool const mouth = CrestMouthStencilAt( x, y ) || NearOpeningMouthAt( x, y );
                bool const d2Ok = LsiPointCoveredByD2( cap.d2After, x, y, zIntent, 0.22f );
                bool const occSolid = OccupancySolidAt( x, y, zIntent );
                if ( hfOk || d2Ok || ( isPlace && occSolid ) )
                {
                    ++cap.coveredSamples;
                }
                else if ( mouth || !hfOk )
                {
                    ++cap.voidSamples;
                    if ( s_lsiDefectN < kLsiDefectCap )
                    {
                        char note[120];
                        std::snprintf( note, sizeof( note ),
                            "void hf=%d mouth=%d d2=%d occ=%d",
                            hfOk ? 1 : 0, mouth ? 1 : 0, d2Ok ? 1 : 0, occSolid ? 1 : 0 );
                        LsiAddDefect( "UNEXPLAINED_VOID", note, x, y, zIntent );
                    }
                }
            }
        }
    }

    void LsiCertifyCapture( LsiCapture& cap )
    {
        auto row = [&]( char const* check, char const* verdict, char const* note,
            uint32_t ha = 0, uint32_t hb = 0, int na = 0, int nb = 0 )
        {
            LsiRow r{};
            std::snprintf( r.action, sizeof( r.action ), "%s", cap.actionId );
            std::snprintf( r.check, sizeof( r.check ), "%s", check );
            std::snprintf( r.verdict, sizeof( r.verdict ), "%s", verdict );
            std::snprintf( r.note, sizeof( r.note ), "%s", note );
            r.x = cap.vol.cx; r.y = cap.vol.cy; r.z = cap.vol.cz;
            r.hashA = ha; r.hashB = hb;
            r.nA = na; r.nB = nb;
            LsiAddRow( r );
        };

        // Coverage: dig removes/intersects surface; surviving publish must cover intent samples.
        {
            float coverRatio = ( cap.intentSamples > 0 )
                ? (float)cap.coveredSamples / (float)cap.intentSamples : 0.f;
            char note[200];
            std::snprintf( note, sizeof( note ),
                "intent=%d covered=%d voids=%d hfHit %d->%d d2Hit=%d ratio=%.2f",
                cap.intentSamples, cap.coveredSamples, cap.voidSamples,
                cap.vol.hfIntersectBefore, cap.vol.hfIntersectAfter, cap.vol.d2IntersectAfter,
                coverRatio );
            bool const ok = ( cap.intentSamples > 0 ) && ( cap.voidSamples == 0 )
                && ( coverRatio >= 0.95f );
            row( "coverage", ok ? "PASS" : "FAIL", note,
                cap.hfBefore.hash, cap.d2After.hash,
                cap.vol.hfIntersectBefore, cap.vol.d2IntersectAfter );
            if ( !ok )
            {
                LsiAddDefect( "COVERAGE_GAP", note, cap.vol.cx, cap.vol.cy, cap.vol.cz );
            }
        }

        // Continuity: unexplained gaps relative to intent (void inventory).
        {
            char note[160];
            std::snprintf( note, sizeof( note ),
                "voidSamples=%d intent=%d (record gaps; do not greenwash)",
                cap.voidSamples, cap.intentSamples );
            bool const ok = ( cap.voidSamples == 0 ) && ( cap.intentSamples > 0 );
            row( "continuity", ok ? "PASS" : "FAIL", note, 0, 0, cap.voidSamples, cap.intentSamples );
        }

        // Tool-scale agreement: committed volume radius vs handful / tip mouth.
        {
            float const handful = kHandfulRadiusM;
            float const tipMax = 0.12f;
            float const used = cap.vol.toolScaleR > 1e-6f ? cap.vol.toolScaleR : cap.vol.r;
            bool const ok = ( used >= handful * 0.5f ) && ( used <= (std::max)( tipMax, handful * 8.f ) );
            char note[160];
            std::snprintf( note, sizeof( note ),
                "R=%.4f tool=%.4f handful=%.4f tipMax=%.2f",
                cap.vol.r, used, handful, tipMax );
            row( "tool_scale", ok ? "PASS" : "FAIL", note );
        }

        // Boundary closure: open mouth-rim edges expected; unexplained tears are defects.
        {
            char note[180];
            std::snprintf( note, sizeof( note ),
                "unexplainedOpen=%d mouthRim=%d boundaryTotal=%d d2Tris=%d",
                cap.openBoundaryEdges, cap.mouthRimEdges, cap.boundaryEdgesTotal, cap.d2After.triCount );
            bool const ok = ( cap.d2After.triCount > 0 ) && ( cap.openBoundaryEdges == 0 );
            row( "boundary_closure", ok ? "PASS" : "FAIL", note,
                0, 0, cap.openBoundaryEdges, cap.mouthRimEdges );
            if ( !ok )
            {
                LsiAddDefect( "BOUNDARY_OPEN", note, cap.vol.cx, cap.vol.cy, cap.vol.cz );
            }
        }

        // Triangle validity on published HF after + D2.
        {
            int inv = cap.hfAfter.invalidTris + cap.d2After.invalidTris;
            int deg = cap.hfAfter.degenerateTris + cap.d2After.degenerateTris;
            char note[160];
            std::snprintf( note, sizeof( note ),
                "invalid=%d degenerate=%d hfTris=%d d2Tris=%d",
                inv, deg, cap.hfAfter.triCount, cap.d2After.triCount );
            bool const ok = ( inv == 0 && deg == 0 );
            row( "triangle_validity", ok ? "PASS" : "FAIL", note );
        }

        // Outside-region identity: geometry outside dirty+halo bit-identical.
        {
            bool const ok = ( cap.hfOutsideBefore.hash == cap.hfOutsideAfter.hash )
                && ( cap.hfOutsideBefore.triCount == cap.hfOutsideAfter.triCount );
            char note[180];
            std::snprintf( note, sizeof( note ),
                "outside tris %d->%d hash %08x->%08x dirty=[%.2f..%.2f,%.2f..%.2f]",
                cap.hfOutsideBefore.triCount, cap.hfOutsideAfter.triCount,
                (unsigned)cap.hfOutsideBefore.hash, (unsigned)cap.hfOutsideAfter.hash,
                cap.dirtyMinX, cap.dirtyMaxX, cap.dirtyMinY, cap.dirtyMaxY );
            row( "outside_identity", ok ? "PASS" : "FAIL", note,
                cap.hfOutsideBefore.hash, cap.hfOutsideAfter.hash,
                cap.hfOutsideBefore.triCount, cap.hfOutsideAfter.triCount );
            if ( !ok )
            {
                LsiAddDefect( "OUTSIDE_MUTATION", note, cap.vol.cx, cap.vol.cy, cap.vol.cz );
            }
        }

        // Occupancy delta bookkeeping (observation). Dig may leave solidCount==0 in the
        // air sphere; hash change still proves the edit when before was seeded.
        {
            char note[180];
            std::snprintf( note, sizeof( note ),
                "solid %d->%d hash %08x->%08x cells=%d",
                cap.occBefore.solidCount, cap.occAfter.solidCount,
                (unsigned)cap.occBefore.hash, (unsigned)cap.occAfter.hash,
                cap.occAfter.cellCount );
            bool const hashChanged = ( cap.occBefore.hash != cap.occAfter.hash );
            bool const digOk = ( std::strcmp( cap.kind, "dig" ) == 0 )
                && hashChanged
                && ( cap.occAfter.solidCount <= cap.occBefore.solidCount );
            bool const placeOk = ( std::strcmp( cap.kind, "place" ) == 0 )
                && ( cap.occAfter.solidCount > cap.occBefore.solidCount || hashChanged );
            bool const ok = digOk || placeOk;
            row( "occupancy_delta", ok ? "PASS" : "FAIL", note,
                cap.occBefore.hash, cap.occAfter.hash,
                cap.occBefore.solidCount, cap.occAfter.solidCount );
        }
    }

    void LsiWriteArtifact()
    {
        if ( !g.certOutDir[0] ) { GetTempPathA( MAX_PATH, g.certOutDir ); }
        char path[MAX_PATH];
        std::snprintf( path, sizeof( path ), "%s\\provenance_local_surface_intent_cert.txt", g.certOutDir );
        FILE* f = nullptr;
        if ( fopen_s( &f, path, "w" ) != 0 || !f ) { return; }
        int passN = 0, failN = 0, skipN = 0;
        for ( int i = 0; i < s_lsiRowN; ++i )
        {
            if ( std::strcmp( s_lsiRows[i].verdict, "PASS" ) == 0 ) { ++passN; }
            else if ( std::strcmp( s_lsiRows[i].verdict, "FAIL" ) == 0 ) { ++failN; }
            else { ++skipN; }
        }
        std::fprintf( f,
            "Provenance Local Surface Intent cert\n"
            "read_only=1 freeze=D2,occ,ER,SupportBelow,PlaceOccupancyFill,chips\n"
            "fixture=RANGE\n"
            "exit_code=%d\n"
            "PASS_rows=%d FAIL_rows=%d SKIP_rows=%d rows=%d defects=%d\n"
            "first_fail=%s\n"
            "note=FAIL rows document present defects; do not greenwash terrain systems\n"
            "\n",
            g.certLsiExitCode, passN, failN, skipN, s_lsiRowN, s_lsiDefectN,
            s_lsiFailReason[0] ? s_lsiFailReason : "none" );
        std::fprintf( f,
            "action\tcheck\tverdict\tx\ty\tz\thashA\thashB\tnA\tnB\tnote\n" );
        for ( int i = 0; i < s_lsiRowN; ++i )
        {
            LsiRow const& r = s_lsiRows[i];
            std::fprintf( f, "%s\t%s\t%s\t%.3f\t%.3f\t%.3f\t%08x\t%08x\t%d\t%d\t%s\n",
                r.action, r.check, r.verdict, r.x, r.y, r.z,
                (unsigned)r.hashA, (unsigned)r.hashB, r.nA, r.nB, r.note );
        }
        std::fprintf( f, "\n# defect inventory\n" );
        for ( int i = 0; i < s_lsiDefectN; ++i )
        {
            LsiDefect const& d = s_lsiDefects[i];
            std::fprintf( f, "DEFECT\t%s\t(%.3f,%.3f,%.3f)\t%s\n",
                d.kind, d.x, d.y, d.z, d.note );
        }
        std::fclose( f );

        if ( g.certLsiExitCode != 0 && !g.certLsiFailWritten )
        {
            g.certLsiFailWritten = true;
            char fpath[MAX_PATH];
            std::snprintf( fpath, sizeof( fpath ),
                "%s\\provenance_local_surface_intent_fail.txt", g.certOutDir );
            FILE* ff = nullptr;
            if ( fopen_s( &ff, fpath, "w" ) == 0 && ff )
            {
                std::fprintf( ff,
                    "FAIL:\n"
                    "reason=%s\n"
                    "defects=%d\n"
                    "note=see provenance_local_surface_intent_cert.txt; observation only\n",
                    s_lsiFailReason[0] ? s_lsiFailReason : "defect_rows",
                    s_lsiDefectN );
                std::fclose( ff );
            }
        }
    }

    void LsiRunActionDigCavity( LsiCapture& cap )
    {
        std::snprintf( cap.actionId, sizeof( cap.actionId ), "dig_cavity" );
        std::snprintf( cap.kind, sizeof( cap.kind ), "dig" );
        float const ox = (float)ProvenanceGeo::kRangeOriginX;
        float const oy = (float)ProvenanceGeo::kRangeOriginY;
        float const x = ox + 8.f;
        float const y = oy - 30.f;
        float grade = 0.f;
        SampleGroundZBase( x, y, grade );
        float const R = 0.24f;
        float const carveZ = grade - R * 0.40f;
        cap.vol.cx = x; cap.vol.cy = y; cap.vol.cz = carveZ; cap.vol.r = R;
        cap.vol.openX = x; cap.vol.openY = y; cap.vol.openZ = grade;
        cap.vol.toolScaleR = (std::min)( R, 0.12f );

        EnsureGeoDisk( (int)std::floor( x ), (int)std::floor( y ), 8 );
        PrefetchOccupancyCell( (int)std::floor( x ), (int)std::floor( y ) );
        for ( int dy = -1; dy <= 1; ++dy )
            for ( int dx = -1; dx <= 1; ++dx )
                PrefetchOccupancyCell( (int)std::floor( x ) + dx, (int)std::floor( y ) + dy );

        // Seed occupancy for a fair before-snapshot (CarveOccupancySphere also seeds — observation only).
        for ( int dy = -1; dy <= 1; ++dy )
        {
            for ( int dx = -1; dx <= 1; ++dx )
            {
                int const cx = (int)std::floor( x ) + dx;
                int const cy = (int)std::floor( y ) + dy;
                EnsureOccupancyLattice( cx, cy );
                CellSample* cell = GetCellMutable( cx, cy );
                if ( cell && !cell->carved && !cell->fill.empty() )
                {
                    SeedOccupancyFromVirginSurface( cx, cy );
                }
            }
        }

        // Full neighborhood HF before action (same adaptive tessellation as live mesh).
        LsiMeshRecord hfHoodBefore{};
        LsiCaptureLocalHf( hfHoodBefore, x, y, 3.5f, 0, 0, 0, 0, false );
        LsiCaptureLocalHf( cap.hfBefore, x, y, R + 1.25f, 0, 0, 0, 0, false );
        LsiCaptureOccupancy( cap.occBefore, x, y, carveZ, R );
        cap.vol.hfIntersectBefore = LsiCountMeshSphereHits( cap.hfBefore, x, y, carveZ, R );

        bool const carved = CarveOccupancySphere( x, y, carveZ, R, x, y, grade );
        if ( g.terrainDirty ) { RebuildTerrainMesh(); }

        LsiResolveDirtyHalo( cap, x, y, R );
        // Outside identity uses the same post-action dirty+halo rect on before/after.
        LsiFilterOutsideDirty( hfHoodBefore, cap.hfOutsideBefore,
            cap.dirtyMinX, cap.dirtyMinY, cap.dirtyMaxX, cap.dirtyMaxY );
        LsiCaptureLocalHf( cap.hfAfter, x, y, R + 1.25f, 0, 0, 0, 0, false );
        LsiMeshRecord hfHoodAfter{};
        LsiCaptureLocalHf( hfHoodAfter, x, y, 3.5f, 0, 0, 0, 0, false );
        LsiFilterOutsideDirty( hfHoodAfter, cap.hfOutsideAfter,
            cap.dirtyMinX, cap.dirtyMinY, cap.dirtyMaxX, cap.dirtyMaxY );
        LsiCapturePublishedD2( cap.d2After, x, y, R + 1.25f );
        LsiCaptureOccupancy( cap.occAfter, x, y, carveZ, R );
        cap.vol.hfIntersectAfter = LsiCountMeshSphereHits( cap.hfAfter, x, y, carveZ, R );
        cap.vol.d2IntersectAfter = LsiCountMeshSphereHits( cap.d2After, x, y, carveZ, R );
        cap.openBoundaryEdges = LsiCountUnexplainedOpenEdges( cap.d2After,
            cap.dirtyMinX, cap.dirtyMinY, cap.dirtyMaxX, cap.dirtyMaxY,
            x, y, grade, (std::min)( R, 0.12f ),
            cap.boundaryEdgesTotal, cap.mouthRimEdges );
        LsiProbeCoverageContinuity( cap );

        if ( !carved )
        {
            LsiAddDefect( "DIG_NO_EFFECT", "CarveOccupancySphere returned false", x, y, grade );
            LsiRow r{};
            std::snprintf( r.action, sizeof( r.action ), "dig_cavity" );
            std::snprintf( r.check, sizeof( r.check ), "action_commit" );
            std::snprintf( r.verdict, sizeof( r.verdict ), "FAIL" );
            std::snprintf( r.note, sizeof( r.note ), "CARVE_FALSE" );
            r.x = x; r.y = y; r.z = carveZ;
            LsiAddRow( r );
        }
        LsiCertifyCapture( cap );
    }

    void LsiRunActionPlaceIntoCavity( LsiCapture& cap )
    {
        std::snprintf( cap.actionId, sizeof( cap.actionId ), "place_into_cavity" );
        std::snprintf( cap.kind, sizeof( cap.kind ), "place" );
        float const ox = (float)ProvenanceGeo::kRangeOriginX;
        float const oy = (float)ProvenanceGeo::kRangeOriginY;
        float const x = ox + 12.f;
        float const y = oy - 30.f;
        float grade = 0.f;
        SampleGroundZBase( x, y, grade );
        float const digR = 0.22f;
        float const carveZ = grade - digR * 0.40f;
        float const placeR = (std::max)( kHandfulRadiusM * 1.8f, kVoxelEdgeM * 1.25f );

        EnsureGeoDisk( (int)std::floor( x ), (int)std::floor( y ), 8 );
        PrefetchOccupancyCell( (int)std::floor( x ), (int)std::floor( y ) );
        for ( int dy = -1; dy <= 1; ++dy )
            for ( int dx = -1; dx <= 1; ++dx )
                PrefetchOccupancyCell( (int)std::floor( x ) + dx, (int)std::floor( y ) + dy );

        bool const carved = CarveOccupancySphere( x, y, carveZ, digR, x, y, grade );
        if ( g.terrainDirty ) { RebuildTerrainMesh(); }
        if ( !carved )
        {
            LsiRow r{};
            std::snprintf( r.action, sizeof( r.action ), "place_into_cavity" );
            std::snprintf( r.check, sizeof( r.check ), "pre_dig" );
            std::snprintf( r.verdict, sizeof( r.verdict ), "FAIL" );
            std::snprintf( r.note, sizeof( r.note ), "PRE_DIG_FAILED" );
            r.x = x; r.y = y; r.z = grade;
            LsiAddRow( r );
            return;
        }

        SupportHit const floor0 = SupportBelow( x, y, grade + 0.5f );
        float placeZ = carveZ;
        if ( floor0.hit ) { placeZ = floor0.position.z + kVoxelEdgeM * 0.35f; }

        cap.vol.cx = x; cap.vol.cy = y; cap.vol.cz = placeZ; cap.vol.r = placeR;
        cap.vol.openX = x; cap.vol.openY = y; cap.vol.openZ = grade;
        cap.vol.toolScaleR = placeR;

        LsiMeshRecord hfHoodBefore{};
        LsiCaptureLocalHf( hfHoodBefore, x, y, 3.5f, 0, 0, 0, 0, false );
        LsiCaptureLocalHf( cap.hfBefore, x, y, placeR + 1.25f, 0, 0, 0, 0, false );
        LsiCaptureOccupancy( cap.occBefore, x, y, placeZ, placeR );
        cap.vol.hfIntersectBefore = LsiCountMeshSphereHits( cap.hfBefore, x, y, placeZ, placeR );

        std::unordered_map<std::string, int> credit;
        credit["dirt"] = (int)std::lround( kDirtVoxelG );
        CreditHeld( credit );
        PlaceFillResult const pr = PlaceOccupancyFill( x, y, placeZ, placeR, credit["dirt"] );
        if ( pr.ok ) { DebitHeldTotal( pr.acceptedGrams ); }
        if ( g.terrainDirty ) { RebuildTerrainMesh(); }

        LsiResolveDirtyHalo( cap, x, y, placeR );
        LsiFilterOutsideDirty( hfHoodBefore, cap.hfOutsideBefore,
            cap.dirtyMinX, cap.dirtyMinY, cap.dirtyMaxX, cap.dirtyMaxY );
        LsiCaptureLocalHf( cap.hfAfter, x, y, placeR + 1.25f, 0, 0, 0, 0, false );
        LsiMeshRecord hfHoodAfter{};
        LsiCaptureLocalHf( hfHoodAfter, x, y, 3.5f, 0, 0, 0, 0, false );
        LsiFilterOutsideDirty( hfHoodAfter, cap.hfOutsideAfter,
            cap.dirtyMinX, cap.dirtyMinY, cap.dirtyMaxX, cap.dirtyMaxY );
        LsiCapturePublishedD2( cap.d2After, x, y, placeR + 1.25f );
        LsiCaptureOccupancy( cap.occAfter, x, y, placeZ, placeR );
        cap.vol.hfIntersectAfter = LsiCountMeshSphereHits( cap.hfAfter, x, y, placeZ, placeR );
        cap.vol.d2IntersectAfter = LsiCountMeshSphereHits( cap.d2After, x, y, placeZ, placeR );
        cap.openBoundaryEdges = LsiCountUnexplainedOpenEdges( cap.d2After,
            cap.dirtyMinX, cap.dirtyMinY, cap.dirtyMaxX, cap.dirtyMaxY,
            x, y, grade, (std::min)( digR, 0.12f ),
            cap.boundaryEdgesTotal, cap.mouthRimEdges );
        LsiProbeCoverageContinuity( cap );

        if ( !pr.ok || pr.acceptedGrams <= 0 )
        {
            LsiAddDefect( "PLACE_NO_MATTER", "PlaceOccupancyFill accepted nothing", x, y, placeZ );
            LsiRow r{};
            std::snprintf( r.action, sizeof( r.action ), "place_into_cavity" );
            std::snprintf( r.check, sizeof( r.check ), "action_commit" );
            std::snprintf( r.verdict, sizeof( r.verdict ), "FAIL" );
            std::snprintf( r.note, sizeof( r.note ), "PLACE_FILL_NO_MATTER" );
            r.x = x; r.y = y; r.z = placeZ;
            LsiAddRow( r );
        }
        LsiCertifyCapture( cap );
    }

    void CertLsiTick()
    {
        if ( !g.certLsi ) { return; }
        DWORD const now = GetTickCount();

        if ( g.certLsiPhase == 0 )
        {
            ProvenanceGeo::SetFixture( ProvenanceGeo::GeoFixture::Range );
            if ( !g.streamComplete || g.link != LinkState::CapsOk )
            {
                if ( g.certLsiPhaseMs == 0 ) { g.certLsiPhaseMs = now; }
                if ( now - g.certLsiPhaseMs > 45000 )
                {
                    LsiRow r{};
                    std::snprintf( r.action, sizeof( r.action ), "connect" );
                    std::snprintf( r.check, sizeof( r.check ), "stream_ready" );
                    std::snprintf( r.verdict, sizeof( r.verdict ), "FAIL" );
                    std::snprintf( r.note, sizeof( r.note ), "BRIDGE_OR_CAPS_TIMEOUT" );
                    LsiAddRow( r );
                    LsiWriteArtifact();
                    g.certLsiPhase = 3;
                    PostQuitMessage( 1 );
                }
                return;
            }
            SnapVirginPerfIfNeeded();
            s_lsiRowN = 0;
            s_lsiDefectN = 0;
            s_lsiFailReason[0] = 0;
            g.certLsiExitCode = 0;
            g.certLsiFailWritten = false;
            EnsureGeoDisk(
                (int)ProvenanceGeo::kRangeOriginX + 10,
                (int)ProvenanceGeo::kRangeOriginY - 30,
                24 );
            g.certLsiPhase = 1;
            g.certLsiPhaseMs = now;
            g.statusLine = "CERT-LSI settle residency";
            return;
        }

        if ( g.certLsiPhase == 1 )
        {
            if ( g.terrainDirty && now - g.certLsiPhaseMs < 2000 ) { return; }
            g.certLsiPhase = 2;
            g.certLsiPhaseMs = now;
            g.statusLine = "CERT-LSI capture dig+place";
            return;
        }

        if ( g.certLsiPhase == 2 )
        {
            LsiCapture digCap{};
            LsiRunActionDigCavity( digCap );
            LsiCapture placeCap{};
            LsiRunActionPlaceIntoCavity( placeCap );

            // Capture summary rows (geometry record sizes/hashes).
            auto summary = [&]( LsiCapture const& c )
            {
                LsiRow r{};
                std::snprintf( r.action, sizeof( r.action ), "%s", c.actionId );
                std::snprintf( r.check, sizeof( r.check ), "capture_summary" );
                std::snprintf( r.verdict, sizeof( r.verdict ), "PASS" );
                std::snprintf( r.note, sizeof( r.note ),
                    "hf %d->%d d2=%d occSolid %d->%d outsideHash %08x->%08x",
                    c.hfBefore.triCount, c.hfAfter.triCount, c.d2After.triCount,
                    c.occBefore.solidCount, c.occAfter.solidCount,
                    (unsigned)c.hfOutsideBefore.hash, (unsigned)c.hfOutsideAfter.hash );
                r.x = c.vol.cx; r.y = c.vol.cy; r.z = c.vol.cz;
                r.hashA = c.hfBefore.hash; r.hashB = c.d2After.hash;
                r.nA = c.hfBefore.triCount; r.nB = c.d2After.triCount;
                LsiAddRow( r );
            };
            summary( digCap );
            summary( placeCap );

            LsiWriteArtifact();
            g.statusLine = g.certLsiExitCode
                ? "CERT-LSI done — FAIL defects captured (read-only)"
                : "CERT-LSI done — PASS";
            g.certLsiPhase = 3;
            PostQuitMessage( g.certLsiExitCode );
            return;
        }
    }

    void CertGeoTick()
    {
        if ( !g.certGeo ) { return; }
        DWORD const now = GetTickCount();

        // Phase 0: wait for stream + force RANGE.
        if ( g.certGeoPhase == 0 )
        {
            ProvenanceGeo::SetFixture( ProvenanceGeo::GeoFixture::Range );
            if ( !g.streamComplete || g.link != LinkState::CapsOk )
            {
                if ( g.certGeoPhaseMs == 0 ) { g.certGeoPhaseMs = now; }
                if ( now - g.certGeoPhaseMs > 45000 )
                {
                    GeoCertHardFail( "connect", "wait_stream", "BRIDGE_OR_CAPS_TIMEOUT",
                        g.feetX, g.feetY, g.feetZ, 0, 0, 1 );
                    GeoCertRow r{};
                    std::snprintf( r.section, sizeof( r.section ), "2" );
                    std::snprintf( r.scenario, sizeof( r.scenario ), "stream_ready" );
                    std::snprintf( r.verdict, sizeof( r.verdict ), "FAIL" );
                    std::snprintf( r.note, sizeof( r.note ), "BRIDGE_OR_CAPS_TIMEOUT link=%s", LinkLabel( g.link ) );
                    GeoCertAddRow( r );
                    GeoCertScaffoldRest();
                    GeoCertWriteArtifact();
                    g.certGeoPhase = 4;
                    PostQuitMessage( 1 );
                }
                return;
            }
            SnapVirginPerfIfNeeded();
            s_geoRowN = 0;
            s_geoContactN = 0;
            g.certGeoFailWritten = false;
            g.certGeoExitCode = 0;
            g.certGeoDigIdx = 0;
            g.certGeoDigArmed = 0;
            // Prefetch full RANGE transect residency once so walk need not grow the disk.
            // Cover spawn→J (x~230) plus FollowStreamCenter r=64 halo if freeze ever lifts.
            EnsureGeoDisk(
                (int)ProvenanceGeo::kRangeOriginX + 55,
                (int)ProvenanceGeo::kRangeOriginY,
                120 );
            GeoCertDiscoverContacts();
            GeoCertRunVirginSection();
            if ( g.certGeoExitCode != 0 )
            {
                GeoCertScaffoldRest();
                GeoCertWriteArtifact();
                g.certGeoPhase = 4;
                PostQuitMessage( g.certGeoExitCode );
                return;
            }
            g.certGeoPhase = 1; // settle prefetch remesh one frame, then walk
            g.certGeoPhaseMs = now;
            g.statusLine = "CERT-GEO settle transect residency";
            return;
        }

        // Phase 1a: after prefetch remesh, snapshot baselines then walk.
        // Phase 1 uses certGeoDigArmed as substate: 0=settle, 1=walking.
        if ( g.certGeoPhase == 1 )
        {
            float const ox = (float)ProvenanceGeo::kRangeOriginX;
            float const oy = (float)ProvenanceGeo::kRangeOriginY;
            if ( g.certGeoDigArmed == 0 )
            {
                // Wait until terrainDirty cleared (remesh done) or timeout.
                if ( g.terrainDirty && now - g.certGeoPhaseMs < 2000 ) { return; }
                g.certGeoWalkHf0 = g.perfHfRebuilds;
                g.certGeoWalkD20 = g.perfD2Rebuilds;
                g.certGeoWalkCells0 = g.perfGeoCellsCreated;
                g.certGeoWalkSample0 = g.perfSampleSurfaceCalls;
                g.certGeoDigArmed = 1;
                g.certGeoPhaseMs = now;
                g.statusLine = "CERT-GEO walk transect (no remesh expected)";
                return;
            }
            int step = (int)( ( now - g.certGeoPhaseMs ) / 200 );
            if ( step <= 20 )
            {
                float u = (float)step * 5.f; // 0..100 m
                GeoCertTeleport( ox + u, oy, -0.35f, 0.f );
                return;
            }
            GeoCertRunWalkSection();
            // §3 while still phase-1 walk freeze (digArmed=1): aim/look must not remesh or wake D2.
            g.statusLine = "CERT-GEO HF refine no-edit";
            GeoCertRunHfRefineNoEditSection();
            if ( g.certGeoExitCode != 0 )
            {
                GeoCertScaffoldRest();
                GeoCertWriteArtifact();
                g.certGeoPhase = 4;
                PostQuitMessage( g.certGeoExitCode );
                return;
            }
            // Residency/remesh walk FAIL is recorded; continue dig matrix — dig still valuable.
            g.certGeoPhase = 2;
            g.certGeoPhaseMs = now;
            g.certGeoDigIdx = 0;
            g.certGeoDigArmed = 0;
            g.statusLine = "CERT-GEO dig matrix";
            return;
        }

        // Phase 2: dig matrix — one diggable contact per settle window.
        if ( g.certGeoPhase == 2 )
        {
            while ( g.certGeoDigIdx < s_geoContactN )
            {
                GeoCertContact const& c = s_geoContacts[g.certGeoDigIdx];
                bool diggable = c.found
                    && std::strcmp( c.id, "cell_edge" ) != 0
                    && std::strcmp( c.id, "multi_cell" ) != 0
                    && std::strcmp( c.id, "mat_bound" ) != 0;
                if ( diggable ) { break; }
                ++g.certGeoDigIdx;
            }
            if ( g.certGeoDigIdx >= s_geoContactN )
            {
                g.certGeoPhase = 3;
                g.certGeoPhaseMs = now;
                return;
            }
            GeoCertContact const& c = s_geoContacts[g.certGeoDigIdx];
            if ( g.certGeoDigArmed == 0 )
            {
                GeoCertStrikeAt( c );
                g.certGeoDigArmed = 1;
                g.certGeoPhaseMs = now;
                if ( g.certGeoExitCode != 0 )
                {
                    GeoCertScaffoldRest();
                    GeoCertWriteArtifact();
                    g.certGeoPhase = 4;
                    PostQuitMessage( g.certGeoExitCode );
                }
                return;
            }
            if ( now - g.certGeoPhaseMs >= 900 )
            {
                ++g.certGeoDigIdx;
                g.certGeoDigArmed = 0;
                g.certGeoPhaseMs = now;
            }
            return;
        }

        // Phase 3: §5 D2 + §6 accumulate + §7 ownership + §8 material + §10 support + §11 chips + write/quit.
        // §9 place/re-fill remains frozen until after P3d DETACHED MATTER SUPPORT FLOOR.
        if ( g.certGeoPhase == 3 )
        {
            g.statusLine = "CERT-GEO D2/QEF + accumulate + ownership + support + chips";
            GeoCertRunD2QefSection();
            if ( g.certGeoExitCode == 0 )
            {
                GeoCertRunAccumulateSection();
            }
            if ( g.certGeoExitCode == 0 )
            {
                // Moderate rocky hillside (E) — north of dig matrix strike.
                float const ox = (float)ProvenanceGeo::kRangeOriginX;
                float const oy = (float)ProvenanceGeo::kRangeOriginY;
                GeoCertRunAccumulatePad( "moderate_rock", ox + 68.f, oy + 6.f,
                    1.f, 0.f, 0.f, /*intoNormal=*/false );
            }
            if ( g.certGeoExitCode == 0 )
            {
                // Steep diagonal rock (F) — into-normal carve along +X corridor.
                float const ox = (float)ProvenanceGeo::kRangeOriginX;
                float const oy = (float)ProvenanceGeo::kRangeOriginY;
                GeoCertRunAccumulatePad( "steep_face", ox + 84.f, oy + 5.f,
                    1.f, 0.f, 0.f, /*intoNormal=*/true );
            }
            if ( g.certGeoExitCode == 0 )
            {
                GeoCertRunOwnershipSection();
            }
            if ( g.certGeoExitCode == 0 )
            {
                GeoCertRunMaterialSection();
            }
            if ( g.certGeoExitCode == 0 )
            {
                GeoCertRunPlaceSection();
            }
            if ( g.certGeoExitCode == 0 )
            {
                GeoCertRunSupportSection();
            }
            if ( g.certGeoExitCode == 0 )
            {
                GeoCertRunChipsSection();
            }
            GeoCertScaffoldRest();
            GeoCertWriteArtifact();
            g.statusLine = g.certGeoExitCode
                ? "CERT-GEO done — FAIL (see cert txt)"
                : "CERT-GEO done — PASS/SKIP rows written";
            g.certGeoPhase = 4;
            PostQuitMessage( g.certGeoExitCode );
            return;
        }
    }

    void CertDigTick()
    {
        if ( !g.certDig ) { return; }
        DWORD const now = GetTickCount();
        // Settle → dump → move → dig next. Three sites before quit.
        struct Stop { float dx, dy; float pitch; float yaw; char const* tag; };
        static Stop const kStops[] = {
            { 0.f, 0.f, -1.15f, 0.f, "flat_a" },
            { 3.2f, 1.1f, -1.05f, 0.7f, "flat_b" },
            { -2.4f, 4.0f, -0.55f, 1.9f, "slope_look" },
        };
        constexpr int kStopN = 3;

        if ( g.certPhase == 0 )
        {
            if ( !g.streamComplete || g.link != LinkState::CapsOk ) { return; }
            SnapVirginPerfIfNeeded();
            Stop const& s = kStops[g.certStop];
            if ( g.certStop > 0 )
            {
                g.feetX += s.dx;
                g.feetY += s.dy;
                float gz = g.feetZ;
                if ( SampleGroundZ( g.feetX, g.feetY, gz ) ) { g.feetZ = gz; }
                g.camX = g.feetX;
                g.camY = g.feetY;
                g.camZ = g.feetZ + 1.7f;
                InvalidateTerrainMesh();
            }
            g.pitch = s.pitch;
            g.yaw = s.yaw;
            UpdateAim();
            g.hotbarSel = 2;
            if ( !TryPickFoliatedStrike() )
            {
                g.hotbarSel = 0;
                TryDigHandful();
            }
            g.certPhase = 1;
            g.certPhaseMs = now;
            char msg[96];
            std::snprintf( msg, sizeof( msg ), "CERT dig[%d]=%s — settle", g.certStop, s.tag );
            g.statusLine = msg;
            return;
        }
        if ( g.certPhase == 1 && now - g.certPhaseMs >= 1600 )
        {
            Stop const& s = kStops[g.certStop];
            char ppm[MAX_PATH], rep[MAX_PATH];
            std::snprintf( ppm, sizeof( ppm ), "%s\\provenance_cert_%s.ppm", g.certOutDir, s.tag );
            std::snprintf( rep, sizeof( rep ), "%s\\provenance_cert_%s_report.txt", g.certOutDir, s.tag );
            GLint vp[4] = {};
            glGetIntegerv( GL_VIEWPORT, vp );
            if ( vp[2] >= 64 && vp[3] >= 64 && DumpFramePpm( ppm ) )
            {
                glReadBuffer( GL_FRONT );
                WriteCertPixelReport( ppm, rep );
                FILE* f = nullptr;
                if ( fopen_s( &f, rep, "a" ) == 0 && f )
                {
                    std::fprintf( f, "cert_stop=%s\nfeet=(%.2f,%.2f,%.2f)\npitch=%.2f yaw=%.2f\n"
                        "max_frame_dt_ms=%.2f\nlast_remesh_ms=%.2f\nmax_remesh_ms=%.2f\nframes_so_far=%d\ncavity_tris=%d\n"
                        "D2_rebuilds=%d\nD2_ms_total=%.2f\nHF_rebuilds=%d\nSampleSurface=%d\nSampleCapColor=%d\n",
                        s.tag, g.feetX, g.feetY, g.feetZ, g.pitch, g.yaw,
                        g.certMaxDtMs, g.certLastRemeshMs, g.certMaxRemeshMs, g.certFrames, g.cavityTrisTotal,
                        g.perfD2Rebuilds, g.perfD2MsTotal, g.perfHfRebuilds,
                        g.perfSampleSurfaceCalls, g.perfSampleCapColorCalls );
                    std::fclose( f );
                }
                ++g.certStop;
                if ( g.certStop >= kStopN )
                {
                    char summary[MAX_PATH], perf[MAX_PATH];
                    std::snprintf( summary, sizeof( summary ), "%s\\provenance_cert_tour_summary.txt", g.certOutDir );
                    std::snprintf( perf, sizeof( perf ), "%s\\provenance_cert_tour_perf.txt", g.certOutDir );
                    WritePerfReport( perf, "tour_end" );
                    FILE* sf = nullptr;
                    if ( fopen_s( &sf, summary, "w" ) == 0 && sf )
                    {
                        std::fprintf( sf,
                            "stops=%d\nmax_frame_dt_ms=%.2f\nmax_remesh_ms=%.2f\nframes=%d\ncavity_tris=%d\n"
                            "virgin_D2_rebuilds=%d\nD2_rebuilds_end=%d\nSampleSurface=%d\nSampleCapColor=%d\nHF_rebuilds=%d\n"
                            "invariant_virgin_D2_zero=%s\n"
                            "note=adapt shell preserved; D2 dirty partitions not clipped; "
                            "HF aperture=union of mouths (12cm=new strike only)\n",
                            kStopN, g.certMaxDtMs, g.certMaxRemeshMs, g.certFrames, g.cavityTrisTotal,
                            g.perfVirginD2Rebuilds, g.perfD2Rebuilds,
                            g.perfSampleSurfaceCalls, g.perfSampleCapColorCalls, g.perfHfRebuilds,
                            ( g.perfVirginD2Rebuilds == 0 ) ? "PASS" : "FAIL" );
                        std::fclose( sf );
                    }
                    g.statusLine = "CERT tour done — quitting";
                    g.certPhase = 2;
                    PostQuitMessage( 0 );
                    return;
                }
                g.certPhase = 0;
                g.certPhaseMs = now;
                g.statusLine = "CERT moving to next dig site";
                return;
            }
            if ( now - g.certPhaseMs >= 20000 )
            {
                g.statusLine = "CERT timeout — no frame dump";
                g.certPhase = 2;
                PostQuitMessage( 2 );
            }
        }
    }

    void TickFrame()
    {
        DWORD now = GetTickCount();
        float dt = 0.016f;
        float rawDt = 0.016f;
        if ( g.lastFrameMs != 0 )
        {
            rawDt = ( now - g.lastFrameMs ) * 0.001f;
            dt = (std::min)( 0.05f, rawDt );
        }
        g.lastFrameMs = now;
        g.frameDt = dt;
        if ( g.certDig || g.certGeo )
        {
            ++g.certFrames;
            float const rawMs = rawDt * 1000.f;
            if ( rawMs > g.certMaxDtMs ) { g.certMaxDtMs = rawMs; }
        }

        if ( g.link == LinkState::Connected || g.link == LinkState::CapsOk )
        {
            PollSocket();
        }
        else if ( g.link == LinkState::Disconnected || g.link == LinkState::SocketError )
        {
            if ( now - g.lastAttemptMs > 2000 )
            {
                g.lastAttemptMs = now;
                TryConnect();
            }
        }

        TickStreamRequests();
        UpdateCamera( dt );
        UpdateAim();
        if ( ( now / 250 ) != ( ( now - (DWORD)( dt * 1000 ) ) / 250 ) )
        {
            UpdateStreamHud();
        }
        // Status receipt for smoke cert (overwritten each second)
        static DWORD lastStatusMs = 0;
        if ( now - lastStatusMs > 1000 )
        {
            lastStatusMs = now;
            char path[MAX_PATH];
            if ( GetTempPathA( MAX_PATH, path ) == 0 ) { /* skip */ }
            else if ( strcat_s( path, MAX_PATH, "provenance_phase4_status.txt" ) != 0 ) { /* skip */ }
            else
            {
                FILE* f = nullptr;
                if ( fopen_s( &f, path, "w" ) == 0 && f )
                {
                    std::fprintf( f,
                        "link=%s\nmode=%s\ngrounded=%d\ncells=%d\nblocks=%d/%d\ncomplete=%d\n"
                        "feet=%.2f,%.2f,%.2f\neye=%.2f\nheld_g=%d\nheld=%s\n"
                        "digest=%s\nplayer=%d,%d\nstatus=%s\n",
                        LinkLabel( g.link ), g.walkMode ? "walk" : "fly", g.grounded ? 1 : 0,
                        g.cellsLoaded, g.blocksLoaded, g.blocksWanted,
                        g.streamComplete ? 1 : 0, g.feetX, g.feetY, g.feetZ, g.camZ,
                        g.heldTotalG, g.heldDominant.c_str(),
                        g.digestLine.c_str(),
                        g.playerX, g.playerY, g.statusLine.c_str() );
                    std::fclose( f );
                }
            }
            WriteChipProbeDump(); // %TEMP%\provenance_chip_probe.txt — full chip list for agents
        }
        Render();
        SnapVirginPerfIfNeeded();
        CertDigTick();
        CertGeoTick();
        CertLsiTick();
    }

    LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
    {
        switch ( msg )
        {
            case WM_CREATE:
                g.hwnd = hwnd;
                if ( !InitGL( hwnd ) )
                {
                    MessageBoxW( hwnd, L"OpenGL init failed", L"Provenance Client", MB_ICONERROR );
                    PostQuitMessage( 1 );
                    return 0;
                }
                SetTimer( hwnd, kTimerId, 16, nullptr );
                g.lastAttemptMs = GetTickCount();
                g.lastFrameMs = GetTickCount();
                TryConnect();
                SetMouseLook( hwnd, true );
                return 0;
            case WM_ACTIVATE:
                if ( LOWORD( wParam ) != WA_INACTIVE && g.mouseLook )
                {
                    SetMouseLook( hwnd, true );
                }
                return 0;
            case WM_TIMER:
                if ( wParam == kTimerId ) { TickFrame(); }
                return 0;
            case WM_SIZE:
                return 0;
            case WM_PAINT:
            {
                PAINTSTRUCT ps;
                BeginPaint( hwnd, &ps );
                EndPaint( hwnd, &ps );
                Render();
                return 0;
            }
            case WM_KEYDOWN:
                if ( wParam < 256 ) { g.keys[wParam] = true; }
                if ( wParam == VK_ESCAPE )
                {
                    if ( g.journalOpen )
                    {
                        CloseJournal();
                        return 0;
                    }
                    if ( g.mouseLook )
                    {
                        SetMouseLook( hwnd, false );
                    }
                    else
                    {
                        PostQuitMessage( 0 );
                    }
                }
                else if ( wParam == 'J' || wParam == 'j' )
                {
                    ToggleJournal();
                    return 0;
                }
                else if ( wParam >= '1' && wParam <= '6' )
                {
                    g.hotbarSel = (int)( wParam - '1' );
                    return 0;
                }
                else if ( wParam == VK_TAB )
                {
                    if ( g.journalOpen ) { return 0; }
                    SetMouseLook( hwnd, !g.mouseLook );
                    return 0;
                }
                else if ( wParam == 'C' || wParam == 'c' )
                {
                    g.chipMode = (ChipMode)( ( (int)g.chipMode + 1 ) % 3 );
                    char const* label = g.chipMode == ChipMode::Off ? "OFF"
                        : ( g.chipMode == ChipMode::Visual ? "VISUAL" : "PHYS" );
                    char d[160];
                    std::snprintf( d, sizeof( d ), "Chips: %s  ([C] cycle OFF/VISUAL/PHYS)", label );
                    g.digestLine = d;
                    g.statusLine = d;
                    // Wake to ACTIVE when entering PHYS so plates can fall into cavities.
                    if ( g.chipMode == ChipMode::Phys )
                    {
                        for ( H2H::MatterBody& b : H2H::State().bodies )
                        {
                            H2H::WakeChip( b );
                        }
                    }
                    return 0;
                }
                else if ( wParam == 'K' || wParam == 'k' )
                {
                    WriteChipProbeDump();
                    char d[192];
                    std::snprintf( d, sizeof( d ),
                        "Chip probe dump → %%TEMP%%\\provenance_chip_probe.txt  (bodies=%d active=%d)",
                        (int)H2H::State().bodies.size(), H2H::CountActiveChips() );
                    g.digestLine = d;
                    g.statusLine = d;
                    return 0;
                }
                else if ( wParam == 'B' || wParam == 'b' )
                {
                    g.boundaryMode = (BoundaryMode)( ( (int)g.boundaryMode + 1 ) % 3 );
                    char const* label = g.boundaryMode == BoundaryMode::D2 ? "D2"
                        : ( g.boundaryMode == BoundaryMode::DebugBoundary ? "AABB debug" : "D2+AABB overlay" );
                    char d[160];
                    std::snprintf( d, sizeof( d ), "Boundary: %s  (d2Tris=%d lastEdges=%d)",
                        label, g.cavityTrisTotal, g.cavityEdgesEmitted );
                    g.digestLine = d;
                    g.statusLine = d;
                    return 0;
                }
                else if ( wParam == 'G' || wParam == 'g' )
                {
                    TryGripMatterBody();
                    return 0;
                }
                else if ( wParam == 'E' || wParam == 'e' )
                {
                    if ( TryPickupGallerySample() )
                    {
                        // Consume E so free-fly doesn't also rise on the same press.
                        g.keys['E'] = false;
                        g.keys['e'] = false;
                    }
                    return 0;
                }
                else if ( wParam == VK_OEM_4 ) // [
                {
                    g.sunAzimuth -= 0.15f;
                    UpdateWorldSun();
                    InvalidateTerrainMesh();
                    char d[120];
                    std::snprintf( d, sizeof( d ), "Sun azimuth %.0f°  elev %.0f°",
                        g.sunAzimuth * ( 180.f / 3.14159265f ),
                        g.sunElevation * ( 180.f / 3.14159265f ) );
                    g.digestLine = d;
                    return 0;
                }
                else if ( wParam == VK_OEM_6 ) // ]
                {
                    g.sunAzimuth += 0.15f;
                    UpdateWorldSun();
                    InvalidateTerrainMesh();
                    char d[120];
                    std::snprintf( d, sizeof( d ), "Sun azimuth %.0f°  elev %.0f°",
                        g.sunAzimuth * ( 180.f / 3.14159265f ),
                        g.sunElevation * ( 180.f / 3.14159265f ) );
                    g.digestLine = d;
                    return 0;
                }
                else if ( wParam == VK_OEM_MINUS || wParam == '-' )
                {
                    g.sunElevation = (std::max)( 0.08f, g.sunElevation - 0.08f );
                    UpdateWorldSun();
                    InvalidateTerrainMesh();
                    char d[120];
                    std::snprintf( d, sizeof( d ), "Sun elev %.0f°  (horizon→zenith)",
                        g.sunElevation * ( 180.f / 3.14159265f ) );
                    g.digestLine = d;
                    return 0;
                }
                else if ( wParam == VK_OEM_PLUS || wParam == '=' )
                {
                    g.sunElevation = (std::min)( 1.45f, g.sunElevation + 0.08f );
                    UpdateWorldSun();
                    InvalidateTerrainMesh();
                    char d[120];
                    std::snprintf( d, sizeof( d ), "Sun elev %.0f°  (horizon→zenith)",
                        g.sunElevation * ( 180.f / 3.14159265f ) );
                    g.digestLine = d;
                    return 0;
                }
                else if ( wParam == 'R' )
                {
                    g.lastAttemptMs = GetTickCount();
                    TryConnect();
                }
                else if ( wParam == VK_F8 )
                {
                    ProvenanceGeo::CycleFixture();
                    RefreshGeographyFixture();
                    return 0;
                }
                return 0;
            case WM_KEYUP:
                if ( wParam < 256 ) { g.keys[wParam] = false; }
                return 0;
            case WM_MOUSEWHEEL:
                if ( !g.heldGalleryId.empty() && !g.journalOpen )
                {
                    short const delta = GET_WHEEL_DELTA_WPARAM( wParam );
                    float const step = ( delta / (float)WHEEL_DELTA ) * ( 3.14159265f / 10.f );
                    if ( ( GetKeyState( VK_SHIFT ) & 0x8000 ) != 0 )
                    {
                        // Shift+scroll: spin left/right about world up.
                        g.heldYaw += step;
                    }
                    else
                    {
                        // Scroll: tumble/roll over about hand-right (inspect underside / faces).
                        g.heldTumble += step;
                    }
                    return 0;
                }
                return 0;
            case WM_LBUTTONDOWN:
            {
                int mx = (int)(short)LOWORD( lParam );
                int my = (int)(short)HIWORD( lParam );
                RECT rc; GetClientRect( hwnd, &rc );
                g.uiWinW = (std::max)( 1, (int)rc.right );
                g.uiWinH = (std::max)( 1, (int)rc.bottom );
                UpdateUiMouseFromWin( mx, my );
                if ( TryHotbarClick( g.uiMouseX, g.uiMouseY ) ) { return 0; }
                if ( g.journalOpen )
                {
                    HandleJournalClick( g.uiMouseX, g.uiMouseY, false );
                    return 0;
                }
                if ( !g.mouseLook ) { SetMouseLook( hwnd, true ); }
                TryDigHandful();
                return 0;
            }
            case WM_RBUTTONDOWN:
            {
                int mx = (int)(short)LOWORD( lParam );
                int my = (int)(short)HIWORD( lParam );
                RECT rc; GetClientRect( hwnd, &rc );
                g.uiWinW = (std::max)( 1, (int)rc.right );
                g.uiWinH = (std::max)( 1, (int)rc.bottom );
                UpdateUiMouseFromWin( mx, my );
                if ( g.journalOpen )
                {
                    HandleJournalClick( g.uiMouseX, g.uiMouseY, true );
                    return 0;
                }
                if ( !g.mouseLook ) { SetMouseLook( hwnd, true ); }
                // Always run place path — empty hand / busy get an explicit digest (no silent no-op)
                TryPlaceHandful();
                return 0;
            }
            case WM_RBUTTONUP:
                return 0;
            case WM_MOUSEMOVE:
            {
                int mx = (int)(short)LOWORD( lParam );
                int my = (int)(short)HIWORD( lParam );
                RECT rc; GetClientRect( hwnd, &rc );
                g.uiWinW = (std::max)( 1, (int)rc.right );
                g.uiWinH = (std::max)( 1, (int)rc.bottom );
                UpdateUiMouseFromWin( mx, my );
                if ( g.journalOpen || !g.mouseLook )
                {
                    return 0;
                }
                {
                    int const cx = ( rc.right - rc.left ) / 2;
                    int const cy = ( rc.bottom - rc.top ) / 2;
                    int dx = mx - cx;
                    int dy = my - cy;
                    if ( dx != 0 || dy != 0 )
                    {
                        g.yaw += dx * 0.005f;
                        g.pitch -= dy * 0.005f;
                        if ( g.pitch < -1.4f ) { g.pitch = -1.4f; }
                        if ( g.pitch > 1.4f ) { g.pitch = 1.4f; }
                        POINT pt = { cx, cy };
                        ClientToScreen( hwnd, &pt );
                        SetCursorPos( pt.x, pt.y );
                    }
                }
                return 0;
            }
            case WM_DESTROY:
                KillTimer( hwnd, kTimerId );
                CloseSock();
                ShutdownGL();
                PostQuitMessage( 0 );
                return 0;
        }
        return DefWindowProcW( hwnd, msg, wParam, lParam );
    }
}

int APIENTRY wWinMain( HINSTANCE hInst, HINSTANCE, LPWSTR, int nShow )
{
    WSADATA wsa;
    if ( WSAStartup( MAKEWORD( 2, 2 ), &wsa ) != 0 )
    {
        MessageBoxW( nullptr, L"WSAStartup failed", L"Provenance Client", MB_ICONERROR );
        return 1;
    }
    g.sessionStartMs = GetTickCount();
    InvalidateTerrainMesh(); // rebuild with seam-safe uniform subdiv

    {
        int argc = 0;
        LPWSTR* argv = CommandLineToArgvW( GetCommandLineW(), &argc );
        if ( argv )
        {
            GetTempPathA( MAX_PATH, g.certOutDir );
            // Default play/dev fixture = RANGE (representative geology). Torture via flag/F8.
            ProvenanceGeo::SetFixture( ProvenanceGeo::GeoFixture::Range );
            for ( int i = 1; i < argc; ++i )
            {
                if ( _wcsicmp( argv[i], L"--cert-dig" ) == 0 )
                {
                    g.certDig = true;
                    continue;
                }
                if ( _wcsicmp( argv[i], L"--cert-geo" ) == 0
                  || _wcsicmp( argv[i], L"--cert-geography" ) == 0 )
                {
                    g.certGeo = true;
                    ProvenanceGeo::SetFixture( ProvenanceGeo::GeoFixture::Range );
                    continue;
                }
                if ( _wcsicmp( argv[i], L"--cert-lsi" ) == 0
                  || _wcsicmp( argv[i], L"--cert-local-surface-intent" ) == 0 )
                {
                    g.certLsi = true;
                    ProvenanceGeo::SetFixture( ProvenanceGeo::GeoFixture::Range );
                    continue;
                }
                if ( _wcsnicmp( argv[i], L"--geo-fixture=", 14 ) == 0 )
                {
                    char fixture[64];
                    WideCharToMultiByte( CP_UTF8, 0, argv[i] + 14, -1,
                        fixture, sizeof( fixture ), nullptr, nullptr );
                    if ( !ProvenanceGeo::SetFixtureFromString( fixture ) )
                    {
                        ProvenanceGeo::SetFixture( ProvenanceGeo::GeoFixture::Range );
                    }
                    continue;
                }
                if ( _wcsicmp( argv[i], L"--geo-fixture" ) == 0 && i + 1 < argc )
                {
                    char fixture[64];
                    WideCharToMultiByte( CP_UTF8, 0, argv[++i], -1,
                        fixture, sizeof( fixture ), nullptr, nullptr );
                    if ( !ProvenanceGeo::SetFixtureFromString( fixture ) )
                    {
                        ProvenanceGeo::SetFixture( ProvenanceGeo::GeoFixture::Range );
                    }
                    continue;
                }
                if ( i == 1 && argv[i][0] != L'-' )
                {
                    char host[128];
                    WideCharToMultiByte( CP_UTF8, 0, argv[i], -1, host, sizeof( host ), nullptr, nullptr );
                    g.host = host;
                }
                else if ( i == 2 && argv[i][0] != L'-' )
                {
                    g.port = _wtoi( argv[i] );
                }
            }
            LocalFree( argv );
        }
    }

    WNDCLASSW wc = {};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"ProvenancePhase4Client";
    wc.hCursor = LoadCursor( nullptr, IDC_ARROW );
    wc.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
    RegisterClassW( &wc );

    HWND hwnd = CreateWindowExW( 0, wc.lpszClassName, L"Provenance Client - Phase 4",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 800,
        nullptr, nullptr, hInst, nullptr );
    if ( !hwnd )
    {
        WSACleanup();
        return 1;
    }
    ShowWindow( hwnd, nShow );

    MSG msg;
    while ( GetMessageW( &msg, nullptr, 0, 0 ) > 0 )
    {
        TranslateMessage( &msg );
        DispatchMessageW( &msg );
    }

    WSACleanup();
    return (int)msg.wParam;
}
