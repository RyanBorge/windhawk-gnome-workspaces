// ==WindhawkMod==
// @id              gnome-workspaces
// @name            GNOME Workspaces
// @description     Replicates GNOME's dynamic workspaces, hot corner, and taskbar scroll on Windows 11
// @version         0.1.0
// @author          RyanBorge
// @github          https://github.com/RyanBorge
// @include         explorer.exe
// @architecture    x86-64
// @compilerOptions -lole32 -loleaut32 -luuid
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# GNOME Workspaces

Replicates GNOME's workspace behavior on Windows 11:
- **Dynamic Workspaces:** Automatically manages virtual desktops so there is always an empty one at the end.
- **Hot Corner:** Top-left hot corner triggers Task View.
- **Taskbar Scroll:** Scroll on the taskbar to switch between virtual desktops.
*/
// ==/WindhawkModReadme==

#include <windows.h>

BOOL Wh_ModInit() {
    Wh_Log(L"GNOME Workspaces: Initializing mod...");
    return TRUE;
}

void Wh_ModUninit() {
    Wh_Log(L"GNOME Workspaces: Uninitializing mod...");
}
