# FSM Cache Controller Simulation (C++)

## Overview

This project simulates a simple cache controller using a **Finite State Machine (FSM)** based on Computer Organization and Architecture concepts.

It demonstrates how cache interacts with CPU and main memory using cycle-by-cycle simulation.

## FSM States

The controller follows these states:

* **Idle** → Wait for CPU request
* **CompareTag** → Check cache hit/miss
* **Allocate** → Load block from memory
* **WriteBack** → Write dirty block to memory

## Features

* Direct-mapped cache
* Write-back policy
* Write-allocate strategy
* Memory with configurable latency
* Cycle-by-cycle state transition tracing

## How to Compile

Open PowerShell in your project folder and run:

```bash
g++ -std=c++11 -O2 -o fsm_cache.exe main.cpp


## How to Run

fsm_cache.exe

## Output Description

Each cycle prints:

 FSM state
 CPU signals (`ready`)
 Memory signals (`valid`, `write`)
 Action taken in that cycle

##  Example Behavior

  Cache hit → data returned immediately
  Cache miss → block fetched from memory
  Dirty block → written back before allocation

## Customization

You can modify the simulation:

 Change request sequence in main()
 Adjust memory latency
 Change cache size(index bits)

## Concept Mapping (Textbook)

| FSM State  | Description                 |
| ---------- | --------------------------- |
| Idle       | Waiting for CPU request     |
| CompareTag | Check hit or miss           |
| Allocate   | Fetch block from memory     |
| WriteBack  | Write dirty block to memory |


## Project Structure

D:\COA_Assignment\
│
├── main.cpp
├── fsm_cache.exe
├── README.md

