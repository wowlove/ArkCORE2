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
SDName: Silverpine_Forest
SD%Complete: 100
SDComment: Quest support: 1886, 435, 452
SDCategory: Silverpine Forest
EndScriptData */

/* ContentData
npc_astor_hadren
npc_deathstalker_erland
pyrewood_ambush
EndContentData */

#include "ScriptPCH.h"
#include "ScriptedEscortAI.h"

/*######
## npc_astor_hadren
######*/

#define GOSSIP_HAH "You're Astor Hadren, right?"
#define GOSSIP_SAH "You've got something I need, Astor. And I'll be taking it now."

class npc_astor_hadren : public CreatureScript
{
public:
    npc_astor_hadren() : CreatureScript("npc_astor_hadren") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_astor_hadrenAI(creature);
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*Sender*/, uint32 Action)
    {
        player->PlayerTalkClass->ClearMenus();
        switch (Action)
        {
            case GOSSIP_ACTION_INFO_DEF + 1:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SAH, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
                player->SEND_GOSSIP_MENU(624, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF + 2:
                player->CLOSE_GOSSIP_MENU();
                creature->setFaction(21);
                if (player)
                    CAST_AI(npc_astor_hadren::npc_astor_hadrenAI, creature->AI())->AttackStart(player);
                break;
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (player->GetQuestStatus(1886) == QUEST_STATUS_INCOMPLETE)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_HAH, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

        player->SEND_GOSSIP_MENU(623, creature->GetGUID());

        return true;
    }

    struct npc_astor_hadrenAI : public ScriptedAI
    {
        npc_astor_hadrenAI(Creature* creature) : ScriptedAI(creature) {}

        void Reset()
        {
            me->setFaction(68);
        }

        void EnterCombat(Unit* /*who*/)
        {
        }

        void JustDied(Unit* /*who*/)
        {
            me->setFaction(68);
        }
    };
};

/*######
## npc_deathstalker_erland
######*/

enum eErland
{
    SAY_QUESTACCEPT     = -1000306,
    SAY_START           = -1000307,
    SAY_AGGRO_1         = -1000308,
    SAY_AGGRO_2         = -1000309,
    SAY_LAST            = -1000310,

    SAY_THANKS          = -1000311,
    SAY_RANE            = -1000312,
    SAY_ANSWER          = -1000313,
    SAY_MOVE_QUINN      = -1000314,

    SAY_GREETINGS       = -1000315,
    SAY_QUINN           = -1000316,
    SAY_ON_BYE          = -1000317,

    QUEST_ESCORTING     = 435,
    NPC_RANE            = 1950,
    NPC_QUINN           = 1951
};

class npc_deathstalker_erland : public CreatureScript
{
public:
    npc_deathstalker_erland() : CreatureScript("npc_deathstalker_erland") { }

    struct npc_deathstalker_erlandAI : public npc_escortAI
    {
        npc_deathstalker_erlandAI(Creature* creature) : npc_escortAI(creature) {}

        void WaypointReached(uint32 i)
        {
            Player* player = GetPlayerForEscort();

            if (!player)
                return;

            switch (i)
            {
            case 1: DoScriptText(SAY_START, me, player);break;
            case 13:
                DoScriptText(SAY_LAST, me, player);
                player->GroupEventHappens(QUEST_ESCORTING, me); break;
            case 14: DoScriptText(SAY_THANKS, me, player); break;
            case 15: {
                    Unit* Rane = me->FindNearestCreature(NPC_RANE, 20);
                    if (Rane)
                        DoScriptText(SAY_RANE, Rane);
                    break;}
            case 16: DoScriptText(SAY_ANSWER, me); break;
            case 17: DoScriptText(SAY_MOVE_QUINN, me); break;
            case 24: DoScriptText(SAY_GREETINGS, me); break;
            case 25: {
                    Unit* Quinn = me->FindNearestCreature(NPC_QUINN, 20);
                    if (Quinn)
                        DoScriptText(SAY_QUINN, Quinn);
                    break;}
            case 26: DoScriptText(SAY_ON_BYE, me, NULL); break;
            }
        }

        void Reset() {}

        void EnterCombat(Unit* who)
        {
            DoScriptText(RAND(SAY_AGGRO_1, SAY_AGGRO_2), me, who);
        }
    };

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
    {
        if (quest->GetQuestId() == QUEST_ESCORTING)
        {
            DoScriptText(SAY_QUESTACCEPT, creature, player);

            if (npc_escortAI* escortAI = CAST_AI(npc_deathstalker_erland::npc_deathstalker_erlandAI, creature->AI()))
                escortAI->Start(true, false, player->GetGUID());
        }

        return true;
    }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_deathstalker_erlandAI(creature);
    }
};

/*######
## pyrewood_ambush
#######*/

#define QUEST_PYREWOOD_AMBUSH 452

#define NPCSAY_INIT "Get ready, they'll be arriving any minute..." //not blizzlike
#define NPCSAY_END "Thanks for your help!" //not blizzlike

static float PyrewoodSpawnPoints[3][4] =
{
    //pos_x   pos_y     pos_z    orien
    //outside
    /*
    {-400.85f, 1513.64f, 18.67f, 0},
    {-397.32f, 1514.12f, 18.67f, 0},
    {-397.44f, 1511.09f, 18.67f, 0},
    */
    //door
    {-396.17f, 1505.86f, 19.77f, 0},
    {-396.91f, 1505.77f, 19.77f, 0},
    {-397.94f, 1504.74f, 19.77f, 0},
};

#define WAIT_SECS 6000

class pyrewood_ambush : public CreatureScript
{
public:
    pyrewood_ambush() : CreatureScript("pyrewood_ambush") { }

    bool OnQuestAccept(Player* player, Creature* creature, const Quest *quest)
    {
        if (quest->GetQuestId() == QUEST_PYREWOOD_AMBUSH && !CAST_AI(pyrewood_ambush::pyrewood_ambushAI, creature->AI())->QuestInProgress)
        {
            CAST_AI(pyrewood_ambush::pyrewood_ambushAI, creature->AI())->QuestInProgress = true;
            CAST_AI(pyrewood_ambush::pyrewood_ambushAI, creature->AI())->Phase = 0;
            CAST_AI(pyrewood_ambush::pyrewood_ambushAI, creature->AI())->KillCount = 0;
            CAST_AI(pyrewood_ambush::pyrewood_ambushAI, creature->AI())->PlayerGUID = player->GetGUID();
        }

        return true;
    }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new pyrewood_ambushAI (creature);
    }

    struct pyrewood_ambushAI : public ScriptedAI
    {
        pyrewood_ambushAI(Creature* creature) : ScriptedAI(creature), Summons(me)
        {
           QuestInProgress = false;
        }

        uint32 Phase;
        int8 KillCount;
        uint32 WaitTimer;
        uint64 PlayerGUID;
        SummonList Summons;

        bool QuestInProgress;

        void Reset()
        {
            WaitTimer = WAIT_SECS;

            if (!QuestInProgress) //fix reset values (see UpdateVictim)
            {
                Phase = 0;
                KillCount = 0;
                PlayerGUID = 0;
                Summons.DespawnAll();
            }
        }

        void EnterCombat(Unit* /*who*/) {}

        void JustSummoned(Creature* summoned)
        {
            Summons.Summon(summoned);
            ++KillCount;
        }

        void SummonedCreatureDespawn(Creature* summoned)
        {
            Summons.Despawn(summoned);
            --KillCount;
        }

        void SummonCreatureWithRandomTarget(uint32 creatureId, int position)
        {
            if (Creature* summoned = me->SummonCreature(creatureId, PyrewoodSpawnPoints[position][0], PyrewoodSpawnPoints[position][1], PyrewoodSpawnPoints[position][2], PyrewoodSpawnPoints[position][3], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15000))
            {
                Player* player = NULL;
                Unit* target = NULL;
                if (PlayerGUID)
                {
                    player = Unit::GetPlayer(*me, PlayerGUID);
                    if (player)
                        target = RAND((Unit*)me, (Unit*)player);
                } else
                    target = me;

                if (target)
                {
                    summoned->setFaction(168);
                    summoned->AddThreat(target, 32.0f);
                    summoned->AI()->AttackStart(target);
                }
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            if (PlayerGUID)
                if (Player* player = Unit::GetPlayer(*me, PlayerGUID))
                    if (player->GetQuestStatus(QUEST_PYREWOOD_AMBUSH) == QUEST_STATUS_INCOMPLETE)
                        player->FailQuest(QUEST_PYREWOOD_AMBUSH);
        }

        void UpdateAI(const uint32 diff)
        {
            //sLog->outString("DEBUG: p(%i) k(%i) d(%u) W(%i)", Phase, KillCount, diff, WaitTimer);

            if (!QuestInProgress)
                return;

            if (KillCount && Phase < 6)
            {
                if (!UpdateVictim()) //reset() on target Despawn...
                    return;

                DoMeleeAttackIfReady();
                return;
            }

            switch (Phase)
            {
                case 0:
                    if (WaitTimer == WAIT_SECS)
                        me->MonsterSay(NPCSAY_INIT, LANG_UNIVERSAL, 0); //no blizzlike

                    if (WaitTimer <= diff)
                    {
                        WaitTimer -= diff;
                        return;
                    }
                    break;
                case 1:
                    SummonCreatureWithRandomTarget(2060, 1);
                    break;
                case 2:
                    SummonCreatureWithRandomTarget(2061, 2);
                    SummonCreatureWithRandomTarget(2062, 0);
                    break;
                case 3:
                    SummonCreatureWithRandomTarget(2063, 1);
                    SummonCreatureWithRandomTarget(2064, 2);
                    SummonCreatureWithRandomTarget(2065, 0);
                    break;
                case 4:
                    SummonCreatureWithRandomTarget(2066, 1);
                    SummonCreatureWithRandomTarget(2067, 0);
                    SummonCreatureWithRandomTarget(2068, 2);
                    break;
                case 5: //end
                    if (PlayerGUID)
                    {
                        if (Player* player = Unit::GetPlayer(*me, PlayerGUID))
                        {
                            me->MonsterSay(NPCSAY_END, LANG_UNIVERSAL, 0); //not blizzlike
                            player->GroupEventHappens(QUEST_PYREWOOD_AMBUSH, me);
                        }
                    }
                    QuestInProgress = false;
                    Reset();
                    break;
            }
            ++Phase; //prepare next phase
        }
    };
};

/*######
## AddSC
######*/

void AddSC_silverpine_forest()
{
    new npc_astor_hadren();
    new npc_deathstalker_erland();
    new pyrewood_ambush();
}