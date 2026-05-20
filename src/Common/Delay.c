#include "includes.h"

void DelayUs(U16 n)// 1US
{
    volatile U32 t = n * 8;
    while(t--){
        __asm__("nop");
    }
}
#include "KD32f328_iwdg.h"

void DelayMs(U16 n)
{
   U16 i=0;
   for (i=0;i<n;i++)
   {
       DelayUs( 1000 );
       IWDG_ReloadCounter(); // NASA Standard: Feed watchdog during intentional slow delays
   }
}
