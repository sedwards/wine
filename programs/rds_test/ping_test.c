/* ping_test.c - Simple test to verify RDS ping/pong functionality */

#include <windows.h>
#include <stdio.h>
#include <time.h>

int main(void)
{
    HDC testDC;
    DWORD start_time = GetTickCount();
    
    printf("=== RDS PING TEST ===\n");
    printf("Test started at %lu ms\n", start_time);
    
    // First test - attempt to create a display DC which should trigger our driver
    printf("\n1. Testing CreateDC(\"DISPLAY\")...\n");
    testDC = CreateDCA("DISPLAY", NULL, NULL, NULL);
    if (testDC) {
        printf("   CreateDC succeeded: %p at %lu ms (+%lu ms)\n", 
               testDC, GetTickCount(), GetTickCount() - start_time);
        
        // Keep the DC alive for 15 seconds to allow ping/pong cycles
        printf("   Keeping DC alive for 15 seconds to observe ping/pong...\n");
        
        for (int i = 0; i < 15; i++) {
            printf("   [%02d] Alive at %lu ms (+%lu ms)\n", 
                   i+1, GetTickCount(), GetTickCount() - start_time);
            
            // Optionally do some drawing to test message sending
            if (i == 5) {
                printf("   [%02d] Drawing test line...\n", i+1);
                MoveToEx(testDC, 10, 10, NULL);
                LineTo(testDC, 100, 100);
            }
            
            if (i == 10) {
                printf("   [%02d] Drawing test rectangle...\n", i+1);
                Rectangle(testDC, 50, 50, 150, 150);
            }
            
            Sleep(1000);
        }
        
        printf("   Cleaning up DC...\n");
        DeleteDC(testDC);
        printf("   DC deleted at %lu ms (+%lu ms)\n", 
               GetTickCount(), GetTickCount() - start_time);
    } else {
        printf("   CreateDC FAILED: %lu at %lu ms (+%lu ms)\n", 
               GetLastError(), GetTickCount(), GetTickCount() - start_time);
    }
    
    printf("\n2. Testing CreateDC(\"WINESCREEN\")...\n");
    testDC = CreateDCA("WINESCREEN", NULL, NULL, NULL);
    if (testDC) {
        printf("   CreateDC succeeded: %p at %lu ms (+%lu ms)\n", 
               testDC, GetTickCount(), GetTickCount() - start_time);
        Sleep(3000);
        DeleteDC(testDC);
        printf("   DC deleted at %lu ms (+%lu ms)\n", 
               GetTickCount(), GetTickCount() - start_time);
    } else {
        printf("   CreateDC FAILED: %lu at %lu ms (+%lu ms)\n", 
               GetLastError(), GetTickCount(), GetTickCount() - start_time);
    }
    
    printf("\n=== Test completed at %lu ms (total: %lu ms) ===\n", 
           GetTickCount(), GetTickCount() - start_time);
    
    return 0;
}