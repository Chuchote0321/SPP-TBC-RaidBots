#pragma once

#include "Common.h"

namespace ai
{
    struct MaulgarFixedAnchor
    {
        float x;
        float y;
        float z;
        float o;
        bool configured;

        MaulgarFixedAnchor()
            : x(0.0f), y(0.0f), z(0.0f), o(0.0f), configured(false) {}

        MaulgarFixedAnchor(float px, float py, float pz, float po, bool enabled)
            : x(px), y(py), z(pz), o(po), configured(enabled) {}
    };

    // ---------------------------------------------------------------------
    // POSITION DATA POLICY
    //
    // Fixed anchors are populated from a Warcraft Logs cohort, not by manually
    // parking nine local characters. WCL replay positions are normalized into
    // a canonical Maulgar room frame, aggregated robustly (median / MAD), then
    // transformed to the local map frame using the council spawn geometry.
    //
    // Values remain disabled until the WCL cohort extraction is complete.
    // See docs/tactics/gruuls_lair/high_king_maulgar/WCL_POSITION_RESEARCH.md.
    // ---------------------------------------------------------------------
    class MaulgarFixedPositions
    {
    public:
        static MaulgarFixedAnchor MaulgarTank()
        {
            return MaulgarFixedAnchor(0.0f, 0.0f, 0.0f, 0.0f, false);
        }

        static MaulgarFixedAnchor BlindeyeTank()
        {
            return MaulgarFixedAnchor(0.0f, 0.0f, 0.0f, 0.0f, false);
        }

        static MaulgarFixedAnchor KigglerTank()
        {
            return MaulgarFixedAnchor(0.0f, 0.0f, 0.0f, 0.0f, false);
        }

        static MaulgarFixedAnchor KroshMageTank()
        {
            return MaulgarFixedAnchor(0.0f, 0.0f, 0.0f, 0.0f, false);
        }

        static MaulgarFixedAnchor OlmWarlockTank()
        {
            return MaulgarFixedAnchor(0.0f, 0.0f, 0.0f, 0.0f, false);
        }

        static MaulgarFixedAnchor FelhunterPaladinStandby()
        {
            return MaulgarFixedAnchor(0.0f, 0.0f, 0.0f, 0.0f, false);
        }

        // Three pull lanes. These are only pre-pull Hunter waiting positions.
        static MaulgarFixedAnchor HunterToMaulgar()
        {
            return MaulgarFixedAnchor(0.0f, 0.0f, 0.0f, 0.0f, false);
        }

        static MaulgarFixedAnchor HunterToBlindeye()
        {
            return MaulgarFixedAnchor(0.0f, 0.0f, 0.0f, 0.0f, false);
        }

        static MaulgarFixedAnchor HunterToKiggler()
        {
            return MaulgarFixedAnchor(0.0f, 0.0f, 0.0f, 0.0f, false);
        }

        static bool PullAnchorsConfigured()
        {
            return
                MaulgarTank().configured &&
                BlindeyeTank().configured &&
                KigglerTank().configured &&
                KroshMageTank().configured &&
                OlmWarlockTank().configured &&
                FelhunterPaladinStandby().configured &&
                HunterToMaulgar().configured &&
                HunterToBlindeye().configured &&
                HunterToKiggler().configured;
        }
    };
}
