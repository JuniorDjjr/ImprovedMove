#include "plugin.h"
#include "CTimer.h"
#include "../injector/assembly.hpp"
#include "IniReader/IniReader.h"
#include "eAnimations.h"

using namespace plugin;
using namespace std;
using namespace injector;

//fstream lg;

bool playerRun[2];

float animBlendMult[400];
float animBlendDefaultMult = 1.0f;

class ImprovedWalk {
public:

    ImprovedWalk() {

		static float g_BlendRatioProgress = 0.07f;
		static float g_AnimBlendSpeed = 4.0f;
		static float g_WalkSpeed = 1.0f;
		static bool g_WalkByDefault = false;
		static bool g_FixForcedWalkSprint = false;
		static float g_LimitForRunStop = -1.0f;
		static int g_LastStopTime = 0;
		static int g_LastCanRunTime = 0;

		float f;
		int i;

		CIniReader ini("ImprovedMove.ini");

		i = ini.ReadInteger("MoveAnims", "WalkByDefault", -1);
		(i == 1) ? g_WalkByDefault = true : false;

		f = ini.ReadFloat("MoveAnims", "WalkSpeed", -1);
		if (f != -1.0f) g_WalkSpeed = f;

		//i = ini.ReadInteger("MoveAnims", "FixForcedWalkSprint", -1);
		//(i == 1) ? g_FixForcedWalkSprint = true : false;

		//f = ini.ReadFloat("MoveAnims", "BlendProgress", -1);
		//if (f != -1.0f) g_BlendRatioProgress = (0.07f * f);

		//f = ini.ReadFloat("MoveAnims", "AnimBlendSpeed", -1);
		//if (f != -1.0f) g_AnimBlendSpeed = (4.0f * f);

		f = ini.ReadFloat("MoveAnims", "LimitForRunStop", -1);
		if (f != -1.0f) g_LimitForRunStop = f;

		animBlendDefaultMult = ini.ReadFloat("Blends", "Default", 1.0f);

		for (int i = 0; i < 400; ++i) {
			f = ini.ReadFloat("Blends", to_string(i), animBlendDefaultMult);
			animBlendMult[i] = f;
		}

		/////////////////////////////////////////

		injector::MakeInline<0x4D466B, 0x4D466B + 6>([](injector::reg_pack& regs)
		{
			regs.ecx = *(uintptr_t*)0xB4EA34; //mov ecx, _ZN12CAnimManager19ms_aAnimAssocGroupsE ; CAnimManager::ms_aAnimAssocGroups
			int animID = *(int*)(regs.esp + 0x24 + 0xC);
			if (animID >= 0 && animID < 400) {
				*(float*)(regs.esp + 0x24 + 0x10) *= animBlendMult[animID];
			}
		});

		//lg.open("ImprovedWalk.log", fstream::out | fstream::trunc);
		//lg.flush();

		//MakeNOP(0x60AFC7, 0x10);

		//WriteMemory<int>(0x688644 + 6, 6, true);

		//WriteMemory<float*>(0x60B070 + 2, &f, true);

		if (g_WalkByDefault)
		{
			injector::MakeInline<0x688466>([](injector::reg_pack& regs)
			{
				uintptr_t returnAddress = 0x688487;

				CPad *pad = (CPad *)regs.ebx;
				CPlayerPed *playerPed = (CPlayerPed *)regs.esi;
				int playerId = (playerPed->m_nPedType == 0) ? 0 : 1;

				if (*(float*)(regs.esp + 0x3C) <= 0.0f)
				{
					playerRun[playerId] = false;
					g_LastCanRunTime = 0;
				}
				else if (playerPed->m_pPlayerData->m_fMoveSpeed > 0.0f || pad->GetSprint() || playerPed->m_nMoveState > 4)
				{
					if (!playerRun[playerId]) g_LastCanRunTime = CTimer::m_snTimeInMilliseconds;
					playerRun[playerId] = true;
				}

				//lg << "blend " << playerPed->m_pPlayerData->m_fMoveBlendRatio << "\n";
				//lg << "state " << playerPed->m_nMoveState << "\n";
				//lg.flush();

				if (!playerRun[playerId] || pad->NewState.m_bPedWalk)
				{
					if (*(float*)(regs.esp + 0x3C) > g_WalkSpeed)
					{
						*(float*)(regs.esp + 0x3C) = g_WalkSpeed;
						returnAddress = 0x68849C;
						//returnAddress = 0x688470;
					}
				}

				*(uintptr_t*)(regs.esp - 0x4) = returnAddress;

				//lg << "blend " << playerData->m_fMoveBlendRatio << "\n";
				//lg << "state " << playerPed->m_nMoveState << "\n";
				//lg.flush();

				//float blend = playerData->m_fMoveBlendRatio;
			});

			injector::MakeInline<0x688644, 0x688644 + 10>([](injector::reg_pack& regs)
			{
				//mov     dword ptr [esi+534h], 7
				CPed* ped = (CPed*)regs.esi;

				CAnimBlendAssociation* run = (CAnimBlendAssociation*)RpAnimBlendClumpGetAssociation(ped->m_pRwClump, ANIM_DEFAULT_WALK_CIVI);
				CAnimBlendAssociation* sprint = (CAnimBlendAssociation*)RpAnimBlendClumpGetAssociation(ped->m_pRwClump, ANIM_DEFAULT_SPRINT_PANIC);

				CAnimBlendAssociation* stop = (CAnimBlendAssociation*)RpAnimBlendClumpGetAssociation(ped->m_pRwClump, ANIM_DEFAULT_RUN_STOP);
				CAnimBlendAssociation* stopr = (CAnimBlendAssociation*)RpAnimBlendClumpGetAssociation(ped->m_pRwClump, ANIM_DEFAULT_RUN_STOPR);

				if (stop || stopr) {
					g_LastStopTime = CTimer::m_snTimeInMilliseconds;
				}

				int state = 6;
				if (sprint || (CTimer::m_snTimeInMilliseconds - g_LastStopTime) < 2000 || (CTimer::m_snTimeInMilliseconds - g_LastCanRunTime) > 700) {
					state = 7;
				}
				ped->m_nMoveState = state;
			});
		}
		else
		{
			if (g_WalkSpeed != 1.0f) WriteMemory<float>(0x68847D + 4, g_WalkSpeed, true);
		}

		//static float G_f = 1.0f;
		//WriteMemory<float*>(0x60B298 + 2, &G_f, true);
		//MakeNOP(0x60B28F, 18, true);

		/*if (g_FixForcedWalkSprint)
		{
			injector::MakeInline<0x60B28F, 0x60B28F + 18>([](injector::reg_pack& regs)
			{
				CPlayerPed *playerPed = (CPlayerPed *)regs.esi;

				playerPed->m_pPlayerData->m_fMoveBlendRatio += (g_BlendRatioProgress * (CTimer::ms_fTimeStep / 1.66666f) * 2.0f);

				//float newSprintBlend = *(float*)(regs.ebp + 0x1C) + 1.0;
				//if (playerPed->m_pPlayerData->m_fMoveBlendRatio > newSprintBlend) {
				//	playerPed->m_pPlayerData->m_fMoveBlendRatio = newSprintBlend;
				//}
			});
		}*/

		if (g_LimitForRunStop != -1.0f) {
			injector::MakeInline<0x60B0EE, 0x60B0EE + 6>([](injector::reg_pack& regs)
			{
				regs.eax = *(int*)(regs.esi + 0x480); //mov     eax, [esi+480h]
				CAnimBlendAssociation* sprint = (CAnimBlendAssociation*)regs.edx;
				if (sprint->m_fBlendAmount < g_LimitForRunStop) {
					*(uintptr_t*)(regs.esp - 0x4) = 0x60B1A3;
				}
			});
		}

		/*if (g_AnimBlendSpeed != 4.0f)
		{
			// change all 4.0
			WriteMemory<float>(0x60B278 + 1, g_AnimBlendSpeed, true);
			WriteMemory<float>(0x60AE22 + 1, g_AnimBlendSpeed, true);
			WriteMemory<float>(0x60AD3F + 1, g_AnimBlendSpeed, true);
			WriteMemory<float>(0x60AF18 + 3, -g_AnimBlendSpeed, true);
			// sprint 2.0
			WriteMemory<float>(0x60B2AF + 1, (g_AnimBlendSpeed / 2), true);
			WriteMemory<float>(0x60B2DA + 3, (g_AnimBlendSpeed / 2), true);
			WriteMemory<float>(0x60B2E1 + 3, -(g_AnimBlendSpeed / 2), true);
		}*/

		/*injector::MakeInline<0x5DFD69, 0x5DFD69 + 6>([](injector::reg_pack& regs)
		{
			//mov     [ecx+CPed.m_fHeadingChangeRate], edx
			*(float*)(regs.ecx + 0x560) = 100.01f;
		});*/
		


		/*injector::MakeInline<0x688520, 0x688520 + 6>([](injector::reg_pack& regs)
		{
			CPlayerData *playerData = (CPlayerData *)regs.ecx;
			CPlayerPed *playerPed = (CPlayerPed *)regs.esi;

			//lg << "blend " << playerData->m_fMoveBlendRatio << "\n";
			//lg << "state " << playerPed->m_nMoveState << "\n";
			//lg.flush();

			//float blend = playerData->m_fMoveBlendRatio;
			//blend += 1.0f;
			//if (blend < 1.0f) blend = 1.0f;
			//asm_fmul(0.02 / blend);
			asm_fmul(0.03);
		});*/

		/*if (g_BlendRatioProgress != 0.07f)
		{
			WriteMemory<float*>(0x688520 + 2, &g_BlendRatioProgress, true);
		}*/


		//WriteMemory<float>(0x60B1B7 + 3, -2.0f);
		//WriteMemory<float>(0x60B1BE + 3, 0.2f);

		

		/*injector::MakeInline<0x60B399, 0x60B399 + 6>([](injector::reg_pack& regs)
		{
			//mov     dword ptr [eax+14h], 3F800000h
			//*(float*)(regs.eax + 0x14) = 1.0;
			//regs.edx = regs.eax;
			//regs.edx = *(DWORD*)(regs.esi + 0x4D4);
			//mov     eax, [esi + CPed.m_pPlayerData]
			regs.eax = *(DWORD*)(regs.esi + 0x480);

			CPlayerPed *playerPed = (CPlayerPed *)regs.esi;

			lg << "animset " << playerPed->m_nMoveState << "\n";
			lg.flush();

		});*/
		

		//WriteMemory<float>(0x60B1B7 + 3, 1.0, true);
		//WriteMemory<float>(0x60B1BE + 3, -1.0, true);
		//WriteMemory<float>(0x60B3D9 + 1, 1.0f);

    }
} improvedWalk;
