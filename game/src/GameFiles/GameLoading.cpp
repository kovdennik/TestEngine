#include "Game.hpp"

#include <BlackBox/3DEngine/I3DEngine.hpp>
#include <ScriptObjects/ScriptObjectStream.hpp>

#if !defined(LINUX)
#	include <dbghelp.h>
//#	pragma comment(lib, "dbghelp.lib")
#else
#	include <stdio.h>
#endif

//////////////////////////////////////////////////////////////////////////
class CCVarSaveDump : public ICVarDumpSink
{
private:
	FILE *m_pFile;
public:
	CCVarSaveDump(FILE *pFile)
	{
		m_pFile=pFile;
	}
	void OnElementFound(ICVar *pCVar) override
	{
		if (pCVar && (pCVar->GetFlags() & VF_DUMPTODISK))
		{
			string szValue = pCVar->GetString();
			int pos;

			// replace \ with \\ 
			pos = 1;
			for(;;)
			{
				pos = szValue.find_first_of("\\", pos);

				if (pos == string::npos)
				{
					break;
				}

				szValue.replace(pos, 1, "\\\\", 2);
				pos+=2;
			}

			// replace " with \" 
			pos = 1;
			for(;;)
			{
				pos = szValue.find_first_of("\"", pos);

				if (pos == string::npos)
				{
					break;
				}

				szValue.replace(pos, 1, "\\\"", 2);
				pos+=2;
			}

			string szLine = pCVar->GetName();
			szLine += " = \"";
			szLine += szValue;
			szLine += "\"\r\n";

			fputs(szLine.c_str(), m_pFile);
		}
	}
};

//////////////////////////////////////////////////////////////////////////
class CActionMapDumpSink : public IActionMapDumpSink
{
private:
	CGame *m_pGame;
	FILE *m_pFile;
public:
	CActionMapDumpSink(CGame *pGame, FILE *pFile)
	{
		m_pGame=pGame;
		m_pFile=pFile;
		fputs("Input:ResetAllBindings();\r\n", m_pFile);
	}
	virtual void OnElementFound(const char *pszActionMapName, IActionMap *pActionMap)
	{
		char pszKey[256];
		char pszMod[256];
		ActionsEnumMap &ActionsMap=m_pGame->GetActionsEnumMap();
		ActionsEnumMapItor It=ActionsMap.begin();
		while (It!=ActionsMap.end())
		{
			ActionInfo &Info=It->second;
			for (int i=0;i<MAX_BINDS_PER_ACTION;i++)
			{
				pActionMap->GetBinding(Info.nId, i, pszKey, pszMod);

				if (strlen(pszKey))
				{
					if (strcmp(pszKey, "\\") == 0)
					{
						strcpy(pszKey, "\\\\");
					}
					else if (strcmp(pszKey, "\"") == 0)
					{
						strcpy(pszKey, "\\\"");
					}

					char szLine[1024] = {0};

					sprintf(szLine, "Input:BindAction(\"%s\", \"%s\", \"%s\", %d);\r\n", It->first.c_str(), pszKey, pszActionMapName, i);
					fputs(szLine, m_pFile);
				}
			}
			++It;
		}
	}
};

//////////////////////////////////////////////////////////////////////////
void CGame::SaveConfiguration( const char *pszSystemCfg,const char *pszGameCfg,const char *sProfileName)
{
	string sSystemCfg = pszSystemCfg;
	string sGameCfg = pszGameCfg;
	if (sProfileName)
	{	
		sSystemCfg=string("Profiles/Player/")+sProfileName+"_"+sSystemCfg;
		sGameCfg=string("Profiles/Player/")+sProfileName+"_"+sGameCfg;		
	}

	FILE *pFile=fopen(sSystemCfg.c_str(), "wb");
	if (pFile)
	{
		fputs("-- [System-Configuration]\r\n", pFile);
		fputs("-- Attention: This file is generated by the system, do not modify! Editing is not recommended! \r\n\r\n", pFile);
		CCVarSaveDump SaveDump(pFile);
		m_pSystem->GetIConsole()->DumpCVars(&SaveDump);		
		fclose(pFile); 
	}

	if (m_pIActionMapManager)
	{
		pFile=fopen(sGameCfg.c_str(), "wb");
		if (pFile)
		{
			fputs("-- [Game-Configuration]\r\n", pFile);
			fputs("-- Attention: This file will be overwritten when updated, so dont add lines ! Editing is not recommended !\r\n\r\n", pFile);
			CActionMapDumpSink SaveActionMaps(this, pFile);
			m_pIActionMapManager->GetActionMaps(&SaveActionMaps);
			// Mouse
			char sValue[32];
			sprintf(sValue, "%4.4f", m_pSystem->GetIConsole()->GetCVar("i_mouse_sensitivity")->GetFVal());
			fputs(string(string("Input:SetMouseSensitivity(")+string(sValue)+string(");\r\n")).c_str(), pFile);

			IInput *pInput=m_pSystem->GetIInput();

			// Special keys
			m_pIActionMapManager->GetInvertedMouse() ? strcpy(sValue, "1") : strcpy(sValue, "nil");
			fputs(string(string("Input:SetInvertedMouse(")+string(sValue)+string(");\r\n")).c_str(), pFile);
			fputs("Input:BindCommandToKey(\"\\\\SkipCutScene\",\"F7\",1);\r\n",pFile);
			fputs("Input:BindCommandToKey(\"\\\\SkipCutScene\",\"spacebar\",1);\r\n",pFile);
			fclose(pFile); 
		}
	}

}

//////////////////////////////////////////////////////////////////////////
void CGame::LoadConfiguration(const string &sSystemCfg,const string &sGameCfg)
{			
	m_pSystem->LoadConfiguration(sSystemCfg.data());

	FILE *pFile=fopen(sGameCfg.c_str(), "rb");
	if (!pFile)
	{
		m_pLog->Log("Error Loading game configuration '%s'",sGameCfg.c_str());
		// if for some reason the game config is not found 
		// (first time, new installation etc.) create a new one with basic stuff in it
		return;
	}
	
	char szLine[512];
	char szBuffer[512];
	while (fgets(szLine,512,pFile))
	{			
		// skip comments
		if (szLine[0]=='-')
			continue; 

		// extract command
		if (!strstr(szLine,";"))
			continue;

		// check for malicious commands
		bool bValid=false;
				
		if (strstr(szLine,"#"))
		{
			// someone is trying to bind script code
			// to a key - silently skip this line
			continue;
		}
		else
		if (strstr(szLine,"Input:ResetAllBindings"))
		{
			// valid command
			bValid=true;
		}
		else
		if (strstr(szLine,"Input:BindAction"))
		{
			// valid command
			bValid=true;
		}
		else
		if (strstr(szLine,"Input:BindCommandToKey"))
		{
			//if (strstr(szLine,"SkipCutScene"))											
			// valid command
			bValid=true;
		}					

		if (bValid)
		{					
			strcpy(szBuffer,szLine);
			//m_pLog->Log("  '%s'",szBuffer);
			m_pSystem->GetIScriptSystem()->ExecuteBuffer(szBuffer,strlen(szBuffer));
		}
		else
		{
			gEnv->pLog->LogError("Invalid game cfg:%s",szLine);
		}
	}

	fclose(pFile);
} 

//////////////////////////////////////////////////////////////////////////
class CCVarSerializeGameSave : public ICVarDumpSink
{
public:
	CCVarSerializeGameSave(CStream *pStm, bool bSave)
	{
		m_pStm=pStm;
		m_bSave=bSave;
		m_nCount=0;
	}

	void OnElementFound(ICVar *pCVar)
	{
		if (m_bSave)
		{		
			m_pStm->Write(pCVar->GetName());
			m_pStm->Write(pCVar->GetString());
		}
		
		m_nCount++;
	}

	int	GetCount() { return(m_nCount); }
	
private:
	CStream *m_pStm;
	bool m_bSave;
	int	m_nCount;
};

//////////////////////////////////////////////////////////////////////////
void SaveName(string &s, string &prof)
{
	if(!s[0]) s = "quicksave";
	for(unsigned int i = 0; i<s.size(); i++) if(!isalnum(s[i])) s[i] = '_';
	s = "profiles/player/" + prof + "/savegames/" + s + ".sav";
};

bool CGame::SaveToStream(CStream& stm, Vec3* pos, Vec3* angles, string sFilename)
{

	if(m_bEditor)				 
	{
		m_pLog->Log("Skipping savegame in editor...");
		return false;
	};

	CScriptObjectStream scriptStream;
	scriptStream.Create(m_pScriptSystem);
	scriptStream.Attach(&stm);

	stm.Reset();

	// save header
	stm.Write(SAVEMAGIC);
	stm.Write((int)PATCH2_SAVEVERSION);

	// get lowercase versions of levelname and missionname
	char szLowerCaseStr[256] = {0};

	strncpy(szLowerCaseStr, g_LevelName->GetString(), 255);
	strlwr(szLowerCaseStr);
	
	// save levelname
	stm.Write(szLowerCaseStr);

	//strncpy(szLowerCaseStr, m_pServer->m_GameContext.strMission.c_str(), 255);
	strncpy(szLowerCaseStr, g_MissionName->GetString(), 255);
	strlwr(szLowerCaseStr);

	// save mission name
	stm.Write(szLowerCaseStr);

	// write current time and date
	SYSTEMTIME pSystemTime;		
	GetLocalTime(&pSystemTime);	// FIXME: this win32 call should be moved to crysystem

	stm.Write((unsigned char)pSystemTime.wHour);	// hour
	stm.Write((unsigned char)pSystemTime.wMinute);// minute
	stm.Write((unsigned char)pSystemTime.wSecond);// second
	stm.Write((unsigned char)pSystemTime.wDay);		// day
	stm.Write((unsigned char)pSystemTime.wMonth);	// month
	stm.Write((unsigned short)pSystemTime.wYear);	// year

	// save savegame name
	stm.Write(sFilename);

	WRITE_COOKIE_NO(stm,0x22);

	CCVarSerializeGameSave tCount(&stm,false);
	m_pSystem->GetIConsole()->DumpCVars(&tCount,VF_SAVEGAME);
	int nCount=tCount.GetCount(); // get the number of cvars to save
	stm.Write(nCount);  
	CCVarSerializeGameSave t(&stm,true); 
	m_pSystem->GetIConsole()->DumpCVars(&t,VF_SAVEGAME); // save them
	ASSERT(t.GetCount()==nCount);



	return true;
}

bool CGame::LoadFromStream(CStream& stm, bool isdemo)
{
	if(IsMultiplayer())
	{
		assert(0);
		m_pLog->LogError("ERROR: LoadFromStream IsMultiplayer=true");				// severe problem - stream is different for MP
    return false;
	}

	m_bIsLoadingLevelFromFile = true;
	//m_pSystem->GetIPhysicalWorld()->GetPhysVars()->bMultiplayer = IsMultiplayer() ? 1:0;
		
	CScriptObjectStream scriptStream;
	scriptStream.Create(m_pScriptSystem);
	scriptStream.Attach(&stm);

	string sMagic;
	stm.Read(sMagic);
	if(strcmp(sMagic.c_str(), SAVEMAGIC))
	{
		m_pLog->LogToConsole("ERROR: this is not a valid savegame file");
		m_bIsLoadingLevelFromFile = false;
		return false;
	};

	int nVersion;
	stm.Read(nVersion);

  stm.SetStreamVersion(nVersion);

	switch (nVersion)
	{
		case SAVEVERSION:
			return LoadFromStream_RELEASEVERSION(stm,isdemo,scriptStream);
		case PATCH1_SAVEVERSION:
			return LoadFromStream_PATCH_1(stm,isdemo,scriptStream);
		// add more here as more patches are released
	}

	if(nVersion!=PATCH1_SAVEVERSION && nVersion!=PATCH2_SAVEVERSION)
	{
		m_pLog->LogToConsole("ERROR: savegame file from different version of the game");
		m_bIsLoadingLevelFromFile = false;
		return false;
	};

	string sLevelName;
	string sMissionName;
	stm.Read(sLevelName);
	stm.Read(sMissionName);

	// read dummy save date and time
	unsigned char bDummy;
	unsigned short wDummy;
	stm.Read(bDummy);	// hour
	stm.Read(bDummy);	// minute
	stm.Read(bDummy);	// second
	stm.Read(bDummy);	// day
	stm.Read(bDummy);	// month
	stm.Read(wDummy);	// year

	// load savegame name
	string sFilename;
	stm.Read(sFilename);

	// [marco] load saved cvars
	string varname,val;
	int nCount,i;
	stm.Read(nCount);
	IConsole *pCon=m_pSystem->GetIConsole();	
	for (i=0;i<nCount;i++)
	{
		if(stm.Read(varname))
		if(stm.Read(val))
		{
			ICVar *pCVar=m_pSystem->GetIConsole()->GetCVar(varname.c_str());
			if (!pCVar)
			{
				m_pSystem->GetILog()->Log("\001 WARNING, CVar %s(%s) was saved but is not present",varname.c_str(),val.c_str());
			}
			else
				pCVar->Set(val.c_str());
		}
		else
		{
			m_pSystem->GetILog()->LogError("CXGame::LoadFromStream %d/%d critical error",i,nCount);
			stm.Debug();
			m_bIsLoadingLevelFromFile = false;
			return false;
		}
	} //i

	bool			bLoadBar = false;
	IConsole *pConsole = m_pSystem->GetIConsole();

	assert(pConsole);


	m_bIsLoadingLevelFromFile = false;

	m_bMapLoadedFromCheckpoint=true;
	
	return true;
}

bool CGame::LoadFromStream_RELEASEVERSION(CStream& str, bool isdemo, CScriptObjectStream& scriptStream)
{
	return false;
}

bool CGame::LoadFromStream_PATCH_1(CStream& str, bool isdemo, CScriptObjectStream& scriptStream)
{
	return false;
}

void CGame::Save(string sFileName, Vec3 * pos, Vec3 * angles, bool bFirstCheckpoint)
{
	#if 0
	if(!m_pClient)
	{
		m_pLog->Log("Cannot save game with no map loaded");
		return;
	};

	if(!m_pServer)
	{
		m_pLog->Log("Cannot save multiplayer game");
		return;
	};
	#endif

	CDefaultStreamAllocator sa;
	CStream stm(3000, &sa);   

	if (SaveToStream(stm, pos, angles,sFileName))
	{
		m_strLastSaveGame = sFileName;
		assert(g_playerprofile);

		if (g_playerprofile->GetString() && strlen(g_playerprofile->GetString()))
		{
			string tmp( g_playerprofile->GetString() );
			SaveName(sFileName, tmp);
		}
		else
		{
			string tmp( "default" );
			SaveName(sFileName, tmp);
		}
		
		m_pLog->LogToConsole("Level saved in %d bytes(%s)", BITS2BYTES(stm.GetSize()), sFileName.c_str());

		// replace / by \ because MakeSureDirectoryPathExists does not work with unix paths
		size_t pos = 1;
		
		for(;;)
		{
			pos = sFileName.find_first_of("/", pos);

			if (pos == string::npos)
			{
				break;
			}

			sFileName.replace(pos, 1, "\\", 1);
			pos+=1;
		}

		if (MakeSureDirectoryPathExists(sFileName.c_str()))
		{
			if(!m_pSystem->WriteCompressedFile((char *)sFileName.c_str(), stm.GetPtr(), stm.GetSize()))
			{
				m_pLog->Log("cannot write savegame to file %s", sFileName.c_str());

				return;
			};
			m_sLastSavedCheckpointFilename = sFileName;

			/*
			// Make screenshot of current location, and save it to a .dds file.			
			if (!bFirstCheckpoint)
				m_fTimeToSaveThumbnail = 1.0f; // Save checkpoint thumbnail 1 second from now.
			else
			{
				m_fTimeToSaveThumbnail = 5.0f; // Save checkpoint thumbnail 5 seconds from now.
			}			
			*/
		}
	};
}

bool CGame::Load(string sFileName)
{
	assert(g_playerprofile);
	
	string tmp( g_playerprofile->GetString() );
	SaveName(sFileName, tmp);

	CDefaultStreamAllocator sa;
	CStream stm(300, &sa); 

	int bitslen=m_pSystem->GetCompressedFileSize((char *)sFileName.c_str());
	if(bitslen==0) 
	{
		return false;
	}

	stm.Resize(bitslen);
	int bitsread = m_pSystem->ReadCompressedFile((char *)sFileName.c_str(), stm.GetPtr(), stm.GetAllocatedSize());
	if(!bitsread)
	{
		m_pLog->Log("No such savegame: %s", sFileName.c_str());
		return false;
	};

	stm.SetSize(bitsread);
	//////////////////////////////////////////////////////////////////////////

	bool ok = LoadFromStream(stm, false);

	//////////////////////////////////////////////////////////////////////////

	if(ok)
		m_strLastSaveGame = sFileName;

	if (!IsMultiplayer())
	{
		SmartScriptObject pMissionScript(m_pScriptSystem);
		m_pScriptSystem->GetGlobalValue("Mission", pMissionScript);

		if (((IScriptObject *)pMissionScript) != 0)
		{
			HSCRIPTFUNCTION pfnOnCheckpointLoaded = 0;

			pMissionScript->GetValue("OnCheckpointLoaded", pfnOnCheckpointLoaded);

			if (pfnOnCheckpointLoaded != -1)
			{
				m_pScriptSystem->BeginCall(pfnOnCheckpointLoaded);
				m_pScriptSystem->PushFuncParam((IScriptObject*)pMissionScript);
				m_pScriptSystem->EndCall();

				m_pScriptSystem->ReleaseFunc(pfnOnCheckpointLoaded);
			}
		}
	}

	AllowQuicksave(true);
	return ok;
}
