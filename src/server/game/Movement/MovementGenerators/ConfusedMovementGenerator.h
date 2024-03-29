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

#ifndef TRINITY_CONFUSEDGENERATOR_H
#define TRINITY_CONFUSEDGENERATOR_H

#include "MovementGenerator.h"
#include "Timer.h"

#define MAX_CONF_WAYPOINTS 24

template<class T>
class ConfusedMovementGenerator
: public MovementGeneratorMedium< T, ConfusedMovementGenerator<T> >
{
    public:
        explicit ConfusedMovementGenerator() : i_nextMoveTime(0) {}

        void Initialize(T &);
        void Finalize(T &);
        void Reset(T &);
        bool Update(T &, const uint32 &);

        MovementGeneratorType GetMovementGeneratorType() { return CONFUSED_MOTION_TYPE; }
    private:
        void _InitSpecific(T &, bool &, bool &);
        TimeTracker i_nextMoveTime;
        float i_waypoints[MAX_CONF_WAYPOINTS+1][3];
        uint32 i_nextMove;
};
#endif

