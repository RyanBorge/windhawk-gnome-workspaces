// ==WindhawkMod==
// @id              gnome-workspaces-win11
// @name            GNOME Workspaces for Windows 11
// @description     Replicates GNOME's dynamic workspaces, hot corner, and taskbar scroll on Windows 11
// @version         0.1.0
// @author          RyanBorge
// @github          https://github.com/RyanBorge/windhawk-gnome-workspaces
// @include         explorer.exe
// @architecture    x86-64
// @compilerOptions -lole32 -loleaut32 -luuid -ldwmapi -luser32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# GNOME Workspaces for Windows 11

Replicates GNOME's workspace workflow on Windows 11 (`explorer.exe`):
- **Dynamic Workspaces:** Automatically maintains `N+1` virtual desktops with an empty workspace at the end.
- **Hot Corner:** Moving the cursor to the top-left corner opens Task View (`Win + Tab`).
- **Taskbar Scroll:** Scrolling over the taskbar switches between adjacent virtual desktops.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- enableDynamicWorkspaces: true
  $name: Ativar Áreas de Trabalho Dinâmicas (Estilo GNOME)
  $description: Mantém sempre uma área de trabalho vazia ao final e remove áreas vazias intermediárias automaticamente.
- enableHotCorner: true
  $name: Ativar Hot Corner no canto superior esquerdo (abre Visão de Tarefas)
  $description: Abre a Visão de Tarefas (Win + Tab) ao posicionar o cursor no canto superior esquerdo do monitor principal.
- hotCornerDelayMs: 150
  $name: Tempo de permanência no canto em ms
  $description: Tempo (em milissegundos) que o cursor deve permanecer no canto superior esquerdo antes de disparar o Hot Corner.
- enableTaskbarScroll: true
  $name: Ativar troca de área de trabalho via Scroll na Barra de Tarefas
  $description: Permite alternar entre áreas de trabalho virtuais rolando a roda do mouse sobre a barra de tarefas.
*/
// ==/WindhawkModSettings==

#include <windows.h>
#undef NTDDI_VERSION
#define NTDDI_VERSION 0x0A000000
#include <objbase.h>
#include <objectarray.h>
#include <servprov.h>
#include <shobjidl.h>
#include <winstring.h>
#include <atomic>

// ============================================================================
// GUIDs & Undocumented COM Interfaces (Windows 11 24H2 / 26H2 - Build 26300+)
// ============================================================================

// {C2F03A33-21F5-47FA-B4BB-156362A2F239}
static const CLSID CLSID_ImmersiveShell = {
    0xC2F03A33, 0x21F5, 0x47FA, {0xB4, 0xBB, 0x15, 0x63, 0x62, 0xA2, 0xF2, 0x39}
};

// {C5E0CDCA-7B6E-41B2-9FC4-D93975CC467B}
static const CLSID CLSID_VirtualDesktopManagerInternal = {
    0xC5E0CDCA, 0x7B6E, 0x41B2, {0x9F, 0xC4, 0xD9, 0x39, 0x75, 0xCC, 0x46, 0x7B}
};

// {3F07F4BE-B107-441A-AF0F-39D82529072C}
[[maybe_unused]] static const IID IID_IVirtualDesktop = {
    0x3F07F4BE, 0xB107, 0x441A, {0xAF, 0x0F, 0x39, 0xD8, 0x25, 0x29, 0x07, 0x2C}
};

// {53F5CA0B-158F-4124-900C-057158060B27}
static const IID IID_IVirtualDesktopManagerInternal = {
    0x53F5CA0B, 0x158F, 0x4124, {0x90, 0x0C, 0x05, 0x71, 0x58, 0x06, 0x0B, 0x27}
};

interface IApplicationView : public IUnknown {};

MIDL_INTERFACE("3F07F4BE-B107-441A-AF0F-39D82529072C")
IVirtualDesktop : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE IsViewVisible(
        IApplicationView* pView,
        BOOL* pfVisible) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetID(
        GUID* pGuid) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetName(
        HSTRING* pName) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetWallpaperPath(
        HSTRING* pPath) = 0;
    virtual HRESULT STDMETHODCALLTYPE IsRemote(
        BOOL* pfRemote) = 0;
};

enum AdjacentDesktopDirection : int {
    DIRECTION_LEFT = 3,
    DIRECTION_RIGHT = 4
};

MIDL_INTERFACE("53F5CA0B-158F-4124-900C-057158060B27")
IVirtualDesktopManagerInternal : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE GetCount(
        UINT* pCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE MoveViewToDesktop(
        IApplicationView* pView,
        IVirtualDesktop* pDesktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE CanViewMoveDesktops(
        IApplicationView* pView,
        BOOL* pfCanViewMoveDesktops) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentDesktop(
        IVirtualDesktop** ppDesktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDesktops(
        IObjectArray** ppDesktops) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAdjacentDesktop(
        IVirtualDesktop* pDesktopReference,
        int uDirection,
        IVirtualDesktop** ppAdjacentDesktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE SwitchDesktop(
        IVirtualDesktop* pDesktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE SwitchDesktopAndMoveForegroundView(
        IVirtualDesktop* pDesktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateDesktop(
        IVirtualDesktop** ppNewDesktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE MoveDesktop(
        IVirtualDesktop* pDesktop,
        UINT uIndex) = 0;
    virtual HRESULT STDMETHODCALLTYPE RemoveDesktop(
        IVirtualDesktop* pRemove,
        IVirtualDesktop* pFallbackDesktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE FindDesktop(
        REFGUID desktopId,
        IVirtualDesktop** ppDesktop) = 0;
};

// ============================================================================
// Global State & Settings
// ============================================================================

struct ModSettings {
    std::atomic<bool> enableDynamicWorkspaces{true};
    std::atomic<bool> enableHotCorner{true};
    std::atomic<int> hotCornerDelayMs{150};
    std::atomic<bool> enableTaskbarScroll{true};
} g_settings;

static IServiceProvider* g_pServiceProvider = nullptr;
static IVirtualDesktopManagerInternal* g_pDesktopManagerInternal = nullptr;
static IVirtualDesktopManager* g_pDesktopManagerPublic = nullptr;
static bool g_comInitializedByMod = false;

static HANDLE g_stopEvent = nullptr;
static HANDLE g_hotCornerThread = nullptr;

// ============================================================================
// Settings Management
// ============================================================================

void LoadSettings() {
    g_settings.enableDynamicWorkspaces = Wh_GetIntSetting(L"enableDynamicWorkspaces") != 0;
    g_settings.enableHotCorner = Wh_GetIntSetting(L"enableHotCorner") != 0;

    int delay = Wh_GetIntSetting(L"hotCornerDelayMs");
    if (delay < 0) {
        delay = 0;
    } else if (delay > 5000) {
        delay = 5000;
    }
    g_settings.hotCornerDelayMs = delay;

    g_settings.enableTaskbarScroll = Wh_GetIntSetting(L"enableTaskbarScroll") != 0;

    Wh_Log(L"Settings loaded: DynamicWorkspaces=%d, HotCorner=%d, DelayMs=%d, TaskbarScroll=%d",
           g_settings.enableDynamicWorkspaces.load(),
           g_settings.enableHotCorner.load(),
           g_settings.hotCornerDelayMs.load(),
           g_settings.enableTaskbarScroll.load());
}

// ============================================================================
// COM Bridge Initialization & Cleanup
// ============================================================================

bool InitializeComBridge() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr)) {
        g_comInitializedByMod = true;
    } else if (hr != RPC_E_CHANGED_MODE) {
        Wh_Log(L"CoInitializeEx failed: 0x%08X", hr);
        return false;
    }

    hr = CoCreateInstance(
        CLSID_ImmersiveShell,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&g_pServiceProvider));
    if (FAILED(hr) || !g_pServiceProvider) {
        Wh_Log(L"CoCreateInstance(CLSID_ImmersiveShell) failed: 0x%08X", hr);
        return false;
    }

    hr = g_pServiceProvider->QueryService(
        CLSID_VirtualDesktopManagerInternal,
        IID_IVirtualDesktopManagerInternal,
        reinterpret_cast<void**>(&g_pDesktopManagerInternal));
    if (FAILED(hr) || !g_pDesktopManagerInternal) {
        Wh_Log(L"QueryService(IVirtualDesktopManagerInternal) failed: 0x%08X", hr);
        return false;
    }

    hr = CoCreateInstance(
        CLSID_VirtualDesktopManager,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&g_pDesktopManagerPublic));
    if (FAILED(hr) || !g_pDesktopManagerPublic) {
        Wh_Log(L"CoCreateInstance(CLSID_VirtualDesktopManager) failed: 0x%08X (non-fatal)", hr);
    }

    return true;
}

void CleanupComBridge() {
    if (g_pDesktopManagerPublic) {
        g_pDesktopManagerPublic->Release();
        g_pDesktopManagerPublic = nullptr;
    }
    if (g_pDesktopManagerInternal) {
        g_pDesktopManagerInternal->Release();
        g_pDesktopManagerInternal = nullptr;
    }
    if (g_pServiceProvider) {
        g_pServiceProvider->Release();
        g_pServiceProvider = nullptr;
    }
    if (g_comInitializedByMod) {
        CoUninitialize();
        g_comInitializedByMod = false;
    }
}

// ============================================================================
// COM Helper Functions
// ============================================================================

UINT GetDesktopCount() {
    if (!g_pDesktopManagerInternal) {
        return 0;
    }
    UINT count = 0;
    HRESULT hr = g_pDesktopManagerInternal->GetCount(&count);
    if (FAILED(hr)) {
        Wh_Log(L"GetCount failed: 0x%08X", hr);
        return 0;
    }
    return count;
}

bool SwitchToAdjacentDesktop(int direction) {
    if (!g_pDesktopManagerInternal) {
        return false;
    }
    if (direction != DIRECTION_LEFT && direction != DIRECTION_RIGHT) {
        return false;
    }

    IVirtualDesktop* pCurrent = nullptr;
    HRESULT hr = g_pDesktopManagerInternal->GetCurrentDesktop(&pCurrent);
    if (FAILED(hr) || !pCurrent) {
        Wh_Log(L"GetCurrentDesktop failed: 0x%08X", hr);
        return false;
    }

    IVirtualDesktop* pAdjacent = nullptr;
    hr = g_pDesktopManagerInternal->GetAdjacentDesktop(pCurrent, direction, &pAdjacent);
    pCurrent->Release();

    if (FAILED(hr) || !pAdjacent) {
        return false;
    }

    hr = g_pDesktopManagerInternal->SwitchDesktop(pAdjacent);
    pAdjacent->Release();

    if (FAILED(hr)) {
        Wh_Log(L"SwitchDesktop failed: 0x%08X", hr);
        return false;
    }
    return true;
}

IVirtualDesktop* CreateNewDesktop() {
    if (!g_pDesktopManagerInternal) {
        return nullptr;
    }
    IVirtualDesktop* pNewDesktop = nullptr;
    HRESULT hr = g_pDesktopManagerInternal->CreateDesktop(&pNewDesktop);
    if (FAILED(hr) || !pNewDesktop) {
        Wh_Log(L"CreateDesktop failed: 0x%08X", hr);
        return nullptr;
    }
    return pNewDesktop;
}

bool RemoveDesktopSafe(IVirtualDesktop* target, IVirtualDesktop* fallback) {
    if (!g_pDesktopManagerInternal || !target) {
        return false;
    }

    IVirtualDesktop* pResolvedFallback = fallback;
    bool releaseFallback = false;

    if (!pResolvedFallback) {
        HRESULT hr = g_pDesktopManagerInternal->GetAdjacentDesktop(
            target, DIRECTION_LEFT, &pResolvedFallback);
        if (FAILED(hr) || !pResolvedFallback) {
            hr = g_pDesktopManagerInternal->GetAdjacentDesktop(
                target, DIRECTION_RIGHT, &pResolvedFallback);
        }
        if (FAILED(hr) || !pResolvedFallback) {
            Wh_Log(L"RemoveDesktopSafe: unable to find a valid fallback desktop (0x%08X)", hr);
            return false;
        }
        releaseFallback = true;
    }

    HRESULT hr = g_pDesktopManagerInternal->RemoveDesktop(target, pResolvedFallback);
    if (releaseFallback && pResolvedFallback) {
        pResolvedFallback->Release();
    }

    if (FAILED(hr)) {
        Wh_Log(L"RemoveDesktop failed: 0x%08X", hr);
        return false;
    }
    return true;
}

// ============================================================================
// GNOME Hot Corner Thread
// ============================================================================

void TriggerTaskViewShortcut() {
    INPUT inputs[4] = {};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_LWIN;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_TAB;

    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = VK_TAB;
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_LWIN;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
}

DWORD WINAPI HotCornerThreadProc(LPVOID /*lpParam*/) {
    bool isTriggered = false;

    while (WaitForSingleObject(g_stopEvent, 25) == WAIT_TIMEOUT) {
        if (!g_settings.enableHotCorner.load()) {
            isTriggered = false;
            continue;
        }

        POINT pt{};
        if (!GetCursorPos(&pt)) {
            continue;
        }

        if (pt.x <= 2 && pt.y <= 2) {
            if (!isTriggered) {
                int delayMs = g_settings.hotCornerDelayMs.load();
                if (delayMs > 0) {
                    if (WaitForSingleObject(g_stopEvent, static_cast<DWORD>(delayMs)) == WAIT_OBJECT_0) {
                        break;
                    }
                }

                if (GetCursorPos(&pt) && pt.x <= 2 && pt.y <= 2) {
                    TriggerTaskViewShortcut();
                    isTriggered = true;
                    Wh_Log(L"Hot Corner triggered Task View (Win + Tab)");
                }
            }
        } else if (pt.x > 20 || pt.y > 20) {
            isTriggered = false;
        }
    }

    return 0;
}

// ============================================================================
// Windhawk Lifecycle Entry Points
// ============================================================================

BOOL Wh_ModInit() {
    Wh_Log(L"GNOME Workspaces: Initializing Module 1...");

    LoadSettings();

    if (InitializeComBridge()) {
        UINT count = GetDesktopCount();
        GUID currentId{};
        IVirtualDesktop* pCurrent = nullptr;
        if (SUCCEEDED(g_pDesktopManagerInternal->GetCurrentDesktop(&pCurrent)) && pCurrent) {
            pCurrent->GetID(&currentId);
            pCurrent->Release();
        }
        Wh_Log(L"COM bridge active. Virtual desktops count: %u | Current Desktop GUID: {%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
               count,
               currentId.Data1, currentId.Data2, currentId.Data3,
               currentId.Data4[0], currentId.Data4[1], currentId.Data4[2], currentId.Data4[3],
               currentId.Data4[4], currentId.Data4[5], currentId.Data4[6], currentId.Data4[7]);
    } else {
        Wh_Log(L"Warning: COM bridge initialization failed.");
    }

    g_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g_stopEvent) {
        Wh_Log(L"CreateEventW failed: %lu", GetLastError());
        CleanupComBridge();
        return FALSE;
    }

    g_hotCornerThread = CreateThread(nullptr, 0, HotCornerThreadProc, nullptr, 0, nullptr);
    if (!g_hotCornerThread) {
        Wh_Log(L"CreateThread(HotCornerThreadProc) failed: %lu", GetLastError());
        CloseHandle(g_stopEvent);
        g_stopEvent = nullptr;
        CleanupComBridge();
        return FALSE;
    }

    return TRUE;
}

void Wh_ModUninit() {
    Wh_Log(L"GNOME Workspaces: Uninitializing mod...");

    if (g_stopEvent) {
        SetEvent(g_stopEvent);
    }

    if (g_hotCornerThread) {
        WaitForSingleObject(g_hotCornerThread, INFINITE);
        CloseHandle(g_hotCornerThread);
        g_hotCornerThread = nullptr;
    }

    if (g_stopEvent) {
        CloseHandle(g_stopEvent);
        g_stopEvent = nullptr;
    }

    CleanupComBridge();
}

void Wh_ModSettingsChanged() {
    Wh_Log(L"GNOME Workspaces: Settings changed, reloading...");
    LoadSettings();
}
