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

#include "Common.h"
#include "GuildMgr.h"

GuildMgr::GuildMgr()
{
    NextGuildId = 1;
}

GuildMgr::~GuildMgr()
{
    for (GuildContainer::iterator itr = GuildStore.begin(); itr != GuildStore.end(); ++itr)
        delete itr->second;
}

void GuildMgr::AddGuild(Guild* guild)
{
    GuildStore[guild->GetId()] = guild;
}

void GuildMgr::RemoveGuild(uint32 guildId)
{
    GuildStore.erase(guildId);
}

uint32 GuildMgr::GenerateGuildId()
{
    if (NextGuildId >= 0xFFFFFFFE)
    {
        sLog->outError("Guild ids overflow!! Can't continue, shutting down server. ");
        World::StopNow(ERROR_EXIT_CODE);
    }
    return NextGuildId++;
}

// Guild collection
Guild* GuildMgr::GetGuildById(uint32 guildId) const
{
    GuildContainer::const_iterator itr = GuildStore.find(guildId);
    if (itr != GuildStore.end())
        return itr->second;

    return NULL;
}

Guild* GuildMgr::GetGuildByName(const std::string& guildName) const
{
    std::string search = guildName;
    std::transform(search.begin(), search.end(), search.begin(), ::toupper);
    for (GuildContainer::const_iterator itr = GuildStore.begin(); itr != GuildStore.end(); ++itr)
    {
        std::string gname = itr->second->GetName();
        std::transform(gname.begin(), gname.end(), gname.begin(), ::toupper);
        if (search == gname)
            return itr->second;
    }
    return NULL;
}

std::string GuildMgr::GetGuildNameById(uint32 guildId) const
{
    if (Guild* guild = GetGuildById(guildId))
        return guild->GetName();

    return "";
}

Guild* GuildMgr::GetGuildByLeader(uint64 guid) const
{
    for (GuildContainer::const_iterator itr = GuildStore.begin(); itr != GuildStore.end(); ++itr)
        if (itr->second->GetLeaderGUID() == guid)
            return itr->second;

    return NULL;
}

void GuildMgr::LoadGuilds()
{
    // 1. Load all guilds
    sLog->outString("Loading guilds definitions...");
    {
        uint32 oldMSTime = getMSTime();

                                                //           0          1       2             3              4              5              6
        QueryResult result = CharacterDatabase.Query("SELECT g.guildid, g.name, g.leaderguid, g.EmblemStyle, g.EmblemColor, g.BorderStyle, g.BorderColor, "
                                                //    7                  8       9       10            11           12         13         14       15
                                                     "g.BackgroundColor, g.info, g.motd, g.createdate, g.BankMoney, COUNT(gbt.guildid), xp, level , m_today_xp, m_xp_cap  "
                                                     "FROM guild g LEFT JOIN guild_bank_tab gbt ON g.guildid = gbt.guildid GROUP BY g.guildid ORDER BY g.guildid ASC");

        if (!result)
        {
            sLog->outString(">> Loaded 0 guild definitions. DB table `guild` is empty.");
            sLog->outString();
            return;
        }
        else
        {
            uint32 count = 0;
            do
            {
                Field* fields = result->Fetch();
                Guild* guild = new Guild();

                if (!guild->LoadFromDB(fields))
                {
                    delete guild;
                    continue;
                }
                QueryResult guildNews = CharacterDatabase.PQuery("SELECT type, date, value1, value2, source_guid, flags FROM guild_news WHERE guildid = %u ORDER BY date DESC", guild->GetId());
                if (guildNews)
                {
                    Field* fields = guildNews->Fetch();
                    guild->LoadGuildNewsFromDB(fields);
                }
                AddGuild(guild);

                ++count;
            }
            while (result->NextRow());

            sLog->outString(">> Loaded %u guild definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
            sLog->outString();
        }
    }

    // 2. Load all guild ranks
    sLog->outString("Loading guild ranks...");
    {
        uint32 oldMSTime = getMSTime();

        // Delete orphaned guild rank entries before loading the valid ones
        CharacterDatabase.DirectExecute("DELETE gr FROM guild_rank gr LEFT JOIN guild g ON gr.guildId = g.guildId WHERE g.guildId IS NULL");

        //                                                         0    1      2       3                4
        QueryResult result = CharacterDatabase.Query("SELECT guildid, rid, rname, rights, BankMoneyPerDay FROM guild_rank ORDER BY guildid ASC, rid ASC");

        if (!result)
        {
            sLog->outString(">> Loaded 0 guild ranks. DB table `guild_rank` is empty.");
            sLog->outString();
        }
        else
        {
            uint32 count = 0;
            do
            {
                Field* fields = result->Fetch();
                uint32 guildId = fields[0].GetUInt32();

                if (Guild* guild = GetGuildById(guildId))
                    guild->LoadRankFromDB(fields);

                ++count;
            }
            while (result->NextRow());

            sLog->outString(">> Loaded %u guild ranks in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
            sLog->outString();
        }
    }

    // 3. Load all guild members
    sLog->outString("Loading guild members...");
    {
        uint32 oldMSTime = getMSTime();

        // Delete orphaned guild member entries before loading the valid ones
        CharacterDatabase.DirectExecute("DELETE gm FROM guild_member gm LEFT JOIN guild g ON gm.guildId = g.guildId WHERE g.guildId IS NULL");

                                                     //          0        1        2     3      4        5                   6
        QueryResult result = CharacterDatabase.Query("SELECT guildid, gm.guid, rank, pnote, offnote, BankResetTimeMoney, BankRemMoney, "
                                                     //   7                  8                 9                  10                11                 12
                                                     "BankResetTimeTab0, BankRemSlotsTab0, BankResetTimeTab1, BankRemSlotsTab1, BankResetTimeTab2, BankRemSlotsTab2, "
                                                     //   13                 14                15                 16                17                 18
                                                     "BankResetTimeTab3, BankRemSlotsTab3, BankResetTimeTab4, BankRemSlotsTab4, BankResetTimeTab5, BankRemSlotsTab5, "
                                                     //   19                 20                21                 22
                                                     "BankResetTimeTab6, BankRemSlotsTab6, BankResetTimeTab7, BankRemSlotsTab7, "
                                                     //   23      24       25       26      27         28               29               30               31               32                33                34
                                                     "c.name, c.level, c.class, c.zone, c.account, c.logout_time, FirstProffLevel, FirstProffSkill, FirstProffRank, SecondProffLevel, SecondProffSkill, SecondProffRank "
                                                     "FROM guild_member gm LEFT JOIN characters c ON c.guid = gm.guid ORDER BY guildid ASC");

        if (!result)
        {
            sLog->outString(">> Loaded 0 guild members. DB table `guild_member` is empty.");
            sLog->outString();
        }
        else
        {
            uint32 count = 0;

            do
            {
                Field* fields = result->Fetch();
                uint32 guildId = fields[0].GetUInt32();

                if (Guild* guild = GetGuildById(guildId))
                    guild->LoadMemberFromDB(fields);

                ++count;
            }
            while (result->NextRow());

            sLog->outString(">> Loaded %u guild members int %u ms", count, GetMSTimeDiffToNow(oldMSTime));
            sLog->outString();
        }
    }

    // 4. Load all guild bank tab rights
    sLog->outString("Loading bank tab rights...");
    {
        uint32 oldMSTime = getMSTime();

        // Delete orphaned guild bank right entries before loading the valid ones
        CharacterDatabase.DirectExecute("DELETE gbr FROM guild_bank_right gbr LEFT JOIN guild g ON gbr.guildId = g.guildId WHERE g.guildId IS NULL");

                                                     //       0        1      2    3        4
        QueryResult result = CharacterDatabase.Query("SELECT guildid, TabId, rid, gbright, SlotPerDay FROM guild_bank_right ORDER BY guildid ASC, TabId ASC");

        if (!result)
        {
            sLog->outString(">> Loaded 0 guild bank tab rights. DB table `guild_bank_right` is empty.");
            sLog->outString();
        }
        else
        {
            uint32 count = 0;
            do
            {
                Field* fields = result->Fetch();
                uint32 guildId = fields[0].GetUInt32();

                if (Guild* guild = GetGuildById(guildId))
                    guild->LoadBankRightFromDB(fields);

                ++count;
            }
            while (result->NextRow());

            sLog->outString(">> Loaded %u bank tab rights in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
            sLog->outString();
        }
    }

    // 5. Load all event logs
    sLog->outString("Loading guild event logs...");
    {
        uint32 oldMSTime = getMSTime();

        CharacterDatabase.DirectPExecute("DELETE FROM guild_eventlog WHERE LogGuid > %u", sWorld->getIntConfig(CONFIG_GUILD_EVENT_LOG_COUNT));

                                                     //          0        1        2          3            4            5        6
        QueryResult result = CharacterDatabase.Query("SELECT guildid, LogGuid, EventType, PlayerGuid1, PlayerGuid2, NewRank, TimeStamp FROM guild_eventlog ORDER BY TimeStamp DESC, LogGuid DESC");

        if (!result)
        {
            sLog->outString(">> Loaded 0 guild event logs. DB table `guild_eventlog` is empty.");
            sLog->outString();
        }
        else
        {
            uint32 count = 0;
            do
            {
                Field* fields = result->Fetch();
                uint32 guildId = fields[0].GetUInt32();

                if (Guild* guild = GetGuildById(guildId))
                    guild->LoadEventLogFromDB(fields);

                ++count;
            }
            while (result->NextRow());

            sLog->outString(">> Loaded %u guild event logs in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
            sLog->outString();
        }
    }

    // 6. Load all bank event logs
    sLog->outString("Loading guild bank event logs...");
    {
        uint32 oldMSTime = getMSTime();

        // Remove log entries that exceed the number of allowed entries per guild
        CharacterDatabase.DirectPExecute("DELETE FROM guild_bank_eventlog WHERE LogGuid > %u", sWorld->getIntConfig(CONFIG_GUILD_BANK_EVENT_LOG_COUNT));

                                                     //          0        1      2        3          4           5            6               7          8
        QueryResult result = CharacterDatabase.Query("SELECT guildid, TabId, LogGuid, EventType, PlayerGuid, ItemOrMoney, ItemStackCount, DestTabId, TimeStamp FROM guild_bank_eventlog ORDER BY TimeStamp DESC, LogGuid DESC");

        if (!result)
        {
            sLog->outString(">> Loaded 0 guild bank event logs. DB table `guild_bank_eventlog` is empty.");
            sLog->outString();
        }
        else
        {
            uint32 count = 0;
            do
            {
                Field* fields = result->Fetch();
                uint32 guildId = fields[0].GetUInt32();

                if (Guild* guild = GetGuildById(guildId))
                    guild->LoadBankEventLogFromDB(fields);

                ++count;
            }
            while (result->NextRow());

            sLog->outString(">> Loaded %u guild bank event logs in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
            sLog->outString();
        }
    }

    // 7. Load all guild bank tabs
    sLog->outString("Loading guild bank tabs...");
    {
        uint32 oldMSTime = getMSTime();

        // Delete orphaned guild bank tab entries before loading the valid ones
        CharacterDatabase.DirectExecute("DELETE gbt FROM guild_bank_tab gbt LEFT JOIN guild g ON gbt.guildId = g.guildId WHERE g.guildId IS NULL");

                                                     //         0        1      2        3        4
        QueryResult result = CharacterDatabase.Query("SELECT guildid, TabId, TabName, TabIcon, TabText FROM guild_bank_tab ORDER BY guildid ASC, TabId ASC");

        if (!result)
        {
            sLog->outString(">> Loaded 0 guild bank tabs. DB table `guild_bank_tab` is empty.");
            sLog->outString();
        }
        else
        {
            uint32 count = 0;
            do
            {
                Field* fields = result->Fetch();
                uint32 guildId = fields[0].GetUInt32();

                if (Guild* guild = GetGuildById(guildId))
                    guild->LoadBankTabFromDB(fields);

                ++count;
            }
            while (result->NextRow());

            sLog->outString(">> Loaded %u guild bank tabs in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
            sLog->outString();
        }
    }

    // 8. Fill all guild bank tabs
    sLog->outString("Filling bank tabs with items...");
    {
        uint32 oldMSTime = getMSTime();

        // Delete orphan guild bank items
        CharacterDatabase.DirectExecute("DELETE gbi FROM guild_bank_item gbi LEFT JOIN guild g ON gbi.guildId = g.guildId WHERE g.guildId IS NULL");

                                                     //          0            1                2      3         4        5      6             7                 8           9           10
        QueryResult result = CharacterDatabase.Query("SELECT creatorGuid, giftCreatorGuid, count, duration, charges, flags, enchantments, randomPropertyId, durability, playedTime, text, "
                                                     //   11       12     13      14         15
                                                     "guildid, TabId, SlotId, item_guid, itemEntry FROM guild_bank_item gbi INNER JOIN item_instance ii ON gbi.item_guid = ii.guid");

        if (!result)
        {
            sLog->outString(">> Loaded 0 guild bank tab items. DB table `guild_bank_item` or `item_instance` is empty.");
            sLog->outString();
        }
        else
        {
            uint32 count = 0;
            do
            {
                Field* fields = result->Fetch();
                uint32 guildId = fields[11].GetUInt32();

                if (Guild* guild = GetGuildById(guildId))
                    guild->LoadBankItemFromDB(fields);

                ++count;
            }
            while (result->NextRow());

            sLog->outString(">> Loaded %u guild bank tab items in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
            sLog->outString();
        }
    }

    // 9. Validate loaded guild data
    sLog->outString("Validating data of loaded guilds...");
    {
        uint32 oldMSTime = getMSTime();
        std::set<Guild*> rm; // temporary storage to avoid modifying GuildStore with RemoveGuild() while iterating

        for (GuildContainer::iterator itr = GuildStore.begin(); itr != GuildStore.end(); ++itr)
        {
            Guild* guild = itr->second;
            if (guild && !guild->Validate())
                rm.insert(guild);
        }
        for (std::set<Guild*>::iterator itr = rm.begin(); itr != rm.end(); ++itr)
        {
            Guild* guild = *itr;
            RemoveGuild(guild->GetId());
            delete guild;
        }

        sLog->outString(">> Validated data of loaded guilds in %u ms", GetMSTimeDiffToNow(oldMSTime));
        sLog->outString();
    }
}

uint32 GetXPForLevel(uint8 level);
uint32 GetXPForGuildLevel(uint8 level);

void GuildMgr::LoadGuildRewards()
{
    QueryResult result = WorldDatabase.Query("SELECT item_entry, price, achievement, standing FROM guild_rewards");

    if (!result)
    {
        sLog->outString();
        sLog->outString(">> Loaded 0 guild reward definitions");
        return;
    }

    uint32 count = 0;
    do
    {
        Field *fields = result->Fetch();

        GuildRewardsEntry reward;
        reward.item = fields[0].GetUInt32();
        reward.price = fields[1].GetUInt32();
        reward.achievement = fields[2].GetUInt32();
        reward.standing = fields[3].GetUInt32();
        mGuildRewards.push_back(reward);

        ++count;
    }while (result->NextRow());

    sLog->outString();
    sLog->outString(">> Loaded %u guild reward definitions.");
}
