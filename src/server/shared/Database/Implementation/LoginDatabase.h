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

#ifndef _LOGINDATABASE_H
#define _LOGINDATABASE_H

#include "DatabaseWorkerPool.h"
#include "MySQLConnection.h"

class LoginDatabaseConnection : public MySQLConnection
{
    public:
        //- Constructors for sync and async connections
        LoginDatabaseConnection(MySQLConnectionInfo& connInfo) : MySQLConnection(connInfo) {}
        LoginDatabaseConnection(ACE_Activation_Queue* q, MySQLConnectionInfo& connInfo) : MySQLConnection(q, connInfo) {}

        //- Loads database type specific prepared statements
        void DoPrepareStatements();
};

typedef DatabaseWorkerPool<LoginDatabaseConnection> LoginDatabaseWorkerPool;

enum LoginDatabaseStatements
{
    /*  Naming standard for defines:
        {DB}_{SET/DEL/ADD/REP}_{Summary of data changed}
        When updating more than one field, consider looking at the calling function
        name for a suiting suffix.
    */

    LOGIN_GET_REALMLIST,
    LOGIN_SET_EXPIREDIPBANS,
    LOGIN_SET_EXPIREDACCBANS,
    LOGIN_GET_IPBANNED,
    LOGIN_SET_IPAUTOBANNED,
    LOGIN_GET_ACCBANNED,
    LOGIN_SET_ACCAUTOBANNED,
    LOGIN_GET_SESSIONKEY,
    LOGIN_SET_VS,
    LOGIN_SET_LOGONPROOF,
    LOGIN_GET_LOGONCHALLENGE,
    LOGIN_SET_FAILEDLOGINS,
    LOGIN_GET_FAILEDLOGINS,
    LOGIN_GET_ACCIDBYNAME,
    LOGIN_GET_NUMCHARSONREALM,
    LOGIN_GET_ACCOUNT_BY_IP,
    LOGIN_SET_IP_BANNED,
    LOGIN_SET_IP_NOT_BANNED,
    LOGIN_SET_ACCOUNT_BANNED,
    LOGIN_SET_ACCOUNT_NOT_BANNED,
    LOGIN_DEL_REALMCHARACTERS,
    LOGIN_ADD_REALMCHARACTERS,
    LOGIN_GET_SUM_REALMCHARS,
    LOGIN_ADD_ACCOUNT,
    LOGIN_ADD_REALM_CHARS,
    LOGIN_DEL_OLD_BANS,
    LOGIN_DEL_OLD_IP_BANS,
    LOGIN_UPDATE_EXPANSION,
    LOGIN_UPDATE_ACCOUNT_LOCK,
    LOGIN_ADD_LOG,
    LOGIN_UPDATE_USERNAME,
    LOGIN_UPDATE_PASSWORD,
    LOGIN_UPDATE_MUTE_TIME,
    LOGIN_UPDATE_LAST_IP,
    LOGIN_UPDATE_ACCOUNT_ONLINE,
    LOGIN_UPDATE_UPTIME_PLAYERS,
    LOGIN_DEL_OLD_LOGS,
    LOGIN_DEL_ACCOUNT_ACCESS,
    LOGIN_DEL_ACCOUNT_ACCESS_BY_REALM,
    LOGIN_INS_ACCOUNT_ACCESS,

    MAX_LOGINDATABASE_STATEMENTS,
};

#endif
