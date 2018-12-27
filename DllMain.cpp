#include <algorithm>
#include <fstream>
#include <stdio.h>
#include <string>
#include <vector>
#include <Windows.h>

#include "Armageddon.hpp"

///////////////////////////////////////////////////////////////

std::vector<WeaponEntry> g_customEntries;

///////////////////////////////////////////////////////////////
// Utility

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

static Weapon stringToWeaponId(std::string const& weaponName)
{
  for (int i = 0; i < Weapon::Count; ++i)
  {
    if (weaponName == WeaponStrings[i])
      return static_cast<Weapon>(i);
  }

  return Weapon::Count;
}

static void readConfig(const char* sFileName)
{
  std::fstream configFile(sFileName);
  if (!configFile.is_open())
    return;

  std::string line;
  while (std::getline(configFile, line))
  {
    // Ignore if line is empty or a comment
    if (line.empty() || line[0] == ';')
      continue;

    // Remove spaces
    line.erase(std::remove(line.begin(), line.end(), ' '), line.end());

    // Split at '='
    size_t splitPos = line.find('=');
    if (splitPos == std::string::npos) 
      continue;

    std::string sWeaponName = line.substr(0, splitPos);
    std::string sCrates = line.substr(splitPos + 1);

    Weapon weaponId = stringToWeaponId(sWeaponName);
    unsigned int crateChance = atoi(sCrates.c_str());

    if (weaponId == Weapon::Count)
      continue;

    // * 20 because that's how the game does it
    g_customEntries.emplace_back(weaponId, crateChance * 20);
  }

#ifdef _DEBUG
  printf("Config from %s\n", sFileName);
  for (auto weaponEntry : g_customEntries)
    printf("WeaponID: %s\tChance: %d\n", WeaponStrings[weaponEntry.id], weaponEntry.chance);

  printf("\n\n");
#endif // _DEBUG
}

///////////////////////////////////////////////////////////////
// Hook stuff

static void __stdcall overrideCrateChances(WeaponEntryArray* pArray)
{
  if (g_customEntries.empty())
    return;

#ifdef _DEBUG
  // print original list for debug
  for (int i = 0; i < pArray->size; ++i)
    printf("WeaponID: %s\tChance: %d\n", WeaponStrings[pArray->entries[i].id], pArray->entries[i].chance);

  printf("\n\n");
#endif // _DEBUG

  pArray->size = g_customEntries.size();
  for (int i = 0; i < g_customEntries.size(); ++i)
  {
    WeaponEntry* pEntry = &pArray->entries[i];
    *pEntry = g_customEntries[i];
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
#ifdef _DEBUG
  initConsole();
#endif // _DEBUG
  readConfig("wkCrateEditor.ini");

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