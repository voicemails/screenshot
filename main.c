#include <windows.h>
#include <stdio.h>

#define SCREEN_HANDLE NULL

int GetScreenWidth() {
    return GetSystemMetrics(SM_CXSCREEN);
}

int GetScreenHeight() {
    return GetSystemMetrics(SM_CYSCREEN);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previousInstance, LPSTR commandLine, int showCommand) {
    HDC screenDeviceContext = GetDC(SCREEN_HANDLE);
    {
        printf("Screen Resolution: %dx%d\n", GetScreenWidth(), GetScreenHeight());
    }
    ReleaseDC(SCREEN_HANDLE, screenDeviceContext);

    return 0;
}
