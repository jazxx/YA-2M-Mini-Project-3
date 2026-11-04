#include "TicketQueue.h"
#include <iomanip>

TicketQueue::TicketQueue(const TicketQueue &other)
{
    for (Node *p = other.front_; p != nullptr; p = p->next)
        enqueue(p->ticket);
}

TicketQueue &TicketQueue::operator=(const TicketQueue &other)
{
    if (this != &other)
    {
        clear();
        for (Node *p = other.front_; p != nullptr; p = p->next)
            enqueue(p->ticket);
    }
    return *this;
}

TicketQueue::~TicketQueue() { clear(); }

void TicketQueue::clear()
{
    while (front_ != nullptr)
    {
        Node *old = front_;
        front_ = front_->next;
        delete old;
    }
    rear_ = nullptr;
    count_ = 0;
}

// ADD: link a new node after the rear
void TicketQueue::enqueue(const Ticket &t)
{
    Node *n = new Node{t, nullptr};
    if (rear_ == nullptr) // empty queue: the node is front and rear
        front_ = n;
    else
        rear_->next = n;
    rear_ = n;
    count_++;
}

// DELETE: unlink the front node
bool TicketQueue::dequeue(Ticket &out)
{
    if (isEmpty())
        return false;
    Node *old = front_;
    out = old->ticket;
    front_ = old->next;
    if (front_ == nullptr) // the queue became empty
        rear_ = nullptr;
    delete old;
    count_--;
    return true;
}

bool TicketQueue::peek(Ticket &out) const
{
    if (isEmpty())
        return false;
    out = front_->ticket;
    return true;
}

// Put a ticket back at the front (reverses a serve)
void TicketQueue::pushFront(const Ticket &t)
{
    Node *n = new Node{t, front_};
    front_ = n;
    if (rear_ == nullptr)
        rear_ = n;
    count_++;
}

// Remove the last ticket (reverses an add). Singly linked, so we walk to the node before rear.
bool TicketQueue::removeRear(Ticket &out)
{
    if (isEmpty())
        return false;
    if (front_ == rear_) // only one node
        return dequeue(out);

    Node *prev = front_;
    while (prev->next != rear_)
        prev = prev->next;

    out = rear_->ticket;
    delete rear_;
    rear_ = prev;
    rear_->next = nullptr;
    count_--;
    return true;
}

// Insert so that the ticket ends up at the given position (1 = front, size+1 = rear).
bool TicketQueue::insertAt(int position, const Ticket &t)
{
    if (position < 1 || position > count_ + 1)
        return false;
    if (position == 1)
    {
        pushFront(t);
        return true;
    }
    if (position == count_ + 1)
    {
        enqueue(t);
        return true;
    }

    Node *prev = front_; // node at position-1
    for (int k = 1; k < position - 1; k++)
        prev = prev->next;

    prev->next = new Node{t, prev->next};
    count_++;
    return true;
}

// Unlink the ticket with this id; report where it was so undo can put it back.
bool TicketQueue::removeById(int id, Ticket &out, int &position)
{
    Node *prev = nullptr;
    Node *p = front_;
    int pos = 1;
    while (p != nullptr && p->ticket.id != id)
    {
        prev = p;
        p = p->next;
        pos++;
    }
    if (p == nullptr)
        return false;

    if (prev == nullptr)
        front_ = p->next; // it was the front
    else
        prev->next = p->next;
    if (p == rear_)
        rear_ = prev; // it was the rear (nullptr if the queue is now empty)

    out = p->ticket;
    position = pos;
    delete p;
    count_--;
    return true;
}

SearchResult TicketQueue::searchById(int id) const
{
    int pos = 1, wait = 0;
    for (Node *p = front_; p != nullptr; p = p->next)
    {
        if (p->ticket.id == id)
            return {pos, wait};
        wait += p->ticket.minutes;
        pos++;
    }
    return {};
}

SearchResult TicketQueue::searchByName(const std::string &name) const
{
    int pos = 1, wait = 0;
    for (Node *p = front_; p != nullptr; p = p->next)
    {
        if (p->ticket.name == name)
            return {pos, wait};
        wait += p->ticket.minutes;
        pos++;
    }
    return {};
}

bool TicketQueue::get(int position, Ticket &out) const
{
    if (position < 1 || position > count_)
        return false;
    Node *p = front_;
    for (int k = 1; k < position; k++)
        p = p->next;
    out = p->ticket;
    return true;
}

int TicketQueue::totalMinutes() const
{
    int total = 0;
    for (Node *p = front_; p != nullptr; p = p->next)
        total += p->ticket.minutes;
    return total;
}

// PRINT: front to rear, without changing the queue
void TicketQueue::print(std::ostream &os, const std::string &label) const
{
    if (isEmpty())
    {
        os << "  (no tickets " << label << ")\n";
        return;
    }
    os << "  pos    id  name                category      min\n";
    int pos = 1;
    for (Node *p = front_; p != nullptr; p = p->next, pos++)
    {
        os << "  " << std::setw(3) << pos
           << "  " << std::setw(4) << p->ticket.id
           << "  " << std::left << std::setw(18) << p->ticket.name
           << "  " << std::setw(12) << p->ticket.category << std::right
           << "  " << std::setw(3) << p->ticket.minutes << "\n";
    }
    os << "  " << count_ << " ticket(s) " << label << ", total " << totalMinutes() << " min\n";
}

bool TicketQueue::equals(const TicketQueue &other) const
{
    if (count_ != other.count_)
        return false;
    for (Node *a = front_, *b = other.front_; a != nullptr; a = a->next, b = b->next)
        if (a->ticket != b->ticket)
            return false;
    return true;
}
