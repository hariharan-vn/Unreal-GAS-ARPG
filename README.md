# GAS ARPG — Technical Design Document

## Overview

This project is a top-down Action RPG prototype built in Unreal Engine 5.5, implementing a melee combat system using the **Gameplay Ability System (GAS)**. The core focus is a modular, data-driven combat framework centered around a **Leap Slam** ability inspired by Path of Exile, with a group-based enemy AI system designed to create rhythmic, readable combat encounters.

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Character Hierarchy](#character-hierarchy)
3. [Attribute System](#attribute-system)
4. [Ability System](#ability-system)
5. [Leap Slam — Deep Dive](#leap-slam--deep-dive)
6. [Weapon System](#weapon-system)
7. [Input System](#input-system)
8. [Enemy AI System](#enemy-ai-system)
9. [Key Design Decisions](#key-design-decisions)
10. [Future Implementations](#future-implementations)

---

## Architecture Overview

The project is built around three architectural principles:

**1. Separation of Concerns**
Every system has a single owner. The ability owns its logic, the task owns movement, the attribute set owns stat storage. No system reaches into another's domain.

**2. Data-Driven Configuration**
Values live in Blueprint-configured `GameplayEffect` assets and `EditDefaultsOnly` properties — not hardcoded in C++. Adding a new weapon type or tuning damage requires no code changes.

**3. Event-Driven Activation**
Abilities are never activated directly. All activation flows through `SendGameplayEventToActor` with `Event.*` tags. This decouples input from ability logic entirely.

```
Input
  └── SendGameplayEventToActor(Event.Ability.LeapSlam)
          └── AbilityTriggers on ULeapSlamAbility listens
                  └── Ability activates
```

---

## Character Hierarchy

```
AARPGCharacterBase               (implements IAbilitySystemInterface)
    ├── AGAS_ARPGCharacter        (Player — ASC lives on PlayerState)
    └── AARPGEnemyCharacter       (Enemy  — ASC lives on Pawn)
```

### Why ASC on PlayerState for the Player

The player's `AbilitySystemComponent` lives on `ARPGPlayerState`, not the pawn. This is a deliberate architectural choice:

- **Persistence across respawns** — attributes and active effects survive pawn destruction
- **UE5 convention** — matches Epic's recommended pattern for multiplayer-ready projects
- **Clean ownership** — `PlayerState` is the logical "owner" of persistent player data

The pawn caches a `TWeakObjectPtr<UAbilitySystemComponent>` for quick access. Initialization is split:

- **Server** — `PossessedBy` → `ARPGPlayerState::PushASCToPawn` → `InitAbilityActorInfo(PlayerState, Pawn)`
- **Client** — `OnRep_PlayerState` → same path

### Why ASC on Pawn for Enemies

Enemies don't persist across respawns. Putting the ASC on the pawn avoids the three-pointer complexity (Controller → Character → PlayerState) that would add unnecessary overhead for AI actors.

---

## Attribute System

`UBasicAttributeSet` covers all combat-relevant stats:

| Attribute | Purpose |
|-----------|---------|
| `Health` | Current health — depleted by damage GEs |
| `MaxHealth` | Health ceiling — clamped against in `PreAttributeChange` |
| `Mana` | Resource consumed by abilities |
| `MaxMana` | Mana ceiling |
| `WeaponAttackSpeed` | Post-local-modifier weapon speed (W in the formula) |
| `GlobalAttackSpeedModifier` | Sum of all global % speed modifiers (G in the formula) |

### Clamping Strategy

Two-stage clamping prevents invalid attribute states:

- **`PreAttributeChange`** — clamps incoming values before they're applied (fast path, no GE context)
- **`PostGameplayEffectExecute`** — re-clamps after GE execution and triggers death detection

`GlobalAttackSpeedModifier` has a floor of `-0.9` — below this, the attack speed formula `(1+G)` approaches zero and produces infinite duration.

### Death Detection

`OnHealthDepleted` fires from `PostGameplayEffectExecute` when Health reaches 0. It applies `State.Dead` as a loose tag to the ASC and broadcasts `FOnDeath` — the character class responds by enabling ragdoll physics without the AttributeSet needing to know anything about character presentation.

### Default Attributes

`GE_DefaultAttributes` (Instant GE) initializes all attributes at possession. **Modifier order matters** — `MaxHealth` and `MaxMana` must be applied before `Health` and `Mana` to prevent `PreAttributeChange` from clamping against a zero max.

---

## Ability System

### Base Class — `UARPGAbilityBase`

All abilities derive from `UARPGAbilityBase`. Tag properties are initialized in `PostInitProperties`, not the constructor, to respect Blueprint CDO timing — constructor tag assignments are overwritten by Blueprint defaults before the CDO is finalized.

### Ability Activation Pattern

Abilities declare `AbilityTriggers` with `Event.Ability.*` tags. Input fires `SendGameplayEventToActor` — the ability listens and activates. This means:

- Input system never holds ability references
- Abilities can be triggered by any system (AI, scripted events, other abilities)
- No `TryActivateAbilitiesByTag` calls anywhere

### Cooldowns and Costs

- **Cooldowns** — `CooldownGameplayEffectClass` on the ability. GAS `CheckCooldown` handles blocking automatically — no manual `ActivationBlockedTags` needed.
- **Mana cost** — `CostGameplayEffectClass` set to `GE_LeapSlamManaCost`. `CommitAbility` applies both cooldown and cost atomically.

---

## Leap Slam — Deep Dive

### Attack Speed Formula

Derived from Path of Exile's Leap Slam mechanics:

```
Duration = (1/W + 0.55) / (1+G)
```

Where:
- **W** = `WeaponAttackSpeed` (post-local-modifier weapon speed, set by `GE_WeaponStats`)
- **G** = `GlobalAttackSpeedModifier` (sum of all global % modifiers)
- **0.55** = flat recovery constant added after local modifiers

Higher attack speed shortens duration — the character jumps faster and covers distance more quickly, matching PoE's documented behavior exactly.

### Why This Formula

> *"Attack speed directly reduces the total animation time of Leap Slam, causing the character to jump faster and cover distance more quickly."*

The formula captures this by making duration inversely proportional to attack speed. A high-attack-speed build feels noticeably snappier. The 0.55 constant provides a minimum floor that prevents the leap from becoming instant at extreme values.

### Distance Scaling

In PoE, Leap Slam always runs at the same speed regardless of distance. This is implemented correctly — `AbilityTask_LeapMovement` uses a fixed duration and linearly interpolates XY position, meaning longer distances produce faster apparent movement.

Arc height is derived as `Distance × ArcHeightRatio` — this keeps the parabola's visual silhouette proportionally consistent at any leap range.

### Three-Phase Montage

```
[Start] → [Loop] → [End]
```

- **Start** — windup/liftoff animation. An `UAN_SendGameplayEvent` notify at the liftoff frame fires `Event.LeapSlam.Liftoff`
- **Loop** — airborne animation. Playrate adjusted to `LoopLength / Duration` so it fills exactly the formula-computed air time
- **End** — landing/impact. Plays at `1.0` playrate — landing impact has fixed weight regardless of attack speed

This gives Start and End physical authenticity while Loop correctly reflects attack speed investment.

### Ability Phase Tracking

```cpp
enum class ELeapSlamPhase : uint8
{
    Idle, Start, Airborne, Landing, End
};
```

`OnMontageCompleted` only calls `EndAbility` when `CurrentPhase == End`. Earlier phases completing (e.g. Start section edge cases) are logged as warnings without incorrectly ending the ability.

### AbilityTask — `UAbilityTask_LeapMovement`

Owns all movement during the leap:

- `Activate()` — captures `StartLocation` post-Start-animation (character may have shifted), computes `ArcHeight` from distance, disables capsule collision, sets `MOVE_Flying`
- `TickTask()` — drives parabolic arc: linear XY interpolation + `Sin(Alpha * PI)` Z offset
- `OnDestroy()` — restores collision, `MOVE_Walking`, input

`SetAnimRootMotionTranslationScale(0.f)` suppresses root motion in the animation sequences, giving the task full positional authority.

### Landing Effects

On `OnLeapCompleted`:

1. **`ApplyLandingDamage`** — sphere overlap at landing location, distance-based falloff, GE application via SetByCaller, knockback via `LaunchCharacter`, stun if target at full life
2. **`ExecuteGameplayCue`** — `GameplayCue.LeapSlam.Landing` fires `UGameplayCue_LeapSlamLanding` for particles, sound, camera shake

Gameplay logic and cosmetics are strictly separated — the cue never affects game state.

### Target Location Pipeline

```
GetHitResultUnderCursor      → raw cursor hit
    → clamp to MaxLeapDistance
        → ground trace (+500Z to -1000Z)
            → offset by capsule half height
                → TargetLocation passed to task
```

The ground trace prevents the character from floating or clipping into uneven terrain.

---

## Weapon System

### `FWeaponData`

Carries all weapon configuration:

```cpp
FGameplayTag WeaponTypeTag;          // Weapon.Type.Sword
float WeaponAttackSpeed;             // W in the formula
float LocalAttackSpeedModifier;      // pre-baked into W via GE
TSubclassOf<AWeaponActor> WeaponActorClass;
FName AttachSocket;                  // hand_r
TSubclassOf<UAnimInstance> AnimBPClass;
```

### `UEquipWeaponAbility`

Triggered by `Event.Weapon.Equip` via `SendGameplayEventToActor`. Reads `UWeaponPickupPayload` from `FGameplayEventData::OptionalObject`:

1. Removes previous weapon GE via cached `FActiveGameplayEffectHandle`
2. Applies `GE_WeaponStats` with `SetByCaller` for `WeaponAttackSpeed`
3. Grants `Weapon.Type.*` tag via `DynamicGrantedTags` — lives and dies with the GE
4. Spawns and attaches `AWeaponActor` to the hand socket
5. Fires `Event.Weapon.Equipped` with spawned actor as `OptionalObject`

### `GE_WeaponStats`

Infinite duration GE:
- **Modifier** — `WeaponAttackSpeed`, Override, SetByCaller `Data.WeaponAttackSpeed`
- **DynamicGrantedTags** — `Weapon.Type.Sword` (set in ability code, not the asset)

### Default Weapon Provisioning

At `PossessedBy`, `GrantDefaultWeapons` fires `Event.Weapon.Equip` for each entry in `DefaultWeapons` — the same code path as a world pickup. No special-casing for default vs picked-up weapons.

### Post-Equip AnimBP Switch

`AGAS_ARPGCharacter` listens for `Event.Weapon.Equipped` via `GenericGameplayEventCallbacks`. On receipt, it calls `SetAnimInstanceClass` using `AnimBPClass` from the spawned `AWeaponActor`. This keeps the character's AnimBP always in sync with the equipped weapon.

---

## Input System

### `FAbilitySlotConfig`

```cpp
TObjectPtr<UInputAction> InputAction;   // IA_Slot1, IA_Slot2
FGameplayTag AbilityTag;                // Event.Ability.LeapSlam
TSubclassOf<UAnimInstance> AnimBPClass; // ABP_LeapSlam
```

Configured entirely in Blueprint — no C++ changes needed to add new ability slots.

### Slot Binding

```cpp
for (int32 i = 0; i < AbilitySlots.Num(); i++)
{
    EIC->BindAction(AbilitySlots[i].InputAction, ETriggerEvent::Triggered,
        this, &AGAS_ARPGCharacter::Input_SelectWeapon, i);
}
```

A single handler receives the slot index as a payload — O(1) regardless of slot count.

### Switch Guard

```cpp
bool AGAS_ARPGCharacter::CanSwitchAbility() const
{
    return !ASC->HasMatchingGameplayTag(AttackingTag); // State.Attacking
}
```

`State.Attacking` is applied automatically by `ActivationOwnedTags` on the ability — no manual tag management. The tag is removed when `EndAbility` fires, re-enabling switching.

---

## Enemy AI System

### Team System

```cpp
enum class ETeam : uint8 { None = 0, Player = 1, Enemy = 2 };
```

Both `AGAS_ARPGCharacter` and `AARPGEnemyCharacter` implement `IGenericTeamAgentInterface`. `AARPGAIController` implements `GetTeamAttitudeTowards` — enemies are Hostile to players, Friendly to other enemies. This drives AI Perception affiliation filtering without any tag checks.

### Perception

Each enemy has a `UAIPerceptionComponent` with `UAISenseConfig_Sight`:

- Sight and lose-sight radii configured in Blueprint
- `PeripheralVisionAngleDegrees` set to a front-facing cone (~60°) — enemies don't have eyes in the back of their head
- `DetectionByAffiliation` — detects enemies (players) only, filtered by team attitude

On `OnTargetPerceptionUpdated`, the enemy reports to `AEnemyGroupManager` rather than making independent decisions.

### `AEnemyGroupManager`

Placed in the level. Owns the group's state machine and coordinates all enemies. This ensures only one enemy attacks at a time and the group behaves as a coherent unit rather than a chaotic swarm.

### Group State Machine

```
Idle
  → player detected within EngageRadius → Circling

Circling (CircleDuration seconds)
  → circle offset rotates at 15°/s
  → slot positions update every tick following player
  → timer expires → Halting

Halting (HaltDuration seconds)
  → all enemies stop
  → SelectNextAttacker (random alive enemy)
  → timer expires → AttackWindup

AttackWindup (AttackWindupDuration seconds)
  → attacker faces player, telegraphs attack
  → timer expires → Attacking

Attacking
  → attacker chases player position (throttled 0.2s)
  → reaches AttackRange → FallBackToSlot → AttackRecovery

AttackRecovery (AttackRecoveryDuration seconds)
  → attacker returns to circle slot
  → timer expires → Circling

Any state + player beyond BreakRadius → Idle
```

### Dynamic Circle Distribution

Slot angles recalculate whenever group size changes:

```
3 enemies → 120° apart
4 enemies → 90° apart
5 enemies → 72° apart
```

The entire ring rotates slowly while circling (`CircleOffset += DeltaTime * 15°`), preventing the formation from looking static. All positions update every tick tracking the player — if the player runs, the circle follows.

### Attack Selection

Fully configurable in Blueprint:
- `MinAttackInterval` / `MaxAttackInterval` — random interval between attacks
- One attacker at a time — next selection waits for recovery to complete
- On attacker death — `OnEnemyDied` resets selection and recalculates slots

---

## Key Design Decisions

### Event-Driven Over Direct Activation

`TryActivateAbilitiesByTag` creates coupling between the caller and the ability system. `SendGameplayEventToActor` treats ability activation as a message — any system can send it, and the ability decides whether to respond. This makes abilities independently testable and AI-triggerable without input changes.

### PlayerState ASC Ownership

Discussed above. The cost is initialization complexity (`PossessedBy` + `OnRep_PlayerState`). The benefit is attribute persistence and multiplayer correctness. For a singleplayer prototype this is overengineering — but it establishes correct foundations for expansion.

### `PostInitProperties` for Tag Initialization

Blueprint CDO defaults aren't available at constructor time. Tags initialized in the constructor are silently overwritten. `PostInitProperties` runs after Blueprint defaults are applied — the correct hook for anything that should be Blueprint-overridable.

### `DynamicGrantedTags` Over Asset Tags

`Weapon.Type.Sword` is granted via `DynamicGrantedTags` on the GE spec in code rather than baked into the `GE_WeaponStats` asset. This means one GE asset works for all weapon types — the type tag is set at runtime from `FWeaponData.WeaponTypeTag`.

### `ELeapSlamPhase` Over Boolean Flags

A boolean `bLandingApplied` can only express two states. A phase enum expresses the full lifecycle cleanly, makes `OnMontageCompleted` unambiguous, and extends naturally when new phases are needed (e.g. a charged variant).

### `AbilityTask_LeapMovement` vs `FRootMotionSource`

`FRootMotionSource_JumpForce` provides a parabolic jump but its duration and arc are tied to velocity, not a formula. A custom ticking task gives precise control over duration, arc height scaling, collision handling, and root motion suppression — all necessary for the attack speed formula to feel correct.

---

## Future Implementations

### Combat

- **`UBasicAttackAbility`** — `UAbilityTask_TickTrace` performs per-frame capsule sweeps between weapon sockets during AnimNotify-driven hit windows. `HitActors` cache prevents multi-hit on the same target per swing.
- **Combo System** — chained basic attacks with increasing damage multipliers, reset on timeout or hit stop.
- **Hit Stop** — brief time dilation on successful hits for impact feedback.
- **Parry / Block** — `ActivationBlockedTags` prevents activation during block; incoming damage GEs check for `State.Blocking` tag.

### Abilities

- **Charged Leap Slam** — hold to charge, release to leap. Charge time scales arc height and landing radius. New `ELeapSlamPhase::Charging` state in the existing phase enum.
- **Dash** — short-range instant repositioning, shares the event-driven activation pattern.
- **Area Denial** — ground-targeted persistent AoE using `UGameplayCueNotify_Actor` for the visual lifetime.

### Weapon System

- **Full `AWeaponPickup` Flow** — proximity overlap + input confirm, already architected. `UEquipWeaponAbility` already handles the payload — the pickup just needs to fire the event.
- **Weapon Switching Mid-Combat** — blocked by `State.Attacking` tag, already implemented.
- **Dual Wield** — two `FWeaponData` slots, two attach sockets, blend tree in AnimBP.

### Attributes and Progression

- **Experience and Leveling** — `GE_LevelUp` (Instant) scales base attributes via curve tables.
- **Status Effects** — Burn, Freeze, Shock as duration GEs with `State.Burning` etc. tags. Interactions (Freeze + Shatter) driven by tag presence checks in `PostGameplayEffectExecute`.
- **Resistance Attributes** — `FireResistance`, `PhysicalResistance` — damage GEs read these as execution calculation inputs.

### Enemy AI

- **Full Attack Ability** — `UEnemyMeleeAbility` fires during `EGroupState::Attacking` when in range, using the same GAS pattern as player abilities.
- **Investigate State** — enemies move to last known player position, search the area, return to spawn if player not found.
- **Multiple Group Managers** — each manages its own territory. Groups become aware of each other when territories overlap.
- **Perception Sharing** — when one enemy spots the player, it can alert nearby group members that haven't seen the player yet.
- **Flanking** — during `AttackWindup`, the attacker repositions to approach from outside the player's camera cone.

### Systems

- **UI / HUD** — Health and Mana bars bound directly to `UBasicAttributeSet` delegates — no polling.
- **GameplayCue Library** — `GameplayCue.LeapSlam.Landing` with Niagara impact, radial camera shake, layered audio.
- **Damage Numbers** — spawned as `UGameplayCueNotify_Actor` at hit location, floats upward and fades.
- **Save System** — serialize AttributeSet base values and active GE handles to persistent storage.

---

## Project Structure

```
Source/GAS_ARPG/
    ├── Character/
    │   ├── ARPGCharacterBase          — shared base, ASC interface, death
    │   ├── GAS_ARPGCharacter          — player, PlayerState ASC
    │   └── ARPGEnemyCharacter         — enemy, pawn ASC, perception
    ├── Player/
    │   └── RPGPlayerState             — owns player ASC and AttributeSet
    ├── GameplayAbilitySystem/
    │   ├── Abilities/
    │   │   ├── ARPGAbilityBase        — shared base, PostInitProperties tags
    │   │   ├── LeapSlamAbility        — leap slam, phase tracking, montage
    │   │   ├── EquipWeaponAbility     — GE application, weapon spawn
    │   │   └── BasicAttackAbility     — (planned) tick trace sweep
    │   ├── Tasks/
    │   │   └── AbilityTask_LeapMovement — parabolic arc, root motion suppression
    │   ├── AttributeSets/
    │   │   └── BasicAttributeSet      — Health, Mana, AttackSpeed
    │   └── GameplayCues/
    │       └── GameplayCue_LeapSlamLanding
    ├── Items/Weapon/
    │   ├── WeaponData                 — FWeaponData struct
    │   ├── WeaponPickup               — world pickup actor
    │   ├── WeaponPickupPayload        — decoupled UObject payload
    │   └── WeaponActor                — visual only, socket provider
    ├── AI/
    │   ├── EnemyGroupManager          — group state machine, slot distribution
    │   └── ARPGAIController           — team attitude solver
    ├── Animation/
    │   └── AN_SendGameplayEvent       — reusable notify, configurable tag
    └── Teams/
        └── ARPGTeams                  — ETeam enum, GetTeamFromActor helper
```

---

## References

- [Path of Exile — Leap Slam Wiki](https://www.poewiki.net/wiki/Leap_Slam#Skill_functions_and_interactions)
- [Unreal Engine — Gameplay Ability System Documentation](https://docs.unrealengine.com/5.0/en-US/gameplay-ability-system-for-unreal-engine/)
- [tranek/GASDocumentation](https://github.com/tranek/GASDocumentation)