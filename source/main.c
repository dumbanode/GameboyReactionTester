// Gameboy Reaction Tester
// main.c
// Cameron Smith
// April 24 2020

// This is a reaction tester for the Nintendo Gameboy
// Using the Beaglebone Black, it will send a random value
// between 0-7. The Gameboy will interpret that value as an instruction
// for the player to complete (Pressing a button). 
// After the player presses the button, the Gameboy will send the 
// reaction time to the Beaglebone.
// This is implemented using a modified version of RIOS to run on Gameboy

#include <gb/gb.h> //gameboy library found online
#include <gb/font.h>
#include <stdio.h>
#include "../graphics/buttonGraphics.c" // graphics
#include "../include/gbTimer.h" // functions to set gb timer attributes

typedef struct task {
   unsigned int period;      // Rate at which the task should tick
   unsigned int elapsedTime; // Time since task's last tick
   void (*TickFct)(void);     // Function to call for task's tick

   unsigned long running;
   unsigned int state;
} task;

task tasks[3];

const unsigned char tasksNum = 1;
const unsigned int periodT1   = 510;
unsigned int gcdEvents = 510;

unsigned long interruptLimit = 16;
unsigned long currInterrupts = 0;

unsigned int timePassed = 0; //keeps track of the time to make a reaction

unsigned char runningTasks[3] = {255}; //Track running tasks-[0] always idleTaskconst 
unsigned long idleTask = 255;       // 0 highest priority, 255 lowest 
unsigned long currentTask = 0;     // Index of highest priority task in runningTasks

unsigned int received = 0; //houses received values from the serial line
int buttonPressed = 0; //indicates whether the player has pressed a button

//function prototypes
void setInterruptLimit(int limit);
void TimerISR(void);
int TickFct_T1(int state);
void setGraphics();
void sendBinary(int n);
int recByte();
int printCommand(int command);
int interpretCommand(int command);
void sendBit(int bit);
void sendHighBit();
void sendLowBit();
void testReceiving();
void testSending();
void printMenu();
void bootReactionTester();

void setInterruptLimit(int limit){
    interruptLimit = limit;
}

void TimerISR(void) {
    unsigned char i;    
    
    disable_interrupts(); // Critical section
    timePassed++;
    currInterrupts++;
    enable_interrupts(); // End critical section 

    //has there been enough interrupts to envoke the ISR?
    if (currInterrupts == interruptLimit){

        // the library gb.h takes care of saving and restoring registers
        // when ISR is called
        // http://gbdk.sourceforge.net/doc/gbdk-doc.pdf page 14
        disable_interrupts(); // Critical section
        currInterrupts = 0;
        enable_interrupts(); // End critical section 
        
        for (i=0; i < tasksNum; ++i) { 
            // Heart of scheduler code       
            if (  ((unsigned int)tasks[i].elapsedTime >= (unsigned int)tasks[i].period)  // Task ready           
                && (runningTasks[currentTask] > i)    // Task priority > current task priority         
                && (!tasks[i].running)         // Task not already running (no self-preemption)         
            ) { 
                
                disable_interrupts(); // Critical section 
                tasks[i].elapsedTime = 0;  // Reset time since last tick          
                tasks[i].running = 1;          // Mark as running          
                currentTask++;          
                runningTasks[currentTask] = i;      // Add to runningTasks
                enable_interrupts(); // End critical section    

                tasks[i].state = (int)tasks[i].TickFct(tasks[i].state);      // Execute tick    
                
                disable_interrupts();; // Critical section          
                tasks[i].running = 0;   
                runningTasks[currentTask] = idleTask; // Remove from runningTasks          
                currentTask -= 1;          
                enable_interrupts(); // End critical section                  
            }            
            // (255 ticks in until an interrupt) * (How many interrupts until ISR dispatched)
            tasks[i].elapsedTime += 255 * (unsigned int)interruptLimit;           
        }    
        
    }
    

}

// receive a full byte from external linux board
int recByte(){
    receive_byte(); // receive _io_in
    while (_io_status == IO_RECEIVING); // Wait for Receive
    if (_io_status == IO_IDLE) { // If IO status returns to Idle then success
        return (int)_io_in;
    }
    else {
        printf("Error\n"); // Else print error
        return -1;
    }
}


// send a standard bit on the line
void sendBit(int bit){
    if (bit == 0){
        _io_out = 0;
        send_byte();
    } else {
        _io_out = 255;
        send_byte();
    }
}

// sending a 1 on the line for .2 seconds == high bit
void sendHighBit(){
    sendBit(1);
    delay(200);
    sendBit(0);
}

// sending a 1 on the line for .4 seconds == low bit
void sendLowBit(){
    sendBit(1);
    delay(400);
    sendBit(0);
}

int playGame(){
    unsigned int reactTime = 0; //keeps track of player's reaction time
    buttonPressed = 0;

    receive_byte(); // receive _io_in
    while (_io_status == IO_RECEIVING); // Wait for Receive
    if (_io_status == IO_IDLE) { // If IO status returns to Idle then success
        received = (int)_io_in;
        sendBit(0);
    }
    else {
        printf("Error\n"); // Else print error
        return -1;
    }

    printCommand(received);
    TimerOn();
    while(buttonPressed!=1){ }

    //button is pressed, record the reaction time
    reactTime = timePassed;
    waitpadup();
    printf("interrupts occured: %u\n", (unsigned int) reactTime);
    printf("Sending to Beaglebone\n");

    sendBinary(reactTime);

    sendBit(1);
    return 1;
}

// decimal to binary code used from the following website
// https://www.programmingsimplified.com/c/source-code/c-program-convert-decimal-to-binary
void sendBinary(int n) 
{ 
    int c, k;
    int firstOneFound = 0; //ignore the 0's until we find the first 1
    for (c = 31; c >= 0; c--)
    {
        k = n >> c;

        if (k & 1) {
            if (firstOneFound == 0)
                firstOneFound = 1;
            sendHighBit();
        }
        else { 
            if (firstOneFound == 1){
                sendLowBit();
            }
        }
    }
} 

void printMenu(){
    printf("\nStart-Boot\n\n");
    printf("Select-Test Rcving\n\n");
    printf("A-Test Sending\n\n");
}

void bootReactionTester(){
    unsigned char i=0;  

    //setup button graphics
    setGraphics();

    // set up the tasks
    tasks[i].period = periodT1;    
    tasks[i].elapsedTime = tasks[i].period;    
    tasks[i].TickFct = TickFct_T1;   
    ++i; 

    // set TimerISR to be the ISR when there's a TIMA overflow
    add_TIM(TimerISR);
    // choose frequency to run gameboy at
    TimerSet(2); 
    // choose how many interrupts until ISR is serviced
    setInterruptLimit(gcdEvents/255);

    // wait until the gameboy connects to the beaglebone
    printf("Waiting for game to start... ");
    received = recByte();

    // beaglebone will send either 100 or 50 to indicate it's ready
    if (received==100 || received ==50){

        // Beaglebone has connected, start the game
        printf("Game start\n");
        received = playGame();

        // clear serial out so Beaglebone doesn't pick up any unwanted values      
        _io_out = 0;
        send_byte();
    }
    else{
        printf("Error, unable to connect");
    }

}

int main(void) {

    //prompt the user what to do
    printf("Welcome\n\n\n");
    printMenu();

    while (1){
        // boot up the reaction tester as normal
        if (joypad() == J_START) {
            waitpadup();
            printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
            bootReactionTester();

            //currently unable to get this to run twice in a row
            //so for the time being, just exit
            return 1;
        }
        // test receiving functions
        else if (joypad() == J_SELECT){
            testReceiving();
            printMenu();
        } 
        // test sending functions 
        else if (joypad() == J_A) {
            testSending();
            printMenu();
        }
    }
}

int TickFct_T1(int state){
    // state transitions
    if (state == 0){
        state = 1;
    } else if (state == 1) {
        state = 1;
    }

    // state actions
    if (state == 1) {
        // check to see if button has been pressed
        buttonPressed = interpretCommand(received);
    }
    
    return state;
    
}

// ---test functions---

// test the receiving capabilities
// if the gameboy receives 0 to 10 successfully, it has passed
void testReceiving(){
    int i = 0;
    int received;
    while (i < 11){
        printf("Receiving... ");
        received = recByte();
        if (received == i) {
            printf("Passed\n");
            sendHighBit();
        } else {
            printf("FAILED\n");
            sendLowBit();
        }
        i++;
    }
}

// test the sending capablities
// send the beaglebone 0 to 10, will send back a value if it was successful.
void testSending(){
    int i = 0;
    int received;
    while (i < 11){
        printf("Sending... ");
        sendBinary(i);
        received = recByte();
        if (received == 1) {
            printf("Passed\n");
        } else {
            printf("FAILED\n");
        }

        i++;
    }
}

// ---graphics functions---

// when a command comes in, pass it into printCommand
// to tell the user which button to press (setting appropriate sprites)
int printCommand(int command){
    if (command == 0){
        //press up
        set_sprite_tile(0, 0x1E);
        set_sprite_tile(1, 0x1F);
        set_sprite_tile(2, 0x20);
        set_sprite_tile(3, 0x21);
    } else if (command == 1){
        //press down
        set_sprite_tile(4, 0x26);
        set_sprite_tile(5, 0x27);
        set_sprite_tile(6, 0x28);
        set_sprite_tile(7, 0x29);
    } else if (command == 2){
        //press left
        set_sprite_tile(8, 0x2E);
        set_sprite_tile(9, 0x2F);
        set_sprite_tile(10, 0x30);
        set_sprite_tile(11, 0x31);
    } else if (command == 3){
        //press right
        set_sprite_tile(12, 0x36);
        set_sprite_tile(13, 0x37);
        set_sprite_tile(14, 0x38);
        set_sprite_tile(15, 0x39);
    } else if (command == 4){
        //press A
        set_sprite_tile(16, 0x3E);
        set_sprite_tile(17, 0x3F);
        set_sprite_tile(18, 0x40);
        set_sprite_tile(19, 0x41);
    } else if (command == 5){
        //press B
        set_sprite_tile(20, 0x46);
        set_sprite_tile(21, 0x47);
        set_sprite_tile(22, 0x48);
        set_sprite_tile(23, 0x49);
    } else if (command == 6){
        //press start
        set_sprite_tile(24, 0x4E);
        set_sprite_tile(25, 0x4F);
        set_sprite_tile(26, 0x50);
        set_sprite_tile(27, 0x51);
    } else if (command == 7){
        //press select
        set_sprite_tile(28, 0x4E);
        set_sprite_tile(29, 0x4F);
        set_sprite_tile(30, 0x50);
        set_sprite_tile(31, 0x51);
    }
}

// checks to see if the user has pressed the correct button
int interpretCommand(int command){
    if (command == 0){
        if (joypad() == J_UP) {
            //waitpadup();
            set_sprite_tile(0, 0x1A);
            set_sprite_tile(1, 0x1B);
            set_sprite_tile(2, 0x1C);
            set_sprite_tile(3, 0x1D);
            return 1;
        }
    } else if (command == 1){
        if (joypad() == J_DOWN) {
            //waitpadup();
            set_sprite_tile(4, 0x22);
            set_sprite_tile(5, 0x23);
            set_sprite_tile(6, 0x24);
            set_sprite_tile(7, 0x25);
            return 1;
        }
    } else if (command == 2){
        if (joypad() == J_LEFT) {
            //waitpadup();
            set_sprite_tile(8, 0x2A);
            set_sprite_tile(9, 0x2B);
            set_sprite_tile(10, 0x2C);
            set_sprite_tile(11, 0x2D);
            return 1;
        }
    } else if (command == 3){
        if (joypad() == J_RIGHT) {
            //waitpadup();
            set_sprite_tile(12, 0x32);
            set_sprite_tile(13, 0x33);
            set_sprite_tile(14, 0x34);
            set_sprite_tile(15, 0x35);
            return 1;
        }
    } else if (command == 4){
        if (joypad() == J_A) {
            //waitpadup();
            set_sprite_tile(16, 0x3A);
            set_sprite_tile(17, 0x3B);
            set_sprite_tile(18, 0x3C);
            set_sprite_tile(19, 0x3D);
            return 1;
        }
    } else if (command == 5){
        if (joypad() == J_B) {
            //waitpadup();
            set_sprite_tile(20, 0x42);
            set_sprite_tile(21, 0x43);
            set_sprite_tile(22, 0x44);
            set_sprite_tile(23, 0x45);
            return 1;
        }
    } else if (command == 6){
        if (joypad() == J_START) {
            //waitpadup();
            set_sprite_tile(24, 0x4A);
            set_sprite_tile(25, 0x4B);
            set_sprite_tile(26, 0x4C);
            set_sprite_tile(27, 0x4D);
            return 1;
        }
    } else if (command == 7){
        if (joypad() == J_SELECT) {
            //waitpadup();
            set_sprite_tile(28, 0x4A);
            set_sprite_tile(29, 0x4B);
            set_sprite_tile(30, 0x4C);
            set_sprite_tile(31, 0x4D);
            return 1;
        }
    } else {
        return 0;
    }
}




void setGraphics(){
    set_sprite_data(0x1A,56,buttonGraphics);
    //set the up button graphic
    set_sprite_tile(0, 0x1A);
    set_sprite_tile(1, 0x1B);
    set_sprite_tile(2, 0x1C);
    set_sprite_tile(3, 0x1D);
    move_sprite(0, 36, 54);
    move_sprite(1, 36, 62);
    move_sprite(2, 44, 54);
    move_sprite(3, 44, 62);

    //set the down button graphic
    set_sprite_tile(4, 0x22);
    set_sprite_tile(5, 0x23);
    set_sprite_tile(6, 0x24);
    set_sprite_tile(7, 0x25);
    move_sprite(4, 36, 84);
    move_sprite(5, 36, 92);
    move_sprite(6, 44, 84);
    move_sprite(7, 44, 92);

    //set the left button graphic
    set_sprite_tile(8, 0x2A);
    set_sprite_tile(9, 0x2B);
    set_sprite_tile(10, 0x2C);
    set_sprite_tile(11, 0x2D);
    move_sprite(8, 21, 69);
    move_sprite(9, 21, 77);
    move_sprite(10, 29, 69);
    move_sprite(11, 29, 77);

    //set the right button graphic
    set_sprite_tile(12, 0x32);
    set_sprite_tile(13, 0x33);
    set_sprite_tile(14, 0x34);
    set_sprite_tile(15, 0x35);
    move_sprite(12, 51, 69);
    move_sprite(13, 51, 77);
    move_sprite(14, 59, 69);
    move_sprite(15, 59, 77);

    //set the A button graphic
    set_sprite_tile(16, 0x3A);
    set_sprite_tile(17, 0x3B);
    set_sprite_tile(18, 0x3C);
    set_sprite_tile(19, 0x3D);
    move_sprite(16, 131, 69);
    move_sprite(17, 131, 77);
    move_sprite(18, 139, 69);
    move_sprite(19, 139, 77);

    //set the B button graphic
    set_sprite_tile(20, 0x42);
    set_sprite_tile(21, 0x43);
    set_sprite_tile(22, 0x44);
    set_sprite_tile(23, 0x45);
    move_sprite(20, 101, 69);
    move_sprite(21, 101, 77);
    move_sprite(22, 109, 69);
    move_sprite(23, 109, 77);

    //set the start button graphic
    set_sprite_tile(24, 0x4A);
    set_sprite_tile(25, 0x4B);
    set_sprite_tile(26, 0x4C);
    set_sprite_tile(27, 0x4D);
    move_sprite(24, 91, 104);
    move_sprite(25, 91, 111);
    move_sprite(26, 99, 104);
    move_sprite(27, 99, 111);

    //set the select button graphic
    set_sprite_tile(28, 0x4A);
    set_sprite_tile(29, 0x4B);
    set_sprite_tile(30, 0x4C);
    set_sprite_tile(31, 0x4D);
    move_sprite(28, 66, 104);
    move_sprite(29, 66, 111);
    move_sprite(30, 74, 104);
    move_sprite(31, 74, 111);
    
   SHOW_SPRITES;
}
