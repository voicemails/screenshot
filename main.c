#include <windows.h>
#include <stdio.h>

#define SCREEN_HANDLE NULL

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previousInstance, LPSTR commandLine, int showCommand) {
    HDC screenDeviceContext = GetDC(SCREEN_HANDLE);

    ReleaseDC(SCREEN_HANDLE, screenDeviceContext);

    return 0;
}
