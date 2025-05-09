#ifndef PROXY_VERSION_DLL_H
#define PROXY_VERSION_DLL_H

#include <Windows.h>
#include <stdio.h>

namespace ProxyVersionDllNs {

    // --- Globals for proxying original version.dll functions ---
    static HMODULE g_hOriginalVersionDll = NULL;

    static FARPROC g_pfnGetFileVersionInfoA = NULL;
    static FARPROC g_pfnGetFileVersionInfoByHandle = NULL;
    static FARPROC g_pfnGetFileVersionInfoExA = NULL;
    static FARPROC g_pfnGetFileVersionInfoExW = NULL;
    static FARPROC g_pfnGetFileVersionInfoSizeA = NULL;
    static FARPROC g_pfnGetFileVersionInfoSizeExA = NULL;
    static FARPROC g_pfnGetFileVersionInfoSizeExW = NULL;
    static FARPROC g_pfnGetFileVersionInfoSizeW = NULL;
    static FARPROC g_pfnGetFileVersionInfoW = NULL;
    static FARPROC g_pfnVerFindFileA = NULL;
    static FARPROC g_pfnVerFindFileW = NULL;
    static FARPROC g_pfnVerInstallFileA = NULL;
    static FARPROC g_pfnVerInstallFileW = NULL;
    static FARPROC g_pfnVerLanguageNameA = NULL;
    static FARPROC g_pfnVerLanguageNameW = NULL;
    static FARPROC g_pfnVerQueryValueA = NULL;
    static FARPROC g_pfnVerQueryValueW = NULL;

    bool InitializeProxy() {
        char systemPath[MAX_PATH];
        if (GetSystemDirectoryA(systemPath, MAX_PATH) == 0) {
            printf("[Begeerte Error] Failed to get system directory.\n");
            return false;
        }

        strcat_s(systemPath, MAX_PATH, "\\version.dll");
        g_hOriginalVersionDll = LoadLibraryA(systemPath);

        if (!g_hOriginalVersionDll) {
            printf("[Begeerte Error] Failed to load original version.dll from %s. Error: %lu\n", systemPath, GetLastError());
            return false;
        }
        printf("[Begeerte Info] Original version.dll loaded.\n");

        g_pfnGetFileVersionInfoA = GetProcAddress(g_hOriginalVersionDll, "GetFileVersionInfoA");
        g_pfnGetFileVersionInfoByHandle = GetProcAddress(g_hOriginalVersionDll, "GetFileVersionInfoByHandle");
        g_pfnGetFileVersionInfoExA = GetProcAddress(g_hOriginalVersionDll, "GetFileVersionInfoExA");
        g_pfnGetFileVersionInfoExW = GetProcAddress(g_hOriginalVersionDll, "GetFileVersionInfoExW");
        g_pfnGetFileVersionInfoSizeA = GetProcAddress(g_hOriginalVersionDll, "GetFileVersionInfoSizeA");
        g_pfnGetFileVersionInfoSizeExA = GetProcAddress(g_hOriginalVersionDll, "GetFileVersionInfoSizeExA");
        g_pfnGetFileVersionInfoSizeExW = GetProcAddress(g_hOriginalVersionDll, "GetFileVersionInfoSizeExW");
        g_pfnGetFileVersionInfoSizeW = GetProcAddress(g_hOriginalVersionDll, "GetFileVersionInfoSizeW");
        g_pfnGetFileVersionInfoW = GetProcAddress(g_hOriginalVersionDll, "GetFileVersionInfoW");
        g_pfnVerFindFileA = GetProcAddress(g_hOriginalVersionDll, "VerFindFileA");
        g_pfnVerFindFileW = GetProcAddress(g_hOriginalVersionDll, "VerFindFileW");
        g_pfnVerInstallFileA = GetProcAddress(g_hOriginalVersionDll, "VerInstallFileA");
        g_pfnVerInstallFileW = GetProcAddress(g_hOriginalVersionDll, "VerInstallFileW");
        g_pfnVerLanguageNameA = GetProcAddress(g_hOriginalVersionDll, "VerLanguageNameA");
        g_pfnVerLanguageNameW = GetProcAddress(g_hOriginalVersionDll, "VerLanguageNameW");
        g_pfnVerQueryValueA = GetProcAddress(g_hOriginalVersionDll, "VerQueryValueA");
        g_pfnVerQueryValueW = GetProcAddress(g_hOriginalVersionDll, "VerQueryValueW");

        if (!g_pfnGetFileVersionInfoA || !g_pfnGetFileVersionInfoSizeA || !g_pfnVerQueryValueA) {
            printf("[Begeerte Warning] Not all expected functions were found in the original version.dll.\n");
        }
        printf("[Begeerte Info] Original version.dll functions mapped.\n");
        return true;
    }

    void CleanupProxy() {
        if (g_hOriginalVersionDll) {
            FreeLibrary(g_hOriginalVersionDll);
            g_hOriginalVersionDll = NULL;
        }
    }

    // --- Exported proxy functions ---
#pragma comment(linker, "/export:GetFileVersionInfoA=Proxy_GetFileVersionInfoA")
#pragma comment(linker, "/export:GetFileVersionInfoByHandle=Proxy_GetFileVersionInfoByHandle")
#pragma comment(linker, "/export:GetFileVersionInfoExA=Proxy_GetFileVersionInfoExA")
#pragma comment(linker, "/export:GetFileVersionInfoExW=Proxy_GetFileVersionInfoExW")
#pragma comment(linker, "/export:GetFileVersionInfoSizeA=Proxy_GetFileVersionInfoSizeA")
#pragma comment(linker, "/export:GetFileVersionInfoSizeExA=Proxy_GetFileVersionInfoSizeExA")
#pragma comment(linker, "/export:GetFileVersionInfoSizeExW=Proxy_GetFileVersionInfoSizeExW")
#pragma comment(linker, "/export:GetFileVersionInfoSizeW=Proxy_GetFileVersionInfoSizeW")
#pragma comment(linker, "/export:GetFileVersionInfoW=Proxy_GetFileVersionInfoW")
#pragma comment(linker, "/export:VerFindFileA=Proxy_VerFindFileA")
#pragma comment(linker, "/export:VerFindFileW=Proxy_VerFindFileW")
#pragma comment(linker, "/export:VerInstallFileA=Proxy_VerInstallFileA")
#pragma comment(linker, "/export:VerInstallFileW=Proxy_VerInstallFileW")
#pragma comment(linker, "/export:VerLanguageNameA=Proxy_VerLanguageNameA")
#pragma comment(linker, "/export:VerLanguageNameW=Proxy_VerLanguageNameW")
#pragma comment(linker, "/export:VerQueryValueA=Proxy_VerQueryValueA")
#pragma comment(linker, "/export:VerQueryValueW=Proxy_VerQueryValueW")

    extern "C" {
        DWORD APIENTRY Proxy_GetFileVersionInfoSizeA(LPCSTR lptstrFilename, LPDWORD lpdwHandle) {
            if (g_pfnGetFileVersionInfoSizeA)
                return ((DWORD(APIENTRY*)(LPCSTR, LPDWORD))g_pfnGetFileVersionInfoSizeA)(lptstrFilename, lpdwHandle);
            SetLastError(ERROR_PROC_NOT_FOUND); return 0;
        }

        BOOL APIENTRY Proxy_GetFileVersionInfoA(LPCSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
            if (g_pfnGetFileVersionInfoA)
                return ((BOOL(APIENTRY*)(LPCSTR, DWORD, DWORD, LPVOID))g_pfnGetFileVersionInfoA)(lptstrFilename, dwHandle, dwLen, lpData);
            SetLastError(ERROR_PROC_NOT_FOUND); return FALSE;
        }

        DWORD APIENTRY Proxy_GetFileVersionInfoSizeW(LPCWSTR lptstrFilename, LPDWORD lpdwHandle) {
            if (g_pfnGetFileVersionInfoSizeW)
                return ((DWORD(APIENTRY*)(LPCWSTR, LPDWORD))g_pfnGetFileVersionInfoSizeW)(lptstrFilename, lpdwHandle);
            SetLastError(ERROR_PROC_NOT_FOUND); return 0;
        }

        BOOL APIENTRY Proxy_GetFileVersionInfoW(LPCWSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
            if (g_pfnGetFileVersionInfoW)
                return ((BOOL(APIENTRY*)(LPCWSTR, DWORD, DWORD, LPVOID))g_pfnGetFileVersionInfoW)(lptstrFilename, dwHandle, dwLen, lpData);
            SetLastError(ERROR_PROC_NOT_FOUND); return FALSE;
        }

        BOOL APIENTRY Proxy_VerQueryValueA(LPCVOID pBlock, LPCSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen) {
            if (g_pfnVerQueryValueA)
                return ((BOOL(APIENTRY*)(LPCVOID, LPCSTR, LPVOID*, PUINT))g_pfnVerQueryValueA)(pBlock, lpSubBlock, lplpBuffer, puLen);
            SetLastError(ERROR_PROC_NOT_FOUND); return FALSE;
        }

        BOOL APIENTRY Proxy_VerQueryValueW(LPCVOID pBlock, LPCWSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen) {
            if (g_pfnVerQueryValueW)
                return ((BOOL(APIENTRY*)(LPCVOID, LPCWSTR, LPVOID*, PUINT))g_pfnVerQueryValueW)(pBlock, lpSubBlock, lplpBuffer, puLen);
            SetLastError(ERROR_PROC_NOT_FOUND); return FALSE;
        }

        DWORD APIENTRY Proxy_GetFileVersionInfoByHandle(DWORD dwFlags, HANDLE hFile, LPDWORD lpdwHandle, LPDWORD lpdwLen, LPVOID lpData) {
            if (g_pfnGetFileVersionInfoByHandle)
                return ((DWORD(APIENTRY*)(DWORD, HANDLE, LPDWORD, LPDWORD, LPVOID))g_pfnGetFileVersionInfoByHandle)(dwFlags, hFile, lpdwHandle, lpdwLen, lpData);
            SetLastError(ERROR_PROC_NOT_FOUND); return 0;
        }

        BOOL APIENTRY Proxy_GetFileVersionInfoExA(DWORD dwFlags, LPCSTR lpszFileName, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
            if (g_pfnGetFileVersionInfoExA)
                return ((BOOL(APIENTRY*)(DWORD, LPCSTR, DWORD, DWORD, LPVOID))g_pfnGetFileVersionInfoExA)(dwFlags, lpszFileName, dwHandle, dwLen, lpData);
            SetLastError(ERROR_PROC_NOT_FOUND); return FALSE;
        }

        BOOL APIENTRY Proxy_GetFileVersionInfoExW(DWORD dwFlags, LPCWSTR lpszFileName, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
            if (g_pfnGetFileVersionInfoExW)
                return ((BOOL(APIENTRY*)(DWORD, LPCWSTR, DWORD, DWORD, LPVOID))g_pfnGetFileVersionInfoExW)(dwFlags, lpszFileName, dwHandle, dwLen, lpData);
            SetLastError(ERROR_PROC_NOT_FOUND); return FALSE;
        }

        DWORD APIENTRY Proxy_GetFileVersionInfoSizeExA(DWORD dwFlags, LPCSTR lpszFileName, LPDWORD lpdwHandle) {
            if (g_pfnGetFileVersionInfoSizeExA)
                return ((DWORD(APIENTRY*)(DWORD, LPCSTR, LPDWORD))g_pfnGetFileVersionInfoSizeExA)(dwFlags, lpszFileName, lpdwHandle);
            SetLastError(ERROR_PROC_NOT_FOUND); return 0;
        }

        DWORD APIENTRY Proxy_GetFileVersionInfoSizeExW(DWORD dwFlags, LPCWSTR lpszFileName, LPDWORD lpdwHandle) {
            if (g_pfnGetFileVersionInfoSizeExW)
                return ((DWORD(APIENTRY*)(DWORD, LPCWSTR, LPDWORD))g_pfnGetFileVersionInfoSizeExW)(dwFlags, lpszFileName, lpdwHandle);
            SetLastError(ERROR_PROC_NOT_FOUND); return 0;
        }

        DWORD APIENTRY Proxy_VerFindFileA(DWORD uFlags, LPCSTR szFileName, LPCSTR szWinDir, LPCSTR szAppDir, LPSTR szCurDir, PUINT puCurDirLen, LPSTR szDestDir, PUINT puDestDirLen) {
            if (g_pfnVerFindFileA)
                return ((DWORD(APIENTRY*)(DWORD, LPCSTR, LPCSTR, LPCSTR, LPSTR, PUINT, LPSTR, PUINT))g_pfnVerFindFileA)(uFlags, szFileName, szWinDir, szAppDir, szCurDir, puCurDirLen, szDestDir, puDestDirLen);
            SetLastError(ERROR_PROC_NOT_FOUND); return 0;
        }

        DWORD APIENTRY Proxy_VerFindFileW(DWORD uFlags, LPCWSTR szFileName, LPCWSTR szWinDir, LPCWSTR szAppDir, LPWSTR szCurDir, PUINT puCurDirLen, LPWSTR szDestDir, PUINT puDestDirLen) {
            if (g_pfnVerFindFileW)
                return ((DWORD(APIENTRY*)(DWORD, LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, PUINT, LPWSTR, PUINT))g_pfnVerFindFileW)(uFlags, szFileName, szWinDir, szAppDir, szCurDir, puCurDirLen, szDestDir, puDestDirLen);
            SetLastError(ERROR_PROC_NOT_FOUND); return 0;
        }

        DWORD APIENTRY Proxy_VerInstallFileA(DWORD uFlags, LPCSTR szSrcFileName, LPCSTR szDestFileName, LPCSTR szSrcDir, LPCSTR szDestDir, LPCSTR szCurDir, LPSTR szTmpFile, PUINT puTmpFileLen) {
            if (g_pfnVerInstallFileA)
                return ((DWORD(APIENTRY*)(DWORD, LPCSTR, LPCSTR, LPCSTR, LPCSTR, LPCSTR, LPSTR, PUINT))g_pfnVerInstallFileA)(uFlags, szSrcFileName, szDestFileName, szSrcDir, szDestDir, szCurDir, szTmpFile, puTmpFileLen);
            SetLastError(ERROR_PROC_NOT_FOUND); return 0;
        }

        DWORD APIENTRY Proxy_VerInstallFileW(DWORD uFlags, LPCWSTR szSrcFileName, LPCWSTR szDestFileName, LPCWSTR szSrcDir, LPCWSTR szDestDir, LPCWSTR szCurDir, LPWSTR szTmpFile, PUINT puTmpFileLen) {
            if (g_pfnVerInstallFileW)
                return ((DWORD(APIENTRY*)(DWORD, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, PUINT))g_pfnVerInstallFileW)(uFlags, szSrcFileName, szDestFileName, szSrcDir, szDestDir, szCurDir, szTmpFile, puTmpFileLen);
            SetLastError(ERROR_PROC_NOT_FOUND); return 0;
        }

        DWORD APIENTRY Proxy_VerLanguageNameA(DWORD wLang, LPSTR szLang, DWORD nSize) {
            if (g_pfnVerLanguageNameA)
                return ((DWORD(APIENTRY*)(DWORD, LPSTR, DWORD))g_pfnVerLanguageNameA)(wLang, szLang, nSize);
            SetLastError(ERROR_PROC_NOT_FOUND); return 0;
        }

        DWORD APIENTRY Proxy_VerLanguageNameW(DWORD wLang, LPWSTR szLang, DWORD nSize) {
            if (g_pfnVerLanguageNameW)
                return ((DWORD(APIENTRY*)(DWORD, LPWSTR, DWORD))g_pfnVerLanguageNameW)(wLang, szLang, nSize);
            SetLastError(ERROR_PROC_NOT_FOUND); return 0;
        }
    }

} // namespace ProxyVersionDllNs

#endif // PROXY_VERSION_DLL_H