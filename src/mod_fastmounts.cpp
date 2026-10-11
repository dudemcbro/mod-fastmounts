#include "Config.h"
#include "Log.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraDefines.h"
#include "SpellAuraEffects.h"

#include <cmath>
#include <unordered_map>

namespace
{
bool sEnable = true;
float sFastLandMountTargetRunRate = 2.5f; // fixed 150% ground mount speed
float sSlowLandMountTargetRunRate = 2.0f; // fixed 100% ground mount speed
struct SpeedState
{
    bool forced = false;
    float targetRunRate = 0.0f;
};
std::unordered_map<ObjectGuid, SpeedState> sSpeedStates;
constexpr uint32 SPELL_MAGIC_ROOSTER = 65917;
constexpr uint32 SPELL_MAGIC_ROOSTER_VARIANT_1 = 66122;
constexpr uint32 SPELL_MAGIC_ROOSTER_VARIANT_2 = 66123;
constexpr uint32 SPELL_MAGIC_ROOSTER_VARIANT_3 = 66124;
constexpr uint32 DISPLAY_MAGIC_ROOSTER = 34655;
constexpr uint32 DISPLAY_MAGIC_ROOSTER_VARIANT_1 = 34731;
constexpr uint32 DISPLAY_MAGIC_ROOSTER_VARIANT_2 = 34732;

bool IsApproximatelyEqual(float a, float b)
{
    return std::fabs(a - b) <= 0.001f;
}

bool IsLandMount(Player const* player)
{
    if (!player)
        return false;

    if (!player->IsMounted())
        return false;

    if (player->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED))
        return false;

    if (player->HasAuraType(SPELL_AURA_FLY))
        return false;

    if (player->IsInFlight())
        return false;

    if (player->GetVehicle())
        return false;

    return true;
}

bool NormalizeMagicRoosterMountAura(Player* player)
{
    if (!player || !player->IsMounted())
        return false;

    auto const& mountedAuras = player->GetAuraEffectsByType(SPELL_AURA_MOUNTED);
    if (mountedAuras.size() <= 1)
        return false;

    bool hasRoosterAura = false;
    for (AuraEffect const* aurEff : mountedAuras)
    {
        if (aurEff && aurEff->GetBase())
        {
            uint32 const roosterId = aurEff->GetBase()->GetId();
            if (roosterId == SPELL_MAGIC_ROOSTER || roosterId == SPELL_MAGIC_ROOSTER_VARIANT_1 ||
                roosterId == SPELL_MAGIC_ROOSTER_VARIANT_2 || roosterId == SPELL_MAGIC_ROOSTER_VARIANT_3)
            {
                hasRoosterAura = true;
                break;
            }
        }
    }

    if (!hasRoosterAura)
        return false;

    // Normalize to a single rooster mount aura to avoid mixed mount buff states.
    player->RemoveAurasByType(SPELL_AURA_MOUNTED);
    player->CastSpell(player, SPELL_MAGIC_ROOSTER, true);
    return true;
}

bool HasAnyMagicRoosterAura(Player const* player)
{
    if (!player)
        return false;

    auto const& mountedAuras = player->GetAuraEffectsByType(SPELL_AURA_MOUNTED);
    for (AuraEffect const* aurEff : mountedAuras)
    {
        if (!aurEff || !aurEff->GetBase())
            continue;

        uint32 const id = aurEff->GetBase()->GetId();
        if (id == SPELL_MAGIC_ROOSTER || id == SPELL_MAGIC_ROOSTER_VARIANT_1 ||
            id == SPELL_MAGIC_ROOSTER_VARIANT_2 || id == SPELL_MAGIC_ROOSTER_VARIANT_3)
            return true;
    }

    return false;
}

bool IsAnyMagicRoosterMount(Player const* player)
{
    if (!player)
        return false;

    if (HasAnyMagicRoosterAura(player))
        return true;

    uint32 const mountDisplayId = player->GetMountID();
    return mountDisplayId == DISPLAY_MAGIC_ROOSTER ||
        mountDisplayId == DISPLAY_MAGIC_ROOSTER_VARIANT_1 ||
        mountDisplayId == DISPLAY_MAGIC_ROOSTER_VARIANT_2;
}

float DetermineTargetRunRate(float currentRunRate)
{
    // Determine mount tier from observed baseline mount speed at mount time.
    return currentRunRate >= 2.0f ? sFastLandMountTargetRunRate : sSlowLandMountTargetRunRate;
}

void RestoreDefaultSpeed(Player* player, SpeedState& state)
{
    if (!player || !state.forced)
        return;

    // Recompute normal speed from active auras/state to avoid leaking mount speed on dismount.
    player->UpdateSpeed(MOVE_RUN, true);
    player->UpdateSpeed(MOVE_RUN_BACK, true);
    state.forced = false;
    state.targetRunRate = 0.0f;
}

void ApplyBoost(Player* player, SpeedState& state)
{
    if (!player)
        return;

    float const currentRun = player->GetSpeedRate(MOVE_RUN);
    float const currentRunBack = player->GetSpeedRate(MOVE_RUN_BACK);
    // Rooster mounts are always fixed to slow tier, even if first update tick
    // started with an unresolved/non-rooster aura state.
    if (IsAnyMagicRoosterMount(player))
    {
        state.targetRunRate = sSlowLandMountTargetRunRate;
        state.forced = true;
    }
    else if (!state.forced)
    {
        state.targetRunRate = DetermineTargetRunRate(currentRun);
        state.forced = true;
    }

    float const targetRun = state.targetRunRate > 0.0f ? state.targetRunRate : sSlowLandMountTargetRunRate;
    float const scale = currentRun > 0.0f ? (targetRun / currentRun) : 1.0f;
    float const targetRunBack = currentRunBack * scale;

    if (!IsApproximatelyEqual(currentRun, targetRun))
        player->SetSpeed(MOVE_RUN, targetRun, true);

    if (!IsApproximatelyEqual(currentRunBack, targetRunBack))
        player->SetSpeed(MOVE_RUN_BACK, targetRunBack, true);
}
} // namespace

class FastMountsWorldScript final : public WorldScript
{
public:
    FastMountsWorldScript() : WorldScript("FastMountsWorldScript") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sEnable = sConfigMgr->GetOption<bool>("FastMounts.Enable", true);
        sFastLandMountTargetRunRate = sConfigMgr->GetOption<float>("FastMounts.FastLandMountTargetRunRate", 2.5f);
        sSlowLandMountTargetRunRate = sConfigMgr->GetOption<float>("FastMounts.SlowLandMountTargetRunRate", 2.0f);

        if (sFastLandMountTargetRunRate < 1.0f)
            sFastLandMountTargetRunRate = 1.0f;

        if (sSlowLandMountTargetRunRate < 1.0f)
            sSlowLandMountTargetRunRate = 1.0f;

        LOG_INFO("module.fastmounts",
            "mod-fastmounts: Enable={}, FastLandMountTargetRunRate={}, SlowLandMountTargetRunRate={}",
            sEnable ? 1 : 0, sFastLandMountTargetRunRate, sSlowLandMountTargetRunRate);
    }
};

class FastMountsPlayerScript final : public PlayerScript
{
public:
    FastMountsPlayerScript()
        : PlayerScript("FastMountsPlayerScript", { PLAYERHOOK_ON_UPDATE, PLAYERHOOK_ON_SPELL_CAST, PLAYERHOOK_ON_LOGOUT })
    {
    }

    void OnPlayerSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
    {
        if (!player || !spell || !spell->GetSpellInfo())
            return;

        if (spell->GetSpellInfo()->Id != SPELL_MAGIC_ROOSTER)
            return;

        if (!player->IsMounted() || player->IsInFlight())
            return;

        // Ensure rooster cast always starts from an unmounted state.
        player->RemoveAurasByType(SPELL_AURA_MOUNTED);
    }

    void OnPlayerUpdate(Player* player, uint32 /*diff*/) override
    {
        if (!player)
            return;

        auto& state = sSpeedStates[player->GetGUID()];

        if (NormalizeMagicRoosterMountAura(player))
        {
            RestoreDefaultSpeed(player, state);
            return;
        }

        if (!sEnable)
        {
            RestoreDefaultSpeed(player, state);
            return;
        }

        if (IsLandMount(player))
        {
            ApplyBoost(player, state);
            return;
        }

        RestoreDefaultSpeed(player, state);
    }

    void OnPlayerLogout(Player* player) override
    {
        if (!player)
            return;

        sSpeedStates.erase(player->GetGUID());
    }
};

void Addmod_fastmountsScripts()
{
    new FastMountsWorldScript();
    new FastMountsPlayerScript();
}
