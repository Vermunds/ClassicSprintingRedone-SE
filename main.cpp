#include "../skse64/PluginAPI.h"
#include "../skse64_common/skse_version.h"
#include "../skse64_common/Relocation.h"
#include "../skse64_common/SafeWrite.h"
#include "../skse64_common/BranchTrampoline.h"
#include "gameinput.h"
#include <shlobj.h>
#include <xbyak/xbyak.h>

//SkyrimSE.exe + 0x704B3
RelocAddr <uintptr_t *> SprintKeyStatusFunction = 0x704B30;
//SkyrimSE.exe + 0x2F4DEF8
RelocAddr <uintptr_t *> SprintStatusAddress = 0x2F4DEF8;
//SkyrimSE.exe + 0x705058
RelocAddr <uintptr_t *> ControlFunction = 0x704FD0;
extern "C" {

	bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info)
	{
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Skyrim Special Edition\\SKSE\\ClassicSprinting.log");
		gLog.SetPrintLevel(IDebugLog::kLevel_Error);
		gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);

		_MESSAGE("Classic Sprinting Redone (SKSE64) for Skyrim Special Edition.");
		_MESSAGE("Initializing...");

		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = "Classic Sprinting Redone (SKSE64)";
		info->version = 1;

		if (skse->isEditor)
		{
			_ERROR("Loaded in editor, mod is disabled.");
			return false;
		}
		if (skse->runtimeVersion != RUNTIME_VERSION_1_5_50)
		{
			_ERROR("Error: Incompatible Skyrim version.");
			return false;
		}

		return true;
	}

	bool SKSEPlugin_Load(const SKSEInterface * skse) {

		if (!g_branchTrampoline.Create(1024 * 64))
		{
			_ERROR("Couldn't create branch trampoline. The mod is disabled.");
			return false;
		}
		if (!g_localTrampoline.Create(1024 * 64, nullptr))
		{
			_ERROR("Couldn't create codegen buffer. The mod is disabled.");
			return false;
		}

		_MESSAGE("Injecting code...");

		//SkyrimSE.exe + 0x704B3
		//This function keeps track of the status of some of the keys. The key is determined by the register R8. If R8 == 0x2 it's the sprinting key.
		//The sprinting status is determined by the value at Skyrim.SE.exe + 0x2F4DEF8 (byte)

		//This is a different function than the original, as it's only called when a key is pressed.
		//Parent function: SkyrimSE.exe + 0x70E000
		//Original function: SkyrimSE.exe + 0x709770
		{
			struct SprintKeyStatus_Code : Xbyak::CodeGenerator
			{
				SprintKeyStatus_Code(void * buf) : Xbyak::CodeGenerator(4096, buf)
				{
					Xbyak::Label keyDown;
					Xbyak::Label newCode;
					Xbyak::Label sprintStatus;
					Xbyak::Label rtrn;

					//Original code
					xorps(xmm0, xmm0);
					ucomiss(xmm0, ptr[rdx + 0x28]);
					jne(keyDown);
					comiss(xmm0, ptr[rdx + 0x2C]);
					ja(keyDown);

					//Key up
					mov(eax, 0x1);
					test(al, al);
					sete(al);
					jmp(newCode);

					L(keyDown);
					//Key down
					xor (eax, eax);
					test(al, al);
					sete(al);
					mov(ptr[rcx + 0x10], al);
					jmp(newCode);

					L(newCode);
					//New code
					cmp(r8, 0x2); //Check R8 to make sure it's the sprinting button
					jne(rtrn);
					push(rbx);
					mov(rbx, SprintStatusAddress.GetUIntPtr());
					mov(rbx, ptr[rbx]);
					mov(ptr[rbx + 0xBDD], al);
					pop(rbx);
					ret();

					L(rtrn);
					ret();
				}
			};

			void * codeBuf = g_localTrampoline.StartAlloc();
			SprintKeyStatus_Code code(codeBuf);
			g_localTrampoline.EndAlloc(code.getCurr());

			g_branchTrampoline.Write6Branch(SprintKeyStatusFunction.GetUIntPtr(), uintptr_t(code.getCode()));
		}

		//Sprinting may still get "stuck" in some cases (e.g opened a menu when still holding the button).
		//To fix this we inject this code that is executed before any control is processed.

		{
			struct ControlFunction_Code : Xbyak::CodeGenerator
			{
				ControlFunction_Code(void * buf) : Xbyak::CodeGenerator(4096, buf)
				{
					Xbyak::Label returnAddress;

					//Original code
					mov(ptr[rsp + 0x8], rbx);
					push(rdi);

					//New code
					push(rbx);
					push(rax);
					mov(rax, 0x0);

					mov(rbx, SprintStatusAddress.GetUIntPtr());
					mov(rbx, ptr[rbx]);
					mov(ptr[rbx + 0xBDD], rax);
					pop(rbx);
					pop(rax);
					jmp(ptr[rip + returnAddress]);

					L(returnAddress);
					dq(ControlFunction.GetUIntPtr() + 0x6);
				}
			};

			void * codeBuf = g_localTrampoline.StartAlloc();
			ControlFunction_Code code(codeBuf);
			g_localTrampoline.EndAlloc(code.getCurr());

			g_branchTrampoline.Write6Branch(ControlFunction.GetUIntPtr(), uintptr_t(code.getCode()));
		}

		_MESSAGE("Classic Sprinting Redone has been successfully started.");

		return true;
	}

};

