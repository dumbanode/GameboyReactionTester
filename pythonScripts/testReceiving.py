# testReceiving.py
# Cameron Smith
# April 24 2020

# this file tests the ability for the beaglebone to receive
# and understand values sent from the Gameboy

import unittest
import gbReact

class testFunction(unittest.TestCase):
    def setUp(self):
        self.func = testReceive()
    def test_1(self):
        self.assertEqual(self.func, True)


def testReceive():
    testCases = 10
    i = 0
    passed = 0
    while (i<=testCases):
        receivedBit = [2]
        gbReact.receiveMultipleBits(receivedBit)
        receivedValue = gbReact.binaryToDecimal(receivedBit)
        if receivedValue == i:
            gbReact.transferByte(1)
            passed = passed+1
        else:
            gbReact.transferByte(0)
        i = i+1
    return passed-1 == testCases

if __name__ == '__main__':
    unittest.main()
