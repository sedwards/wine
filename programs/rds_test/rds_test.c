#include <windows.h>
#include <stdio.h>
#include <wingdi.h>

int main(void)
{
    HDC hdc;
    BOOL result;
    
    printf("=== RDS Test Program ===\n");
    printf("Testing Wine RDS driver communication...\n\n");
    
    // Test 1: Create a display DC (should load winerds.drv)
    printf("Test 1: Creating DISPLAY DC...\n");
    hdc = CreateDCA("DISPLAY", NULL, NULL, NULL);
    if (hdc) {
        printf("SUCCESS: CreateDCA returned HDC: %p\n", hdc);
        
        // Test 2: Basic drawing operations
        printf("\nTest 2: Drawing operations...\n");
        
        printf("Drawing line from (10,10) to (100,100)...\n");
        MoveToEx(hdc, 10, 10, NULL);
        result = LineTo(hdc, 100, 100);
        printf("LineTo result: %s\n", result ? "SUCCESS" : "FAILED");
        
        printf("Drawing rectangle (50,50,150,150)...\n");
        result = Rectangle(hdc, 50, 50, 150, 150);
        printf("Rectangle result: %s\n", result ? "SUCCESS" : "FAILED");
        
        printf("Drawing text at (20,200)...\n");
        result = TextOutW(hdc, 20, 200, L"Hello RDS World!", 16);
        printf("TextOut result: %s\n", result ? "SUCCESS" : "FAILED");
        
        // Test 3: Device capabilities
        printf("\nTest 3: Device capabilities...\n");
        int width = GetDeviceCaps(hdc, HORZRES);
        int height = GetDeviceCaps(hdc, VERTRES);
        int bpp = GetDeviceCaps(hdc, BITSPIXEL);
        printf("Screen resolution: %dx%d @ %d bpp\n", width, height, bpp);
        
        // Clean up
        DeleteDC(hdc);
        printf("\nDisplay DC deleted.\n");
    } else {
        printf("FAILED: CreateDCA failed with error %lu\n", GetLastError());
        return 1;
    }
    
    printf("\n=== Test Complete ===\n");
    printf("If termsrv.exe is running, check its output for RDS messages.\n");
    
    return 0;
}