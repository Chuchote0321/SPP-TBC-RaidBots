#include "botpch.h"
#include "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/MaulgarFormationManager.h"
#include "playerbot/strategy/raid/gruuls_lair/high_king_maulgar/MaulgarFixedPositions.h"
#include "playerbot/PlayerbotAI.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

using namespace ai;

namespace
{
    constexpr float FORMATION_EPS = 0.001f;

    constexpr float TANK_MOVE_TOLERANCE       = 2.0f;
    constexpr float RANGED_MOVE_TOLERANCE     = 2.5f;
    constexpr float HEALER_MOVE_TOLERANCE     = 3.0f;
    constexpr float MELEE_MOVE_TOLERANCE      = 1.75f;

    constexpr float MAULGAR_PULL_DISTANCE     = 22.0f;
    constexpr float BLINDEYE_PULL_DISTANCE    = 17.0f;
    constexpr float OLM_PULL_DISTANCE         = 17.0f;
    constexpr float COUNCIL_LATERAL_OFFSET    = 7.0f;

    // Krosh Blast Wave makes melee proximity undesirable. This range keeps the
    // Mage Tank outside the dangerous close area while still inside 30 yd
    // Spellsteal range with tolerance for movement.
    constexpr float KROSH_MAGE_RANGE          = 26.0f;

    // Kiggler is a ranged-tank assignment; stay close to max practical range.
    constexpr float KIGGLER_TANK_RANGE        = 27.0f;

    constexpr float OLM_WARLOCK_RANGE         = 18.0f;

    constexpr float HEALER_BACKLINE_DISTANCE  = 13.0f;
    constexpr float RANGED_TARGET_RANGE       = 24.0f;
    constexpr float MELEE_REAR_RANGE          = 3.0f;

    constexpr float WHIRLWIND_SAFE_RANGE      = 18.0f;
    constexpr float WHIRLWIND_ESCAPE_RANGE    = 21.0f;

    constexpr uint32 FORMATION_MOVE_ID_BASE   = 0x4D415500; // "MAU\0"

    struct Vec2
    {
        float x;
        float y;

        Vec2() : x(0.0f), y(0.0f) {}
        Vec2(float px, float py) : x(px), y(py) {}

        Vec2 operator+(Vec2 const& rhs) const { return Vec2(x + rhs.x, y + rhs.y); }
        Vec2 operator-(Vec2 const& rhs) const { return Vec2(x - rhs.x, y - rhs.y); }
        Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
    };

    struct Slot
    {
        float x;
        float y;
        float z;

        Slot() : x(0.0f), y(0.0f), z(0.0f) {}
        Slot(float px, float py, float pz) : x(px), y(py), z(pz) {}
    };

    struct MaulgarFormationState
    {
        bool initialized;

        Vec2 councilCenter;
        Vec2 entranceDir;
        Vec2 sideDir;

        Slot maulgarAnchor;
        Slot blindeyeAnchor;
        Slot olmAnchor;
        Slot felStandbyAnchor;

        Vec2 kroshInitial;
        Vec2 kigglerInitial;

        MaulgarFormationState() : initialized(false) {}
    };

    std::map<Map*, MaulgarFormationState> s_maulgarFormation;

    float Length(Vec2 const& v)
    {
        return std::sqrt(v.x * v.x + v.y * v.y);
    }

    Vec2 Normalize(Vec2 v)
    {
        const float len = Length(v);
        if (len < FORMATION_EPS)
            return Vec2(1.0f, 0.0f);

        return Vec2(v.x / len, v.y / len);
    }

    Vec2 Perpendicular(Vec2 const& v)
    {
        return Vec2(-v.y, v.x);
    }

    Vec2 Position2(WorldObject const* obj)
    {
        return obj ? Vec2(obj->GetPositionX(), obj->GetPositionY()) : Vec2();
    }

    Slot MakeSlot(Vec2 const& p, float z)
    {
        return Slot(p.x, p.y, z);
    }

    MaulgarFormationState* State(PlayerbotAI* ai)
    {
        if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
            return nullptr;

        auto itr = s_maulgarFormation.find(ai->GetBot()->GetMap());
        if (itr == s_maulgarFormation.end() || !itr->second.initialized)
            return nullptr;

        return &itr->second;
    }

    bool ProtectedHumanGuid(uint32 guid)
    {
        return guid >= 4501 && guid <= 4509;
    }

    bool MoveToSlot(
        PlayerbotAI* ai,
        Slot const& slot,
        float tolerance,
        uint32 moveIdOffset)
    {
        if (!ai || !ai->GetBot())
            return false;

        Player* bot = ai->GetBot();
        const uint32 lowGuid = bot->GetObjectGuid().GetCounter();

        // Never issue server-side formation movement to protected human actors.
        if (ProtectedHumanGuid(lowGuid))
            return false;

        if (bot->IsWithinDist3d(slot.x, slot.y, slot.z, tolerance))
            return false;

        bot->GetMotionMaster()->MovePoint(
            FORMATION_MOVE_ID_BASE + moveIdOffset,
            slot.x,
            slot.y,
            slot.z,
            FORCED_MOVEMENT_RUN,
            true);

        return true;
    }

    // Stable ordinal derived from the current group. No permanent Raid1/2/3
    // position table is needed, and every present bot gets a deterministic slot.
    uint32 GroupOrdinal(PlayerbotAI* ai)
    {
        if (!ai || !ai->GetBot())
            return 0;

        std::vector<uint32> guids;
        const std::vector<Player*> players = ai->GetPlayersInGroup();

        for (Player* player : players)
        {
            if (!player || !player->IsAlive())
                continue;

            const uint32 low = player->GetObjectGuid().GetCounter();
            if (ProtectedHumanGuid(low))
                continue;

            guids.push_back(low);
        }

        std::sort(guids.begin(), guids.end());
        guids.erase(std::unique(guids.begin(), guids.end()), guids.end());

        const uint32 self = ai->GetBot()->GetObjectGuid().GetCounter();
        auto itr = std::find(guids.begin(), guids.end(), self);
        return itr == guids.end() ? 0 : uint32(std::distance(guids.begin(), itr));
    }

    Vec2 DirectionFromCouncilTo(
        MaulgarFormationState const& state,
        WorldObject const* target)
    {
        if (!target)
            return state.entranceDir;

        Vec2 dir = Position2(target) - state.councilCenter;
        if (Length(dir) < 1.0f)
            return state.entranceDir;

        return Normalize(dir);
    }

    Slot FixedSlot(MaulgarFixedAnchor const& anchor)
    {
        return Slot(anchor.x, anchor.y, anchor.z);
    }

    Slot RelativeTargetSlot(
        MaulgarFormationState const& state,
        WorldObject const* target,
        float range,
        float lateral)
    {
        if (!target)
            return Slot();

        // Prefer the raid/entrance side of each target, with a small per-player
        // lateral spread. This keeps ranged/healers from stacking on one pixel.
        Vec2 towardEntrance = Normalize(
            (state.councilCenter + state.entranceDir * 28.0f) - Position2(target));

        Vec2 side = Perpendicular(towardEntrance);
        Vec2 p = Position2(target) +
                 towardEntrance * range +
                 side * lateral;

        return MakeSlot(p, target->GetPositionZ());
    }
}

void MaulgarFormationManager::Reset(PlayerbotAI* ai)
{
    if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap())
        return;

    s_maulgarFormation.erase(ai->GetBot()->GetMap());
}

bool MaulgarFormationManager::EnsureMaulgarFrame(
    PlayerbotAI* ai,
    Creature* maulgar,
    Creature* krosh,
    Creature* olm,
    Creature* kiggler,
    Creature* blindeye)
{
    if (!ai || !ai->GetBot() || !ai->GetBot()->GetMap() ||
        !maulgar || !krosh || !olm || !kiggler || !blindeye)
    {
        return false;
    }

    Map* map = ai->GetBot()->GetMap();
    MaulgarFormationState& state = s_maulgarFormation[map];

    if (state.initialized)
        return true;

    const WorldObject* council[5] =
    {
        maulgar, krosh, olm, kiggler, blindeye
    };

    Vec2 center;
    float zAverage = 0.0f;

    for (WorldObject const* obj : council)
    {
        center = center + Position2(obj);
        zAverage += obj->GetPositionZ();
    }

    center = center * (1.0f / 5.0f);
    zAverage /= 5.0f;

    // The raid centroid at the instant of pull is a robust entrance-direction
    // cue and avoids hard-coding one DB's absolute Gruul's Lair coordinates.
    Vec2 raidCenter;
    uint32 raidCount = 0;

    const std::vector<Player*> players = ai->GetPlayersInGroup();
    for (Player* player : players)
    {
        if (!player || !player->IsAlive() || player->GetMap() != map)
            continue;

        raidCenter = raidCenter + Position2(player);
        ++raidCount;
    }

    if (raidCount > 0)
        raidCenter = raidCenter * (1.0f / float(raidCount));
    else
        raidCenter = Position2(ai->GetBot());

    Vec2 entranceDir = Normalize(raidCenter - center);

    // Fallback if the raid centroid is accidentally almost on top of the
    // council center: use the Maulgar-to-center radial direction.
    if (Length(raidCenter - center) < 2.0f)
        entranceDir = Normalize(Position2(maulgar) - center);

    state.councilCenter = center;
    state.entranceDir = entranceDir;
    state.sideDir = Perpendicular(entranceDir);

    // Source-consistent dynamic geometry:
    // - Maulgar toward the entrance side and off-axis from the raid;
    // - Blindeye/Olm together on the opposite side;
    // - Krosh/Kiggler remain close to their original caster positions.
    state.maulgarAnchor = MakeSlot(
        center +
            entranceDir * MAULGAR_PULL_DISTANCE +
            state.sideDir * 10.0f,
        maulgar->GetPositionZ());

    state.blindeyeAnchor = MakeSlot(
        center -
            entranceDir * BLINDEYE_PULL_DISTANCE +
            state.sideDir * COUNCIL_LATERAL_OFFSET,
        blindeye->GetPositionZ());

    state.olmAnchor = MakeSlot(
        center -
            entranceDir * OLM_PULL_DISTANCE -
            state.sideDir * COUNCIL_LATERAL_OFFSET,
        olm->GetPositionZ());

    // Prot Paladin standby is deliberately between Olm/Blindeye and the center,
    // so a newly spawned Wild Fel Stalker can be picked up quickly.
    state.felStandbyAnchor = MakeSlot(
        center -
            entranceDir * 10.0f -
            state.sideDir * 3.0f,
        zAverage);

    state.kroshInitial = Position2(krosh);
    state.kigglerInitial = Position2(kiggler);
    state.initialized = true;

    sLog.outDetail(
        "[EncounterAI][Maulgar][Formation] frame captured center=(%.2f,%.2f) "
        "entrance=(%.3f,%.3f)",
        center.x, center.y, entranceDir.x, entranceDir.y);

    return true;
}

bool MaulgarFormationManager::EnsureTankAnchor(
    PlayerbotAI* ai,
    Creature* target,
    MaulgarTankAnchorRole role)
{
    if (!ai || !ai->GetBot())
        return false;

    MaulgarFixedAnchor anchor;

    switch (role)
    {
        case MaulgarTankAnchorRole::Maulgar:
            anchor = MaulgarFixedPositions::MaulgarTank();
            break;
        case MaulgarTankAnchorRole::Blindeye:
            anchor = MaulgarFixedPositions::BlindeyeTank();
            break;
        case MaulgarTankAnchorRole::Olm:
            anchor = MaulgarFixedPositions::OlmWarlockTank();
            break;
        case MaulgarTankAnchorRole::FelhunterStandby:
            anchor = MaulgarFixedPositions::FelhunterPaladinStandby();
            break;
        default:
            return false;
    }

    if (!anchor.configured)
        return false;

    // Fixed anchor means the Tank waits for its target to arrive. Do not drag
    // the Tank toward the boss before threat is established; Misdirection /
    // dedicated pull mechanics bring the council member to this point.
    return MoveToSlot(
        ai,
        FixedSlot(anchor),
        TANK_MOVE_TOLERANCE,
        10u + uint32(role));
}

bool MaulgarFormationManager::EnsureKroshMagePosition(
    PlayerbotAI* ai,
    Creature* krosh)
{
    if (!ai || !krosh || !krosh->IsAlive())
        return false;

    MaulgarFixedAnchor anchor =
        MaulgarFixedPositions::KroshMageTank();

    if (!anchor.configured)
        return false;

    return MoveToSlot(
        ai,
        FixedSlot(anchor),
        RANGED_MOVE_TOLERANCE,
        30);
}

bool MaulgarFormationManager::EnsureKigglerPosition(
    PlayerbotAI* ai,
    Creature* kiggler)
{
    if (!ai || !kiggler || !kiggler->IsAlive())
        return false;

    MaulgarFixedAnchor anchor =
        MaulgarFixedPositions::KigglerTank();

    if (!anchor.configured)
        return false;

    return MoveToSlot(
        ai,
        FixedSlot(anchor),
        RANGED_MOVE_TOLERANCE,
        31);
}

bool MaulgarFormationManager::EnsureOlmWarlockPosition(
    PlayerbotAI* ai,
    Creature* olm)
{
    if (!ai || !olm || !olm->IsAlive())
        return false;

    MaulgarFixedAnchor anchor =
        MaulgarFixedPositions::OlmWarlockTank();

    if (!anchor.configured)
        return false;

    return MoveToSlot(
        ai,
        FixedSlot(anchor),
        RANGED_MOVE_TOLERANCE,
        32);
}

bool MaulgarFormationManager::EnsureHealerPosition(PlayerbotAI* ai)
{
    MaulgarFormationState* state = State(ai);
    if (!state || !ai || !ai->GetBot())
        return false;

    const uint32 ordinal = GroupOrdinal(ai);

    // Five-healer-friendly line across the entrance side. GroupOrdinal makes
    // this deterministic without a second hard-coded roster table.
    const int32 lane = int32(ordinal % 7) - 3;
    Vec2 p =
        state->councilCenter +
        state->entranceDir * HEALER_BACKLINE_DISTANCE +
        state->sideDir * (float(lane) * 3.5f);

    return MoveToSlot(
        ai,
        MakeSlot(p, ai->GetBot()->GetPositionZ()),
        HEALER_MOVE_TOLERANCE,
        40 + (ordinal % 16));
}

bool MaulgarFormationManager::EnsureRangedPosition(
    PlayerbotAI* ai,
    Unit* target)
{
    MaulgarFormationState* state = State(ai);
    if (!state || !target || !target->IsAlive())
        return false;

    const uint32 ordinal = GroupOrdinal(ai);
    const int32 lane = int32(ordinal % 9) - 4;
    const float lateral = float(lane) * 2.25f;

    Slot slot = RelativeTargetSlot(
        *state,
        target,
        RANGED_TARGET_RANGE,
        lateral);

    return MoveToSlot(
        ai,
        slot,
        RANGED_MOVE_TOLERANCE,
        60 + (ordinal % 24));
}

bool MaulgarFormationManager::EnsureMeleePosition(
    PlayerbotAI* ai,
    Unit* target)
{
    if (!ai || !ai->GetBot() || !target || !target->IsAlive())
        return false;

    const uint32 ordinal = GroupOrdinal(ai);

    // Rear arc around the target. Small angular offsets prevent every melee
    // actor from occupying one point while preserving rear/side attack geometry.
    const int32 lane = int32(ordinal % 5) - 2;
    const float angle =
        target->GetOrientation() +
        M_PI_F +
        float(lane) * 0.18f;

    float x = 0.0f;
    float y = 0.0f;
    target->GetNearPoint2d(x, y, MELEE_REAR_RANGE, angle);

    return MoveToSlot(
        ai,
        Slot(x, y, target->GetPositionZ()),
        MELEE_MOVE_TOLERANCE,
        90 + (ordinal % 24));
}

bool MaulgarFormationManager::HandleMaulgarWhirlwind(
    PlayerbotAI* ai,
    Creature* maulgar)
{
    MaulgarFormationState* state = State(ai);
    if (!state || !ai || !ai->GetBot() || !maulgar || !maulgar->IsAlive())
        return false;

    Player* bot = ai->GetBot();

    // While Whirlwind is active, returning to Normal Rotation would risk an
    // immediate chase back into melee. Therefore this method returns true even
    // after reaching the safe radius, keeping melee rotation blocked until the
    // aura ends.
    if (bot->IsWithinDist3d(
            maulgar->GetPositionX(),
            maulgar->GetPositionY(),
            maulgar->GetPositionZ(),
            WHIRLWIND_SAFE_RANGE))
    {
        Vec2 away = Position2(bot) - Position2(maulgar);
        if (Length(away) < 1.0f)
            away = state->entranceDir;

        away = Normalize(away);

        Vec2 escape =
            Position2(maulgar) +
            away * WHIRLWIND_ESCAPE_RANGE;

        MoveToSlot(
            ai,
            MakeSlot(escape, bot->GetPositionZ()),
            2.0f,
            120 + (GroupOrdinal(ai) % 24));
    }

    return true;
}

bool MaulgarFormationManager::IsMeleeFormationActor(uint32 guid)
{
    switch (guid)
    {
        // Melee DPS
        case 17: case 22: case 37: case 32: case 30: case 175:
        case 39: case 56: case 63: case 53: case 46: case 387:
        case 83: case 12: case 103: case 126: case 174: case 400:

        // Prot Paladin transient pickup / standby actors
        case 31: case 97: case 98:
            return true;

        default:
            return false;
    }
}
