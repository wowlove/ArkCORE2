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

#include "HomeMovementGenerator.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "WorldPacket.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"

void HomeMovementGenerator<Creature>::Initialize(Creature & owner)
{
    owner.AddUnitState(UNIT_STAT_EVADE);
    _setTargetLocation(owner);
}

void HomeMovementGenerator<Creature>::Reset(Creature &)
{
}

void HomeMovementGenerator<Creature>::_setTargetLocation(Creature & owner)
{
    if (!&owner)
        return;

    if (owner.HasUnitState(UNIT_STAT_ROOT | UNIT_STAT_STUNNED | UNIT_STAT_DISTRACTED))
        return;

    Movement::MoveSplineInit init(owner);
    float x, y, z, o;
    // at apply we can select more nice return points base at current movegen
    //if (owner.GetMotionMaster()->empty() || !owner.GetMotionMaster()->top()->GetResetPosition(owner, x, y, z))
    //{
    owner.GetHomePosition(x, y, z, o);
    init.SetFacing(o);
    //}
    init.MoveTo(x, y, z);
    init.SetWalk(false);
    init.Launch();

    arrived = false;
    owner.ClearUnitState(UNIT_STAT_ALL_STATE & ~UNIT_STAT_EVADE);
}

bool HomeMovementGenerator<Creature>::Update(Creature &owner, const uint32 time_diff)
{
    arrived = owner.movespline->Finalized();
    return !arrived;
}

void HomeMovementGenerator<Creature>::Finalize(Creature& owner)
{
    if (arrived)
    {
        owner.ClearUnitState(UNIT_STAT_EVADE);
        owner.SetWalk(true);
        owner.LoadCreaturesAddon(true);
        owner.AI()->JustReachedHome();
    }
}
