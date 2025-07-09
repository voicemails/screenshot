#include <windows.h>
#include <stdio.h>

#define SCREEN_HANDLE NULL

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previousInstance, LPSTR commandLine, int showCommand) {
    HDC screenDeviceContext = GetDC(SCREEN_HANDLE);
    {
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        printf("Screen Resolution: %dx%d\n", screenWidth, screenHeight);
    }
    ReleaseDC(SCREEN_HANDLE, screenDeviceContext);

    return 0;
}
