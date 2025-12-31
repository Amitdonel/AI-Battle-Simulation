# 🧠 AI Battle Simulation – Final Project

A tactical AI-driven battle simulation written in **C++** using **OpenGL (GLUT)**.  
This project demonstrates decision-making, coordination, and combat between two autonomous teams controlled entirely by AI agents.

---

## 🎯 Project Overview

The simulation models a battlefield where two opposing teams compete until one is eliminated.

Each team is composed of multiple **agent types**, each with a distinct role and behavior.  
Agents cooperate using **pathfinding**, **danger evaluation**, **line-of-sight**, and **command hierarchy** to achieve tactical objectives.

The entire system is rendered in real-time using OpenGL.

---

## 🧩 Core AI Concepts Implemented

✅ **Finite State Machine (FSM)** per agent  
✅ **A\*** pathfinding with optional danger-aware cost  
✅ **Commander-based coordination system**  
✅ **Dynamic danger map** based on enemy visibility  
✅ **Line of Sight (Bresenham algorithm)**  
✅ **Autonomous combat behavior**  
✅ **Resource management (ammo, grenades, healing)**  

---

## 🧑‍🤝‍🧑 Agent Types

### 🧠 Commander
- High-level decision maker
- Issues orders such as **ATTACK**, **MOVE**, **HEAL**, **RESUPPLY**
- Aggregates visibility information from all teammates
- Adapts strategy dynamically as the battle evolves

### ⚔️ Warrior
- Front-line combat unit
- Uses bullets and grenades
- Attacks enemies based on proximity and line-of-sight
- Requests resupply when out of ammunition

### 🩺 Medic
- Locates fallen teammates
- Navigates safely to revive them
- Returns to base after completing medical tasks

### 📦 Provider
- Supplies warriors with ammo and grenades
- Uses safe routing when possible
- Returns to home position after resupply

---

## 🗺️ Environment & World Logic

- Grid-based map
- Terrain types:
  - 🌿 Empty (walkable)
  - 🌲 Trees (walkable, blocks vision)
  - 🪨 Rocks (blocked)
  - 🌊 Water (blocked)
  - 🟨 Supply stations (ammo / medical)
- Procedural placement of obstacles and resources
- Safe spawn zones for each team

---

## ⚠️ Safety & Danger System

Each team maintains a **Safety Map**:
- Computes danger values per cell
- Based on:
  - Enemy proximity
  - Enemy visibility
- Used by:
  - A* pathfinding (risk-aware routing)
  - Commander decision making

This creates more realistic and tactical movement behavior.

---

## 🎮 Rendering & Visualization

- 2D real-time rendering with **OpenGL (GLUT)**
- Visual indicators for:
  - Terrain
  - Agents
  - Combat actions
- End-game banner announcing the winning team 🏆

---

## 🛠️ Technologies Used

- **C++**
- **OpenGL / GLUT**
- Object-Oriented Programming
- AI Algorithms:
  - A*
  - FSM
  - Heuristics
- Data Structures & STL

---

## ▶️ How to Run

### Requirements
- C++ compiler
- OpenGL + GLUT (or freeglut)
- Windows / Linux environment

### Example (Linux):
```bash
g++ *.cpp -lglut -lGL -lGLU -o battle
./battle
