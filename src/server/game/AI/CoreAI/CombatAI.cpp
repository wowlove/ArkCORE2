/*
 * Copyright (C) 2011-2012 ArkCORE2 <http://www.arkania.net/>
 * Copyright (C) 2010-2012 Project SkyFire <http://www.projectskyfire.org/> 
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2012 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "CombatAI.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "Vehicle.h"
#include "ObjectAccessor.h"

int AggressorAI::Permissible(const Creature* creature)
{
    // have some hostile factions, it will be selected by IsHostileTo check at MoveInLineOfSight
    if (!creature->isCivilian() && !creature->IsNeutralToAll())
        return PERMIT_BASE_PROACTIVE;

    return PERMIT_BASE_NO;
}

void AggressorAI::UpdateAI(const uint32 /*diff*/)
{
    if (!UpdateVictim())
        return;

    DoMeleeAttackIfReady();
}

// some day we will delete these useless things
int CombatAI::Permissible(const Creature* /*creature*/)
{
    return PERMIT_BASE_NO;
}

int ArcherAI::Permissible(const Creature* /*creature*/)
{
    return PERMIT_BASE_NO;
}

int TurretAI::Permissible(const Creature* /*creature*/)
{
    return PERMIT_BASE_NO;
}

int VehicleAI::Permissible(const Creature* /*creature*/)
{
    return PERMIT_BASE_NO;
}

void CombatAI::InitializeAI()
{
    for (uint32 i = 0; i < CREATURE_MAX_SPELLS; ++i)
        if (me->_spells[i] && sSpellMgr->GetSpellInfo(me->_spells[i]))
            spells.push_back(me->_spells[i]);

    CreatureAI::InitializeAI();
}

void CombatAI::Reset()
{
    events.Reset();
}

void CombatAI::JustDied(Unit* killer)
{
    for (SpellVct::iterator i = spells.begin(); i != spells.end(); ++i)
        if (AISpellInfo[*i].condition == AICOND_DIE)
            me->CastSpell(killer, *i, true);
}

void CombatAI::EnterCombat(Unit* who)
{
    for (SpellVct::iterator i = spells.begin(); i != spells.end(); ++i)
    {
        if (AISpellInfo[*i].condition == AICOND_AGGRO)
            me->CastSpell(who, *i, false);
        else if (AISpellInfo[*i].condition == AICOND_COMBAT)
            events.ScheduleEvent(*i, AISpellInfo[*i].cooldown + rand()%AISpellInfo[*i].cooldown);
    }
}

void CombatAI::UpdateAI(const uint32 diff)
{
    if (!UpdateVictim())
        return;

    events.Update(diff);

    if (me->HasUnitState(UNIT_STAT_CASTING))
        return;

    if (uint32 spellId = events.ExecuteEvent())
    {
        DoCast(spellId);
        events.ScheduleEvent(spellId, AISpellInfo[spellId].cooldown + rand()%AISpellInfo[spellId].cooldown);
    }
    else
        DoMeleeAttackIfReady();
}

/////////////////
//CasterAI
/////////////////

void CasterAI::InitializeAI()
{
    CombatAI::InitializeAI();

    m_attackDist = 30.0f;
    for (SpellVct::iterator itr = spells.begin(); itr != spells.end(); ++itr)
        if (AISpellInfo[*itr].condition == AICOND_COMBAT && m_attackDist > GetAISpellInfo(*itr)->maxRange)
            m_attackDist = GetAISpellInfo(*itr)->maxRange;
    if (m_attackDist == 30.0f)
        m_attackDist = MELEE_RANGE;
}

void CasterAI::EnterCombat(Unit* who)
{
    if (spells.empty())
        return;

    uint32 spell = rand()%spells.size();
    uint32 count = 0;
    for (SpellVct::iterator itr = spells.begin(); itr != spells.end(); ++itr, ++count)
    {
        if (AISpellInfo[*itr].condition == AICOND_AGGRO)
            me->CastSpell(who, *itr, false);
        else if (AISpellInfo[*itr].condition == AICOND_COMBAT)
        {
            uint32 cooldown = GetAISpellInfo(*itr)->realCooldown;
            if (count == spell)
            {
                DoCast(spells[spell]);
                cooldown += me->GetCurrentSpellCastTime(*itr);
            }
            events.ScheduleEvent(*itr, cooldown);
        }
    }
}

void CasterAI::UpdateAI(const uint32 diff)
{
    if (!UpdateVictim())
        return;

    events.Update(diff);

    if (me->HasUnitState(UNIT_STAT_CASTING))
        return;

    if (uint32 spellId = events.ExecuteEvent())
    {
        DoCast(spellId);
        uint32 casttime = me->GetCurrentSpellCastTime(spellId);
        events.ScheduleEvent(spellId, (casttime ? casttime : 500) + GetAISpellInfo(spellId)->realCooldown);
    }
}

//////////////
//ArcherAI
//////////////

ArcherAI::ArcherAI(Creature* c) : CreatureAI(c)
{
    if (!me->_spells[0])
        sLog->outError("ArcherAI set for creature (entry = %u) with spell1=0. AI will do nothing", me->GetEntry());

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(me->_spells[0]);
    m_minRange = spellInfo ? spellInfo->GetMinRange(false) : 0;

    if (!m_minRange)
        m_minRange = MELEE_RANGE;
    me->_CombatDistance = spellInfo ? spellInfo->GetMaxRange(false) : 0;
    me->_SightDistance = me->_CombatDistance;
}

void ArcherAI::AttackStart(Unit* who)
{
    if (!who)
        return;

    if (me->IsWithinCombatRange(who, m_minRange))
    {
        if (me->Attack(who, true) && !who->IsFlying())
            me->GetMotionMaster()->MoveChase(who);
    }
    else
    {
        if (me->Attack(who, false) && !who->IsFlying())
            me->GetMotionMaster()->MoveChase(who, me->_CombatDistance);
    }

    if (who->IsFlying())
        me->GetMotionMaster()->MoveIdle();
}

void ArcherAI::UpdateAI(const uint32 /*diff*/)
{
    if (!UpdateVictim())
        return;

    if (!me->IsWithinCombatRange(me->getVictim(), m_minRange))
        DoSpellAttackIfReady(me->_spells[0]);
    else
        DoMeleeAttackIfReady();
}

//////////////
//TurretAI
//////////////

TurretAI::TurretAI(Creature* c) : CreatureAI(c)
{
    if (!me->_spells[0])
        sLog->outError("TurretAI set for creature (entry = %u) with spell1=0. AI will do nothing", me->GetEntry());

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(me->_spells[0]);
    m_minRange = spellInfo ? spellInfo->GetMinRange(false) : 0;
    me->_CombatDistance = spellInfo ? spellInfo->GetMaxRange(false) : 0;
    me->_SightDistance = me->_CombatDistance;
}

bool TurretAI::CanAIAttack(const Unit* /*who*/) const
{
    // TODO: use one function to replace it
    if (!me->IsWithinCombatRange(me->getVictim(), me->_CombatDistance)
        || (m_minRange && me->IsWithinCombatRange(me->getVictim(), m_minRange)))
        return false;
    return true;
}

void TurretAI::AttackStart(Unit* who)
{
    if (who)
        me->Attack(who, false);
}

void TurretAI::UpdateAI(const uint32 /*diff*/)
{
    if (!UpdateVictim())
        return;

    DoSpellAttackIfReady(me->_spells[0]);
}

//////////////
//VehicleAI
//////////////

VehicleAI::VehicleAI(Creature* c) : CreatureAI(c), m_vehicle(c->GetVehicleKit()), m_IsVehicleInUse(false), m_ConditionsTimer(VEHICLE_CONDITION_CHECK_TIME)
{
    LoadConditions();
    m_DoDismiss = false;
    m_DismissTimer = VEHICLE_DISMISS_TIME;
}

//NOTE: VehicleAI::UpdateAI runs even while the vehicle is mounted
void VehicleAI::UpdateAI(const uint32 diff)
{
    CheckConditions(diff);

    if (m_DoDismiss)
    {
        if (m_DismissTimer < diff)
        {
            m_DoDismiss = false;
            me->SetVisible(false);
            me->DespawnOrUnsummon();
        }else m_DismissTimer -= diff;
    }
}

void VehicleAI::Reset()
{
    me->SetVisible(true);
}

void VehicleAI::OnCharmed(bool apply)
{
    if (m_IsVehicleInUse && !apply && !conditions.empty())//was used and has conditions
    {
        m_DoDismiss = true;//needs reset
        me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_PLAYER_VEHICLE);
        me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
    }
    else if (apply)
        m_DoDismiss = false;//in use again
    m_DismissTimer = VEHICLE_DISMISS_TIME;//reset timer
    m_IsVehicleInUse = apply;
}

void VehicleAI::LoadConditions()
{
    conditions = sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_CREATURE_TEMPLATE_VEHICLE, me->GetEntry());
    if (!conditions.empty())
        sLog->outDebug(LOG_FILTER_CONDITIONSYS, "VehicleAI::LoadConditions: loaded %u conditions", uint32(conditions.size()));
}

void VehicleAI::CheckConditions(const uint32 diff)
{
    if (m_ConditionsTimer < diff)
    {
        if (!conditions.empty())
        {
            for (SeatMap::iterator itr = m_vehicle->Seats.begin(); itr != m_vehicle->Seats.end(); ++itr)
                if (Unit* passenger = ObjectAccessor::GetUnit(*m_vehicle->GetBase(), itr->second.Passenger))
                {
                    if (Player* player = passenger->ToPlayer())
                    {
                        if (!sConditionMgr->IsPlayerMeetToConditions(player, conditions))
                        {
                            player->ExitVehicle();
                            return;//check other pessanger in next tick
                        }
                    }
                }
        }
        m_ConditionsTimer = VEHICLE_CONDITION_CHECK_TIME;
    } else m_ConditionsTimer -= diff;
}
