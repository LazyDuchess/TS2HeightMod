#include "pch.h"
#include "height.h"
#include "LDHooking/hooking.h"
#include "LDCommon/config.h"
#include "LDCommon/filesys.h"

#define HOOK1_ADDR 0x6ECDF6
#define SETAGEHEIGHT_ADDR 0x6ECA70
#define STUB_ADDR 0xD888A0

namespace Height {

	int gender = 0;
	float multiplier = 1.0f;
	float personDataHeight = 1.0f;
	float personDataHeightMultiplier = 0.0011f;

	char* stubPush;
	char* hook1Return;

	float elderMaleMultiplier = 0.935f;
	float elderFemaleMultiplier = 0.935f;
	
	float adultMaleMultiplier = 1.0f;
	float adultFemaleMultiplier = 0.96f;

	float teenMaleMultiplier = 1.0f;
	float teenFemaleMultiplier = 0.96f;

	void loadConfig()
	{
		bool cfgResult = false;

		ConfigObject config = ConfigObject(FileSys::GetAbsolutePathAuto(L"Height.cfg"), cfgResult);
		if (!cfgResult)
			config = ConfigObject(FileSys::GetAbsolutePathAuto(L"mods/Height.cfg"), cfgResult);

		if (cfgResult)
		{
			personDataHeightMultiplier = config.GetFloat(L"Multiplier", personDataHeightMultiplier);

			elderMaleMultiplier = config.GetFloat(L"ElderMale", elderMaleMultiplier);
			elderFemaleMultiplier = config.GetFloat(L"ElderFemale", elderFemaleMultiplier);

			adultMaleMultiplier = config.GetFloat(L"AdultMale", adultMaleMultiplier);
			adultFemaleMultiplier = config.GetFloat(L"AdultFemale", adultFemaleMultiplier);

			teenMaleMultiplier = config.GetFloat(L"TeenMale", teenMaleMultiplier);
			teenFemaleMultiplier = config.GetFloat(L"TeenFemale", teenFemaleMultiplier);
		}
	}

	__declspec(naked) void hook1()
	{
		__asm {
			movss xmm0, [multiplier]
			//mov eax,[esp+2C]
			mulss xmm0, [esp + 0x2C]
			addss xmm0, [personDataHeight]
			movss[multiplier], xmm0
			mov eax, [multiplier]

			//maybe not?
			mov[esp + 0x2C], eax

			mov ecx, [esp + 64]
			jmp hook1Return;
		}
	}

	char* setAgeHeightReturn;

	__declspec(naked) void setAgeHeight()
	{
		__asm {
			//Store values that are used later
			push esi
			push edx
			push ecx
			//---------------------------------

			//Get Edith Object for cPerson in ECX

			//Set default 1.0 height multiplier
			mov[multiplier], 0x3f800000
			mov[personDataHeight], 0x00000000
			mov eax, fs: [0x00000000]
			mov fs : [0x00000000] , esp
			/*
			push -01
			push Sims2EP9RPC.exe+D886E8
			push eax
			sub esp,24*/
			//push ebx
			xor ebx, ebx
			//push esi
			mov esi, ecx
			mov[esp + 0x10], ebx
			mov eax, [esi + 0x04]
			mov ecx, [eax + 0x04]
			mov edx, [ecx + esi + 0x04]
			lea eax, [esp + 0x10]
			lea ecx, [ecx + esi + 0x04]
			push eax
			push 0x5A69D3D5
			mov[esp + 0x3c], ebx
			call dword ptr[edx + 0x10]

			//--------------------------

			//I guess GetEdithObject returns the edith object in ESP+0x10. Or maybe it's specifically for person data.
			mov ecx, [esp + 0x10]
			mov edx, [ecx]
			//This is awful
			/*
			lea ecx,[esp+0B]
			push ecx
			push Sims2EP9RPC.exe+E35950
			lea ecx,[esp+24]
			call Sims2EP9RPC.exe+1298
			mov [esp+18],Sims2EP9RPC.exe+EF46F0
			mov [esp+28],ebx
			mov ecx,[esp+10]
			mov edx,[ecx]
			//lea ecx,[esp+1C]

			//Get the species now. Fuck. Person data 0xBA
			*/
			push 0x01
			push 0x000000BA
			mov byte ptr[esp + 0x3C], 0x01
			call dword ptr[edx + 0x28]
			movsx eax, ax
			cmp eax, 0x0
			jnz returnEarly

			//Get Person Data Height. 0xD

			push 0x01
			push 0x0000000D
			mov byte ptr[esp + 0x3C], 0x01
			call dword ptr[edx + 0x28]
			movsx eax, ax
			mov[personDataHeight], eax
			fild[personDataHeight]
			fmul[personDataHeightMultiplier]
			fstp[personDataHeight]

			//Get gender. 0 male, 1 female. Person data 0x41

			push 0x01
			push 0x00000041
			mov byte ptr[esp + 0x3C], 0x01
			call dword ptr[edx + 0x28]
			movsx eax, ax
			mov[gender], eax

			//Get age. 0x33 old person! 0x13 adult. 0x10 teen. 0x3 child. 0x2 tod. Person data 0x3A

			push 0x01
			push 0x0000003A
			mov byte ptr[esp + 0x3C], 0x01
			call dword ptr[edx + 0x28]
			movsx eax, ax
			cmp eax, 0x33
			je elderHeight
			cmp eax, 0x13
			je heightF
			cmp eax, 0x10
			je teenHeightF
			jmp returnEarly

			elderHeight :
			cmp[gender], 0x1
				je elderHeightF
				//0.935 height multiplier
				mov eax, [elderMaleMultiplier]
				mov[multiplier], eax
				jmp returnEarly

				elderHeightF :
			//0.935 height multiplier
			mov eax, [elderFemaleMultiplier]
			mov[multiplier], eax
				jmp returnEarly

				heightF :
			cmp[gender], 0x0
				je heightM
				mov eax, [adultFemaleMultiplier]
				//0.96 height multiplier
				mov[multiplier], eax
				jmp returnEarly

				heightM:
			mov eax, [adultMaleMultiplier]
			mov[multiplier], eax
				jmp returnEarly

				teenHeightF :
			cmp[gender], 0x0
				je teenHeightM
				mov eax, [teenFemaleMultiplier]
				//0.935 height multiplier
				mov[multiplier], eax
				jmp returnEarly

				teenHeightM:
			mov eax, [teenMaleMultiplier]
			mov[multiplier], eax
			jmp returnEarly

				returnEarly :
			mov esp, fs : [0x00000000]

				//Retrieve previous values
				pop ecx
				pop edx
				pop esi
				//---------------------------------

				//Back to the original code
			originalcode:
			push -0x01
				push stubPush
				exit :
			jmp setAgeHeightReturn
		}
	}

	void Run() {
		loadConfig();
		HMODULE module = GetModuleHandleA(NULL);
		char* modBase = (char*)module;
		stubPush = modBase + STUB_ADDR;
		hook1Return = modBase + HOOK1_ADDR + 8;
		Hooking::MakeJMP((BYTE*)(modBase + HOOK1_ADDR), (DWORD)hook1, 8);
		setAgeHeightReturn = modBase + SETAGEHEIGHT_ADDR + 7;
		Hooking::MakeJMP((BYTE*)(modBase + SETAGEHEIGHT_ADDR), (DWORD)setAgeHeight, 7);
	}
}