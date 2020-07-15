// gbTimer.c
// Cameron Smith
// March 6 2020

// this file handles the gameboy's frequency settings, timer settings,
// and saving/restoring contexts
#include <gb/gb.h>
#include "../include/gbTimer.h"


void TimerOn(){
    set_interrupts(TIM_IFLAG | VBL_IFLAG);
    enable_interrupts();
}

//decide what frequency to run gameboy at
// 0: 4.096 KHZ
// 1: 262.133 KHz
// 2: 65.536 KHz
// 3: 16.384 KHz
void TimerSet(int freqSelect){

    // which freq do you want?
    if (freqSelect == 0){
        //set it to 4096 Hertz
        //16 interrupts per second
        TAC_REG = 0x04U;
    } else if (freqSelect == 1){
        //set it to 262144 Hertz
        // 1024 interrupts per second
        TAC_REG = 0x05U;
    } else if (freqSelect == 2){
        //set it to 65536 Hertz
        // 256 interrupts per second
        TAC_REG = 0x06U;
    } else if (freqSelect == 3){
        //set it to 16384 Hertz
        //64 interrupts per second
        TAC_REG = 0x07U;
    }
}
