#pragma once
#include <string>
#include "Ticket.h"

enum class ActionType
{
    Add,
    Serve,
    Cancel
};

std::string toString(ActionType type);

// Everything undo needs to reverse one command.
struct Action
{
    ActionType type = ActionType::Add;
    Ticket ticket;    // the ticket that was added, served or cancelled
    int position = 0; // for Cancel: where the ticket was in the line (1 = front)
};

// LIFO stack of actions, built on a singly linked list (the head is the top).
// Linked, so the undo history has no fixed capacity.
class ActionStack
{
private:
    struct Node
    {
        Action action;
        Node *next;
    };

    Node *top_ = nullptr;
    int count_ = 0;

public:
    ActionStack() = default;
    ActionStack(const ActionStack &) = delete;            // not needed; avoid accidental shallow copies
    ActionStack &operator=(const ActionStack &) = delete;
    ~ActionStack();

    bool isEmpty() const { return top_ == nullptr; }
    int size() const { return count_; }
    void clear();

    void push(const Action &a);     // O(1)
    bool pop(Action &out);          // O(1)
    bool peek(Action &out) const;   // O(1)
};
