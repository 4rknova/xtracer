#ifdef _WIN32

#include <Windows.h>

#ifdef __cplusplus
	extern "C" {
#endif  /* __cplusplus */

#pragma warning(push)
#pragma warning(disable : 4100) // unreferenced formal parameter

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpvoid)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls(hModule);
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}

	return TRUE;
}

#pragma warning (pop)

#ifdef __cplusplus
	}
#endif  /* __cplusplus */

#endif /* _WIN32 */
