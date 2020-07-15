# gbReact.py
# Cameron Smith
# April 24 2020

# this script sends the Gameboy a value from 0-7 after a random interval
# it then grabs the reaction time from the Gameboy and displays it

import Adafruit_BBIO.GPIO as GPIO
from time import sleep, time
import random

serialInPin="P9_11"
clockPin="P9_12"
serialOutPin="P9_15"
GPIO.setup(serialInPin,GPIO.IN)
GPIO.setup(clockPin, GPIO.OUT)
GPIO.setup(serialOutPin, GPIO.OUT)

GPIO.output(clockPin, GPIO.HIGH)

#grab what is currently on the serialInPin at this moment in time
def receiveBit():
    recBit = GPIO.input(serialInPin)
    return recBit

# grab a value from the gameboy
# if serialInPin is high for more than .4 seconds => 0
# else it's a 1
def receiveBitValue():
    bitToReturn = -1;
    finished = 0;
    timeOutCount = time()
    # keep checking the serialInPin for an input
    # once it goes high, count long it stays high for
    # depending on time elapsed, it will be a 1 or 0
    # if nothing comes in for 5 seconds, time out
    while finished == 0:
        bitReceived = receiveBit();
        # once we got the bit, record how long it stays on the line
        # this will determine whether it's a 1 or 0
        if bitReceived == 1:
            # start the timer, stop the timer once a 0 comes on the line
            start = time()
            while bitReceived == 1:
                bitReceived = receiveBit()
            timeElapsed = time() - start
            #print "Time elapsed is: ", timeElapsed, "\n"
            # check to make sure there are no weird errors
            if timeElapsed > 1 or timeElapsed < .2:
                # if anything weird/unexpected comes down the line
                # label it 3, 3's will be ignored later
                bitToReturn = 3
            elif timeElapsed > .4:
                bitToReturn = 0
            else:
                bitToReturn = 1
            finished = 1
        currTime = time() - timeOutCount
        if currTime > 5:
            #print "time out\n"
            return bitToReturn

    return bitToReturn;


def transferBit(bit):
    if bit == 1:
        GPIO.output(serialOutPin, GPIO.HIGH)
    else:
        GPIO.output(serialOutPin, GPIO.LOW)
    GPIO.output(clockPin, GPIO.LOW)
    sleep(.000061)
    GPIO.output(clockPin, GPIO.HIGH)
    sleep(.000061)


def transferByte(byte):
    sleep(.2)
    binByte = format(byte, '#010b')
    strByte = str(binByte)
    
    transferBit(int(strByte[2]))
    transferBit(int(strByte[3]))
    transferBit(int(strByte[4]))
    transferBit(int(strByte[5]))
    transferBit(int(strByte[6]))
    transferBit(int(strByte[7]))
    transferBit(int(strByte[8]))
    transferBit(int(strByte[9]))
    sleep(.0002)


def gameStart():
    # wait a random amount of time and then send instruction
    sleep(random.randrange(15))
    toSend = random.randrange(7)
    #print "Sending: ", toSend
    transferByte(toSend)
    finished = 0;
    # wait until the gameboy sends a 1 back to send next instruction
    while finished!=1:
        finished = waitForFinish()

# grab multiple bits from the gameboy
def receiveMultipleBits(bitHolder):
    # keep appending values until the gameboy stops sending
    while bitHolder[-1] != -1:
        bitHolder.append(receiveBitValue())

# wait for the gameboy to finish the instruction
def waitForFinish():
    finished=0
    #keep checking if the gameboy finished the instruction
    while finished == 0:
        finished = receiveBit()
    return 1


def binaryToDecimal(binaryNum):
    length = len(binaryNum)
    i = length-2
    result = 0;
    powersOfTwo = 1
    while binaryNum[i] != 2:
        if binaryNum[i] != 3:
            result = result + (binaryNum[i] * powersOfTwo)
            powersOfTwo = powersOfTwo * 2
        i = i-1
    return result

def main():
    # make sure nothing is on the serial line
    receiveBit()
    #initiate game to start
    transferByte(100)

    #wait until the game to finish
    gameStart()

    #game has finished, grab the reaction time
    receivedBit = [2]
    receiveMultipleBits(receivedBit)
    #print "receivedBit is ", receivedBit

    #convert the reaction time to decimal
    reactTime = binaryToDecimal(receivedBit)
    #print "Reaction Time is: ", reactTime
   
    # convert reaction time to seconds
    # there are 256 interrupts a second for the gameboy
    seconds = float(reactTime / 256.0)

    print "Reaction time is: ", seconds, "seconds"

    GPIO.cleanup()
if __name__ == '__main__':
    main()
