/*	EQEMu: Everquest Server Emulator
	Copyright (C) 2001-2002 EQEMu Development Team (http://eqemu.org)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#ifndef MOB_H
#define MOB_H

#include "common.h"
#include "entity.h"
#include "hate_list.h"
#include "pathing.h"
#include "position.h"
#include <set>
#include <vector>
#include <memory>

char* strn0cpy(char* dest, const char* source, uint32 size);

#define MAX_SPECIAL_ATTACK_PARAMS 8

class EGNode;
class Client;
class EQApplicationPacket;
class Group;
class ItemInst;
class NPC;
class Raid;
struct Item_Struct;
struct NewSpawn_Struct;
struct PlayerPositionUpdateServer_Struct;

class Mob : public Entity {
public:
	enum CLIENT_CONN_STATUS { CLIENT_CONNECTING, CLIENT_CONNECTED, CLIENT_LINKDEAD,
						CLIENT_KICKED, DISCONNECTED, CLIENT_ERROR, CLIENT_CONNECTINGALL };
	enum eStandingPetOrder { SPO_Follow, SPO_Sit, SPO_Guard };

	struct SpecialAbility {
		SpecialAbility() {
			level = 0;
			timer = nullptr;
			for(int i = 0; i < MAX_SPECIAL_ATTACK_PARAMS; ++i) {
				params[i] = 0;
			}
		}

		~SpecialAbility() {
			safe_delete(timer);
		}

		int level;
		Timer *timer;
		int params[MAX_SPECIAL_ATTACK_PARAMS];
	};

	Mob(const char*	in_name,
		const char*	in_lastname,
		int32		in_cur_hp,
		int32		in_max_hp,
		uint8		in_gender,
		uint16		in_race,
		uint8		in_class,
		bodyType	in_bodytype,
		uint8		in_deity,
		uint8		in_level,
		uint32		in_npctype_id,
		float		in_size,
		float		in_runspeed,
		const glm::vec4& position,
		uint8		in_light,
		uint8		in_texture,
		uint8		in_helmtexture,
		uint16		in_ac,
		uint16		in_atk,
		uint16		in_str,
		uint16		in_sta,
		uint16		in_dex,
		uint16		in_agi,
		uint16		in_int,
		uint16		in_wis,
		uint16		in_cha,
		uint8		in_haircolor,
		uint8		in_beardcolor,
		uint8		in_eyecolor1, // the eyecolors always seem to be the same, maybe left and right eye?
		uint8		in_eyecolor2,
		uint8		in_hairstyle,
		uint8		in_luclinface,
		uint8		in_beard,
		uint32		in_drakkin_heritage,
		uint32		in_drakkin_tattoo,
		uint32		in_drakkin_details,
		uint32		in_armor_tint[_MaterialCount],
		uint8		in_aa_title,
		uint8		in_see_invis, // see through invis
		uint8		in_see_invis_undead, // see through invis vs. undead
		uint8		in_see_hide,
		uint8		in_see_improved_hide,
		int32		in_hp_regen,
		int32		in_mana_regen,
		uint8		in_qglobal,
		uint8		in_maxlevel,
		uint32		in_scalerate,
		uint8		in_armtexture,
		uint8		in_bracertexture,
		uint8		in_handtexture,
		uint8		in_legtexture,
		uint8		in_feettexture
	);
	virtual ~Mob();

	inline virtual bool IsMob() const { return true; }
	inline virtual bool InZone() const { return true; }

	//Somewhat sorted: needs documenting!

	//Attack
	virtual void RogueBackstab(Mob* other, bool min_damage = false, int ReuseTime = 10);
	virtual void RogueAssassinate(Mob* other);
	float MobAngle(Mob *other = 0, float ourx = 0.0f, float oury = 0.0f) const;
	// greater than 90 is behind
	inline bool BehindMob(Mob *other = 0, float ourx = 0.0f, float oury = 0.0f) const
		{ return (!other || other == this) ? true : MobAngle(other, ourx, oury) > 90.0f; }
	// less than 56 is in front, greater than 56 is usually where the client generates the messages
	inline bool InFrontMob(Mob *other = 0, float ourx = 0.0f, float oury = 0.0f) const
		{ return (!other || other == this) ? true : MobAngle(other, ourx, oury) < 56.0f; }
	bool IsFacingMob(Mob *other); // kind of does the same as InFrontMob, but derived from client
	float HeadingAngleToMob(Mob *other); // to keep consistent with client generated messages
	virtual void RangedAttack(Mob* other) { }
	virtual void ThrowingAttack(Mob* other) { }
	uint16 GetThrownDamage(int16 wDmg, int32& TotalDmg, int& minDmg);
	// 13 = Primary (default), 14 = secondary
	virtual bool Attack(Mob* other, int Hand = MainPrimary, bool FromRiposte = false, bool IsStrikethrough = false,
		bool IsFromSpell = false, ExtraAttackOptions *opts = nullptr) = 0;
	int MonkSpecialAttack(Mob* other, uint8 skill_used);
	virtual void TryBackstab(Mob *other,int ReuseTime = 10);
	void TriggerDefensiveProcs(const ItemInst* weapon, Mob *on, uint16 hand = MainPrimary, int damage = 0);
	virtual bool AvoidDamage(Mob* attacker, int32 &damage, bool CanRiposte = true);
	virtual bool CheckHitChance(Mob* attacker, SkillUseTypes skillinuse, int Hand, int16 chance_mod = 0);
	virtual void TryCriticalHit(Mob *defender, uint16 skill, int32 &damage, ExtraAttackOptions *opts = nullptr);
	void TryPetCriticalHit(Mob *defender, uint16 skill, int32 &damage);
	virtual bool TryFinishingBlow(Mob *defender, SkillUseTypes skillinuse);
	uint32 TryHeadShot(Mob* defender, SkillUseTypes skillInUse);
	uint32 TryAssassinate(Mob* defender, SkillUseTypes skillInUse, uint16 ReuseTime);
	virtual void DoRiposte(Mob* defender);
	void ApplyMeleeDamageBonus(uint16 skill, int32 &damage);
	virtual void MeleeMitigation(Mob *attacker, int32 &damage, int32 minhit, ExtraAttackOptions *opts = nullptr);
	virtual int32 GetMeleeMitDmg(Mob *attacker, int32 damage, int32 minhit, float mit_rating, float atk_rating);
	bool CombatRange(Mob* other);
	virtual inline bool IsBerserk() { return false; } // only clients
	void RogueEvade(Mob *other);
	void CommonOutgoingHitSuccess(Mob* defender, int32 &damage, SkillUseTypes skillInUse);
	void CommonBreakInvisible();
	bool HasDied();

	//Appearance
	void SendLevelAppearance();
	void SendStunAppearance();
	void SendAppearanceEffect(uint32 parm1, uint32 parm2, uint32 parm3, uint32 parm4, uint32 parm5,
		Client *specific_target=nullptr);
	void SendTargetable(bool on, Client *specific_target = nullptr);
	virtual void SendWearChange(uint8 material_slot);
	virtual void SendTextureWC(uint8 slot, uint16 texture, uint32 hero_forge_model = 0, uint32 elite_material = 0,
		uint32 unknown06 = 0, uint32 unknown18 = 0);
	virtual void SetSlotTint(uint8 material_slot, uint8 red_tint, uint8 green_tint, uint8 blue_tint);
	virtual void WearChange(uint8 material_slot, uint16 texture, uint32 color, uint32 hero_forge_model = 0);
	void DoAnim(const int animnum, int type=0, bool ackreq = true, eqFilterType filter = FilterNone);
	void ProjectileAnimation(Mob* to, int item_id, bool IsArrow = false, float speed = 0,
		float angle = 0, float tilt = 0, float arc = 0, const char *IDFile = nullptr, SkillUseTypes skillInUse = SkillArchery);
	void ChangeSize(float in_size, bool bNoRestriction = false);
	inline uint8 SeeInvisible() const { return see_invis; }
	inline bool SeeInvisibleUndead() const { return see_invis_undead; }
	inline bool SeeHide() const { return see_hide; }
	inline bool SeeImprovedHide() const { return see_improved_hide; }
	bool IsInvisible(Mob* other = 0) const;
	void SetInvisible(uint8 state);
	bool AttackAnimation(SkillUseTypes &skillinuse, int Hand, const ItemInst* weapon);

	//Song
	bool UseBardSpellLogic(uint16 spell_id = 0xffff, int slot = -1);
	bool ApplyNextBardPulse(uint16 spell_id, Mob *spell_target, uint16 slot);
	void BardPulse(uint16 spell_id, Mob *caster);

	//Spell
	void SendSpellEffect(uint32 effectid, uint32 duration, uint32 finish_delay, bool zone_wide,
		uint32 unk020, bool perm_effect = false, Client *c = nullptr);
	bool IsBeneficialAllowed(Mob *target);
	virtual int GetCasterLevel(uint16 spell_id);
	void ApplySpellsBonuses(uint16 spell_id, uint8 casterlevel, StatBonuses* newbon, uint16 casterID = 0,
		uint8 WornType = 0, uint32 ticsremaining = 0, int buffslot = -1,
		bool IsAISpellEffect = false, uint16 effect_id = 0, int32 se_base = 0, int32 se_limit = 0, int32 se_max = 0);
	void NegateSpellsBonuses(uint16 spell_id);
	virtual float GetActSpellRange(uint16 spell_id, float range, bool IsBard = false);
	virtual int32 GetActSpellDamage(uint16 spell_id, int32 value, Mob* target = nullptr);
	virtual int32 GetActDoTDamage(uint16 spell_id, int32 value, Mob* target);
	virtual int32 GetActSpellHealing(uint16 spell_id, int32 value, Mob* target = nullptr);
	virtual int32 GetActSpellCost(uint16 spell_id, int32 cost){ return cost;}
	virtual int32 GetActSpellDuration(uint16 spell_id, int32 duration);
	virtual int32 GetActSpellCasttime(uint16 spell_id, int32 casttime);
	float ResistSpell(uint8 resist_type, uint16 spell_id, Mob *caster, bool use_resist_override = false,
		int resist_override = 0, bool CharismaCheck = false, bool CharmTick = false, bool IsRoot = false);
	int ResistPhysical(int level_diff, uint8 caster_level);
	uint16 GetSpecializeSkillValue(uint16 spell_id) const;
	void SendSpellBarDisable();
	void SendSpellBarEnable(uint16 spellid);
	void ZeroCastingVars();
	virtual void SpellProcess();
	virtual bool CastSpell(uint16 spell_id, uint16 target_id, uint16 slot = USE_ITEM_SPELL_SLOT, int32 casttime = -1,
		int32 mana_cost = -1, uint32* oSpellWillFinish = 0, uint32 item_slot = 0xFFFFFFFF,
		uint32 timer = 0xFFFFFFFF, uint32 timer_duration = 0, uint32 type = 0, int16 *resist_adjust = nullptr);
	virtual bool DoCastSpell(uint16 spell_id, uint16 target_id, uint16 slot = 10, int32 casttime = -1,
		int32 mana_cost = -1, uint32* oSpellWillFinish = 0, uint32 item_slot = 0xFFFFFFFF,
		uint32 timer = 0xFFFFFFFF, uint32 timer_duration = 0, uint32 type = 0, int16 resist_adjust = 0);
	void CastedSpellFinished(uint16 spell_id, uint32 target_id, uint16 slot, uint16 mana_used,
		uint32 inventory_slot = 0xFFFFFFFF, int16 resist_adjust = 0);
	bool SpellFinished(uint16 spell_id, Mob *target, uint16 slot = 10, uint16 mana_used = 0,
		uint32 inventory_slot = 0xFFFFFFFF, int16 resist_adjust = 0, bool isproc = false);
	virtual bool SpellOnTarget(uint16 spell_id, Mob* spelltar, bool reflect = false,
		bool use_resist_adjust = false, int16 resist_adjust = 0, bool isproc = false);
	virtual bool SpellEffect(Mob* caster, uint16 spell_id, float partial = 100);
	virtual bool DetermineSpellTargets(uint16 spell_id, Mob *&spell_target, Mob *&ae_center,
		CastAction_type &CastAction);
	virtual bool CheckFizzle(uint16 spell_id);
	virtual bool CheckSpellLevelRestriction(uint16 spell_id);
	virtual bool IsImmuneToSpell(uint16 spell_id, Mob *caster);
	virtual float GetAOERange(uint16 spell_id);
	void InterruptSpell(uint16 spellid = SPELL_UNKNOWN);
	void InterruptSpell(uint16, uint16, uint16 spellid = SPELL_UNKNOWN);
	inline bool IsCasting() const { return((casting_spell_id != 0)); }
	uint16 CastingSpellID() const { return casting_spell_id; }
	bool DoCastingChecks();
	bool TryDispel(uint8 caster_level, uint8 buff_level, int level_modifier);
	bool TrySpellProjectile(Mob* spell_target,  uint16 spell_id, float speed = 1.5f);
	void ResourceTap(int32 damage, uint16 spell_id);
	void TryTriggerThreshHold(int32 damage, int effect_id, Mob* attacker);
	bool CheckSpellCategory(uint16 spell_id, int category_id, int effect_id);
	void CalcDestFromHeading(float heading, float distance, float MaxZDiff, float StartX, float StartY, float &dX, float &dY, float &dZ);
	void BeamDirectional(uint16 spell_id, int16 resist_adjust);
	void ConeDirectional(uint16 spell_id, int16 resist_adjust);

	//Buff
	void BuffProcess();
	virtual void DoBuffTic(uint16 spell_id, int slot, uint32 ticsremaining, uint8 caster_level, Mob* caster = 0);
	void BuffFadeBySpellID(uint16 spell_id);
	void BuffFadeByEffect(int effectid, int skipslot = -1);
	void BuffFadeAll();
	void BuffFadeNonPersistDeath();
	void BuffFadeDetrimental();
	void BuffFadeBySlot(int slot, bool iRecalcBonuses = true);
	void BuffFadeDetrimentalByCaster(Mob *caster);
	void BuffFadeBySitModifier();
	void BuffModifyDurationBySpellID(uint16 spell_id, int32 newDuration);
	int AddBuff(Mob *caster, const uint16 spell_id, int duration = 0, int32 level_override = -1);
	int CanBuffStack(uint16 spellid, uint8 caster_level, bool iFailIfOverwrite = false);
	int CalcBuffDuration(Mob *caster, Mob *target, uint16 spell_id, int32 caster_level_override = -1);
	void SendPetBuffsToClient();
	virtual int GetCurrentBuffSlots() const { return 0; }
	virtual int GetCurrentSongSlots() const { return 0; }
	virtual int GetCurrentDiscSlots() const { return 0; }
	virtual int GetMaxBuffSlots() const { return 0; }
	virtual int GetMaxSongSlots() const { return 0; }
	virtual int GetMaxDiscSlots() const { return 0; }
	virtual int GetMaxTotalSlots() const { return 0; }
	virtual void InitializeBuffSlots() { buffs = nullptr; current_buff_count = 0; }
	virtual void UninitializeBuffSlots() { }
	EQApplicationPacket *MakeBuffsPacket(bool for_target = true);
	void SendBuffsToClient(Client *c);
	inline Buffs_Struct* GetBuffs() { return buffs; }
	void DoGravityEffect();
	void DamageShield(Mob* other, bool spell_ds = false);
	int32 RuneAbsorb(int32 damage, uint16 type);
	bool FindBuff(uint16 spellid);
	bool FindType(uint16 type, bool bOffensive = false, uint16 threshold = 100);
	int16 GetBuffSlotFromType(uint16 type);
	uint16 GetSpellIDFromSlot(uint8 slot);
	int CountDispellableBuffs();
	void CheckNumHitsRemaining(NumHit type, int32 buff_slot = -1, uint16 spell_id = SPELL_UNKNOWN);
	bool HasNumhits() const { return has_numhits; }
	inline void Numhits(bool val) { has_numhits = val; }
	bool HasMGB() const { return has_MGB; }
	inline void SetMGB(bool val) { has_MGB = val; }
	bool HasProjectIllusion() const { return has_ProjectIllusion ; }
	inline void SetProjectIllusion(bool val) { has_ProjectIllusion  = val; }
	void SpreadVirus(uint16 spell_id, uint16 casterID);
	bool IsNimbusEffectActive(uint32 nimbus_effect);
	void SetNimbusEffect(uint32 nimbus_effect);
	inline virtual uint32 GetNimbusEffect1() const { return nimbus_effect1; }
	inline virtual uint32 GetNimbusEffect2() const { return nimbus_effect2; }
	inline virtual uint32 GetNimbusEffect3() const { return nimbus_effect3; }
	void RemoveNimbusEffect(int effectid);
	inline const glm::vec3& GetTargetRingLocation() const { return m_TargetRing; }
	inline float GetTargetRingX() const { return m_TargetRing.x; }
	inline float GetTargetRingY() const { return m_TargetRing.y; }
	inline float GetTargetRingZ() const { return m_TargetRing.z; }
	inline bool HasEndurUpkeep() const { return endur_upkeep; }
	inline void SetEndurUpkeep(bool val) { endur_upkeep = val; }

	//Basic Stats/Inventory
	virtual void SetLevel(uint8 in_level, bool command = false) { level = in_level; }
	void TempName(const char *newname = nullptr);
	void SetTargetable(bool on);
	bool IsTargetable() const { return m_targetable; }
	bool HasShieldEquiped() const { return has_shieldequiped; }
	inline void SetShieldEquiped(bool val) { has_shieldequiped = val; }
	bool HasTwoHandBluntEquiped() const { return has_twohandbluntequiped; }
	inline void SetTwoHandBluntEquiped(bool val) { has_twohandbluntequiped = val; }
	virtual uint16 GetSkill(SkillUseTypes skill_num) const { return 0; }
	virtual uint32 GetEquipment(uint8 material_slot) const { return(0); }
	virtual int32 GetEquipmentMaterial(uint8 material_slot) const;
	virtual int32 GetHerosForgeModel(uint8 material_slot) const;
	virtual uint32 GetEquipmentColor(uint8 material_slot) const;
	virtual uint32 IsEliteMaterialItem(uint8 material_slot) const;
	bool CanClassEquipItem(uint32 item_id);
	bool AffectedBySpellExcludingSlot(int slot, int effect);
	virtual bool Death(Mob* killerMob, int32 damage, uint16 spell_id, SkillUseTypes attack_skill) = 0;
	virtual void Damage(Mob* from, int32 damage, uint16 spell_id, SkillUseTypes attack_skill,
		bool avoidable = true, int8 buffslot = -1, bool iBuffTic = false) = 0;
	inline virtual void SetHP(int32 hp) { if (hp >= max_hp) cur_hp = max_hp; else cur_hp = hp;}
	bool ChangeHP(Mob* other, int32 amount, uint16 spell_id = 0, int8 buffslot = -1, bool iBuffTic = false);
	inline void SetOOCRegen(int32 newoocregen) {oocregen = newoocregen;}
	virtual void Heal();
	virtual void HealDamage(uint32 ammount, Mob* caster = nullptr, uint16 spell_id = SPELL_UNKNOWN);
	virtual void SetMaxHP() { cur_hp = max_hp; }
	virtual inline uint16 GetBaseRace() const { return base_race; }
	virtual inline uint8 GetBaseGender() const { return base_gender; }
	virtual inline uint16 GetDeity() const { return deity; }
	inline uint16 GetRace() const { return race; }
	inline uint8 GetGender() const { return gender; }
	inline uint8 GetTexture() const { return texture; }
	inline uint8 GetHelmTexture() const { return helmtexture; }
	inline uint8 GetHairColor() const { return haircolor; }
	inline uint8 GetBeardColor() const { return beardcolor; }
	inline uint8 GetEyeColor1() const { return eyecolor1; }
	inline uint8 GetEyeColor2() const { return eyecolor2; }
	inline uint8 GetHairStyle() const { return hairstyle; }
	inline uint8 GetLuclinFace() const { return luclinface; }
	inline uint8 GetBeard() const { return beard; }
	inline uint8 GetDrakkinHeritage() const { return drakkin_heritage; }
	inline uint8 GetDrakkinTattoo() const { return drakkin_tattoo; }
	inline uint8 GetDrakkinDetails() const { return drakkin_details; }
	inline uint32 GetArmorTint(uint8 i) const { return armor_tint[(i < _MaterialCount) ? i : 0]; }
	inline uint8 GetClass() const { return class_; }
	inline uint8 GetLevel() const { return level; }
	inline uint8 GetOrigLevel() const { return orig_level; }
	inline const char* GetName() const { return name; }
	inline const char* GetOrigName() const { return orig_name; }
	inline const char* GetLastName() const { return lastname; }
	const char *GetCleanName();
	virtual void SetName(const char *new_name = nullptr) { new_name ? strn0cpy(name, new_name, 64) :
		strn0cpy(name, GetName(), 64); return; };
	inline Mob* GetTarget() const { return target; }
	virtual void SetTarget(Mob* mob);
	virtual inline float GetHPRatio() const { return max_hp == 0 ? 0 : ((float)cur_hp/max_hp*100); }
	inline virtual int32 GetAC() const { return AC + itembonuses.AC + spellbonuses.AC; }
	inline virtual int32 GetATK() const { return ATK + itembonuses.ATK + spellbonuses.ATK; }
	inline virtual int32 GetATKBonus() const { return itembonuses.ATK + spellbonuses.ATK; }
	inline virtual int32 GetSTR() const { return STR + itembonuses.STR + spellbonuses.STR; }
	inline virtual int32 GetSTA() const { return STA + itembonuses.STA + spellbonuses.STA; }
	inline virtual int32 GetDEX() const { return DEX + itembonuses.DEX + spellbonuses.DEX; }
	inline virtual int32 GetAGI() const { return AGI + itembonuses.AGI + spellbonuses.AGI; }
	inline virtual int32 GetINT() const { return INT + itembonuses.INT + spellbonuses.INT; }
	inline virtual int32 GetWIS() const { return WIS + itembonuses.WIS + spellbonuses.WIS; }
	inline virtual int32 GetCHA() const { return CHA + itembonuses.CHA + spellbonuses.CHA; }
	inline virtual int32 GetMR() const { return MR + itembonuses.MR + spellbonuses.MR; }
	inline virtual int32 GetFR() const { return FR + itembonuses.FR + spellbonuses.FR; }
	inline virtual int32 GetDR() const { return DR + itembonuses.DR + spellbonuses.DR; }
	inline virtual int32 GetPR() const { return PR + itembonuses.PR + spellbonuses.PR; }
	inline virtual int32 GetCR() const { return CR + itembonuses.CR + spellbonuses.CR; }
	inline virtual int32 GetCorrup() const { return Corrup + itembonuses.Corrup + spellbonuses.Corrup; }
	inline virtual int32 GetPhR() const { return PhR; } // PhR bonuses not implemented yet
	inline StatBonuses GetItemBonuses() const { return itembonuses; }
	inline StatBonuses GetSpellBonuses() const { return spellbonuses; }
	inline StatBonuses GetAABonuses() const { return aabonuses; }
	inline virtual int32 GetMaxSTR() const { return GetSTR(); }
	inline virtual int32 GetMaxSTA() const { return GetSTA(); }
	inline virtual int32 GetMaxDEX() const { return GetDEX(); }
	inline virtual int32 GetMaxAGI() const { return GetAGI(); }
	inline virtual int32 GetMaxINT() const { return GetINT(); }
	inline virtual int32 GetMaxWIS() const { return GetWIS(); }
	inline virtual int32 GetMaxCHA() const { return GetCHA(); }
	inline virtual int32 GetMaxMR() const { return 255; }
	inline virtual int32 GetMaxPR() const { return 255; }
	inline virtual int32 GetMaxDR() const { return 255; }
	inline virtual int32 GetMaxCR() const { return 255; }
	inline virtual int32 GetMaxFR() const { return 255; }
	inline virtual int32 GetDelayDeath() const { return 0; }
	inline int32 GetHP() const { return cur_hp; }
	inline int32 GetMaxHP() const { return max_hp; }
	virtual int32 CalcMaxHP();
	inline int32 GetMaxMana() const { return max_mana; }
	inline int32 GetMana() const { return cur_mana; }
	int32 GetItemHPBonuses();
	int32 GetSpellHPBonuses();
	virtual const int32& SetMana(int32 amount);
	inline float GetManaRatio() const { return max_mana == 0 ? 100 :
		((static_cast<float>(cur_mana) / max_mana) * 100); }
	virtual int32 CalcMaxMana();
	uint32 GetNPCTypeID() const { return npctype_id; }
	inline const glm::vec4& GetPosition() const { return m_Position; }
	inline const float GetX() const { return m_Position.x; }
	inline const float GetY() const { return m_Position.y; }
	inline const float GetZ() const { return m_Position.z; }
	inline const float GetHeading() const { return m_Position.w; }
	inline const float GetSize() const { return size; }
	inline const float GetBaseSize() const { return base_size; }
	inline const float GetTarX() const { return m_TargetLocation.x; }
	inline const float GetTarY() const { return m_TargetLocation.y; }
	inline const float GetTarZ() const { return m_TargetLocation.z; }
	inline const float GetTarVX() const { return m_TargetV.x; }
	inline const float GetTarVY() const { return m_TargetV.y; }
	inline const float GetTarVZ() const { return m_TargetV.z; }
	inline const float GetTarVector() const { return tar_vector; }
	inline const uint8 GetTarNDX() const { return tar_ndx; }
	bool IsBoat() const;

	//Group
	virtual bool HasRaid() = 0;
	virtual bool HasGroup() = 0;
	virtual Raid* GetRaid() = 0;
	virtual Group* GetGroup() = 0;

	//Faction
	virtual inline int32 GetPrimaryFaction() const { return 0; }

	//Movement
	void Warp(const glm::vec3& location);
	inline bool IsMoving() const { return moving; }
	virtual void SetMoving(bool move) { moving = move; m_Delta = glm::vec4(); }
	virtual void GoToBind(uint8 bindnum = 0) { }
	virtual void Gate();
	float GetWalkspeed() const { return(_GetMovementSpeed(-47)); }
	float GetRunspeed() const { return(_GetMovementSpeed(0)); }
	float GetBaseRunspeed() const { return runspeed; }
	float GetMovespeed() const { return IsRunning() ? GetRunspeed() : GetWalkspeed(); }
	bool IsRunning() const { return m_is_running; }
	void SetRunning(bool val) { m_is_running = val; }
	virtual void GMMove(float x, float y, float z, float heading = 0.01, bool SendUpdate = true);
	void SetDelta(const glm::vec4& delta);
	void SetTargetDestSteps(uint8 target_steps) { tar_ndx = target_steps; }
	void SendPosUpdate(uint8 iSendToSelf = 0);
	void MakeSpawnUpdateNoDelta(PlayerPositionUpdateServer_Struct* spu);
	void MakeSpawnUpdate(PlayerPositionUpdateServer_Struct* spu);
	void SendPosition();
	void SetFlyMode(uint8 flymode);
	inline void Teleport(glm::vec3 NewPosition) { m_Position.x = NewPosition.x; m_Position.y = NewPosition.y;
		m_Position.z = NewPosition.z; };

	//AI
	static uint32 GetLevelCon(uint8 mylevel, uint8 iOtherLevel);
	inline uint32 GetLevelCon(uint8 iOtherLevel) const {
		return this ? GetLevelCon(GetLevel(), iOtherLevel) : CON_GREEN; }
	virtual void AddToHateList(Mob* other, uint32 hate = 0, int32 damage = 0, bool iYellForHelp = true,
		bool bFrenzy = false, bool iBuffTic = false);
	bool RemoveFromHateList(Mob* mob);
	void SetHateAmountOnEnt(Mob* other, int32 hate = 0, int32 damage = 0) { hate_list.SetHateAmountOnEnt(other,hate,damage);}
	void HalveAggro(Mob *other) { uint32 in_hate = GetHateAmount(other); SetHateAmountOnEnt(other, (in_hate > 1 ? in_hate / 2 : 1)); }
	void DoubleAggro(Mob *other) { uint32 in_hate = GetHateAmount(other); SetHateAmountOnEnt(other, (in_hate ? in_hate * 2 : 1)); }
	uint32 GetHateAmount(Mob* tmob, bool is_dam = false) { return hate_list.GetEntHateAmount(tmob,is_dam);}
	uint32 GetDamageAmount(Mob* tmob) { return hate_list.GetEntHateAmount(tmob, true);}
	Mob* GetHateTop() { return hate_list.GetEntWithMostHateOnList(this);}
	Mob* GetHateDamageTop(Mob* other) { return hate_list.GetDamageTopOnHateList(other);}
	Mob* GetHateRandom() { return hate_list.GetRandomEntOnHateList();}
	Mob* GetHateMost() { return hate_list.GetEntWithMostHateOnList();}
	bool IsEngaged() { return(!hate_list.IsHateListEmpty()); }
	bool HateSummon();
	void FaceTarget(Mob* MobToFace = 0);
	void SetHeading(float iHeading) { if(m_Position.w != iHeading) { pLastChange = Timer::GetCurrentTime();
		m_Position.w = iHeading; } }
	void WipeHateList();
	void AddFeignMemory(Client* attacker);
	void RemoveFromFeignMemory(Client* attacker);
	void ClearFeignMemory();
	void PrintHateListToClient(Client *who) { hate_list.PrintHateListToClient(who); }
	std::list<struct_HateList*>& GetHateList() { return hate_list.GetHateList(); }
	bool CheckLosFN(Mob* other);
	bool CheckLosFN(float posX, float posY, float posZ, float mobSize);
	inline void SetChanged() { pLastChange = Timer::GetCurrentTime(); }
	inline const uint32 LastChange() const { return pLastChange; }
	inline void SetLastLosState(bool value) { last_los_check = value; }
	inline bool CheckLastLosState() const { return last_los_check; }

	//Quest
	void QuestReward(Client *c = nullptr, uint32 silver = 0, uint32 gold = 0, uint32 platinum = 0);
	void CameraEffect(uint32 duration, uint32 intensity, Client *c = nullptr, bool global = false);
	inline bool GetQglobal() const { return qglobal; }

	//Other Packet
	void CreateDespawnPacket(EQApplicationPacket* app, bool Decay);
	void CreateHorseSpawnPacket(EQApplicationPacket* app, const char* ownername, uint16 ownerid, Mob* ForWho = 0);
	void CreateSpawnPacket(EQApplicationPacket* app, Mob* ForWho = 0);
	static void CreateSpawnPacket(EQApplicationPacket* app, NewSpawn_Struct* ns);
	virtual void FillSpawnStruct(NewSpawn_Struct* ns, Mob* ForWho);
	void CreateHPPacket(EQApplicationPacket* app);
	void SendHPUpdate();

	//Util
	static uint32 RandomTimer(int min, int max);
	static uint8 GetDefaultGender(uint16 in_race, uint8 in_gender = 0xFF);
	static bool IsPlayerRace(uint16 in_race);
	uint16 GetSkillByItemType(int ItemType);
	uint8 GetItemTypeBySkill(SkillUseTypes skill);
	virtual void MakePet(uint16 spell_id, const char* pettype, const char *petname = nullptr);
	virtual void MakePoweredPet(uint16 spell_id, const char* pettype, int16 petpower, const char *petname = nullptr, float in_size = 0.0f);
	bool IsWarriorClass() const;
	char GetCasterClass() const;
	uint8 GetArchetype() const;
	void SetZone(uint32 zone_id, uint32 instance_id);
	void ShowStats(Client* client);
	void ShowBuffs(Client* client);
	void ShowBuffList(Client* client);
	bool PlotPositionAroundTarget(Mob* target, float &x_dest, float &y_dest, float &z_dest,
		bool lookForAftArc = true);

	//Procs
	bool AddRangedProc(uint16 spell_id, uint16 iChance = 3, uint16 base_spell_id = SPELL_UNKNOWN);
	bool RemoveRangedProc(uint16 spell_id, bool bAll = false);
	bool HasRangedProcs() const;
	bool AddDefensiveProc(uint16 spell_id, uint16 iChance = 3, uint16 base_spell_id = SPELL_UNKNOWN);
	bool RemoveDefensiveProc(uint16 spell_id, bool bAll = false);
	bool HasDefensiveProcs() const;
	bool HasSkillProcs() const;
	bool HasSkillProcSuccess() const;
	bool AddProcToWeapon(uint16 spell_id, bool bPerma = false, uint16 iChance = 3, uint16 base_spell_id = SPELL_UNKNOWN);
	bool RemoveProcFromWeapon(uint16 spell_id, bool bAll = false);
	bool HasProcs() const;
	bool IsCombatProc(uint16 spell_id);

	//More stuff to sort:
	virtual bool IsRaidTarget() const { return false; };
	virtual bool IsAttackAllowed(Mob *target, bool isSpellAttack = false);
	bool IsTargeted() const { return (targeted > 0); }
	inline void IsTargeted(int in_tar) { targeted += in_tar; if(targeted < 0) targeted = 0;}
	void SetFollowID(uint32 id) { follow = id; }
	void SetFollowDistance(uint32 dist) { follow_dist = dist; }
	uint32 GetFollowID() const { return follow; }
	uint32 GetFollowDistance() const { return follow_dist; }

	virtual void Message(uint32 type, const char* message, ...) { }
	virtual void Message_StringID(uint32 type, uint32 string_id, uint32 distance = 0) { }
	virtual void Message_StringID(uint32 type, uint32 string_id, const char* message, const char* message2 = 0,
		const char* message3 = 0, const char* message4 = 0, const char* message5 = 0, const char* message6 = 0,
		const char* message7 = 0, const char* message8 = 0, const char* message9 = 0, uint32 distance = 0) { }
	virtual void FilteredMessage_StringID(Mob *sender, uint32 type, eqFilterType filter, uint32 string_id) { }
	virtual void FilteredMessage_StringID(Mob *sender, uint32 type, eqFilterType filter,
			uint32 string_id, const char *message1, const char *message2 = nullptr,
			const char *message3 = nullptr, const char *message4 = nullptr,
			const char *message5 = nullptr, const char *message6 = nullptr,
			const char *message7 = nullptr, const char *message8 = nullptr,
			const char *message9 = nullptr) { }
	void Say(const char *format, ...);
	void Say_StringID(uint32 string_id, const char *message3 = 0, const char *message4 = 0, const char *message5 = 0,
		const char *message6 = 0, const char *message7 = 0, const char *message8 = 0, const char *message9 = 0);
	void Say_StringID(uint32 type, uint32 string_id, const char *message3 = 0, const char *message4 = 0, const char *message5 = 0,
		const char *message6 = 0, const char *message7 = 0, const char *message8 = 0, const char *message9 = 0);
	void Shout(const char *format, ...);
	void Emote(const char *format, ...);
	void QuestJournalledSay(Client *QuestInitiator, const char *str);
	int32 GetItemStat(uint32 itemid, const char *identifier);

	int16 CalcFocusEffect(focusType type, uint16 focus_id, uint16 spell_id, bool best_focus=false);
	uint8 IsFocusEffect(uint16 spellid, int effect_index, bool AA=false,uint32 aa_effect=0);
	void SendIllusionPacket(uint16 in_race, uint8 in_gender = 0xFF, uint8 in_texture = 0xFF, uint8 in_helmtexture = 0xFF,
		uint8 in_haircolor = 0xFF, uint8 in_beardcolor = 0xFF, uint8 in_eyecolor1 = 0xFF, uint8 in_eyecolor2 = 0xFF,
		uint8 in_hairstyle = 0xFF, uint8 in_luclinface = 0xFF, uint8 in_beard = 0xFF, uint8 in_aa_title = 0xFF,
		uint32 in_drakkin_heritage = 0xFFFFFFFF, uint32 in_drakkin_tattoo = 0xFFFFFFFF,
		uint32 in_drakkin_details = 0xFFFFFFFF, float in_size = -1.0f);
	bool RandomizeFeatures(bool send_illusion = true, bool set_variables = true);
	virtual void Stun(int duration);
	virtual void UnStun();
	inline void Silence(bool newval) { silenced = newval; }
	inline void Amnesia(bool newval) { amnesiad = newval; }
	void TemporaryPets(uint16 spell_id, Mob *target, const char *name_override = nullptr, uint32 duration_override = 0, bool followme=true, bool sticktarg=false);
	void TypesTemporaryPets(uint32 typesid, Mob *target, const char *name_override = nullptr, uint32 duration_override = 0, bool followme=true, bool sticktarg=false);
	void WakeTheDead(uint16 spell_id, Mob *target, uint32 duration);
	void Spin();
	void Kill();
	bool PassCharismaCheck(Mob* caster, uint16 spell_id);
	bool TryDeathSave();
	bool TryDivineSave();
	void DoBuffWearOffEffect(uint32 index);
	void TryTriggerOnCast(uint32 spell_id, bool aa_trigger);
	void TriggerOnCast(uint32 focus_spell, uint32 spell_id, bool aa_trigger);
	bool TrySpellTrigger(Mob *target, uint32 spell_id, int effect);
	void TryTriggerOnValueAmount(bool IsHP = false, bool IsMana = false, bool IsEndur = false, bool IsPet = false);
	void TryTwincast(Mob *caster, Mob *target, uint32 spell_id);
	void TrySympatheticProc(Mob *target, uint32 spell_id);
	bool TryFadeEffect(int slot);
	uint16 GetSpellEffectResistChance(uint16 spell_id);
	int16 GetHealRate(uint16 spell_id, Mob* caster = nullptr);
	int32 GetVulnerability(Mob* caster, uint32 spell_id, uint32 ticsremaining);
	int32 GetFcDamageAmtIncoming(Mob *caster, uint32 spell_id, bool use_skill = false, uint16 skill=0);
	int32 GetFocusIncoming(focusType type, int effect, Mob *caster, uint32 spell_id);
	int16 GetSkillDmgTaken(const SkillUseTypes skill_used);
	void DoKnockback(Mob *caster, uint32 pushback, uint32 pushup);
	int16 CalcResistChanceBonus();
	int16 CalcFearResistChance();
	void TrySpellOnKill(uint8 level, uint16 spell_id);
	bool TrySpellOnDeath();
	void CastOnCurer(uint32 spell_id);
	void CastOnCure(uint32 spell_id);
	void CastOnNumHitFade(uint32 spell_id);
	void SlowMitigation(Mob* caster);
	int16 GetCritDmgMob(uint16 skill);
	int16 GetMeleeDamageMod_SE(uint16 skill);
	int16 GetMeleeMinDamageMod_SE(uint16 skill);
	int16 GetCrippBlowChance();
	int16 GetSkillReuseTime(uint16 skill);
	int16 GetCriticalChanceBonus(uint16 skill);
	int16 GetSkillDmgAmt(uint16 skill);
	bool TryReflectSpell(uint32 spell_id);
	bool CanBlockSpell() const { return(spellbonuses.BlockNextSpell); }
	bool DoHPToManaCovert(uint16 mana_cost = 0);
	int32 ApplySpellEffectiveness(Mob* caster, int16 spell_id, int32 value, bool IsBard = false);
	int8 GetDecayEffectValue(uint16 spell_id, uint16 spelleffect);
	int32 GetExtraSpellAmt(uint16 spell_id, int32 extra_spell_amt, int32 base_spell_dmg);
	void MeleeLifeTap(int32 damage);
	bool PassCastRestriction(bool UseCastRestriction = true, int16 value = 0, bool IsDamage = true);
	bool ImprovedTaunt();
	bool TryRootFadeByDamage(int buffslot, Mob* attacker);
	float GetSlowMitigation() const { return slow_mitigation; }
	void CalcSpellPowerDistanceMod(uint16 spell_id, float range, Mob* caster = nullptr);
	inline int16 GetSpellPowerDistanceMod() const { return SpellPowerDistanceMod; };
	inline void SetSpellPowerDistanceMod(int16 value) { SpellPowerDistanceMod = value; };
	int32 GetSpellStat(uint32 spell_id, const char *identifier, uint8 slot = 0);

	void ModSkillDmgTaken(SkillUseTypes skill_num, int value);
	int16 GetModSkillDmgTaken(const SkillUseTypes skill_num);
	void ModVulnerability(uint8 resist, int16 value);
	int16 GetModVulnerability(const uint8 resist);

	void SetAllowBeneficial(bool value) { m_AllowBeneficial = value; }
	bool GetAllowBeneficial() { if (m_AllowBeneficial || GetSpecialAbility(ALLOW_BENEFICIAL)){return true;} return false; }
	void SetDisableMelee(bool value) { m_DisableMelee = value; }
	bool IsMeleeDisabled() { if (m_DisableMelee || GetSpecialAbility(DISABLE_MELEE)){return true;} return false; }

	bool IsOffHandAtk() const { return offhand; }
	inline void OffHandAtk(bool val) { offhand = val; }

	void SetFlurryChance(uint8 value) { SetSpecialAbilityParam(SPECATK_FLURRY, 0, value); }
	uint8 GetFlurryChance() { return GetSpecialAbilityParam(SPECATK_FLURRY, 0); }

	static uint32 GetAppearanceValue(EmuAppearance iAppearance);
	void SendAppearancePacket(uint32 type, uint32 value, bool WholeZone = true, bool iIgnoreSelf = false, Client *specific_target=nullptr);
	void SetAppearance(EmuAppearance app, bool iIgnoreSelf = true);
	inline EmuAppearance GetAppearance() const { return _appearance; }
	inline const uint8 GetRunAnimSpeed() const { return pRunAnimSpeed; }
	inline void SetRunAnimSpeed(int8 in) { if (pRunAnimSpeed != in) { pRunAnimSpeed = in; pLastChange = Timer::GetCurrentTime(); } }
	bool IsDestructibleObject() { return destructibleobject; }
	void SetDestructibleObject(bool in) { destructibleobject = in; }

	inline uint8 GetInnateLightType() { return m_Light.Type.Innate; }
	inline uint8 GetEquipmentLightType() { return m_Light.Type.Equipment; }
	inline uint8 GetSpellLightType() { return m_Light.Type.Spell; }

	virtual void UpdateEquipmentLight() { m_Light.Type.Equipment = 0; m_Light.Level.Equipment = 0; }
	inline void SetSpellLightType(uint8 lightType) { m_Light.Type.Spell = (lightType & 0x0F); m_Light.Level.Spell = m_Light.TypeToLevel(m_Light.Type.Spell); }

	inline uint8 GetActiveLightType() { return m_Light.Type.Active; }
	bool UpdateActiveLight(); // returns true if change, false if no change

	LightProfile_Struct* GetLightProfile() { return &m_Light; }

	Mob* GetPet();
	void SetPet(Mob* newpet);
	virtual Mob* GetOwner();
	virtual Mob* GetOwnerOrSelf();
	Mob* GetUltimateOwner();
	void SetPetID(uint16 NewPetID);
	inline uint16 GetPetID() const { return petid; }
	inline PetType GetPetType() const { return typeofpet; }
	void SetPetType(PetType p) { typeofpet = p; }
	inline int16 GetPetPower() const { return (petpower < 0) ? 0 : petpower; }
	void SetPetPower(int16 p) { if (p < 0) petpower = 0; else petpower = p; }
	bool IsFamiliar() const { return(typeofpet == petFamiliar); }
	bool IsAnimation() const { return(typeofpet == petAnimation); }
	bool IsCharmed() const { return(typeofpet == petCharmed); }
	bool IsTargetLockPet() const { return(typeofpet == petTargetLock); }
	inline uint32 GetPetTargetLockID() { return pet_targetlock_id; };
	inline void SetPetTargetLockID(uint32 value) { pet_targetlock_id = value; };
	void SetOwnerID(uint16 NewOwnerID);
	inline uint16 GetOwnerID() const { return ownerid; }
	inline virtual bool HasOwner() { if(GetOwnerID()==0){return false;} return( entity_list.GetMob(GetOwnerID()) != 0); }
	inline virtual bool IsPet() { return(HasOwner() && !IsMerc()); }
	inline bool HasPet() const { if(GetPetID()==0){return false;} return (entity_list.GetMob(GetPetID()) != 0);}
	inline bool HasTempPetsActive() const { return(hasTempPet); }
	inline void SetTempPetsActive(bool i) { hasTempPet = i; }
	inline int16 GetTempPetCount() const { return count_TempPet; }
	inline void SetTempPetCount(int16 i) { count_TempPet = i; }
	bool HasPetAffinity() { if (aabonuses.GivePetGroupTarget || itembonuses.GivePetGroupTarget || spellbonuses.GivePetGroupTarget) return true; return false; }
	inline bool IsPetOwnerClient() const { return pet_owner_client; }
	inline void SetPetOwnerClient(bool value) { pet_owner_client = value; }
	inline bool IsTempPet() const { return _IsTempPet; }
	inline void SetTempPet(bool value) { _IsTempPet = value; }

	inline const bodyType GetBodyType() const { return bodytype; }
	inline const bodyType GetOrigBodyType() const { return orig_bodytype; }
	void SetBodyType(bodyType new_body, bool overwrite_orig);

	uint8 invisible, see_invis;
	bool invulnerable, invisible_undead, invisible_animals, sneaking, hidden, improved_hidden;
	bool see_invis_undead, see_hide, see_improved_hide;
	bool qglobal;

	virtual void SetAttackTimer();
	inline void SetInvul(bool invul) { invulnerable=invul; }
	inline bool GetInvul(void) { return invulnerable; }
	inline void SetExtraHaste(int Haste) { ExtraHaste = Haste; }
	virtual int GetHaste();

	uint8 GetWeaponDamageBonus(const Item_Struct* Weapon);
	uint16 GetDamageTable(SkillUseTypes skillinuse);
	virtual int GetMonkHandToHandDamage(void);

	bool CanThisClassDoubleAttack(void) const;
	bool CanThisClassDualWield(void) const;
	bool CanThisClassRiposte(void) const;
	bool CanThisClassDodge(void) const;
	bool CanThisClassParry(void) const;
	bool CanThisClassBlock(void) const;

	int GetMonkHandToHandDelay(void);
	uint32 GetClassLevelFactor();
	void Mesmerize();
	inline bool IsMezzed() const { return mezzed; }
	inline bool IsStunned() const { return stunned; }
	inline bool IsSilenced() const { return silenced; }
	inline bool IsAmnesiad() const { return amnesiad; }

	int32 ReduceDamage(int32 damage);
	int32 AffectMagicalDamage(int32 damage, uint16 spell_id, const bool iBuffTic, Mob* attacker);
	int32 ReduceAllDamage(int32 damage);

	virtual void DoSpecialAttackDamage(Mob *who, SkillUseTypes skill, int32 max_damage, int32 min_damage = 1, int32 hate_override = -1, int ReuseTime = 10, bool HitChance=false, bool CanAvoid=true);
	virtual void DoThrowingAttackDmg(Mob* other, const ItemInst* RangeWeapon=nullptr, const Item_Struct* AmmoItem=nullptr, uint16 weapon_damage=0, int16 chance_mod=0,int16 focus=0, int ReuseTime=0, uint32 range_id=0, int AmmoSlot=0, float speed = 4.0f);
	virtual void DoMeleeSkillAttackDmg(Mob* other, uint16 weapon_damage, SkillUseTypes skillinuse, int16 chance_mod=0, int16 focus=0, bool CanRiposte=false, int ReuseTime=0);
	virtual void DoArcheryAttackDmg(Mob* other,  const ItemInst* RangeWeapon=nullptr, const ItemInst* Ammo=nullptr, uint16 weapon_damage=0, int16 chance_mod=0, int16 focus=0, int ReuseTime=0, uint32 range_id=0, uint32 ammo_id=0, const Item_Struct *AmmoItem=nullptr, int AmmoSlot=0, float speed= 4.0f);
	bool TryProjectileAttack(Mob* other, const Item_Struct *item, SkillUseTypes skillInUse, uint16 weapon_dmg, const ItemInst* RangeWeapon, const ItemInst* Ammo, int AmmoSlot, float speed);
	void ProjectileAttack();
	inline bool HasProjectileAttack() const { return ActiveProjectileATK; }
	inline void SetProjectileAttack(bool value) { ActiveProjectileATK = value; }
	float GetRangeDistTargetSizeMod(Mob* other);
	bool CanDoSpecialAttack(Mob *other);
	bool Flurry(ExtraAttackOptions *opts);
	bool Rampage(ExtraAttackOptions *opts);
	bool AddRampage(Mob*);
	void ClearRampage();
	void AreaRampage(ExtraAttackOptions *opts);

	void StartEnrage();
	void ProcessEnrage();
	bool IsEnraged();
	void Taunt(NPC* who, bool always_succeed, float chance_bonus = 0);

	virtual void AI_Init();
	virtual void AI_Start(uint32 iMoveDelay = 0);
	virtual void AI_Stop();
	virtual void AI_ShutDown();
	virtual void AI_Process();

	const char* GetEntityVariable(const char *id);
	void SetEntityVariable(const char *id, const char *m_var);
	bool EntityVariableExists(const char *id);

	void AI_Event_Engaged(Mob* attacker, bool iYellForHelp = true);
	void AI_Event_NoLongerEngaged();

	FACTION_VALUE GetSpecialFactionCon(Mob* iOther);
	inline const bool IsAIControlled() const { return pAIControlled; }
	inline const float GetAggroRange() const { return (spellbonuses.AggroRange == -1) ? pAggroRange : spellbonuses.AggroRange; }
	inline const float GetAssistRange() const { return (spellbonuses.AssistRange == -1) ? pAssistRange : spellbonuses.AssistRange; }


	inline void SetPetOrder(eStandingPetOrder i) { pStandingPetOrder = i; }
	inline const eStandingPetOrder GetPetOrder() const { return pStandingPetOrder; }
	inline void SetHeld(bool nState) { held = nState; }
	inline const bool IsHeld() const { return held; }
	inline void SetNoCast(bool nState) { nocast = nState; }
	inline const bool IsNoCast() const { return nocast; }
	inline void SetFocused(bool nState) { focused = nState; }
	inline const bool IsFocused() const { return focused; }
	inline const bool IsRoamer() const { return roamer; }
	inline const bool IsRooted() const { return rooted || permarooted; }
	inline const bool HasVirus() const { return has_virus; }
	int GetSnaredAmount();
	inline const bool IsPseudoRooted() const { return pseudo_rooted; }
	inline void SetPseudoRoot(bool prState) { pseudo_rooted = prState; }

	int GetCurWp() { return cur_wp; }

	//old fear function
	//void SetFeared(Mob *caster, uint32 duration, bool flee = false);
	float GetFearSpeed();
	bool IsFeared() { return curfp; } // This returns true if the mob is feared or fleeing due to low HP
	//old fear: inline void StartFleeing() { SetFeared(GetHateTop(), FLEE_RUN_DURATION, true); }
	inline void StartFleeing() { flee_mode = true; CalculateNewFearpoint(); }
	void ProcessFlee();
	void CheckFlee();
	inline bool IsBlind() { return spellbonuses.IsBlind; }

	inline bool			CheckAggro(Mob* other) {return hate_list.IsEntOnHateList(other);}
	float				CalculateHeadingToTarget(float in_x, float in_y);
	bool				CalculateNewPosition(float x, float y, float z, float speed, bool checkZ = false);
	virtual bool		CalculateNewPosition2(float x, float y, float z, float speed, bool checkZ = true);
	float				CalculateDistance(float x, float y, float z);
	float				GetGroundZ(float new_x, float new_y, float z_offset=0.0);
	void				SendTo(float new_x, float new_y, float new_z);
	void				SendToFixZ(float new_x, float new_y, float new_z);
	void				NPCSpecialAttacks(const char* parse, int permtag, bool reset = true, bool remove = false);
	inline uint32		DontHealMeBefore() const { return pDontHealMeBefore; }
	inline uint32		DontBuffMeBefore() const { return pDontBuffMeBefore; }
	inline uint32		DontDotMeBefore() const { return pDontDotMeBefore; }
	inline uint32		DontRootMeBefore() const { return pDontRootMeBefore; }
	inline uint32		DontSnareMeBefore() const { return pDontSnareMeBefore; }
	inline uint32		DontCureMeBefore() const { return pDontCureMeBefore; }
	void				SetDontRootMeBefore(uint32 time) { pDontRootMeBefore = time; }
	void				SetDontHealMeBefore(uint32 time) { pDontHealMeBefore = time; }
	void				SetDontBuffMeBefore(uint32 time) { pDontBuffMeBefore = time; }
	void				SetDontDotMeBefore(uint32 time) { pDontDotMeBefore = time; }
	void				SetDontSnareMeBefore(uint32 time) { pDontSnareMeBefore = time; }
	void				SetDontCureMeBefore(uint32 time) { pDontCureMeBefore = time; }

	// calculate interruption of spell via movement of mob
	void SaveSpellLoc() { m_SpellLocation = glm::vec3(m_Position); }
	inline float GetSpellX() const {return m_SpellLocation.x;}
	inline float GetSpellY() const {return m_SpellLocation.y;}
	inline float GetSpellZ() const {return m_SpellLocation.z;}
	inline bool IsGrouped() const { return isgrouped; }
	void SetGrouped(bool v);
	inline bool IsRaidGrouped() const { return israidgrouped; }
	void SetRaidGrouped(bool v);
	inline uint16 IsLooting() const { return entity_id_being_looted; }
	void SetLooting(uint16 val) { entity_id_being_looted = val; }

	bool CheckWillAggro(Mob *mob);

	void InstillDoubt(Mob *who);
	int16 GetResist(uint8 type) const;
	Mob* GetShieldTarget() const { return shield_target; }
	void SetShieldTarget(Mob* mob) { shield_target = mob; }
	bool HasActiveSong() const { return(bardsong != 0); }
	bool Charmed() const { return charmed; }
	static uint32 GetLevelHP(uint8 tlevel);
	uint32 GetZoneID() const; //for perl
	virtual int32 CheckAggroAmount(uint16 spell_id, bool isproc = false);
	virtual int32 CheckHealAggroAmount(uint16 spell_id, uint32 heal_possible = 0);
	virtual uint32 GetAA(uint32 aa_id) const { return(0); }

	uint32 GetInstrumentMod(uint16 spell_id) const;
	int CalcSpellEffectValue(uint16 spell_id, int effect_id, int caster_level = 1, Mob *caster = nullptr, int ticsremaining = 0);
	int CalcSpellEffectValue_formula(int formula, int base, int max, int caster_level, uint16 spell_id, int ticsremaining = 0);
	virtual int CheckStackConflict(uint16 spellid1, int caster_level1, uint16 spellid2, int caster_level2, Mob* caster1 = nullptr, Mob* caster2 = nullptr, int buffslot = -1);
	uint32 GetCastedSpellInvSlot() const { return casting_spell_inventory_slot; }

	// HP Event
	inline int GetNextHPEvent() const { return nexthpevent; }
	void SetNextHPEvent( int hpevent );
	void SendItemAnimation(Mob *to, const Item_Struct *item, SkillUseTypes skillInUse, float velocity= 4.0);
	inline int& GetNextIncHPEvent() { return nextinchpevent; }
	void SetNextIncHPEvent( int inchpevent );

	inline bool DivineAura() const { return spellbonuses.DivineAura; }
 	inline bool Sanctuary() const { return spellbonuses.Sanctuary; }

	bool HasNPCSpecialAtk(const char* parse);
	int GetSpecialAbility(int ability);
	int GetSpecialAbilityParam(int ability, int param);
	void SetSpecialAbility(int ability, int level);
	void SetSpecialAbilityParam(int ability, int param, int value);
	void StartSpecialAbilityTimer(int ability, uint32 time);
	void StopSpecialAbilityTimer(int ability);
	Timer *GetSpecialAbilityTimer(int ability);
	void ClearSpecialAbilities();
	void ProcessSpecialAbilities(const std::string &str);

	Shielders_Struct shielder[MAX_SHIELDERS];
	Trade* trade;

	inline glm::vec4 GetCurrentWayPoint() const { return m_CurrentWayPoint; }
	inline float GetCWPP() const { return(static_cast<float>(cur_wp_pause)); }
	inline int GetCWP() const { return(cur_wp); }
	void SetCurrentWP(uint16 waypoint) { cur_wp = waypoint; }
	virtual FACTION_VALUE GetReverseFactionCon(Mob* iOther) { return FACTION_INDIFFERENT; }

	inline bool IsTrackable() const { return(trackable); }
	Timer* GetAIThinkTimer() { return AIthink_timer.get(); }
	Timer* GetAIMovementTimer() { return AImovement_timer.get(); }
	Timer GetAttackTimer() { return attack_timer; }
	Timer GetAttackDWTimer() { return attack_dw_timer; }
	inline bool IsFindable() { return findable; }
	inline uint8 GetManaPercent() { return (uint8)((float)cur_mana / (float)max_mana * 100.0f); }
	virtual uint8 GetEndurancePercent() { return 0; }

	inline virtual bool IsBlockedBuff(int16 SpellID) { return false; }
	inline virtual bool IsBlockedPetBuff(int16 SpellID) { return false; }

	void SetGlobal(const char *varname, const char *newvalue, int options, const char *duration, Mob *other = nullptr);
	void TarGlobal(const char *varname, const char *value, const char *duration, int npcid, int charid, int zoneid);
	void DelGlobal(const char *varname);

	inline void SetEmoteID(uint16 emote) { emoteid = emote; }
	inline uint16 GetEmoteID() { return emoteid; }

	bool 	HasSpellEffect(int effectid);
	int 	mod_effect_value(int effect_value, uint16 spell_id, int effect_type, Mob* caster);
	float 	mod_hit_chance(float chancetohit, SkillUseTypes skillinuse, Mob* attacker);
	float 	mod_riposte_chance(float ripostchance, Mob* attacker);
	float	mod_block_chance(float blockchance, Mob* attacker);
	float	mod_parry_chance(float parrychance, Mob* attacker);
	float	mod_dodge_chance(float dodgechance, Mob* attacker);
	float	mod_monk_weight(float monkweight, Mob* attacker);
	float	mod_mitigation_rating(float mitigation_rating, Mob* attacker);
	float	mod_attack_rating(float attack_rating, Mob* defender);
	int32	mod_kick_damage(int32 dmg);
	int32	mod_bash_damage(int32 dmg);
	int32	mod_frenzy_damage(int32 dmg);
	int32	mod_monk_special_damage(int32 ndamage, SkillUseTypes skill_type);
	int32	mod_backstab_damage(int32 ndamage);
	int		mod_archery_bonus_chance(int bonuschance, const ItemInst* RangeWeapon);
	uint32	mod_archery_bonus_damage(uint32 MaxDmg, const ItemInst* RangeWeapon);
	int32	mod_archery_damage(int32 TotalDmg, bool hasbonus, const ItemInst* RangeWeapon);
	uint16	mod_throwing_damage(uint16 MaxDmg);
	int32	mod_cast_time(int32 cast_time);
	int		mod_buff_duration(int res, Mob* caster, Mob* target, uint16 spell_id);
	int		mod_spell_stack(uint16 spellid1, int caster_level1, Mob* caster1, uint16 spellid2, int caster_level2, Mob* caster2);
	int		mod_spell_resist(int resist_chance, int level_mod, int resist_modifier, int target_resist, uint8 resist_type, uint16 spell_id, Mob* caster);
	void	mod_spell_cast(uint16 spell_id, Mob* spelltar, bool reflect, bool use_resist_adjust, int16 resist_adjust, bool isproc);
	bool    mod_will_aggro(Mob *attacker, Mob *on);

	//Command #Tune functions
	int32 Tune_MeleeMitigation(Mob* GM, Mob *attacker, int32 damage, int32 minhit, ExtraAttackOptions *opts = nullptr, int Msg =0,	int ac_override=0, int atk_override=0, int add_ac=0, int add_atk = 0);
	virtual int32 Tune_GetMeleeMitDmg(Mob* GM, Mob *attacker, int32 damage, int32 minhit, float mit_rating, float atk_rating);
	uint32 Tune_GetMeanDamage(Mob* GM, Mob *attacker, int32 damage, int32 minhit, ExtraAttackOptions *opts = nullptr, int Msg = 0,int ac_override=0, int atk_override=0, int add_ac=0, int add_atk = 0);
	void Tune_FindATKByPctMitigation(Mob* defender, Mob *attacker, float pct_mitigation,  int interval = 50, int max_loop = 100, int ac_override=0,int Msg =0);
	void Tune_FindACByPctMitigation(Mob* defender, Mob *attacker, float pct_mitigation,  int interval = 50, int max_loop = 100, int atk_override=0,int Msg =0);
	float Tune_CheckHitChance(Mob* defender, Mob* attacker, SkillUseTypes skillinuse, int Hand, int16 chance_mod, int Msg = 1,int acc_override=0, int avoid_override=0, int add_acc=0, int add_avoid = 0);
	void Tune_FindAccuaryByHitChance(Mob* defender, Mob *attacker, float hit_chance, int interval, int max_loop, int avoid_override, int Msg = 0);
	void Tune_FindAvoidanceByHitChance(Mob* defender, Mob *attacker, float hit_chance, int interval, int max_loop, int acc_override, int Msg = 0);

protected:
	void CommonDamage(Mob* other, int32 &damage, const uint16 spell_id, const SkillUseTypes attack_skill, bool &avoidable, const int8 buffslot, const bool iBuffTic);
	static uint16 GetProcID(uint16 spell_id, uint8 effect_index);
	float _GetMovementSpeed(int mod) const;
	virtual bool MakeNewPositionAndSendUpdate(float x, float y, float z, float speed, bool checkZ);

	virtual bool AI_EngagedCastCheck() { return(false); }
	virtual bool AI_PursueCastCheck() { return(false); }
	virtual bool AI_IdleCastCheck() { return(false); }


	bool IsFullHP;
	bool moved;

	std::vector<uint16> RampageArray;
	std::map<std::string, std::string> m_EntityVariables;

	int16 SkillDmgTaken_Mod[HIGHEST_SKILL+2];
	int16 Vulnerability_Mod[HIGHEST_RESIST+2];
	bool m_AllowBeneficial;
	bool m_DisableMelee;

	bool isgrouped;
	bool israidgrouped;
	bool pendinggroup;
	uint16 entity_id_being_looted; //the id of the entity being looted, 0 if not looting.
	uint8 texture;
	uint8 helmtexture;
	uint8 armtexture;
	uint8 bracertexture;
	uint8 handtexture;
	uint8 legtexture;
	uint8 feettexture;
	bool multitexture;

	int AC;
	int32 ATK;
	int32 STR;
	int32 STA;
	int32 DEX;
	int32 AGI;
	int32 INT;
	int32 WIS;
	int32 CHA;
	int32 MR;
	int32 CR;
	int32 FR;
	int32 DR;
	int32 PR;
	int32 Corrup;
	int32 PhR;
	bool moving;
	int targeted;
	bool findable;
	bool trackable;
	int32 cur_hp;
	int32 max_hp;
	int32 base_hp;
	int32 cur_mana;
	int32 max_mana;
	int32 hp_regen;
	int32 mana_regen;
	int32 oocregen;
	uint8 maxlevel;
	uint32 scalerate;
	Buffs_Struct *buffs;
	uint32 current_buff_count;
	StatBonuses itembonuses;
	StatBonuses spellbonuses;
	StatBonuses aabonuses;
	uint16 petid;
	uint16 ownerid;
	PetType typeofpet;
	int16 petpower;
	uint32 follow;
	uint32 follow_dist;
	bool no_target_hotkey;

	uint8 gender;
	uint16 race;
	uint8 base_gender;
	uint16 base_race;
	uint8 class_;
	bodyType bodytype;
	bodyType orig_bodytype;
	uint16 deity;
	uint8 level;
	uint8 orig_level;
	uint32 npctype_id;
	glm::vec4 m_Position;
	uint16 animation;
	float base_size;
	float size;
	float runspeed;
	uint32 pLastChange;
	bool held;
	bool nocast;
	bool focused;
	void CalcSpellBonuses(StatBonuses* newbon);
	virtual void CalcBonuses();
	void TrySkillProc(Mob *on, uint16 skill, uint16 ReuseTime, bool Success = false, uint16 hand = 0, bool IsDefensive = false); // hand = MainCharm?
	bool PassLimitToSkill(uint16 spell_id, uint16 skill);
	bool PassLimitClass(uint32 Classes_, uint16 Class_);
	void TryDefensiveProc(const ItemInst* weapon, Mob *on, uint16 hand = MainPrimary);
	void TryWeaponProc(const ItemInst* inst, const Item_Struct* weapon, Mob *on, uint16 hand = MainPrimary);
	void TrySpellProc(const ItemInst* inst, const Item_Struct* weapon, Mob *on, uint16 hand = MainPrimary);
	void TryWeaponProc(const ItemInst* weapon, Mob *on, uint16 hand = MainPrimary);
	void ExecWeaponProc(const ItemInst* weapon, uint16 spell_id, Mob *on);
	virtual float GetProcChances(float ProcBonus, uint16 hand = MainPrimary);
	virtual float GetDefensiveProcChances(float &ProcBonus, float &ProcChance, uint16 hand = MainPrimary, Mob *on = nullptr);
	virtual float GetSpecialProcChances(uint16 hand);
	virtual float GetAssassinateProcChances(uint16 ReuseTime);
	virtual float GetSkillProcChances(uint16 ReuseTime, uint16 hand = 0); // hand = MainCharm?
	uint16 GetWeaponSpeedbyHand(uint16 hand);
	int GetWeaponDamage(Mob *against, const Item_Struct *weapon_item);
	int GetWeaponDamage(Mob *against, const ItemInst *weapon_item, uint32 *hate = nullptr);
	int GetKickDamage();
	int GetBashDamage();
	virtual void ApplySpecialAttackMod(SkillUseTypes skill, int32 &dmg, int32 &mindmg);
	virtual int16 GetFocusEffect(focusType type, uint16 spell_id) { return 0; }
	void CalculateNewFearpoint();
	float FindGroundZ(float new_x, float new_y, float z_offset=0.0);
	glm::vec3 UpdatePath(float ToX, float ToY, float ToZ, float Speed, bool &WaypointChange, bool &NodeReached);
	void PrintRoute();

	virtual float GetSympatheticProcChances(uint16 spell_id, int16 ProcRateMod, int32 ItemProcRate = 0);

	enum {MAX_PROCS = 4};
	tProc PermaProcs[MAX_PROCS];
	tProc SpellProcs[MAX_PROCS];
	tProc DefensiveProcs[MAX_PROCS];
	tProc RangedProcs[MAX_PROCS];

	char name[64];
	char orig_name[64];
	char clean_name[64];
	char lastname[64];

	glm::vec4 m_Delta;

	LightProfile_Struct m_Light;

	float fixedZ;
	EmuAppearance _appearance;
	uint8 pRunAnimSpeed;
	bool m_is_running;

	Timer attack_timer;
	Timer attack_dw_timer;
	Timer ranged_timer;
	float attack_speed; //% increase/decrease in attack speed (not haste)
	int8 attack_delay; //delay between attacks in 10ths of seconds
	int16 slow_mitigation; // Allows for a slow mitigation (100 = 100%, 50% = 50%)
	Timer tic_timer;
	Timer mana_timer;

	//spell casting vars
	Timer spellend_timer;
	uint16 casting_spell_id;
	glm::vec3 m_SpellLocation;
	int attacked_count;
	bool delaytimer;
	uint16 casting_spell_targetid;
	uint16 casting_spell_slot;
	uint16 casting_spell_mana;
	uint32 casting_spell_inventory_slot;
	uint32 casting_spell_timer;
	uint32 casting_spell_timer_duration;
	uint32 casting_spell_type;
	int16 casting_spell_resist_adjust;
	bool casting_spell_checks;
	uint16 bardsong;
	uint8 bardsong_slot;
	uint32 bardsong_target_id;

	bool ActiveProjectileATK;
	tProjatk ProjectileAtk[MAX_SPELL_PROJECTILE];

	glm::vec3 m_RewindLocation;

	Timer rewind_timer;

	// Currently 3 max nimbus particle effects at a time
	uint32 nimbus_effect1;
	uint32 nimbus_effect2;
	uint32 nimbus_effect3;

	uint8 haircolor;
	uint8 beardcolor;
	uint8 eyecolor1; // the eyecolors always seem to be the same, maybe left and right eye?
	uint8 eyecolor2;
	uint8 hairstyle;
	uint8 luclinface; //
	uint8 beard;
	uint32 drakkin_heritage;
	uint32 drakkin_tattoo;
	uint32 drakkin_details;
	uint32 armor_tint[_MaterialCount];

	uint8 aa_title;

	Mob* shield_target;

	int ExtraHaste; // for the #haste command
	bool mezzed;
	bool stunned;
	bool charmed; //this isnt fully implemented yet
	bool rooted;
	bool silenced;
	bool amnesiad;
	bool inWater; // Set to true or false by Water Detection code if enabled by rules
	bool has_virus; // whether this mob has a viral spell on them
	uint16 viral_spells[MAX_SPELL_TRIGGER*2]; // Stores the spell ids of the viruses on target and caster ids
	bool offhand;
	bool has_shieldequiped;
	bool has_twohandbluntequiped;
	bool has_numhits;
	bool has_MGB;
	bool has_ProjectIllusion;
	int16 SpellPowerDistanceMod;
	bool last_los_check;
	bool pseudo_rooted;
	bool endur_upkeep;

	// Bind wound
	Timer bindwound_timer;
	Mob* bindwound_target;

	Timer stunned_timer;
	Timer spun_timer;
	Timer bardsong_timer;
	Timer gravity_timer;
	Timer viral_timer;
	uint8 viral_timer_counter;

	// MobAI stuff
	eStandingPetOrder pStandingPetOrder;
	uint32 minLastFightingDelayMoving;
	uint32 maxLastFightingDelayMoving;
	float pAggroRange;
	float pAssistRange;
	std::unique_ptr<Timer> AIthink_timer;
	std::unique_ptr<Timer> AImovement_timer;
	std::unique_ptr<Timer> AItarget_check_timer;
	bool movetimercompleted;
	bool permarooted;
	std::unique_ptr<Timer> AIscanarea_timer;
	std::unique_ptr<Timer> AIwalking_timer;
	std::unique_ptr<Timer> AIfeignremember_timer;
	uint32 pLastFightingDelayMoving;
	HateList hate_list;
	std::set<uint32> feign_memory_list;
	// This is to keep track of mobs we cast faction mod spells on
	std::map<uint32,int32> faction_bonuses; // Primary FactionID, Bonus
	void AddFactionBonus(uint32 pFactionID,int32 bonus);
	int32 GetFactionBonus(uint32 pFactionID);
	// This is to keep track of item faction modifiers
	std::map<uint32,int32> item_faction_bonuses; // Primary FactionID, Bonus
	void AddItemFactionBonus(uint32 pFactionID,int32 bonus);
	int32 GetItemFactionBonus(uint32 pFactionID);
	void ClearItemFactionBonuses();

	void CalculateFearPosition();
	uint32 move_tic_count;

	bool flee_mode;
	Timer flee_timer;

	bool pAIControlled;
	bool roamer;
	bool logging_enabled;

	int wandertype;
	int pausetype;

	int cur_wp;
	glm::vec4 m_CurrentWayPoint;
	int cur_wp_pause;


	int patrol;
	glm::vec3 m_FearWalkTarget;
	bool curfp;

	// Pathing
	//
	glm::vec3 PathingDestination;
	glm::vec3 PathingLastPosition;
	int PathingLoopCount;
	int PathingLastNodeVisited;
	std::deque<int> Route;
	LOSType PathingLOSState;
	Timer *PathingLOSCheckTimer;
	Timer *PathingRouteUpdateTimerShort;
	Timer *PathingRouteUpdateTimerLong;
	bool DistractedFromGrid;
	int PathingTraversedNodes;

	uint32 pDontHealMeBefore;
	uint32 pDontBuffMeBefore;
	uint32 pDontDotMeBefore;
	uint32 pDontRootMeBefore;
	uint32 pDontSnareMeBefore;
	uint32 pDontCureMeBefore;

	// hp event
	int nexthpevent;
	int nextinchpevent;

	//temppet
	bool hasTempPet;
	bool _IsTempPet;
	int16 count_TempPet;
	bool pet_owner_client; //Flags regular and pets as belonging to a client
	uint32 pet_targetlock_id;

	EGNode *_egnode; //the EG node we are in
	glm::vec3 m_TargetLocation;
	uint8 tar_ndx;
	float tar_vector;
	glm::vec3 m_TargetV;
	float test_vector;

	glm::vec3 m_TargetRing;

	uint32 m_spellHitsLeft[38]; // Used to track which spells will have their numhits incremented when spell finishes casting, 38 Buffslots
	int flymode;
	bool m_targetable;
	int QGVarDuration(const char *fmt);
	void InsertQuestGlobal(int charid, int npcid, int zoneid, const char *name, const char *value, int expdate);
	uint16 emoteid;

	SpecialAbility SpecialAbilities[MAX_SPECIAL_ATTACK];
	bool bEnraged;
	bool destructibleobject;

private:
	void _StopSong(); //this is not what you think it is
	Mob* target;
};

#endif

