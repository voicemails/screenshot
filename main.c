#include <windows.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define SCREEN_HANDLE NULL

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previousInstance, LPSTR commandLine, int showCommand) {
    /* TODO: Error handling (checking return values, etc.)
     */
    const int SCREEN_WIDTH = GetSystemMetrics(SM_CXSCREEN);
    const int SCREEN_HEIGHT = GetSystemMetrics(SM_CYSCREEN);
    const int SCREEN_AREA = SCREEN_WIDTH * SCREEN_HEIGHT;

    HDC screenDeviceContext = GetDC(SCREEN_HANDLE);
    {
        HDC screenCompatibleDeviceContext = CreateCompatibleDC(screenDeviceContext);
        {
            BITMAPINFO screenBitmapInfo = {0};

            BITMAPINFOHEADER screenBitmapHeader = {0};
            screenBitmapHeader.biSize = sizeof(screenBitmapHeader);
            screenBitmapHeader.biWidth = SCREEN_WIDTH;
            screenBitmapHeader.biHeight = -SCREEN_HEIGHT;
            screenBitmapHeader.biPlanes = 1;
            screenBitmapHeader.biBitCount = 32;
            screenBitmapHeader.biCompression = BI_RGB;
            screenBitmapInfo.bmiHeader = screenBitmapHeader;

            /* The bitmap has a maximum of 2^32 colors. If the biCompression member is BI_RGB, the bmiColors member is NULL.
             * https://learn.microsoft.com/en-us/windows/win32/wmdm/-bitmapinfoheader
             * 
             * screenBitmapInfo.bmiColors[0].rgbRed = 0xFF;
             * screenBitmapInfo.bmiColors[1].rgbGreen = 0xFF;
             * screenBitmapInfo.bmiColors[2].rgbBlue = 0xFF;
             */

            uint32_t *screenCaptureBits;
            HBITMAP screenCaptureBitmap = CreateDIBSection(screenCompatibleDeviceContext, &screenBitmapInfo, DIB_RGB_COLORS, (void **) &screenCaptureBits, NULL, 0);
            {
                HBITMAP replacedBitmap = SelectObject(screenCompatibleDeviceContext, screenCaptureBitmap);
                {
                    BOOL bitBlockTransferSucceeded = BitBlt(screenCompatibleDeviceContext, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, screenDeviceContext, 0, 0, SRCCOPY);

                    printf("Red: %x\n", (screenCaptureBits[0] & 0x00FF0000) >> (8 * 2));
                    printf("Green: %x\n", (screenCaptureBits[0] & 0x0000FF00) >> (8 * 1));
                    printf("Blue: %x\n", (screenCaptureBits[0] & 0x000000FF) >> (8 * 0));
                }
                /* TODO: Review the Remarks section in the page below to see if the replaced bitmap
                 * should be selected back into the screen compatible device context.
                 * https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-selectobject
                 */
                SelectObject(screenCompatibleDeviceContext, replacedBitmap);
            }
            DeleteObject(screenCaptureBitmap);
        }
        DeleteDC(screenCompatibleDeviceContext);
    }
    ReleaseDC(SCREEN_HANDLE, screenDeviceContext);

    return 0;
}
