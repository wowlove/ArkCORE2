/*
 * Copyright (C) 2011-2012 ArkCORE2 <http://www.arkania.net/>
 * Copyright (C) 2010-2012 Project SkyFire <http://www.projectskyfire.org/> 
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2012 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/* ScriptData
SDName: Boss_Grilek
SD%Complete: 100
SDComment:
SDCategory: Zul'Gurub
EndScriptData */

#include "ScriptPCH.h"
#include "zulgurub.h"

#define SPELL_AVARTAR                 24646                //The Enrage Spell
#define SPELL_GROUNDTREMOR            6524

class boss_grilek : public CreatureScript
{
    public:
        boss_grilek() : CreatureScript("boss_grilek") {}

        struct boss_grilekAI : public ScriptedAI
        {
            boss_grilekAI(Creature* creature) : ScriptedAI(creature) {}

            uint32 Avartar_Timer;
            uint32 GroundTremor_Timer;

            void Reset()
            {
                Avartar_Timer = urand(15000, 25000);
                Avartar_Timer      = 15000 + rand()%10000;
                GroundTremor_Timer = urand(8000, 16000);
            }

            void EnterCombat(Unit* /*who*/) {}

            void UpdateAI(const uint32 diff)
            {
                //Return since we have no target
                if (!UpdateVictim())
                    return;

                //Avartar_Timer
                if (Avartar_Timer <= diff)
                {
                    DoCast(me, SPELL_AVARTAR);
                    Unit* target = NULL;

                    target = SelectTarget(SELECT_TARGET_RANDOM, 1);

                    if (DoGetThreat(me->getVictim()))
                        DoModifyThreatPercent(me->getVictim(), -50);
                    if (target)
                        AttackStart(target);

                    Avartar_Timer = urand(25000, 35000);
                } else Avartar_Timer -= diff;

                //GroundTremor_Timer
                if (GroundTremor_Timer <= diff)
                {
                    DoCast(me->getVictim(), SPELL_GROUNDTREMOR);
                    GroundTremor_Timer = urand(12000, 16000);
                } else GroundTremor_Timer -= diff;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_grilekAI(creature);
        }
};

void AddSC_boss_grilek()
{
    new boss_grilek();
}