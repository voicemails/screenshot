#include <windows.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define SCREEN_HANDLE NULL
#define BITS_PER_BYTE 8

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
            /* NOTE: Temporarily changed "biHeight" to +SCREEN_HEIGHT instead of -SCREEN_HEIGHT.
             * This was done so that the image would not appear flipped when writing the bitmap out to a file.
             */
            /* screenBitmapHeader.biHeight = -SCREEN_HEIGHT;
             */
            screenBitmapHeader.biHeight = SCREEN_HEIGHT;
            screenBitmapHeader.biPlanes = 1;
            screenBitmapHeader.biBitCount = 32;
            screenBitmapHeader.biCompression = BI_RGB;

            screenBitmapHeader.biSizeImage = SCREEN_AREA * sizeof(uint32_t);
            /* screenBitmapHeader.biSizeImage = 0;
             * "Specifies the size, in bytes, of the image. This can be set to 0 for uncompressed RGB bitmaps"
             * 
             * screenBitmapHeader.biClrImportant = 0;
             * "If this value is zero, all colors are important"
             *
             * https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader
             */


            screenBitmapInfo.bmiHeader = screenBitmapHeader;

            /* "The bitmap has a maximum of 2^32 colors. If the biCompression member is BI_RGB, the bmiColors member is NULL"
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

                    /* https://learn.microsoft.com/en-us/windows/win32/gdi/storing-an-image
                     */
                    HANDLE imageFile = CreateFile("screenshot.bmp", GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                    {
                        /* Start writing the bitmap to a file in "BMP file format"
                         * Review the "File structure" section below for more information.
                         * https://en.wikipedia.org/wiki/BMP_file_format
                         */
                        BITMAPFILEHEADER bitmapFileHeader = {0};
                        bitmapFileHeader.bfType = 0x4D42; /* File signature for bmp */
                        /* https://stackoverflow.com/questions/25713117/what-is-the-difference-between-bisizeimage-bisize-and-bfsize
                         */
                        bitmapFileHeader.bfSize = sizeof(bitmapFileHeader) + screenBitmapHeader.biSize + screenBitmapHeader.biSizeImage;
                        bitmapFileHeader.bfReserved1 = 0;
                        bitmapFileHeader.bfReserved2 = 0;
                        bitmapFileHeader.bfOffBits = sizeof(bitmapFileHeader) + screenBitmapHeader.biSize;


                        BOOL fileWritten;
                        DWORD bytesWritten = 0;
                        fileWritten = WriteFile(imageFile, &bitmapFileHeader, sizeof(bitmapFileHeader), &bytesWritten, NULL);
                        fileWritten = WriteFile(imageFile, &screenBitmapHeader, sizeof(screenBitmapHeader), &bytesWritten, NULL);
                        fileWritten = WriteFile(imageFile, screenCaptureBits, screenBitmapHeader.biSizeImage, &bytesWritten, NULL);
                    }
                    BOOL fileHandleClosed = CloseHandle(imageFile);
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
