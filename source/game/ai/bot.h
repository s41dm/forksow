#ifndef AI_BOT_H
#define AI_BOT_H

#include "static_vector.h"
#include "dangers_detector.h"
#include "bot_brain.h"
#include "ai_base_ai.h"
#include "vec3.h"

#include "bot_movement.h"
#include "bot_weapon_selector.h"
#include "bot_fire_target_cache.h"
#include "bot_tactical_spots_cache.h"
#include "bot_weight_config.h"

#include "bot_goals.h"
#include "bot_actions.h"

class AiSquad;
class AiBaseEnemyPool;

struct AiAlertSpot
{
    int id;
    Vec3 origin;
    float radius;
    float regularEnemyInfluenceScale;
    float carrierEnemyInfluenceScale;

    AiAlertSpot(int id_,
                const Vec3 &origin_,
                float radius_,
                float regularEnemyInfluenceScale_ = 1.0f,
                float carrierEnemyInfluenceScale_ = 1.0f)
        : id(id_),
          origin(origin_),
          radius(radius_),
          regularEnemyInfluenceScale(regularEnemyInfluenceScale_),
          carrierEnemyInfluenceScale(carrierEnemyInfluenceScale_) {}
};

class Bot: public Ai
{
    friend class AiManager;
    friend class BotEvolutionManager;
    friend class AiBaseTeamBrain;
    friend class BotBrain;
    friend class AiSquad;
    friend class AiBaseEnemyPool;
    friend class BotFireTargetCache;
    friend class BotItemsSelector;
    friend class BotWeaponSelector;
    friend class BotBaseGoal;
    friend class BotGrabItemGoal;
    friend class BotKillEnemyGoal;
    friend class BotRunAwayGoal;
    friend class BotReactToDangerGoal;
    friend class BotReactToThreatGoal;
    friend class BotReactToEnemyLostGoal;
    friend class BotAttackOutOfDespairGoal;
    friend class BotTacticalSpotsCache;
    friend class WorldState;

    friend struct BotMovementState;
    friend class BotMovementPredictionContext;
    friend class BotBaseMovementAction;
    friend class BotDummyMovementAction;
    friend class BotMoveOnLadderMovementAction;
    friend class BotHandleTriggeredJumppadMovementAction;
    friend class BotLandOnSavedAreasMovementAction;
    friend class BotRidePlatformMovementAction;
    friend class BotSwimMovementAction;
    friend class BotFlyUntilLandingMovementAction;
    friend class BotCampASpotMovementAction;
    friend class BotWalkCarefullyMovementAction;
    friend class BotGenericRunBunnyingMovementAction;
    friend class BotBunnyStraighteningReachChainMovementAction;
    friend class BotBunnyToBestShortcutAreaMovementAction;
    friend class BotBunnyInVelocityDirectionMovementAction;
    friend class BotWalkToBestNearbyTacticalSpotMovementAction;
    friend class BotCombatDodgeSemiRandomlyToTargetMovementAction;
public:
    static constexpr auto PREFERRED_TRAVEL_FLAGS =
        TFL_WALK | TFL_WALKOFFLEDGE | TFL_JUMP | TFL_AIR | TFL_TELEPORT | TFL_JUMPPAD;
    static constexpr auto ALLOWED_TRAVEL_FLAGS =
        PREFERRED_TRAVEL_FLAGS | TFL_WATER | TFL_WATERJUMP | TFL_SWIM | TFL_LADDER | TFL_ELEVATOR;

    Bot(edict_t *self_, float skillLevel_);
    virtual ~Bot() override
    {
        AiAasRouteCache::ReleaseInstance(routeCache);
    }

    inline float Skill() const { return skillLevel; }
    inline bool IsReady() const { return level.ready[PLAYERNUM(self)]; }

    void Pain(const edict_t *enemy, float kick, int damage)
    {
        botBrain.OnPain(enemy, kick, damage);
    }
    void OnEnemyDamaged(const edict_t *enemy, int damage)
    {
        botBrain.OnEnemyDamaged(enemy, damage);
    }

    inline void OnAttachedToSquad(AiSquad *squad)
    {
        botBrain.OnAttachedToSquad(squad);
        isInSquad = true;
    }
    inline void OnDetachedFromSquad(AiSquad *squad)
    {
        botBrain.OnDetachedFromSquad(squad);
        isInSquad = false;
    }
    inline bool IsInSquad() const { return isInSquad; }

    inline unsigned LastAttackedByTime(const edict_t *attacker)
    {
        return botBrain.LastAttackedByTime(attacker);
    }
    inline unsigned LastTargetTime(const edict_t *target)
    {
        return botBrain.LastTargetTime(target);
    }
    inline void OnEnemyRemoved(const Enemy *enemy)
    {
        botBrain.OnEnemyRemoved(enemy);
    }
    inline void OnNewThreat(const edict_t *newThreat, const AiFrameAwareUpdatable *threatDetector)
    {
        botBrain.OnNewThreat(newThreat, threatDetector);
    }

    inline void SetAttitude(const edict_t *ent, int attitude)
    {
        botBrain.SetAttitude(ent, attitude);
    }
    inline void ClearOverriddenEntityWeights()
    {
        botBrain.ClearOverriddenEntityWeights();
    }
    inline void OverrideEntityWeight(const edict_t *ent, float weight)
    {
        botBrain.OverrideEntityWeight(ent, weight);
    }

    inline float GetBaseOffensiveness() const { return botBrain.GetBaseOffensiveness(); }
    inline float GetEffectiveOffensiveness() const { return botBrain.GetEffectiveOffensiveness(); }
    inline void SetBaseOffensiveness(float baseOffensiveness)
    {
        botBrain.SetBaseOffensiveness(baseOffensiveness);
    }

    inline const int *Inventory() const { return self->r.client->ps.inventory; }

    typedef void (*AlertCallback)(void *receiver, Bot *bot, int id, float alertLevel);

    void EnableAutoAlert(const AiAlertSpot &alertSpot, AlertCallback callback, void *receiver);
    void DisableAutoAlert(int id);

    inline int Health() const
    {
        return self->r.client->ps.stats[STAT_HEALTH];
    }
    inline int Armor() const
    {
        return self->r.client->ps.stats[STAT_ARMOR];
    }
    inline bool CanAndWouldDropHealth() const
    {
        return GT_asBotWouldDropHealth(self->r.client);
    }
    inline void DropHealth()
    {
        GT_asBotDropHealth(self->r.client);
    }
    inline bool CanAndWouldDropArmor() const
    {
        return GT_asBotWouldDropArmor(self->r.client);
    }
    inline void DropArmor()
    {
        GT_asBotDropArmor(self->r.client);
    }
    inline float PlayerDefenciveAbilitiesRating() const
    {
        return GT_asPlayerDefenciveAbilitiesRating(self->r.client);
    }
    inline float PlayerOffenciveAbilitiesRating() const
    {
        return GT_asPlayerOffensiveAbilitiesRating(self->r.client);
    }
    inline int DefenceSpotId() const { return defenceSpotId; }
    inline int OffenseSpotId() const { return offenseSpotId; }
    inline void ClearDefenceAndOffenceSpots()
    {
        defenceSpotId = -1;
        offenseSpotId = -1;
    }
    inline void SetDefenceSpotId(int spotId)
    {
        defenceSpotId = spotId;
        offenseSpotId = -1;
    }
    inline void SetOffenseSpotId(int spotId)
    {
        defenceSpotId = -1;
        offenseSpotId = spotId;
    }
    inline float Fov() const { return 110.0f + 69.0f * Skill(); }
    inline float FovDotFactor() const { return cosf((float)DEG2RAD(Fov() / 2)); }

    inline BotBaseGoal *GetGoalByName(const char *name) { return botBrain.GetGoalByName(name); }
    inline BotBaseAction *GetActionByName(const char *name) { return botBrain.GetActionByName(name); }

    inline BotScriptGoal *AllocScriptGoal() { return botBrain.AllocScriptGoal(); }
    inline BotScriptAction *AllocScriptAction() { return botBrain.AllocScriptAction(); }

    inline const BotWeightConfig &WeightConfig() const { return weightConfig; }
    inline BotWeightConfig &WeightConfig() { return weightConfig; }

    inline void OnInterceptedPredictedEvent(int ev, int parm)
    {
        movementPredictionContext.OnInterceptedPredictedEvent(ev, parm);
    }
    inline void OnInterceptedPMoveTouchTriggers(pmove_t *pm, const vec3_t previousOrigin)
    {
        movementPredictionContext.OnInterceptedPMoveTouchTriggers(pm, previousOrigin);
    }
protected:
    virtual void Frame() override;
    virtual void Think() override;

    virtual void PreFrame() override
    {
        // We should update weapons status each frame since script weapons may be changed each frame.
        // These statuses are used by firing methods, so actual weapon statuses are required.
        UpdateScriptWeaponsStatus();
    }

    virtual void OnNavTargetTouchHandled() override
    {
        botBrain.selectedNavEntity.InvalidateNextFrame();
    }

    virtual void TouchedOtherEntity(const edict_t *entity) override;
private:
    void RegisterVisibleEnemies();

    inline bool IsPrimaryAimEnemy(const edict_t *enemy) const { return botBrain.IsPrimaryAimEnemy(enemy); }

    BotWeightConfig weightConfig;
    DangersDetector dangersDetector;
    BotBrain botBrain;

    float skillLevel;

    SelectedEnemies selectedEnemies;
    SelectedWeapons selectedWeapons;

    BotWeaponSelector weaponsSelector;

    BotTacticalSpotsCache tacticalSpotsCache;

    BotFireTargetCache builtinFireTargetCache;
    BotFireTargetCache scriptFireTargetCache;

    BotGrabItemGoal grabItemGoal;
    BotKillEnemyGoal killEnemyGoal;
    BotRunAwayGoal runAwayGoal;
    BotReactToDangerGoal reactToDangerGoal;
    BotReactToThreatGoal reactToThreatGoal;
    BotReactToEnemyLostGoal reactToEnemyLostGoal;
    BotAttackOutOfDespairGoal attackOutOfDespairGoal;

    BotGenericRunToItemAction genericRunToItemAction;
    BotPickupItemAction pickupItemAction;
    BotWaitForItemAction waitForItemAction;

    BotKillEnemyAction killEnemyAction;
    BotAdvanceToGoodPositionAction advanceToGoodPositionAction;
    BotRetreatToGoodPositionAction retreatToGoodPositionAction;
    BotSteadyCombatAction steadyCombatAction;
    BotGotoAvailableGoodPositionAction gotoAvailableGoodPositionAction;
    BotAttackFromCurrentPositionAction attackFromCurrentPositionAction;

    BotGenericRunAvoidingCombatAction genericRunAvoidingCombatAction;
    BotStartGotoCoverAction startGotoCoverAction;
    BotTakeCoverAction takeCoverAction;

    BotStartGotoRunAwayTeleportAction startGotoRunAwayTeleportAction;
    BotDoRunAwayViaTeleportAction doRunAwayViaTeleportAction;
    BotStartGotoRunAwayJumppadAction startGotoRunAwayJumppadAction;
    BotDoRunAwayViaJumppadAction doRunAwayViaJumppadAction;
    BotStartGotoRunAwayElevatorAction startGotoRunAwayElevatorAction;
    BotDoRunAwayViaElevatorAction doRunAwayViaElevatorAction;
    BotStopRunningAwayAction stopRunningAwayAction;

    BotDodgeToSpotAction dodgeToSpotAction;

    BotTurnToThreatOriginAction turnToThreatOriginAction;

    BotTurnToLostEnemyAction turnToLostEnemyAction;
    BotStartLostEnemyPursuitAction startLostEnemyPursuitAction;
    BotStopLostEnemyPursuitAction stopLostEnemyPursuitAction;

    // Must be initialized before any of movement actions constructors is called
    StaticVector<BotBaseMovementAction *, 16> movementActions;

    BotDummyMovementAction dummyMovementAction;
    BotHandleTriggeredJumppadMovementAction handleTriggeredJumppadMovementAction;
    BotLandOnSavedAreasMovementAction landOnSavedAreasSetMovementAction;
    BotRidePlatformMovementAction ridePlatformMovementAction;
    BotSwimMovementAction swimMovementAction;
    BotFlyUntilLandingMovementAction flyUntilLandingMovementAction;
    BotCampASpotMovementAction campASpotMovementAction;
    BotWalkCarefullyMovementAction walkCarefullyMovementAction;
    BotBunnyStraighteningReachChainMovementAction bunnyStraighteningReachChainMovementAction;
    BotBunnyToBestShortcutAreaMovementAction bunnyToBestShortcutAreaMovementAction;
    BotBunnyInVelocityDirectionMovementAction bunnyInVelocityDirectionMovementAction;
    BotWalkToBestNearbyTacticalSpotMovementAction walkToBestNearbyTacticalSpotMovementAction;
    BotCombatDodgeSemiRandomlyToTargetMovementAction combatDodgeSemiRandomlyToTargetMovementAction;

    BotMovementState movementState;

    BotMovementPredictionContext movementPredictionContext;

    unsigned vsayTimeout;

    bool isInSquad;

    int defenceSpotId;
    int offenseSpotId;

    struct AlertSpot: public AiAlertSpot
    {
        unsigned lastReportedAt;
        float lastReportedScore;
        AlertCallback callback;
        void *receiver;

        AlertSpot(const AiAlertSpot &spot, AlertCallback callback_, void *receiver_)
            : AiAlertSpot(spot),
              lastReportedAt(0),
              lastReportedScore(0.0f),
              callback(callback_),
              receiver(receiver_) {};

        inline void Alert(Bot *bot, float score)
        {
            callback(receiver, bot, id, score);
            lastReportedAt = level.time;
            lastReportedScore = score;
        }
    };

    static constexpr unsigned MAX_ALERT_SPOTS = 3;
    StaticVector<AlertSpot, MAX_ALERT_SPOTS> alertSpots;

    void CheckAlertSpots(const StaticVector<edict_t *, MAX_CLIENTS> &visibleTargets);

    static constexpr unsigned MAX_SCRIPT_WEAPONS = 3;

    StaticVector<AiScriptWeaponDef, MAX_SCRIPT_WEAPONS> scriptWeaponDefs;
    StaticVector<int, MAX_SCRIPT_WEAPONS> scriptWeaponCooldown;

    unsigned lastTouchedTeleportAt;
    unsigned lastTouchedJumppadAt;
    unsigned lastTouchedElevatorAt;

    unsigned similarWorldStateInstanceId;

    class AimingRandomHolder
    {
        unsigned valuesTimeoutAt[3];
        float values[3];
    public:
        inline AimingRandomHolder()
        {
            std::fill_n(valuesTimeoutAt, 3, 0);
            std::fill_n(values, 3, 0.5f);
        }
        inline float GetCoordRandom(int coordNum)
        {
            if (valuesTimeoutAt[coordNum] <= level.time)
            {
                values[coordNum] = random();
                valuesTimeoutAt[coordNum] = level.time + 128 + From0UpToMax(256, random());
            }
            return values[coordNum];
        }
    };

    AimingRandomHolder aimingRandomHolder;

    void UpdateScriptWeaponsStatus();

    void MovementFrame(BotInput *input);
    bool CanChangeWeapons() const;
    void ChangeWeapons(const SelectedWeapons &selectedWeapons);
    void ChangeWeapon(int weapon);
    void FireWeapon(BotInput *input);
    virtual void OnBlockedTimeout() override;
    void SayVoiceMessages();
    void GhostingFrame();
    void ActiveFrame();
    void CallGhostingClientThink(const BotInput &input);
    void CallActiveClientThink(const BotInput &input);

    void OnRespawn();

    void ApplyPendingTurnToLookAtPoint(BotInput *input, BotMovementPredictionContext *context = nullptr) const;
    void ApplyInput(BotInput *input, BotMovementPredictionContext *context = nullptr);

    // Returns true if current look angle worth pressing attack
    bool CheckShot(const AimParams &aimParams, const BotInput *input,
                   const SelectedEnemies &selectedEnemies, const GenericFireDef &fireDef);

    void LookAtEnemy(float coordError, const vec3_t fire_origin, vec3_t target, BotInput *input);
    void PressAttack(const GenericFireDef *fireDef, const GenericFireDef *builtinFireDef,
                     const GenericFireDef *scriptFireDef, BotInput *input);

    inline bool HasEnemy() const { return selectedEnemies.AreValid(); }
    inline bool IsEnemyAStaticSpot() const { return selectedEnemies.IsStaticSpot(); }
    inline const edict_t *EnemyTraceKey() const { return selectedEnemies.TraceKey(); }
    inline const bool IsEnemyOnGround() const { return selectedEnemies.OnGround(); }
    inline Vec3 EnemyOrigin() const { return selectedEnemies.LastSeenOrigin(); }
    inline Vec3 EnemyLookDir() const { return selectedEnemies.LookDir(); }
    inline unsigned EnemyFireDelay() const { return selectedEnemies.FireDelay(); }
    inline Vec3 EnemyVelocity() const { return selectedEnemies.LastSeenVelocity(); }
    inline Vec3 EnemyMins() const { return selectedEnemies.Mins(); }
    inline Vec3 EnemyMaxs() const { return selectedEnemies.Maxs(); }

    static constexpr unsigned MAX_SAVED_LANDING_AREAS = BotMovementPredictionContext::MAX_SAVED_LANDING_AREAS;
    StaticVector<int, MAX_SAVED_LANDING_AREAS> savedLandingAreas;

    void CheckTargetProximity();

public:
    // These methods are exposed mostly for script interface
    inline unsigned NextSimilarWorldStateInstanceId()
    {
        return ++similarWorldStateInstanceId;
    }
    inline void SetNavTarget(NavTarget *navTarget)
    {
        botBrain.SetNavTarget(navTarget);
    }
    inline void SetNavTarget(const Vec3 &navTargetOrigin, float reachRadius)
    {
        botBrain.SetNavTarget(navTargetOrigin, reachRadius);
    }
    inline void ResetNavTarget()
    {
        botBrain.ResetNavTarget();
    }
    inline void SetCampingSpot(const AiCampingSpot &campingSpot)
    {
        movementState.campingSpotState.Activate(campingSpot);
    }
    inline void ResetCampingSpot()
    {
        movementState.campingSpotState.Deactivate();
    }
    inline bool HasActiveCampingSpot() const
    {
        return movementState.campingSpotState.IsActive();
    }
    inline void SetPendingLookAtPoint(const AiPendingLookAtPoint &lookAtPoint, unsigned timeoutPeriod)
    {
        movementState.pendingLookAtPointState.Activate(lookAtPoint, timeoutPeriod);
    }
    inline void ResetPendingLookAtPoint()
    {
        movementState.pendingLookAtPointState.Deactivate();
    }
    inline bool HasPendingLookAtPoint() const
    {
        return movementState.pendingLookAtPointState.IsActive();
    }
    const SelectedNavEntity &GetSelectedNavEntity() const
    {
        return botBrain.selectedNavEntity;
    }
    const SelectedEnemies &GetSelectedEnemies() const { return selectedEnemies; }
    SelectedMiscTactics &GetMiscTactics() { return botBrain.selectedTactics; }
    const SelectedMiscTactics &GetMiscTactics() const { return botBrain.selectedTactics; }

    inline bool WillAdvance() const { return botBrain.WillAdvance(); }
    inline bool WillRetreat() const { return botBrain.WillRetreat(); }

    inline bool ShouldBeSilent() const { return botBrain.ShouldBeSilent(); }
    inline bool ShouldMoveCarefully() const { return botBrain.ShouldMoveCarefully(); }

    inline bool ShouldAttack() const { return botBrain.ShouldAttack(); }
    inline bool ShouldKeepXhairOnEnemy() const { return botBrain.ShouldKeepXhairOnEnemy(); }

    inline bool WillAttackMelee() const { return botBrain.WillAttackMelee(); }
    inline bool ShouldRushHeadless() const { return botBrain.ShouldRushHeadless(); }
};

#endif
