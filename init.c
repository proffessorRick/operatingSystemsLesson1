/* ***************************************************
   ***                                             ***
   ***   EOS Init (c) 2020-2021 by Elmer Hoeksema  ***
   ***                                             ***
   ***************************************************
  
   Copyright (c) 2020-2021, Elmer Hoeksema
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
       * Redistributions of source code must retain the above copyright
         notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
 
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "eos.h"

#define EOS_PI3ID                           0x0D03
#define EOS_PI4ID                           0x0D08
#define EOS_PI3BASE                         0x3F000000
#define EOS_PI4BASE                         0xFE000000

#define EOS_GPIO_BASE                       eosBase + 0x00200000
#define EOS_GPFSEL0                         (EOS_GPIO_BASE + 0x00) // 0x00 is for register 0
#define EOS_GPFSEL1                         (EOS_GPIO_BASE + 0x04) // 0x04 is for register 1
#define EOS_GPSET0                          (EOS_GPIO_BASE + 0x1C)
#define EOS_GPCLR0                          (EOS_GPIO_BASE + 0x28)

#define EOS_UART0_BASE                      eosBase + 0x00201000
#define EOS_UART0_DR                        (EOS_UART0_BASE + 0x00)
#define EOS_UART0_FR                        (EOS_UART0_BASE + 0x18)

#define EOS_ASCII_ESC                       0x1B
#define EOS_TERM_CLS                        "[2J"
#define EOS_TERM_HOME                       "[H"

UI64 eosBase;

void stage2(UI32 piId, UI32 exceptionLevel, UI64 sp);
void hang();
void blink();
void putcs(char c);
char getcs();
void clearsScreen();
void prints(char* str);
void printsln(char* str);

/* entry point
   DO NOT USE ANY VARIABLES OR FUNCTION CALLS IN THIS FUNCTION!!!
   If you do, gcc will add a stack adjustment at the top of the function,
   before the stack is initialized. This will result in a hang at boot!!!
*/
void start() {
  asm volatile (
    " mov  x0, #0x00003000     \n " // set stack pointer
    " mov  sp, x0              \n "
    " mrs  x0, midr_el1        \n " // get pi version id
    " and  x0, x0, #0x0000FFF0 \n " // mask out other bits
    " lsr  x0, x0, #4          \n " // shift bits to end
    " mrs  x1, CurrentEL       \n " // get current exception level
    " and  x1, x1, #0x0000000C \n " // mask out other bits
    " lsr  x1, x1, #2          \n " // shift bits to end
    " mov  x2, sp              \n "
    " bl   stage2              \n " // go to stage 2
  );

  // Execution should never reach this line, but just to be sure
  for(;;) asm volatile ( " wfe \n "); // hang
}

void stage2(UI32 piId, UI32 exceptionLevel, UI64 sp) {
  // if not pi 3 or pi4 -> hang
  if((piId != EOS_PI3ID) && (piId != EOS_PI4ID)) hang();

  if(piId == EOS_PI3ID) {
    eosBase = EOS_PI3BASE;
  } else {
    eosBase = EOS_PI4BASE;
  }

  clearsScreen();
  printsln("EOS 1.2 (c) 2020-2021 by Elmer Hoeksema");

  blink();

  hang();
}

void hang() {
  for(;;) asm volatile ( " wfe \n "); // hang
}

void blink() {
  put32(EOS_GPFSEL0, (get32(EOS_GPFSEL0) & 0xFFFFFE3F) | 0x00000040); // GPIO 2
  put32(EOS_GPFSEL0, (get32(EOS_GPFSEL0) & 0xFFFFF1FF) | 0x00000200); // GPIO 3
  put32(EOS_GPFSEL1, (get32(EOS_GPFSEL1) & 0xFFE3FFFF) | 0x00040000); // GPIO 16
  for(;;) {
    // In this case: GPIO 2 is for green, 3 for orange and 16 for red //
    // Phase 1: turn on green and turn off red and orange //
    put32(EOS_GPSET0, 0x00010008);
    put32(EOS_GPCLR0, 0x00000004);

    for(UI32 i = 0; i < 5000000; i++); // This is a delay //

    // Phase 2: turn on orange and turn off green //
    put32(EOS_GPSET0, 0x00010004);
    put32(EOS_GPCLR0, 0x00000008);
    
    for(UI32 i = 0; i < 5000000; i++); // This is a delay //

    // Phase 3: turn on red and turn off orange //
    put32(EOS_GPSET0, 0x0000000C);
    put32(EOS_GPCLR0, 0x00010000);

    for(UI32 i = 0; i < 5000000; i++); // This is a delay //
  }
}

void putcs(char c) {
  while(get32(EOS_UART0_FR) & BIT(5));
  put32(EOS_UART0_DR, c);
}

char getcs() {
  while(get32(EOS_UART0_FR) & BIT(4));
  UI32 data = get32(EOS_UART0_DR);

  return data;
}

void clearsScreen() {
  // Use VT100 commands to clear screen and move cursor to position 0, 0
  putcs((char)EOS_ASCII_ESC);
  prints(EOS_TERM_CLS);
  putcs((char)EOS_ASCII_ESC);
  prints(EOS_TERM_HOME);
}

void prints(char* str) {
  for (UI32 i = (UI32)0; str[i] != '\0'; i ++) {
    putcs((char)str[i]);
  }
}

void printsln(char* str) {
  prints(str);
  putcs('\r');
  putcs('\n');
}

