
// todo: thought I had a way in the c++ class desctructor that slammed the 
#ifndef _SAMPLEALLOC_
#define _SAMPLEALLOC_

inline void* __cdecl operator new (size_t size);
inline void __cdecl operator delete(void * lpv);


LPVOID ALLOC(ULONG cb);
void FREE(void* pv);
LPVOID REALLOC(void *pv, ULONG cb);

#ifdef _DEBUG

#define MEMINITVALUE 0xff
#define MEMFREEVALUE 0xfe

#endif // DEBUG


#endif // _SAMPLEALLOC_
