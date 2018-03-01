# file-transmitter

John Creps
Nick Khoury
Prof. Jack Tan
CSCI 4211 - Intro to Networks
Sliding Window Programming Project
20 July 2015
(c) 2015

README FILE

Use the make file to compile.

Developed and tested on Ubuntu

This is a C++ program implementats a simple file transfer protocol (FTP) 
that uses TCP sockets. It simulates the Go-Back-N (GBN) sliding window 
protocol.

In order to simulate a real world environment, it has the option to force
packets to be out of sequence, lost, damaged, and/or simulate lost
acknowledgement packets. Situational errors can be randomly generated 
or user-specified.

The program uses MD5 has utility to ensure the integrity of files it sends.

The user is promped for and able to choose packet size, timeout interval, 
sliding window size, range of sequence numbers, and situational errors.
