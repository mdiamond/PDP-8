# PDP-8 Emulator


A program that emulates the PDP-8 computer system. Assigned by Dr. James Peterson for CS 429: Computer Organization & Architecture.

## Input format

This program takes input from a single text file with each line formatted as follows:  
`memory location:data`  
with the exception of one line formatted as follows:  
`EP:memory location`  
where memory location and data must be 12 bit hexadecimal values. It uses the memory location specified by EP as an entry point to begin computation. It first executes the instruction in the memory location specified by EP, and then executes according to the rules of the PDP-8 architecture. There are 4096 memory locations indexed from 0-4095.

## Command Line Arguments

-v: verbose mode  
At each instruction print out the state of the machine. This includes the program counter, instructions being executed, and each register.
