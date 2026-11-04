#include "ActionStack.h"

std::string toString(ActionType type)
{
    switch (type)
    {
    case ActionType::Add:
        return "add";
    case ActionType::Serve:
        return "serve";
    case ActionType::Cancel:
        return "cancel";
    }
    return "?";
}

ActionStack::~ActionStack() { clear(); }

void ActionStack::clear()
{
    Action a;
    while (pop(a))
    {
    }
}

// ADD: the new node becomes the new top
void ActionStack::push(const Action &a)
{
    top_ = new Node{a, top_};
    count_++;
}

// DELETE: unlink the top node
bool ActionStack::pop(Action &out)
{
    if (isEmpty())
        return false;
    Node *old = top_;
    out = old->action;
    top_ = old->next;
    delete old;
    count_--;
    return true;
}

bool ActionStack::peek(Action &out) const
{
    if (isEmpty())
        return false;
    out = top_->action;
    return true;
}
