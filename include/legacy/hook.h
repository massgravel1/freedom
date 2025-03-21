#pragma once

#include "memory.h"

#include <windows.h>

#include <stdint.h>
#include <assert.h>
#include <random>  // For random number generation

// Placeholder functions - actual implementations would be needed.
bool detour_32(BYTE *src, BYTE *dst, const uintptr_t len);
BYTE *trampoline_32(BYTE *src, BYTE *dst, const uintptr_t len);

struct Detour32 {};
struct Trampoline32 {};

// Obfuscation helper functions (illustrative)
BYTE* obfuscated_jump(BYTE *src, BYTE *dst) {
    //Replace the original jump instruction with a series of instructions
    // that perform an equivalent jump, while obfuscating it.
    // This is a simplified example.  A good obfuscator will be MUCH more complex
    // and take the target architecture into account.
    BYTE jump_code[] = {
        0x50,             // PUSH EAX
        0x68, 0x00, 0x00, 0x00, 0x00,  // PUSH dst (replace 0x00 with the address of dst)
        0xC3              // RETN (return, which effectively jumps to the address pushed on the stack)
    };
    //Calculate the offset to dst, place the correct data there
    uintptr_t dst_addr = (uintptr_t)dst;
    jump_code[3] = (dst_addr) & 0xFF;       // Least significant byte
    jump_code[4] = (dst_addr >> 8) & 0xFF;  // Second least significant byte
    jump_code[5] = (dst_addr >> 16) & 0xFF; // Third least significant byte
    jump_code[6] = (dst_addr >> 24) & 0xFF; // Most significant byte

    memcpy(src, jump_code, sizeof(jump_code)); //Overwrites original code with obfuscated jump
    return src+sizeof(jump_code);
}

// Random delay (to potentially make timing attacks less effective, though it's very basic)
void random_delay() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, 100); // Delay for 1-100 milliseconds
    Sleep(distrib(gen));
}

template <typename T>
struct Hook
{
    BYTE *src = 0;
    BYTE *dst = 0;
    BYTE *PtrToGatewayFnPtr = 0;
    uintptr_t len = 0;
    BYTE originalBytes[32] = {0}; //Increased size, because obfuscation might require more bytes
    bool enabled = false;
    bool free_gateway = true;

    Hook() {}
    Hook(uintptr_t src, BYTE *dst, uintptr_t len) : src((BYTE *)src), dst(dst), len(len) {}
    Hook(BYTE *src, BYTE *dst, uintptr_t len) : src(src), dst(dst), len(len) {}
    Hook(BYTE *src, BYTE *dst, BYTE *PtrToGatewayFnPtr, uintptr_t len) : src(src), dst(dst), len(len), PtrToGatewayFnPtr(PtrToGatewayFnPtr) {}
    Hook(uintptr_t src, BYTE *dst, BYTE *PtrToGatewayFnPtr, uintptr_t len) : src((BYTE *)src), dst(dst), len(len), PtrToGatewayFnPtr(PtrToGatewayFnPtr) {}
    Hook(const char *exportName, const char *modName, BYTE *dst, BYTE *PtrToGatewayFnPtr, uintptr_t len) : dst(dst), len(len), PtrToGatewayFnPtr(PtrToGatewayFnPtr)
    {
        HMODULE hMod = GetModuleHandleA(modName);
        this->src = (BYTE *)GetProcAddress(hMod, exportName);
    }
    void Enable();
    void Disable();
};

template <>
inline void Hook<Trampoline32>::Enable()
{
    if (!enabled)
    {
        // More robust check, based on the *actual* size needed by the trampoline.
        // In reality, a trampoline may need more than 16 bytes, depending on how it is created.
        uintptr_t actual_trampoline_size = 16; //Replace this with the actual size after trampoline generation
        if (len > actual_trampoline_size) //Should be updated when trampoline_32 is called
        {
            //In the case of a real, complex situation, instead of assert, you could try to re-generate the
            //trampoline function with a larger size, and re-try.  This could also be obfuscated, by
            //attempting multiple sizes.
            return;
        }

        // Store original bytes *before* obfuscation
        memcpy(originalBytes, src, len);
        *(uintptr_t *)PtrToGatewayFnPtr = (uintptr_t)trampoline_32(src, dst, len); //TRAMPOLINE IS GENERATED
        enabled = true;
    }
}

template <>
inline void Hook<Trampoline32>::Disable()
{
    if (enabled)
    {
        // Restore original bytes
        internal_memory_patch(src, originalBytes, len);  //Restore original code

        if (free_gateway) {
            VirtualFree(*(LPVOID *)PtrToGatewayFnPtr, 0, MEM_RELEASE); //Free Memory
        }
        enabled = false;
    }
}

template <>
inline void Hook<Detour32>::Enable()
{
    if (!enabled)
    {
        //Check the minimum length before the actual obfuscation of the code.
        //In reality, there could be many checks.
        if (len < 5)
        {
            return;
        }

        //Store the original bytes.
        memcpy(originalBytes, src, len); //Store Original Bytes

        //Obfuscate the jump:
        BYTE* new_src = obfuscated_jump(src, dst);

        //The length is now the size of the obfuscated code.
        //Ensure the length matches the obfuscated function length.
        len = new_src - src;

        //Delay to throw off timing analysis
        random_delay();

        // Set enabled flag only after the function is ready.
        enabled = true;
    }
}

template <>
inline void Hook<Detour32>::Disable()
{
    if (enabled)
    {
        internal_memory_patch(src, originalBytes, len); //Restore original bytes
        random_delay(); //Delay to throw off timing analysis
        enabled = false;
    }
}
