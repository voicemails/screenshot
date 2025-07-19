#include <windows.h>
#include <windowsx.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define SCREEN_HANDLE NULL
#define BITS_PER_BYTE 8
#define BMP_FILE_SIGNATURE 0x4D42
#define MAX_FORMATTED_DATE_LENGTH 100
#define DOT_BMP_LENGTH 4
#define BRIGHTNESS_FACTOR 0.5

#define S_KEY 0x53

#define MIN(l, r) ((l) < (r) ? (l) : (r))
#define MAX(l, r) ((l) < (r) ? (r) : (l))

int SCREEN_WIDTH;
int SCREEN_HEIGHT;
int SCREEN_AREA;

HDC screenCompatibleDeviceContext;
uint32_t *screenCaptureBits;
POINT mouseDown = { .x = -1, .y = -1 };
POINT mouseUp = { .x = -1, .y = -1 };

BOOL leftMouseDown = FALSE;

BOOL saveScreenshot = FALSE;
RECT partialScreenshot = { .left = -1, .top = -1, .right = -1, .bottom = -1 };
int screenshotWidth = -1;
int screenshotHeight = -1;
int screenshotArea = -1;

LRESULT WindowProcedure(HWND window, UINT message, WPARAM wParameter, LPARAM lParameter) {
    switch(message) {
        case WM_PAINT:
            {
                RECT clientRectangle;
                BOOL rectangleRetrieved = GetClientRect(window, &clientRectangle);

                PAINTSTRUCT paint;
                HDC windowDeviceContext = BeginPaint(window, &paint);

                uint32_t *dimmedScreenBits = (uint32_t *) malloc(sizeof(uint32_t) * SCREEN_WIDTH * SCREEN_HEIGHT);

                uint8_t *blue;
                uint8_t *green;
                uint8_t *red;

                for (int y = 0; y < SCREEN_HEIGHT; y++) {
                    for (int x = 0; x < SCREEN_WIDTH; x++) {
                        int i = y * SCREEN_WIDTH + x;
                        dimmedScreenBits[i] = screenCaptureBits[i];

                        if (x >= MIN(mouseDown.x, mouseUp.x) && x <= MAX(mouseDown.x, mouseUp.x) &&
                            y >= MIN(mouseDown.y, mouseUp.y) && y <= MAX(mouseDown.y, mouseUp.y)) {
                            continue;
                        }

                        blue = ((uint8_t *) &dimmedScreenBits[i]) + 0;
                        green = ((uint8_t *) &dimmedScreenBits[i]) + 1;
                        red = ((uint8_t *) &dimmedScreenBits[i]) + 2;

                        *blue = *blue * BRIGHTNESS_FACTOR;
                        *green = *green * BRIGHTNESS_FACTOR;
                        *red = *red * BRIGHTNESS_FACTOR;
                    }
                }

                HBITMAP dimmedScreenBitmap = CreateBitmap(SCREEN_WIDTH, SCREEN_HEIGHT, 1, 32, dimmedScreenBits);
                HBITMAP screenCaptureBitmap = SelectObject(screenCompatibleDeviceContext, dimmedScreenBitmap);

                BOOL bitBlockTransferSucceeded = BitBlt(windowDeviceContext, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, screenCompatibleDeviceContext, 0, 0, SRCCOPY);

                assert(dimmedScreenBitmap == SelectObject(screenCompatibleDeviceContext, screenCaptureBitmap));
                DeleteObject(dimmedScreenBitmap);

                free(dimmedScreenBits);
                EndPaint(window, &paint);

            } break;

        case WM_LBUTTONDOWN:
            leftMouseDown = TRUE;

            mouseDown.x = GET_X_LPARAM(lParameter);
            mouseDown.y = GET_Y_LPARAM(lParameter);
            break;

        case WM_MOUSEMOVE:
            if (leftMouseDown) {
                mouseUp.x = GET_X_LPARAM(lParameter);
                mouseUp.y = GET_Y_LPARAM(lParameter);
                InvalidateRect(window, NULL, TRUE);
                UpdateWindow(window);
            }

            break;

        case WM_LBUTTONUP:
            leftMouseDown = FALSE;

            mouseUp.x = GET_X_LPARAM(lParameter);
            mouseUp.y = GET_Y_LPARAM(lParameter);
            InvalidateRect(window, NULL, TRUE);
            UpdateWindow(window);

            partialScreenshot.left = MIN(mouseDown.x, mouseUp.x);
            partialScreenshot.right = MAX(mouseDown.x, mouseUp.x);

            partialScreenshot.top = MIN(mouseDown.y, mouseUp.y);
            partialScreenshot.bottom = MAX(mouseDown.y, mouseUp.y);

            screenshotWidth = partialScreenshot.right - partialScreenshot.left;
            screenshotHeight = partialScreenshot.bottom - partialScreenshot.top;
            screenshotArea = screenshotWidth * screenshotHeight;

            break;


        case WM_KEYDOWN:
            if (wParameter == VK_ESCAPE) {
                PostMessage(window, WM_CLOSE, (WPARAM) NULL, (LPARAM) NULL);
            }
            else if (wParameter == S_KEY) {
                PostMessage(window, WM_CLOSE, (WPARAM) NULL, (LPARAM) NULL);
                saveScreenshot = TRUE;
            }
            break;

        case WM_CLOSE:
            DestroyWindow(window);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(window, message, wParameter, lParameter);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previousInstance, LPSTR commandLine, int showCommand) {
    SCREEN_WIDTH = GetSystemMetrics(SM_CXSCREEN);
    SCREEN_HEIGHT = GetSystemMetrics(SM_CYSCREEN);
    SCREEN_AREA = SCREEN_WIDTH * SCREEN_HEIGHT;
    /* TODO: Error handling (checking return values, etc.)
     */
    HDC screenDeviceContext = GetDC(SCREEN_HANDLE);
    {
        screenCompatibleDeviceContext = CreateCompatibleDC(screenDeviceContext);
        {
            BITMAPINFO screenBitmapInfo = {0};

            BITMAPINFOHEADER screenBitmapHeader = {0};
            screenBitmapHeader.biSize = sizeof(screenBitmapHeader);
            screenBitmapHeader.biWidth = SCREEN_WIDTH;
            screenBitmapHeader.biHeight = -SCREEN_HEIGHT;
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

            HBITMAP screenCaptureBitmap = CreateDIBSection(screenCompatibleDeviceContext, &screenBitmapInfo, DIB_RGB_COLORS, (void **) &screenCaptureBits, NULL, 0);
            {
                HBITMAP replacedBitmap = SelectObject(screenCompatibleDeviceContext, screenCaptureBitmap);
                {
                    BOOL bitBlockTransferSucceeded = BitBlt(screenCompatibleDeviceContext, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, screenDeviceContext, 0, 0, SRCCOPY);

                    SYSTEMTIME time;
                    GetLocalTime(&time);
                    char screenshotName[MAX_FORMATTED_DATE_LENGTH + DOT_BMP_LENGTH];
                    /* https://stackoverflow.com/questions/153890/printing-leading-0s-in-c
                     * https://www.ibm.com/docs/en/workload-automation/10.2.2?topic=troubleshooting-date-time-format-reference-strftime
                     */

                    /* TODO: Update file name format to be Windows and Linux compatible.
                     * For this to work, Linux can run Wine.
                     */
                    snprintf(screenshotName, sizeof(screenshotName),
                             "%d-%02d-%02d_%02d;%02d;%02d.%03d.bmp",
                             time.wYear, time.wMonth, time.wDay,
                             time.wHour, time.wMinute, time.wSecond,
                             time.wMilliseconds);


                    WNDCLASS windowClass = {0};
                    windowClass.lpfnWndProc = WindowProcedure;
                    windowClass.hInstance = instance;
                    windowClass.lpszClassName = "Screenshot Class";
                    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
                    RegisterClass(&windowClass);
                    HWND fullscreenWindow = CreateWindow(windowClass.lpszClassName, "Full Screenshot", 
                                                         WS_POPUP, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                                                         NULL, NULL, windowClass.hInstance, NULL);

                    BOOL windowShown;
                    windowShown = ShowWindow(fullscreenWindow, showCommand);
                    MSG message;
                    while (GetMessage(&message, fullscreenWindow, 0, 0) > 0) {
                        TranslateMessage(&message);
                        DispatchMessage(&message);
                    }

                    if (!saveScreenshot || screenshotWidth < 1 || screenshotHeight < 1) {
                        return 0;
                    }

                    /* https://learn.microsoft.com/en-us/windows/win32/gdi/storing-an-image
                     */
                    HANDLE imageFile = CreateFile(screenshotName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                    {
                        /* Start writing the bitmap to a file in "BMP file format"
                         * Review the "File structure" section below for more information.
                         * https://en.wikipedia.org/wiki/BMP_file_format
                         */
                        BITMAPFILEHEADER bitmapFileHeader = {0};
                        bitmapFileHeader.bfType = BMP_FILE_SIGNATURE;
                        /* https://stackoverflow.com/questions/25713117/what-is-the-difference-between-bisizeimage-bisize-and-bfsize
                         */
                        bitmapFileHeader.bfSize = sizeof(bitmapFileHeader) + screenBitmapHeader.biSize + screenBitmapHeader.biSizeImage;
                        bitmapFileHeader.bfReserved1 = 0;
                        bitmapFileHeader.bfReserved2 = 0;
                        bitmapFileHeader.bfOffBits = sizeof(bitmapFileHeader) + screenBitmapHeader.biSize;

                        BOOL fileWritten;
                        DWORD bytesWritten = 0;
                        fileWritten = WriteFile(imageFile, &bitmapFileHeader, sizeof(bitmapFileHeader), &bytesWritten, NULL);
                        screenBitmapHeader.biHeight = SCREEN_HEIGHT;    /* Reassign positive height before writing bitmap info out to a file. */

                        screenBitmapHeader.biWidth = screenshotWidth;
                        screenBitmapHeader.biHeight = screenshotHeight;
                        fileWritten = WriteFile(imageFile, &screenBitmapHeader, sizeof(screenBitmapHeader), &bytesWritten, NULL);

                        DWORD imageBytesWritten = 0;
                        for (int y = partialScreenshot.bottom; y >= partialScreenshot.top; y--) {
                            fileWritten = WriteFile(imageFile, &screenCaptureBits[y * SCREEN_WIDTH + partialScreenshot.left], screenshotWidth * sizeof(screenCaptureBits[0]), &bytesWritten, NULL);
                            imageBytesWritten += bytesWritten;
                        }
                        assert(imageBytesWritten == screenBitmapHeader.biSizeImage);
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
