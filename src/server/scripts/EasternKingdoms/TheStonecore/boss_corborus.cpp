/*
 * Copyright (C) 2011-2012 ArkCORE2 <http://www.arkania.net/>
 * Copyright (C) 2010-2012 Project SkyFire <http://www.projectskyfire.org/> 
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

#include "ScriptPCH.h"
#include "the_stonecore.h"

enum Yells
{
};

enum Spells
{
	SPELL_CRYSTAL_BARRAGE            = 86881,     // Crystal Barrage normal
	SPELL_CRYSTAL_BARRAGE_H          = 92648,     // Crystal Barrage heroic
	SPELL_DAMPENING_WAVE             = 82415,     // Dampening Wave normal
	SPELL_DAMPENING_WAVE_H           = 92650,     // Dampening Wave heroic
	SPELL_BURROW                     = 26381,     // Burrow
	SPELL_ROCK_BORE                  = 80028,     // Rock Bore
};

class npc_rock_borer : public CreatureScript
{
public:
	npc_rock_borer() : CreatureScript("npc_rock_borer") { }

	CreatureAI* GetAI(Creature* creature) const
	{
		return new npc_rock_borerAI(creature);
	}

    struct npc_rock_borerAI : public ScriptedAI
	{
		npc_rock_borerAI(Creature* creature) : ScriptedAI(creature)
		{
			instance = creature->GetInstanceScript();
		}

        InstanceScript* instance;

        uint32 _SpellBoreTimer;

        void Reset()
		{
			_SpellBoreTimer = 6000;
		}

        void EnterCombar(Unit* ) { }

        void MoveInLineOfSight(Unit* who) { }

        void UpdateAI(const uint32 Diff)
		{
			if (_SpellBoreTimer <= Diff)
			{
				DoCast(me->getVictim(),SPELL_ROCK_BORE);
				_SpellBoreTimer = 6000;
			}
			else
				_SpellBoreTimer -= Diff;

            DoMeleeAttackIfReady();
		}
	};
};

class boss_corborus : public CreatureScript
{
public:
    boss_corborus() : CreatureScript("boss_corborus") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_corborusAI (creature);
    }

    struct boss_corborusAI : public ScriptedAI
    {
        boss_corborusAI(Creature* creature) : ScriptedAI(creature), Summons(me)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
		SummonList Summons;

        uint32 _SummonBorerTimer;
		uint32 b_BorrowTimer;
		uint32 _CrystalTimer;
		uint32 _DampeningTimer;
		bool b_BORROW;               //bool var type for invisible mode

        void Reset()
		{
			instance->SetData(DATA_CORBORUS_EVENT,NOT_STARTED);
			_SummonBorerTimer    = 33000;
			b_BORROW             = 0;
			_CrystalTimer        = 13600;
			_DampeningTimer      = 25000;
			Summons.DespawnAll();
		}

		void JustDied(Unit* )
		{
			if (instance)
				instance->SetData(DATA_CORBORUS_EVENT,DONE);
			Summons.DespawnAll();
		}

        void EnterCombat(Unit* /*who*/)
		{
			instance->SetData(DATA_CORBORUS_EVENT,IN_PROGRESS);
			DoZoneInCombat();
		}

		void SummonRockBorer()
		{
			if (!IsHeroic())
			    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM,0))
			        for (uint8 i=0;i<2;++i)                                   //summon 2 Rock Borer Normal Mode
				me->SummonCreature(NPC_ROCK_BORER,target->GetPositionX(),target->GetPositionY(),target->GetPositionZ(),0.0f, TEMPSUMMON_CORPSE_DESPAWN);

            if (IsHeroic())
			    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM,0))
			        for (uint8 i=0;i<4;++i)                                  //summon 4 Rock Borer Heroic Mode
				me->SummonCreature(NPC_ROCK_BORER,target->GetPositionX(),target->GetPositionY(),target->GetPositionZ(),0.0f, TEMPSUMMON_CORPSE_DESPAWN);
		}

		void JustSummoned(Creature* summoned)
        {
            summoned->SetInCombatWithZone();

            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                summoned->AI()->AttackStart(target);

            switch (summoned->GetEntry()) { }
            Summons.Summon(summoned);
        }

		void SummonedCreatureDespawn(Creature* summon)
        {
            Summons.Despawn(summon);
        }

        void UpdateAI(const uint32 Diff)
        {
            if (!UpdateVictim())
				return;

			if (_SummonBorerTimer <= Diff)
			{
				instance->DoSendNotifyToInstance("INSTANCE MESSAGE: Rock Borer are spawned"); // Notify to players when Rock Borer are spawn
				b_BORROW = 1;
				DoCast(me,SPELL_BURROW);
				SummonRockBorer();
				_SummonBorerTimer = 30000;
				b_BorrowTimer = 9000;
			}
			else
			{
				_SummonBorerTimer -= Diff;
				b_BorrowTimer -= Diff;
			}
            if (b_BorrowTimer <= Diff)
				b_BORROW = 0;

			if (_CrystalTimer <= Diff)
			{
				if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
					if (!IsHeroic())
					{
                        DoCast(target,SPELL_CRYSTAL_BARRAGE);
					}
					else
                        DoCast(target,SPELL_CRYSTAL_BARRAGE_H);

                    _CrystalTimer = 11000;
			}
			else
				_CrystalTimer -= Diff;

            if (_DampeningTimer <= Diff)
			{
				if (!IsHeroic())
					DoCast(SPELL_DAMPENING_WAVE);
				else
					DoCast(SPELL_DAMPENING_WAVE_H);

                _DampeningTimer = 20000;
			}
			else
				_DampeningTimer -= Diff;

			if (b_BORROW == 0)                    //if not invisible
				DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_corborus()
{
    new boss_corborus();
    new npc_rock_borer();
}
