#include <stdio.h>
#include <set>
#include <Windows.h>

#include "Chances.hpp"

struct WeaponEntryArray
{
  unsigned int size;
  unsigned int maxSize;
  WeaponEntry* entries;
};

HANDLE hstdin, hstdout;
FILE* pfstdin;
FILE* pfstdout;

static void initConsole()
{
  AllocConsole();
  freopen_s(&pfstdout, "CONOUT$", "w", stdout);
  freopen_s(&pfstdin, "CONIN$", "r", stdin);
  hstdin = GetStdHandle(STD_INPUT_HANDLE);
  hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
}

static bool writeMemory(DWORD_PTR dwAddress, const void* cpvPatch, DWORD dwSize)
{
  DWORD dwProtect;
  if (VirtualProtect((void*)dwAddress, dwSize, PAGE_READWRITE, &dwProtect)) //Unprotect the memory
    memcpy((void*)dwAddress, cpvPatch, dwSize);
  else
    return false;

  return VirtualProtect((void*)dwAddress, dwSize, dwProtect, new DWORD); //Reprotect the memory
}

///////////////////////////////////////////////////////////////
// Hook stuff

static void __stdcall overrideCrateChances(WeaponEntryArray* pArray)
{
  int modifiedEntries = 0;
  std::set<unsigned int> usedWeapons;

  // print original list for debug
  for (int i = 0; i < pArray->size; ++i)
    printf("WeaponID: %s\tChance: %d\n", WeaponStrings[pArray->entries[i].id], pArray->entries[i].chance);

  printf("\n\n");

  pArray->size = szCustomChances;
  for (int i = 0; i < szCustomChances; ++i)
  {
    WeaponEntry* pEntry = &pArray->entries[i];
    *pEntry = CustomChances[i];
  }
}

static void __declspec(naked) hookFunc()
{
  __asm
  {
    lea eax, [esp +0x20]
    push eax
    call overrideCrateChances
    // Original opcode replaced by call
    mov eax, [ebp+0x34]
    mov ecx, [eax+0x24]
    ret
  }
}


///////////////////////////////////////////////////////////////

void main()
{
  initConsole();
  //printf("I like cats\n");

  int hookAddress = reinterpret_cast<int>(&hookFunc);
  int relative = hookAddress - 0x533575;
    
  writeMemory(0x533570, "\xE8", 1); // call
  writeMemory(0x533571, &relative, 4); // relative address to hookFunc
  writeMemory(0x533575, "\x90", 1); // nop
}

DWORD WINAPI DllMain(HMODULE hInstance, DWORD dwReason, LPVOID lpReserved)
{
  switch (dwReason)
  {
  case DLL_PROCESS_ATTACH:
    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)main, 0, 0, 0);
    break;
  case DLL_PROCESS_DETACH:
    break;
  }

  return 1;
}