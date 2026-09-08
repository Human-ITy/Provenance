#include "WorldSystem_Provenance.h"

namespace EE
{
    //-------------------------------------------------------------------------
    // Geological ancestry used by the certification fixtures.
    //
    // File-local duplicate of the canonical Granite ancestry ID. It has
    // internal linkage and therefore does not create cross-TU ownership.
    //-------------------------------------------------------------------------

    static constexpr uint32_t s_graniteGeologicalAncestryID =
        7001u;

    //-------------------------------------------------------------------------
    // Provenance helper
    //-------------------------------------------------------------------------

    static void InitializeSingleContribution(
        ProvenanceWorldSystem::MatterBody& body )
    {
        body.m_numContributors =
            1;

        body.m_contributors[0].m_sourceBodyID =
            body.m_bodyID;

        body.m_contributors[0].m_provenanceID =
            body.m_provenanceID;

        body.m_contributors[0].m_massGrams =
            body.m_massGrams;
    }

    //-------------------------------------------------------------------------
    // Certification workbench
    //-------------------------------------------------------------------------

    void ProvenanceWorldSystem::InitializeMatterBodies()
    {
        uint32_t constexpr testMassGrams =
            12000;

        float constexpr workbenchY =
            12.5f;

        float constexpr baseZ =
            0.08f;

        //-------------------------------------------------------------------------
        // B1 — Granite Fragment
        //-------------------------------------------------------------------------

        m_matterBodies[0].m_bodyID =
            1;

        m_matterBodies[0].m_parentBodyID =
            0;

        m_matterBodies[0].m_provenanceID =
            1001;

        m_matterBodies[0].m_geologicalAncestryID =
            s_graniteGeologicalAncestryID;

        m_matterBodies[0].m_material =
            ProvenanceMaterialID::Granite;

        m_matterBodies[0].m_bodyState =
            ProvenanceBodyState::Fragment;

        m_matterBodies[0].m_massGrams =
            testMassGrams;

        m_matterBodies[0].m_centerX =
            11.5f;

        m_matterBodies[0].m_centerY =
            workbenchY;

        m_matterBodies[0].m_baseZ =
            baseZ;

        m_matterBodies[0].m_geometrySalt =
            0x11111111u;

        InitializeSingleContribution(
            m_matterBodies[0] );

        //-------------------------------------------------------------------------
        // B2 — Granite Loose Aggregate
        //-------------------------------------------------------------------------

        m_matterBodies[1].m_bodyID =
            2;

        m_matterBodies[1].m_parentBodyID =
            0;

        m_matterBodies[1].m_provenanceID =
            1002;

        m_matterBodies[1].m_geologicalAncestryID =
            s_graniteGeologicalAncestryID;

        m_matterBodies[1].m_material =
            ProvenanceMaterialID::Granite;

        m_matterBodies[1].m_bodyState =
            ProvenanceBodyState::LooseAggregate;

        m_matterBodies[1].m_massGrams =
            testMassGrams;

        m_matterBodies[1].m_centerX =
            7.0f;

        m_matterBodies[1].m_centerY =
            workbenchY;

        m_matterBodies[1].m_baseZ =
            baseZ;

        m_matterBodies[1].m_geometrySalt =
            0x22000000u;

        InitializeSingleContribution(
            m_matterBodies[1] );

        //-------------------------------------------------------------------------
        // B3 — Granite Bonded Body
        //-------------------------------------------------------------------------

        m_matterBodies[2].m_bodyID =
            3;

        m_matterBodies[2].m_parentBodyID =
            0;

        m_matterBodies[2].m_provenanceID =
            1003;

        m_matterBodies[2].m_geologicalAncestryID =
            s_graniteGeologicalAncestryID;

        m_matterBodies[2].m_material =
            ProvenanceMaterialID::Granite;

        m_matterBodies[2].m_bodyState =
            ProvenanceBodyState::Bonded;

        m_matterBodies[2].m_massGrams =
            testMassGrams;

        m_matterBodies[2].m_centerX =
            2.5f;

        m_matterBodies[2].m_centerY =
            workbenchY;

        m_matterBodies[2].m_baseZ =
            baseZ;

        m_matterBodies[2].m_geometrySalt =
            0x31000001u;

        m_matterBodies[2].m_numContributors =
            3;

        m_matterBodies[2].m_contributors[0].m_sourceBodyID =
            31;

        m_matterBodies[2].m_contributors[0].m_provenanceID =
            2031;

        m_matterBodies[2].m_contributors[0].m_massGrams =
            4000;

        m_matterBodies[2].m_contributors[1].m_sourceBodyID =
            32;

        m_matterBodies[2].m_contributors[1].m_provenanceID =
            2032;

        m_matterBodies[2].m_contributors[1].m_massGrams =
            5000;

        m_matterBodies[2].m_contributors[2].m_sourceBodyID =
            33;

        m_matterBodies[2].m_contributors[2].m_provenanceID =
            2033;

        m_matterBodies[2].m_contributors[2].m_massGrams =
            3000;

        uint32_t bondedContributionMass =
            0;

        for ( int32_t i = 0;
              i <
              m_matterBodies[2].m_numContributors;
              ++i )
        {
            bondedContributionMass +=
                m_matterBodies[2].m_contributors[i].m_massGrams;
        }

        EE_ASSERT(
            bondedContributionMass ==
            m_matterBodies[2].m_massGrams );

        //-------------------------------------------------------------------------
        // B4 — Soil Clod
        //-------------------------------------------------------------------------

        m_matterBodies[3].m_bodyID =
            4;

        m_matterBodies[3].m_parentBodyID =
            0;

        m_matterBodies[3].m_provenanceID =
            1004;

        m_matterBodies[3].m_geologicalAncestryID =
            0;

        m_matterBodies[3].m_material =
            ProvenanceMaterialID::Soil;

        m_matterBodies[3].m_bodyState =
            ProvenanceBodyState::Fragment;

        m_matterBodies[3].m_massGrams =
            testMassGrams;

        m_matterBodies[3].m_centerX =
            -2.5f;

        m_matterBodies[3].m_centerY =
            workbenchY;

        m_matterBodies[3].m_baseZ =
            baseZ;

        m_matterBodies[3].m_geometrySalt =
            0x41000001u;

        InitializeSingleContribution(
            m_matterBodies[3] );

        //-------------------------------------------------------------------------
        // B5 — Soil Loose
        //-------------------------------------------------------------------------

        m_matterBodies[4].m_bodyID =
            5;

        m_matterBodies[4].m_parentBodyID =
            0;

        m_matterBodies[4].m_provenanceID =
            1005;

        m_matterBodies[4].m_geologicalAncestryID =
            0;

        m_matterBodies[4].m_material =
            ProvenanceMaterialID::Soil;

        m_matterBodies[4].m_bodyState =
            ProvenanceBodyState::LooseAggregate;

        m_matterBodies[4].m_massGrams =
            testMassGrams;

        m_matterBodies[4].m_centerX =
            -7.0f;

        m_matterBodies[4].m_centerY =
            workbenchY;

        m_matterBodies[4].m_baseZ =
            baseZ;

        m_matterBodies[4].m_geometrySalt =
            0x42000001u;

        InitializeSingleContribution(
            m_matterBodies[4] );

        //-------------------------------------------------------------------------
        // B6 — Soil Compacted
        //-------------------------------------------------------------------------

        m_matterBodies[5].m_bodyID =
            6;

        m_matterBodies[5].m_parentBodyID =
            0;

        m_matterBodies[5].m_provenanceID =
            1006;

        m_matterBodies[5].m_geologicalAncestryID =
            0;

        m_matterBodies[5].m_material =
            ProvenanceMaterialID::Soil;

        m_matterBodies[5].m_bodyState =
            ProvenanceBodyState::Compacted;

        m_matterBodies[5].m_massGrams =
            testMassGrams;

        m_matterBodies[5].m_centerX =
            -11.5f;

        m_matterBodies[5].m_centerY =
            workbenchY;

        m_matterBodies[5].m_baseZ =
            baseZ;

        m_matterBodies[5].m_geometrySalt =
            0x43000001u;

        InitializeSingleContribution(
            m_matterBodies[5] );

        m_haveMatterBodies =
            true;
    }

}
