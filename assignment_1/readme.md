# Assignment 1: Multi-threaded Producer-Consumer

This project demonstrates a thread-safe implementation of the Producer-Consumer problem in C. It uses a bounded stack as a buffer and leverages POSIX threads and condition variables (`pthread_cond_t`) to synchronize access between threads.

## Overview

- **Producer:** Generates 10,000 random numbers, pushes them to a stack buffer, and logs all numbers to `all.txt`.
- **Consumers:** There are two consumer threads, one for even numbers and one for odd numbers. Each consumer pops numbers from the stack and writes them to `even.txt` or `odd.txt` depending on parity.

## How to Compile and Run

Use the following commands to compile and execute the program:

```bash
gcc -O2 -o main main.c -lpthread
time ./main