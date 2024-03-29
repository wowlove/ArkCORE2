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

#include "ScriptPCH.h"
#include "ScriptSystem.h"
#include "ObjectMgr.h"
#include "DatabaseEnv.h"

ScriptPointVector const SystemMgr::_empty;

void SystemMgr::LoadScriptTexts()
{
    sLog->outString("TSCR: Loading Script Texts...");
    LoadTrinityStrings("script_texts", TEXT_SOURCE_RANGE, 1+(TEXT_SOURCE_RANGE*2));

    sLog->outString("TSCR: Loading Script Texts additional data...");
    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query("SELECT entry, sound, type, language, emote FROM script_texts");

    if (!result)
    {
        sLog->outString(">> Loaded 0 additional Script Texts data. DB table `script_texts` is empty.");
        sLog->outString();
        return;
    }

    uint32 uiCount = 0;

    do
    {
        Field* fields = result->Fetch();
        StringTextData temp;

        int32 Id           = fields[0].GetInt32();
        temp.SoundId     = fields[1].GetUInt32();
        temp.Type        = fields[2].GetUInt32();
        temp.Language    = fields[3].GetUInt32();
        temp.Emote       = fields[4].GetUInt32();

        if (Id >= 0)
        {
            sLog->outErrorDb("TSCR: Entry %i in table `script_texts` is not a negative value.", Id);
            continue;
        }

        if (Id > TEXT_SOURCE_RANGE || Id <= TEXT_SOURCE_RANGE*2)
        {
            sLog->outErrorDb("TSCR: Entry %i in table `script_texts` is out of accepted entry range for table.", Id);
            continue;
        }

        if (temp.SoundId)
        {
            if (!GetSoundEntriesStore()->LookupEntry(temp.SoundId))
                sLog->outErrorDb("TSCR: Entry %i in table `script_texts` has soundId %u but sound does not exist.", Id, temp.SoundId);
        }

        if (!GetLanguageDescByID(temp.Language))
            sLog->outErrorDb("TSCR: Entry %i in table `script_texts` using Language %u but Language does not exist.", Id, temp.Language);

        if (temp.Type > CHAT_TYPE_ZONE_YELL)
            sLog->outErrorDb("TSCR: Entry %i in table `script_texts` has Type %u but this Chat Type does not exist.", Id, temp.Type);

        m_mTextDataMap[Id] = temp;
        ++uiCount;
    } while (result->NextRow());

    sLog->outString(">> Loaded %u additional Script Texts data in %u ms", uiCount, GetMSTimeDiffToNow(oldMSTime));
    sLog->outString();
}

void SystemMgr::LoadScriptTextsCustom()
{
    sLog->outString("TSCR: Loading Custom Texts...");
    LoadTrinityStrings("custom_texts", TEXT_SOURCE_RANGE*2, 1+(TEXT_SOURCE_RANGE*3));

    sLog->outString("TSCR: Loading Custom Texts additional data...");

    QueryResult result = WorldDatabase.Query("SELECT entry, sound, type, language, emote FROM custom_texts");

    if (!result)
    {
        sLog->outString(">> Loaded 0 additional Custom Texts data. DB table `custom_texts` is empty.");
        sLog->outString();
        return;
    }

    uint32 uiCount = 0;

    do
    {
        Field* fields = result->Fetch();
        StringTextData temp;

        int32 iId              = fields[0].GetInt32();
        temp.SoundId        = fields[1].GetUInt32();
        temp.Type           = fields[2].GetUInt32();
        temp.Language       = fields[3].GetUInt32();
        temp.Emote          = fields[4].GetUInt32();

        if (iId >= 0)
        {
            sLog->outErrorDb("TSCR: Entry %i in table `custom_texts` is not a negative value.", iId);
            continue;
        }

        if (iId > TEXT_SOURCE_RANGE * 2 || iId <= TEXT_SOURCE_RANGE * 3)
        {
            sLog->outErrorDb("TSCR: Entry %i in table `custom_texts` is out of accepted entry range for table.", iId);
            continue;
        }

        if (temp.SoundId)
        {
            if (!GetSoundEntriesStore()->LookupEntry(temp.SoundId))
                sLog->outErrorDb("TSCR: Entry %i in table `custom_texts` has soundId %u but sound does not exist.", iId, temp.SoundId);
        }

        if (!GetLanguageDescByID(temp.Language))
            sLog->outErrorDb("TSCR: Entry %i in table `custom_texts` using Language %u but Language does not exist.", iId, temp.Language);

        if (temp.Type > CHAT_TYPE_ZONE_YELL)
            sLog->outErrorDb("TSCR: Entry %i in table `custom_texts` has Type %u but this Chat Type does not exist.", iId, temp.Type);

        m_mTextDataMap[iId] = temp;
        ++uiCount;
    } while (result->NextRow());

    sLog->outString(">> Loaded %u additional Custom Texts data.", uiCount);
    sLog->outString();
}

void SystemMgr::LoadScriptWaypoints()
{
    uint32 oldMSTime = getMSTime();

    // Drop Existing Waypoint list
    m_mPointMoveMap.clear();

    uint64 uiCreatureCount = 0;

    // Load Waypoints
    QueryResult result = WorldDatabase.Query("SELECT COUNT(entry) FROM script_waypoint GROUP BY entry");
    if (result)
        uiCreatureCount = result->GetRowCount();

    sLog->outString("TSCR: Loading Script Waypoints for " UI64FMTD " creature(s)...", uiCreatureCount);

    result = WorldDatabase.Query("SELECT entry, pointid, location_x, location_y, location_z, waittime FROM script_waypoint ORDER BY pointid");

    if (!result)
    {
        sLog->outString(">> Loaded 0 Script Waypoints. DB table `script_waypoint` is empty.");
        sLog->outString();
        return;
    }

    uint32 count = 0;

    do
    {
        Field* pFields = result->Fetch();
        ScriptPointMove temp;

        temp.uiCreatureEntry   = pFields[0].GetUInt32();
        uint32 uiEntry          = temp.uiCreatureEntry;
        temp.uiPointId         = pFields[1].GetUInt32();
        temp.fX                = pFields[2].GetFloat();
        temp.fY                = pFields[3].GetFloat();
        temp.fZ                = pFields[4].GetFloat();
        temp.uiWaitTime        = pFields[5].GetUInt32();

        CreatureTemplate const* pCInfo = sObjectMgr->GetCreatureTemplate(temp.uiCreatureEntry);

        if (!pCInfo)
        {
            sLog->outErrorDb("TSCR: DB table script_waypoint has waypoint for non-existant creature entry %u", temp.uiCreatureEntry);
            continue;
        }

        if (!pCInfo->ScriptID)
            sLog->outErrorDb("TSCR: DB table script_waypoint has waypoint for creature entry %u, but creature does not have ScriptName defined and then useless.", temp.uiCreatureEntry);

        m_mPointMoveMap[uiEntry].push_back(temp);
        ++count;
    } while (result->NextRow());

    sLog->outString(">> Loaded %u Script Waypoint nodes in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    sLog->outString();
}
