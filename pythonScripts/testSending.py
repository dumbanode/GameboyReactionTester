# testSending.py
# Cameron Smith
# April 24 2020

# This file tests out the ability to send values
# over to the Gameboy, test passes if Gameboy is able to
# determine the values from 0 to the number of testCases

import unittest
import gbReact

class testFunction(unittest.TestCase):
    def setUp(self):
        self.func = testTransfer()
    def test_1(self):
        self.assertEqual(self.func, True)


def testTransfer():
    testCases = 10
    i = 0
    passed = 0
    while (i<=testCases):
        gbReact.transferByte(i)
        finished = -1
        while finished == -1:
            finished = gbReact.receiveBitValue()
        passed = passed + finished
        i = i+1
    return passed-1 == testCases

if __name__ == '__main__':
    unittest.main()
