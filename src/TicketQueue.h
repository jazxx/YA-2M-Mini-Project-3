#pragma once
#include <iostream>
#include <string>
#include "Ticket.h"

// Result of a search: position in line (1 = front, -1 = not found)
// and the estimated wait = sum of the minutes of the tickets in front of it.
struct SearchResult
{
    int position = -1;
    int waitMinutes = 0;
};

// FIFO queue of tickets, built on a singly linked list with front and rear pointers.
// Besides enqueue/dequeue it supports the extra operations the help desk needs:
// deleting from the middle (cancel) and putting tickets back (undo).
class TicketQueue
{
private:
    struct Node
    {
        Ticket ticket;
        Node *next;
    };

    Node *front_ = nullptr; // dequeue here
    Node *rear_ = nullptr;  // enqueue here
    int count_ = 0;

public:
    TicketQueue() = default;
    TicketQueue(const TicketQueue &other);            // deep copy
    TicketQueue &operator=(const TicketQueue &other); // deep copy
    ~TicketQueue();

    bool isEmpty() const { return count_ == 0; }
    int size() const { return count_; }
    void clear();

    // --- core queue operations, all O(1) ---
    void enqueue(const Ticket &t);
    bool dequeue(Ticket &out);
    bool peek(Ticket &out) const;

    // --- operations used by cancel and undo ---
    void pushFront(const Ticket &t);                          // O(1)  undo serve
    bool removeRear(Ticket &out);                             // O(n)  undo add
    bool insertAt(int position, const Ticket &t);             // O(n)  undo cancel (1..size+1)
    bool removeById(int id, Ticket &out, int &position);      // O(n)  cancel

    // --- look without changing the order, O(n) ---
    SearchResult searchById(int id) const;
    SearchResult searchByName(const std::string &name) const; // first match
    bool get(int position, Ticket &out) const;                // 1 = front
    int totalMinutes() const;
    void print(std::ostream &os = std::cout, const std::string &label = "waiting") const;
    bool equals(const TicketQueue &other) const;              // same tickets, same order
};
