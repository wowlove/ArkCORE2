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

#include "Log.h"
#include "ObjectAccessor.h"
#include "CreatureAI.h"
#include "ObjectMgr.h"
#include "TemporarySummon.h"

TempSummon::TempSummon(SummonPropertiesEntry const* properties, Unit* owner, bool isWorldObject) :
Creature(isWorldObject), m_Properties(properties), m_type(TEMPSUMMON_MANUAL_DESPAWN),
_timer(0), m_lifetime(0)
{
    m_summonerGUID = owner ? owner->GetGUID() : 0;
    m_unitTypeMask |= UNIT_MASK_SUMMON;
}

Unit* TempSummon::GetSummoner() const
{
    return m_summonerGUID ? ObjectAccessor::GetUnit(*this, m_summonerGUID) : NULL;
}

void TempSummon::Update(uint32 diff)
{
    Creature::Update(diff);

    if (_deathState == DEAD)
    {
        UnSummon();
        return;
    }
    switch (m_type)
    {
        case TEMPSUMMON_MANUAL_DESPAWN:
            break;
        case TEMPSUMMON_TIMED_DESPAWN:
        {
            if (_timer <= diff)
            {
                UnSummon();
                return;
            }

            _timer -= diff;
            break;
        }
        case TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT:
        {
            if (!isInCombat())
            {
                if (_timer <= diff)
                {
                    UnSummon();
                    return;
                }

                _timer -= diff;
            }
            else if (_timer != m_lifetime)
                _timer = m_lifetime;

            break;
        }

        case TEMPSUMMON_CORPSE_TIMED_DESPAWN:
        {
            if (_deathState == CORPSE)
            {
                if (_timer <= diff)
                {
                    UnSummon();
                    return;
                }

                _timer -= diff;
            }
            break;
        }
        case TEMPSUMMON_CORPSE_DESPAWN:
        {
            // if _deathState is DEAD, CORPSE was skipped
            if (_deathState == CORPSE || _deathState == DEAD)
            {
                UnSummon();
                return;
            }

            break;
        }
        case TEMPSUMMON_DEAD_DESPAWN:
        {
            if (_deathState == DEAD)
            {
                UnSummon();
                return;
            }
            break;
        }
        case TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN:
        {
            // if _deathState is DEAD, CORPSE was skipped
            if (_deathState == CORPSE || _deathState == DEAD)
            {
                UnSummon();
                return;
            }

            if (!isInCombat())
            {
                if (_timer <= diff)
                {
                    UnSummon();
                    return;
                }
                else
                    _timer -= diff;
            }
            else if (_timer != m_lifetime)
                _timer = m_lifetime;
            break;
        }
        case TEMPSUMMON_TIMED_OR_DEAD_DESPAWN:
        {
            // if _deathState is DEAD, CORPSE was skipped
            if (_deathState == DEAD)
            {
                UnSummon();
                return;
            }

            if (!isInCombat() && isAlive())
            {
                if (_timer <= diff)
                {
                    UnSummon();
                    return;
                }
                else
                    _timer -= diff;
            }
            else if (_timer != m_lifetime)
                _timer = m_lifetime;
            break;
        }
        default:
            UnSummon();
            sLog->outError("Temporary summoned creature (entry: %u) have unknown type %u of ", GetEntry(), m_type);
            break;
    }
}

void TempSummon::InitStats(uint32 duration)
{
    ASSERT(!isPet());

    _timer = duration;
    m_lifetime = duration;

    if (m_type == TEMPSUMMON_MANUAL_DESPAWN)
        m_type = (duration == 0) ? TEMPSUMMON_DEAD_DESPAWN : TEMPSUMMON_TIMED_DESPAWN;

    Unit* owner = GetSummoner();

    if (owner && isTrigger() && _spells[0])
    {
        setFaction(owner->getFaction());
        SetLevel(owner->getLevel());
        if (owner->GetTypeId() == TYPEID_PLAYER)
            _ControlledByPlayer = true;
    }

    if (!m_Properties)
        return;

    if (owner)
    {
        if (uint32 slot = m_Properties->Slot)
        {
            if (owner->m_SummonSlot[slot] && owner->m_SummonSlot[slot] != GetGUID())
            {
                Creature* oldSummon = GetMap()->GetCreature(owner->m_SummonSlot[slot]);
                if (oldSummon && oldSummon->isSummon())
                    oldSummon->ToTempSummon()->UnSummon();
            }
            owner->m_SummonSlot[slot] = GetGUID();
        }
    }

    if (m_Properties->Faction)
        setFaction(m_Properties->Faction);
    else if (IsVehicle() && owner) // properties should be vehicle
        setFaction(owner->getFaction());
}

void TempSummon::InitSummon()
{
    Unit* owner = GetSummoner();
    if (owner)
    {
        if (owner->GetTypeId() == TYPEID_UNIT && owner->ToCreature()->IsAIEnabled)
            owner->ToCreature()->AI()->JustSummoned(this);
        if (IsAIEnabled)
            AI()->IsSummonedBy(owner);
    }
}

void TempSummon::SetTempSummonType(TempSummonType type)
{
    m_type = type;
}

void TempSummon::UnSummon(uint32 msTime)
{
    if (msTime)
    {
        ForcedUnsummonDelayEvent* pEvent = new ForcedUnsummonDelayEvent(*this);

        _Events.AddEvent(pEvent, _Events.CalculateTime(msTime));
        return;
    }

    //ASSERT(!isPet());
    if (isPet())
    {
        ((Pet*)this)->Remove(PET_SLOT_OTHER_PET);
        ASSERT(!IsInWorld());
        return;
    }

    Unit* owner = GetSummoner();
    if (owner && owner->GetTypeId() == TYPEID_UNIT && owner->ToCreature()->IsAIEnabled)
        owner->ToCreature()->AI()->SummonedCreatureDespawn(this);

    AddObjectToRemoveList();
}

bool ForcedUnsummonDelayEvent::Execute(uint64 /*e_time*/, uint32 /*p_time*/)
{
    _owner.UnSummon();
    return true;
}

void TempSummon::RemoveFromWorld()
{
    if (!IsInWorld())
        return;

    if (m_Properties)
        if (uint32 slot = m_Properties->Slot)
            if (Unit* owner = GetSummoner())
                if (owner->m_SummonSlot[slot] == GetGUID())
                    owner->m_SummonSlot[slot] = 0;

    //if (GetOwnerGUID())
    //    sLog->outError("Unit %u has owner guid when removed from world", GetEntry());

    Creature::RemoveFromWorld();
}

Minion::Minion(SummonPropertiesEntry const* properties, Unit* owner, bool isWorldObject) : TempSummon(properties, owner, isWorldObject), _owner(owner)
{
    ASSERT(_owner);
    m_unitTypeMask |= UNIT_MASK_MINION;
    m_followAngle = PET_FOLLOW_ANGLE;
}

void Minion::InitStats(uint32 duration)
{
    TempSummon::InitStats(duration);

    SetReactState(REACT_PASSIVE);

    SetCreatorGUID(_owner->GetGUID());
    setFaction(_owner->getFaction());

    _owner->SetMinion(this, true, PET_SLOT_UNK_SLOT);
}

void Minion::RemoveFromWorld()
{
    if (!IsInWorld())
        return;

    _owner->SetMinion(this, false, PET_SLOT_UNK_SLOT);
    TempSummon::RemoveFromWorld();
}

bool Minion::IsGuardianPet() const
{
    return isPet() || (m_Properties && m_Properties->Category == SUMMON_CATEGORY_PET);
}

Guardian::Guardian(SummonPropertiesEntry const* properties, Unit* owner, bool isWorldObject) : Minion(properties, owner, isWorldObject), m_bonusSpellDamage(0)
{
    memset(m_statFromOwner, 0, sizeof(float)*MAX_STATS);
    m_unitTypeMask |= UNIT_MASK_GUARDIAN;
    if (properties && properties->Type == SUMMON_TYPE_PET)
    {
        m_unitTypeMask |= UNIT_MASK_CONTROLABLE_GUARDIAN;
        InitCharmInfo();
    }
}

void Guardian::InitStats(uint32 duration)
{
    Minion::InitStats(duration);

    InitStatsForLevel(_owner->getLevel());

    if (_owner->GetTypeId() == TYPEID_PLAYER && HasUnitTypeMask(UNIT_MASK_CONTROLABLE_GUARDIAN))
        m_charmInfo->InitCharmCreateSpells();

    SetReactState(REACT_AGGRESSIVE);
}

void Guardian::InitSummon()
{
    TempSummon::InitSummon();

    if (_owner->GetTypeId() == TYPEID_PLAYER
        && _owner->GetMinionGUID() == GetGUID()
        && !_owner->GetCharmGUID())
        _owner->ToPlayer()->CharmSpellInitialize();
}

Puppet::Puppet(SummonPropertiesEntry const* properties, Unit* owner) : Minion(properties, owner, false) //maybe true?
{
    ASSERT(owner->GetTypeId() == TYPEID_PLAYER);
    _owner = (Player*)owner;
    m_unitTypeMask |= UNIT_MASK_PUPPET;
}

void Puppet::InitStats(uint32 duration)
{
    Minion::InitStats(duration);
    SetLevel(_owner->getLevel());
    SetReactState(REACT_PASSIVE);
}

void Puppet::InitSummon()
{
    Minion::InitSummon();
    if (!SetCharmedBy(_owner, CHARM_TYPE_POSSESS))
        ASSERT(false);
}

void Puppet::Update(uint32 time)
{
    Minion::Update(time);
    //check if caster is channelling?
    if (IsInWorld())
    {
        if (!isAlive())
        {
            UnSummon();
            // TODO: why long distance .die does not remove it
        }
    }
}

void Puppet::RemoveFromWorld()
{
    if (!IsInWorld())
        return;

    RemoveCharmedBy(NULL);
    Minion::RemoveFromWorld();
}
