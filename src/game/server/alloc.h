/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ALLOC_H
#define GAME_SERVER_ALLOC_H

#include <new>

#include <base/system.h>
#ifndef __has_feature
#define __has_feature(x) 0
#endif
#if __has_feature(address_sanitizer)
#include <sanitizer/asan_interface.h>
#else
#define ASAN_POISON_MEMORY_REGION(addr, size) \
	((void)(addr), (void)(size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) \
	((void)(addr), (void)(size))
#endif

#if __cplusplus >= 201703L
#define MAYBE_UNUSED [[maybe_unused]]
#elif defined(__GNUC__)
#define MAYBE_UNUSED __attribute__((unused))
#else
#define MAYBE_UNUSED
#endif

#define MACRO_ALLOC_HEAP() \
public: \
	void *operator new(size_t Size) \
	{ \
		void *p = malloc(Size); \
		mem_zero(p, Size); \
		return p; \
	} \
	void operator delete(void *pPtr) \
	{ \
		free(pPtr); \
	} \
\
private:

#define MACRO_ALLOC_POOL_ID() \
public: \
	void *operator new(size_t Size, int id); \
	void operator delete(void *p, int id); \
	void operator delete(void *p); /* NOLINT(misc-new-delete-overloads) */ \
\
private:

#if __has_feature(address_sanitizer)
#define MACRO_ALLOC_GET_SIZE(POOLTYPE) ((sizeof(POOLTYPE) + 7) & ~7)
#else
#define MACRO_ALLOC_GET_SIZE(POOLTYPE) (sizeof(POOLTYPE))
#endif

#define MACRO_ALLOC_POOL_ID_IMPL(POOLTYPE, PoolSize) \
	static char ms_PoolData##POOLTYPE[PoolSize][MACRO_ALLOC_GET_SIZE(POOLTYPE)] = {{0}}; \
	static int ms_PoolUsed##POOLTYPE[PoolSize] = {0}; \
	MAYBE_UNUSED static int ms_PoolDummy##POOLTYPE = (ASAN_POISON_MEMORY_REGION(ms_PoolData##POOLTYPE, sizeof(ms_PoolData##POOLTYPE)), 0); \
	void *POOLTYPE::operator new(size_t Size, int id) \
	{ \
		dbg_assert(sizeof(POOLTYPE) >= Size, "size error"); \
		dbg_assert(!ms_PoolUsed##POOLTYPE[id], "already used"); \
		/*dbg_msg("pool", "++ %s %d", #POOLTYPE, id);*/ \
		ASAN_UNPOISON_MEMORY_REGION(ms_PoolData##POOLTYPE[id], sizeof(ms_PoolData##POOLTYPE[id])); \
		ms_PoolUsed##POOLTYPE[id] = 1; \
		mem_zero(ms_PoolData##POOLTYPE[id], sizeof(ms_PoolData##POOLTYPE[id])); \
		return ms_PoolData##POOLTYPE[id]; \
	} \
	void POOLTYPE::operator delete(void *p, int id) \
	{ \
		dbg_assert(ms_PoolUsed##POOLTYPE[id], "not used"); \
		dbg_assert(id == (POOLTYPE *)p - (POOLTYPE *)ms_PoolData##POOLTYPE, "invalid id"); \
		/*dbg_msg("pool", "-- %s %d", #POOLTYPE, id);*/ \
		ms_PoolUsed##POOLTYPE[id] = 0; \
		mem_zero(ms_PoolData##POOLTYPE[id], sizeof(ms_PoolData##POOLTYPE[id])); \
		ASAN_POISON_MEMORY_REGION(ms_PoolData##POOLTYPE[id], sizeof(ms_PoolData##POOLTYPE[id])); \
	} \
	void POOLTYPE::operator delete(void *p) /* NOLINT(misc-new-delete-overloads) */ \
	{ \
		int id = (POOLTYPE *)p - (POOLTYPE *)ms_PoolData##POOLTYPE; \
		dbg_assert(ms_PoolUsed##POOLTYPE[id], "not used"); \
		/*dbg_msg("pool", "-- %s %d", #POOLTYPE, id);*/ \
		ms_PoolUsed##POOLTYPE[id] = 0; \
		mem_zero(ms_PoolData##POOLTYPE[id], sizeof(ms_PoolData##POOLTYPE[id])); \
		ASAN_POISON_MEMORY_REGION(ms_PoolData##POOLTYPE[id], sizeof(ms_PoolData##POOLTYPE[id])); \
	}

#endif
