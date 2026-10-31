#pragma once

#include <cstddef>
#include <cstdlib>

// Memory-limited allocator for lua_newstate(), header-only so it can be unit
// tested without linking Lua.

// Limit Lua scripts to 32 MB of memory to prevent DoS via runaway allocation.
static constexpr size_t kLuaMemoryLimit = 32 * 1024 * 1024;

struct LuaAllocState {
    size_t used = 0;
    size_t limit = kLuaMemoryLimit;
};

// Follows the lua_Alloc contract:
//  - when ptr == NULL, osize is a type tag (LUA_TSTRING etc.), NOT a size, so
//    the old block size is 0;
//  - nsize == 0 frees (a free of NULL is a no-op);
//  - if the allocator fails, the old block stays valid and the accounting
//    must not change.
inline void *luaLimitedAlloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    auto *state = static_cast<LuaAllocState *>(ud);
    const size_t old = ptr ? osize : 0;
    if (nsize == 0) {
        if (ptr) {
            state->used = (state->used >= old) ? state->used - old : 0;
            std::free(ptr);
        }
        return nullptr;
    }
    const size_t avail = (state->limit > state->used) ? state->limit - state->used : 0;
    if (nsize > old && nsize - old > avail)
        return nullptr;  // deny — Lua raises a memory error; old block untouched
    void *newPtr = std::realloc(ptr, nsize);
    if (!newPtr)
        return nullptr;  // realloc failed: old block still valid, used unchanged
    state->used = state->used - old + nsize;
    return newPtr;
}
