# NexusMatch Trading Engine  

**NexusMatch** is a high-performance, interactive order book designed to simulate the core execution logic of a modern **financial exchange**. Built with standard Data Structures and Algorithms like AVL Trees , it enforces strict **Price-Time Priority (FIFO)** and supports complex conditional execution like **Cascading Stop-Loss** triggers.

---

##  Core Architecture (The DSA)

To achieve the speed required for high-frequency trading simulations, NexusMatch utilizes a hybrid approach to data management:

### 1. AVL Tree (Price Discovery)
* **Mechanism:** Orders are sorted into self-balancing binary search trees by price. 
* **Performance:** Ensures finding the **Best Bid** (highest buyer) or **Best Ask** (lowest seller) is always $O(\log P)$, preventing performance degradation as the market grows.


### 2. Doubly Linked List (Time Priority)
* **Mechanism:** Each node in the AVL tree contains a doubly linked list.
* **Logic:** This manages the "Line" at a specific price. If two traders offer the same price, the order at the **Head** of the list is matched first, ensuring perfect $O(1)$ time priority.


### 3. Unordered Map (Hash Map)
* **Mechanism:** Provides instant $O(1)$ tracking of orders via their unique ID. This prevents duplicate active orders for a single trader and allows for future instant-cancellation features.

### 4. Min-Heap (Stop-Loss Triggers)
* **Mechanism:** A priority queue (Min-Heap) keeps "Hidden" stop-loss orders.
* **Logic:** It monitors the "most urgent" trigger price. After every trade, the engine checks the heap top in $O(1)$ time. If a trigger is hit, the order is automatically "woken up" and injected into the main tree.


---

##  Features

**1.High-Concurrency Match Logic** :
The engine utilizes a sophisticated Price-Time Priority algorithm, ensuring that trades are executed first by the most competitive price, then by the exact sequence of arrival. This prevents "front-running" and guarantees a fair, deterministic environment for all market participants.

**2.Automated Risk & Liquidation Management** :
Beyond simple limit orders, the system integrates a low-latency Conditional Trigger Engine. By utilizing a specialized Min-Heap, it monitors hidden stop-loss orders in real-time, allowing for automated capital protection and the simulation of complex market chain reactions.

**3.Real-Time Market Depth Visualization** : 
The system features a professional-grade  Order Book Dashboard, providing an instantaneous view of the market "Spread." It dynamically renders the gap between current Bids and Asks, allowing for a clear analysis of market liquidity and potential price slippage before trades are executed.


---

##  Command Guide

| Command | Syntax | Description |
| :--- | :--- | :--- |
| **BUY** | `BUY [ID] [QTY] [PRICE]` | Places a limit buy order in the book. |
| **SELL** | `SELL [ID] [QTY] [PRICE]` | Places a limit sell order in the book. |
| **STOP** | `STOP [ID] [TRG] [PRC] [QTY]` | Sets a stop-loss: triggers at `TRG`, sells at `PRC`. |
| **BOARD** | `BOARD` | Displays the live Order Book and Spread. |
| **EXIT** | `EXIT` | Safely terminates the trading session. |

---

##  Compilation & Usage

1.  **Compile** using any C++11 (or higher) compiler:
    ```bash
    g++ -o NexusMatch main.cpp
    ```

2.  **Run** the engine:
    ```bash
    ./NexusMatch
    ```

---

##  Sample Test Scenario
**1. Market Setup:** Seller s1 is at $91; Buyer b1 is at $90. A hidden Stop-Loss (c3) is set to trigger at $92.

```bash
SELL s1 10 91
BUY b1 10 90
STOP c3 92 89 5
BOARD
```
**2. The Catalyst:** Buyer b7 enters with a limit price of $91.

```bash
BUY b7 15 91
```
**3. Execution:** The engine matches b7 and s1 at $91.

**4. Chain Reaction:** The trade at $91 is below the $92 threshold. This instantly triggers c3's stop-loss, which enters the market as a sell order and matches the remainder of b7's order at $89.

**Result:** The system proves its ability to handle multi-level matching and real-time trigger injection within a single transaction cycle.




---

