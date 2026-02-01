// Copyright (C) Benjamín Gajardo (also known as +KZ)

// has code from scoreworker.cpp

#include "scoreworker.h"

#include <base/log.h>
#include <base/system.h>
#include <base/helper_kz.h>

#include <engine/server/databases/connection.h>
#include <engine/server/databases/connection_pool.h>
#include <engine/server/sql_string_helpers.h>
#include <engine/shared/config.h>

#include <cmath>

// "6b407e81-8b77-3e04-a207-8da17f37d000"
// "save-no-save-id@ddnet.tw"
static const CUuid UUID_NO_SAVE_ID =
	{{0x6b, 0x40, 0x7e, 0x81, 0x8b, 0x77, 0x3e, 0x04,
		0xa2, 0x07, 0x8d, 0xa1, 0x7f, 0x37, 0xd0, 0x00}};

bool CScoreWorker::SaveKaizoTeam(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize, const char * apCode)
{
	const auto *pData = dynamic_cast<const CSqlTeamSaveData *>(pGameData);
	auto *pResult = dynamic_cast<CScoreSaveResult *>(pGameData->m_pResult.get());

	if(w == Write::NORMAL_SUCCEEDED)
	{
		// write succeeded on mysql server. delete from sqlite again
		char aBuf[128] = {0};
		str_format(aBuf, sizeof(aBuf),
			"DELETE FROM %s_kaizo_saves_backup WHERE Code = ?",
			pSqlServer->GetPrefix());
		if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		{
			return false;
		}
		pSqlServer->BindString(1, apCode);
		bool End;
		return pSqlServer->Step(&End, pError, ErrorSize);
	}
	if(w == Write::NORMAL_FAILED)
	{
		char aBuf[256] = {0};
		bool End;
		// move to non-tmp table succeeded. delete from backup again
		str_format(aBuf, sizeof(aBuf),
			"INSERT INTO %s_kaizo_saves SELECT * FROM %s_kaizo_saves_backup WHERE Code = ?",
			pSqlServer->GetPrefix(), pSqlServer->GetPrefix());
		if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		{
			return false;
		}
		pSqlServer->BindString(1, apCode);
		if(!pSqlServer->Step(&End, pError, ErrorSize))
		{
			return false;
		}

		// move to non-tmp table succeeded. delete from backup again
		str_format(aBuf, sizeof(aBuf),
			"DELETE FROM %s_kaizo_saves_backup WHERE Code = ?",
			pSqlServer->GetPrefix());
		if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		{
			return false;
		}
		pSqlServer->BindString(1, apCode);
		return pSqlServer->Step(&End, pError, ErrorSize);
	}

	char aSaveId[UUID_MAXSTRSIZE];
	FormatUuid(pResult->m_SaveId, aSaveId, UUID_MAXSTRSIZE);

	char *pSaveState = pResult->m_SavedTeam.GetKaizoString();
	char aBuf[65536];

		char aCode[128] = {0};
		str_copy(aCode, apCode, sizeof(aCode));

		str_format(aBuf, sizeof(aBuf),
			"%s INTO %s_kaizo_saves%s(Savegame, Map, Code, Timestamp, Server, SaveId, DDNet7) "
			"VALUES (?, ?, ?, CURRENT_TIMESTAMP, ?, ?, %s)",
			pSqlServer->InsertIgnore(), pSqlServer->GetPrefix(),
			w == Write::NORMAL ? "" : "_backup", pSqlServer->False());
		if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		{
			return false;
		}
		pSqlServer->BindString(1, pSaveState);
		pSqlServer->BindString(2, pData->m_aMap);
		pSqlServer->BindString(3, aCode);
		pSqlServer->BindString(4, pData->m_aServer);
		pSqlServer->BindString(5, aSaveId);
		pSqlServer->Print();
		int NumInserted;
		if(!pSqlServer->ExecuteUpdate(&NumInserted, pError, ErrorSize))
		{
			return false;
		}
		if(NumInserted == 1)
		{
			pResult->m_Status = CScoreSaveResult::SAVE_SUCCESS;
			if(w == Write::NORMAL)
			{
				pResult->m_aBroadcast[0] = '\0';
				if(str_comp(pData->m_aServer, g_Config.m_SvSqlServerName) == 0)
				{
					/*str_format(pResult->m_aMessage, sizeof(pResult->m_aMessage),
						"Team successfully saved by %s. Use '/load %s' to continue",
						pData->m_aClientName, aCode);*/
				}
				else
				{
					/*str_format(pResult->m_aMessage, sizeof(pResult->m_aMessage),
						"Team successfully saved by %s. Use '/load %s' on %s to continue",
						pData->m_aClientName, aCode, pData->m_aServer);*/
				}
			}
			else
			{
				/*str_copy(pResult->m_aBroadcast,
					"Database connection failed, teamsave written to a file instead. On official DDNet servers this will automatically be inserted into the database every full hour.",
					sizeof(pResult->m_aBroadcast));*/
				if(str_comp(pData->m_aServer, g_Config.m_SvSqlServerName) == 0)
				{
					/*str_format(pResult->m_aMessage, sizeof(pResult->m_aMessage),
						"Team successfully saved by %s. The database connection failed, using generated save code instead to avoid collisions. Use '/load %s' to continue",
						pData->m_aClientName, aCode);*/
					pResult->m_aServer[0] = '\0';
				}
				else
				{
					/*str_format(pResult->m_aMessage, sizeof(pResult->m_aMessage),
						"Team successfully saved by %s. The database connection failed, using generated save code instead to avoid collisions. Use '/load %s' on %s to continue",
						pData->m_aClientName, aCode, pData->m_aServer);*/
				}
				pResult->m_Status = CScoreSaveResult::SAVE_FALLBACKFILE;
			}
		}
	

	if(
		pResult->m_Status != CScoreSaveResult::SAVE_SUCCESS &&
		pResult->m_Status != CScoreSaveResult::SAVE_WARNING &&
		pResult->m_Status != CScoreSaveResult::SAVE_FALLBACKFILE)
	{
		dbg_msg("sql", "ERROR: This save-code already exists");
		pResult->m_Status = CScoreSaveResult::SAVE_FAILED;
		//str_copy(pResult->m_aMessage, "This save-code already exists", sizeof(pResult->m_aMessage));
	}
	return true;
}

bool CScoreWorker::LoadKaizoTeam(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize)
{
	if(w == Write::NORMAL_SUCCEEDED || w == Write::BACKUP_FIRST)
		return true;
	const auto *pData = dynamic_cast<const CSqlTeamLoadRequest *>(pGameData);
	auto *pResult = dynamic_cast<CScoreSaveResult *>(pGameData->m_pResult.get());
	pResult->m_Status = CScoreSaveResult::LOAD_FAILED;

	char aCurrentTimestamp[512];
	pSqlServer->ToUnixTimestamp("CURRENT_TIMESTAMP", aCurrentTimestamp, sizeof(aCurrentTimestamp));
	char aTimestamp[512];
	pSqlServer->ToUnixTimestamp("Timestamp", aTimestamp, sizeof(aTimestamp));

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf),
		"SELECT Savegame, %s-%s AS Ago, SaveId "
		"FROM %s_kaizo_saves "
		"where Code = ? AND Map = ? AND DDNet7 = %s",
		aCurrentTimestamp, aTimestamp,
		pSqlServer->GetPrefix(), pSqlServer->False());
	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		return false;
	}
	pSqlServer->BindString(1, pData->m_aCode);
	pSqlServer->BindString(2, pData->m_aMap);

	bool End;
	if(!pSqlServer->Step(&End, pError, ErrorSize))
	{
		return false;
	}
	if(End)
	{
		//str_copy(pResult->m_aMessage, "No such savegame for this map", sizeof(pResult->m_aMessage));
		return true;
	}

	pResult->m_SaveId = UUID_NO_SAVE_ID;
	if(!pSqlServer->IsNull(3))
	{
		char aSaveId[UUID_MAXSTRSIZE];
		pSqlServer->GetString(3, aSaveId, sizeof(aSaveId));
		if(ParseUuid(&pResult->m_SaveId, aSaveId) || pResult->m_SaveId == UUID_NO_SAVE_ID)
		{
			//str_copy(pResult->m_aMessage, "Unable to load savegame: SaveId corrupted", sizeof(pResult->m_aMessage));
			return true;
		}
	}

	char aSaveString[65536];
	pSqlServer->GetString(1, aSaveString, sizeof(aSaveString));
	int Num = pResult->m_SavedTeam.FromKaizoString(aSaveString);

	if(Num != 0)
	{
		//str_copy(pResult->m_aMessage, "Unable to load savegame: data corrupted", sizeof(pResult->m_aMessage));
		return true;
	}

	bool Found = false;
	for(int i = 0; i < pResult->m_SavedTeam.GetMembersCount(); i++)
	{
		if(str_comp(pResult->m_SavedTeam.m_pSavedTees[i].GetName(), pData->m_aRequestingPlayer) == 0)
		{
			Found = true;
			break;
		}
	}
	if(!Found)
	{
		/*str_copy(pResult->m_aMessage, "This save exists, but you are not part of it. "
					      "Make sure you use the same name as you had when saving. "
					      "If you saved with an already used code, you get a new random save code, "
					      "check ddnet-saves.txt in config_directory.",
			sizeof(pResult->m_aMessage));*/
		return true;
	}

	int Since = pSqlServer->GetInt(2);
	if(Since < g_Config.m_SvSaveSwapGamesDelay)
	{
		//str_format(pResult->m_aMessage, sizeof(pResult->m_aMessage),
		//	"You have to wait %d seconds until you can load this savegame",
		//	g_Config.m_SvSaveSwapGamesDelay - Since);
		return true;
	}

	bool CanLoad = pResult->m_SavedTeam.MatchPlayers(
		pData->m_aClientNames, pData->m_aClientId, pData->m_NumPlayer,
		pResult->m_aMessage, sizeof(pResult->m_aMessage));

	if(!CanLoad)
		return true;

	str_format(aBuf, sizeof(aBuf),
		"DELETE FROM %s_kaizo_saves "
		"WHERE Code = ? AND Map = ? AND SaveId %s",
		pSqlServer->GetPrefix(),
		pResult->m_SaveId != UUID_NO_SAVE_ID ? "= ?" : "IS NULL");
	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		return false;
	}
	pSqlServer->BindString(1, pData->m_aCode);
	pSqlServer->BindString(2, pData->m_aMap);
	char aUuid[UUID_MAXSTRSIZE];
	if(pResult->m_SaveId != UUID_NO_SAVE_ID)
	{
		FormatUuid(pResult->m_SaveId, aUuid, sizeof(aUuid));
		pSqlServer->BindString(3, aUuid);
	}
	pSqlServer->Print();
	int NumDeleted;
	if(!pSqlServer->ExecuteUpdate(&NumDeleted, pError, ErrorSize))
	{
		return false;
	}

	if(NumDeleted != 1)
	{
		//str_copy(pResult->m_aMessage, "Unable to load savegame: loaded on a different server", sizeof(pResult->m_aMessage));
		return true;
	}

	pResult->m_Status = CScoreSaveResult::LOAD_SUCCESS;
	//str_copy(pResult->m_aMessage, "Loading successfully done", sizeof(pResult->m_aMessage));
	return true;
}
