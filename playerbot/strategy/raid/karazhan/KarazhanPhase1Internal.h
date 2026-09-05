#pragma once

#include <vector>

class Creature;
class Player;
class PlayerbotAI;
class Unit;
class WorldObject;

namespace ai
{
    namespace karazhan_phase1_detail
    {
        struct Vec2
        {
            float x;
            float y;

            Vec2() : x(0.0f), y(0.0f) {}
            Vec2(float px, float py) : x(px), y(py) {}

            Vec2 operator+(Vec2 const& rhs) const
            {
                return Vec2(x + rhs.x, y + rhs.y);
            }

            Vec2 operator-(Vec2 const& rhs) const
            {
                return Vec2(x - rhs.x, y - rhs.y);
            }

            Vec2 operator*(float scalar) const
            {
                return Vec2(x * scalar, y * scalar);
            }
        };

        struct Slot
        {
            float x;
            float y;
            float z;
            float orientation;
            bool valid;

            Slot()
                : x(0.0f), y(0.0f), z(0.0f),
                  orientation(0.0f), valid(false) {}

            Slot(float px, float py, float pz, float po = 0.0f)
                : x(px), y(py), z(pz),
                  orientation(po), valid(true) {}
        };

        float Length(Vec2 const& value);
        Vec2 Normalize(Vec2 value);
        Vec2 Perpendicular(Vec2 const& value);
        Vec2 Position2(WorldObject const* object);

        std::vector<Player*> SortedGroup(PlayerbotAI* ai);
        std::vector<Player*> SortedTanks(PlayerbotAI* ai);
        Vec2 RaidCentroid(PlayerbotAI* ai);
        Slot RespawnCenter(Unit* target);

        bool CandidateValid(
            Player* actor,
            Unit* target,
            Slot& slot,
            float minimumTargetDistance,
            float maximumTargetDistance);

        bool MoveToward(
            PlayerbotAI* ai,
            Slot const& finalSlot,
            float tolerance,
            uint32 moveId);

        Slot FarSideTankSlot(
            PlayerbotAI* ai,
            Unit* target,
            Slot const& center,
            float offset);

        Slot BehindTargetSlot(
            PlayerbotAI* ai,
            Unit* target,
            float distance);

        uint32 RangedOrdinal(PlayerbotAI* ai, Player* actor);

        void ResetMovement(PlayerbotAI* ai);
        void ResetAttumen(PlayerbotAI* ai);
        void ResetMoroes(PlayerbotAI* ai);
        void ResetMaiden(PlayerbotAI* ai);
    }
}
