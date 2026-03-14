#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include <algorithm>
#include <iomanip>
#include <sstream>

using namespace std;

//  UI TERMINAL STYLING 
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define RED     "\033[1;31m"
#define CYAN    "\033[1;36m"
#define BLUE    "\033[1;34m"


// DATA MODELS


// The individual "Trade Slip" for a user
struct Order {
    string id;
    bool isBuy;
    double price;
    int qty;

    // Doubly Linked List pointers to manage the "Line" at a specific price
    Order *prev, *next;

    Order(string _id, bool _buy, double _p, int _q) 
        : id(_id), isBuy(_buy), price(_p), qty(_q), prev(nullptr), next(nullptr) {}
};

// A "Hidden" order that only activates when the market hits a certain price
struct StopLoss {
    double triggerPrice;
    string id;
    double limitPrice;
    int qty;

    // The Heap uses this to keep the most urgent trigger at the top
    bool operator<(const StopLoss& other) const {
        return triggerPrice < other.triggerPrice; 
    }
};

// A "Folder" in our filing cabinet that holds everyone wanting the same price
struct PriceLevel {
    double price;
    int height;
    Order *head, *tail; // The "Line" of people waiting for this price
    PriceLevel *left, *right; // Tree pointers for the AVL structure

    PriceLevel(double p) 
        : price(p), height(1), head(nullptr), tail(nullptr), left(nullptr), right(nullptr) {}
};

// ENGINE CORE

class OrderBook {
private:
    PriceLevel *buyTree = nullptr;  // Shelves for the Buyers
    PriceLevel *sellTree = nullptr; // Shelves for the Sellers
    unordered_map<string, Order*> orderMap; // Directory to find orders by ID
    priority_queue<StopLoss> stopLossHeap;  // The hidden "Traps"

    // AVL TREE UTILITIES 
    // These keep the "Price Shelves" balanced so searching is fast

    int getHeight(PriceLevel* n) { return n ? n->height : 0; }

    int getBalance(PriceLevel* n) { return n ? getHeight(n->left) - getHeight(n->right) : 0; }

    PriceLevel* rotateRight(PriceLevel* y) {
        PriceLevel* x = y->left; y->left = x->right; x->right = y;
        y->height = max(getHeight(y->left), getHeight(y->right)) + 1;
        x->height = max(getHeight(x->left), getHeight(x->right)) + 1;
        return x;
    }

    PriceLevel* rotateLeft(PriceLevel* x) {
        PriceLevel* y = x->right; x->right = y->left; y->left = x;
        x->height = max(getHeight(x->left), getHeight(x->right)) + 1;
        y->height = max(getHeight(y->left), getHeight(y->right)) + 1;
        return y;
    }

    // Surgical removal of an empty price folder
    PriceLevel* deleteLevel(PriceLevel* root, double price) {
        if (!root) return nullptr;

        if (price < root->price) root->left = deleteLevel(root->left, price);
        else if (price > root->price) root->right = deleteLevel(root->right, price);
        else {
            if (!root->left || !root->right) {
                PriceLevel* temp = root->left ? root->left : root->right;
                delete root;
                return temp;
            }
            PriceLevel* temp = root->right;
            while (temp->left) temp = temp->left;
            root->price = temp->price;
            root->head = temp->head;
            root->tail = temp->tail;
            root->right = deleteLevel(root->right, temp->price);
        }

        root->height = 1 + max(getHeight(root->left), getHeight(root->right));
        int b = getBalance(root);

        // Re-balance logic
        if (b > 1 && getBalance(root->left) >= 0)
         return rotateRight(root);
        if (b > 1 && getBalance(root->left) < 0) {
             root->left = rotateLeft(root->left);
              return rotateRight(root);
             }
        if (b < -1 && getBalance(root->right) <= 0) return rotateLeft(root);
        if (b < -1 && getBalance(root->right) > 0) {
             root->right = rotateRight(root->right);
              return rotateLeft(root);
             }
        return root;
    }

    // Placing a new price folder on the shelf
    PriceLevel* insertAVL(PriceLevel* node, double price, Order* ord) {
        if (!node) { 
            PriceLevel* n = new PriceLevel(price); 
            n->head = n->tail = ord; 
            return n; 
        }

        if (price < node->price) node->left = insertAVL(node->left, price, ord);
        else if (price > node->price) node->right = insertAVL(node->right, price, ord);
        else { 
            // Price level already exists; add the new person to the back of the line
            ord->prev = node->tail;
            node->tail->next = ord;
            node->tail = ord;
            return node; 
        }

        node->height = 1 + max(getHeight(node->left), getHeight(node->right));
        int b = getBalance(node);

        if (b > 1 && price < node->left->price) return rotateRight(node);
        if (b < -1 && price > node->right->price) return rotateLeft(node);
        if (b > 1 && price > node->left->price) { 
            node->left = rotateLeft(node->left); 
            return rotateRight(node);
         }
        if (b < -1 && price < node->right->price) { 
            node->right = rotateRight(node->right);
             return rotateLeft(node); 
            }
        return node;
    }

    // Find the highest Bidder or the lowest Seller
    PriceLevel* findBest(PriceLevel* n, bool findMax) {
        if (!n) return nullptr;
        while (findMax ? n->right : n->left) n = (findMax ? n->right : n->left);
        return n;
    }

public:
    //  PUBLIC API 

    void submit(string id, bool isBuy, double price, int qty) {
        if (orderMap.find(id) != orderMap.end()) {
            cout << RED << "  [!] User '" << id << "' already has an order. Only one active order allowed." << RESET << endl;
            return;
        }

        Order* ord = new Order(id, isBuy, price, qty);
        orderMap[id] = ord;

        if (isBuy) buyTree = insertAVL(buyTree, price, ord);
        else sellTree = insertAVL(sellTree, price, ord);
        
        cout << GREEN << "  [+] " << id << " placed " << (isBuy ? "BUY" : "SELL") << " order for " << qty << " units." << RESET << endl;
        
        match(); // Check if this new order creates a trade
    }

    void submitStopLoss(string id, double trigger, double limit, int qty) {
        stopLossHeap.push({trigger, id, limit, qty});
        cout << BLUE << "  [?] Stop-Loss activated for " << id << ". Waiting for trigger at $" << trigger << RESET << endl;
    }

    void match() {
        double lastTradePrice = -1;

        while (true) {
            PriceLevel* bestBid = findBest(buyTree, true);
            PriceLevel* bestAsk = findBest(sellTree, false);

            // Stop matching if no overlap exists
            if (!bestBid || !bestAsk || bestBid->price < bestAsk->price) break;

            Order* b = bestBid->head;
            Order* s = bestAsk->head;
            int tradeQty = min(b->qty, s->qty);
            lastTradePrice = bestAsk->price;

            cout << YELLOW << BOLD << "  >> TRADE: " << b->id << " & " << s->id << " exchanged " << tradeQty << " shares @ $" << lastTradePrice << RESET << endl;

            b->qty -= tradeQty;
            s->qty -= tradeQty;

            // Clear out filled orders
            if (b->qty == 0) {
                orderMap.erase(b->id);
                bestBid->head = b->next;
                if (bestBid->head) bestBid->head->prev = nullptr;
                else buyTree = deleteLevel(buyTree, bestBid->price);
                delete b;
            }

            if (s->qty == 0) {
                orderMap.erase(s->id);
                bestAsk->head = s->next;
                if (bestAsk->head) bestAsk->head->prev = nullptr;
                else sellTree = deleteLevel(sellTree, bestAsk->price);
                delete s;
            }
        }

        // If a trade happened, check if any Stop-Loss "Traps" were sprung
        if (lastTradePrice != -1) checkTriggers(lastTradePrice);
    }

    void checkTriggers(double price) {
        while (!stopLossHeap.empty() && price <= stopLossHeap.top().triggerPrice) {
            StopLoss sl = stopLossHeap.top();
            stopLossHeap.pop();
            cout << RED << "  [*] TRIGGER: Market hit $" << price << ". " << sl.id << "'s Stop-Loss is now a SELL order." << RESET << endl;
            submit(sl.id, false, sl.limitPrice, sl.qty); 
        }
    }

    void showDashboard() {
        cout << "\n" << CYAN << BOLD << "=================== [ LIVE ORDER BOOK ] ===================" << RESET << endl;
        cout << left << setw(12) << "TRADER" << " | " << setw(7) << "SIDE" << " | " << setw(8) << "PRICE" << " | " << "QTY" << endl;
        cout << "-----------------------------------------------------------" << endl;
        
        printRecursive(sellTree, false); // Sellers: Low to High
        cout << "-------------------- ( MARKET SPREAD ) --------------------" << endl;
        printRecursive(buyTree, true);  // Buyers: High to Low
        
        cout << CYAN << BOLD << "===========================================================" << RESET << "\n" << endl;
    }

    void printRecursive(PriceLevel* n, bool descending) {
        if (!n) return;
        if (descending) {
            printRecursive(n->right, descending);
            Order* curr = n->head;
            while(curr) {
                cout << left << setw(12) << curr->id << " | " << GREEN << setw(7) << "BUY" << RESET << " | $" << setw(7) << n->price << " | " << curr->qty << endl;
                curr = curr->next;
            }
            printRecursive(n->left, descending);
        } else {
            printRecursive(n->left, descending);
            Order* curr = n->head;
            while(curr) {
                cout << left << setw(12) << curr->id << " | " << RED << setw(7) << "SELL" << RESET << " | $" << setw(7) << n->price << " | " << curr->qty << endl;
                curr = curr->next;
            }
            printRecursive(n->right, descending);
        }
    }
};

//MAIN INTERFACE


int main() {
    OrderBook exchange;
    string line;

    cout << CYAN << BOLD << "====================================================" << endl;
    cout << "           NEXUSMATCH TRADING SYSTEM " << endl;
    cout << "   Commands: BUY/SELL [NAME] [QTY] [PRICE]" << endl;
    cout << "             STOP [NAME] [TRIGGER] [PRICE] [QTY]" << endl;
    cout << "             BOARD | EXIT" << endl;
    cout << "====================================================" << RESET << endl;

    while (cout << CYAN << "Engine >> " << RESET && getline(cin, line)) {
        if (line.empty()) continue;
        
        stringstream ss(line);
        string cmd, id;
        double p, t; 
        int q;

        if (!(ss >> cmd)) continue;
        for (auto &c : cmd) c = toupper(c);

        if (cmd == "BUY" || cmd == "SELL") {
            if (ss >> id >> q >> p) {
                exchange.submit(id, (cmd == "BUY"), p, q);
            } else {
                cout << RED << "  [!] Syntax Error. Use: BUY/SELL [NAME] [QTY] [PRICE]" << RESET << endl;
            }
        } 
        else if (cmd == "STOP") {
            if (ss >> id >> t >> p >> q) {
                exchange.submitStopLoss(id, t, p, q);
            } else {
                cout << RED << "  [!] Syntax Error. Use: STOP [NAME] [TRIGGER] [PRICE] [QTY]" << RESET << endl;
            }
        }
        else if (cmd == "BOARD") {
            exchange.showDashboard();
        }
        else if (cmd == "EXIT") {
            break;
        }
        else {
            cout << RED << "  [!] Unknown command. Use BUY, SELL, STOP, or BOARD." << RESET << endl;
        }
    }

    cout << CYAN << "[SYSTEM] Closing Exchange." << RESET << endl;
    return 0;
}