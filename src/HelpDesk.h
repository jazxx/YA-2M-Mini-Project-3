#pragma once
#include <string>
#include "Ticket.h"
#include "TicketQueue.h"
#include "ActionStack.h"

// The help desk: the waiting line (queue), the served log, and the action history (stack).
// It only changes state and reports results; printing is done by the command line.
class HelpDesk
{
private:
    TicketQueue waiting_;  // tickets in arrival order: served from the front
    TicketQueue served_;   // served tickets, in the order they were served
    ActionStack history_;  // every add, serve and cancel, so undo can reverse it
    int nextId_ = 1;

public:
    // add: enqueue a new ticket with the next id. False if the input is invalid.
    bool add(const std::string &name, const std::string &category, int minutes,
             Ticket &out, int arrival = 0);

    // serve: dequeue the front ticket and record it as served. False if nobody is waiting.
    bool serve(Ticket &out);

    // cancel: remove a waiting ticket from anywhere in the line; the others keep their order.
    // position is where it was (1 = front). False if no waiting ticket has this id.
    bool cancel(int id, Ticket &out, int &position);

    // undo: pop the last action and reverse it. undone is the action that was reversed.
    //   add    -> remove the ticket from the rear and give its id back
    //   serve  -> take it off the served log and put it back at the front
    //   cancel -> put it back at its old position
    bool undo(Action &undone);

    // load: replace the waiting line with the tickets in a CSV file. The served log and the
    // undo history are cleared, and new ids continue after the largest id in the file.
    // On error nothing changes.
    bool load(const std::string &path, int &count, std::string &error);

    // save: write the waiting line (front to rear) to a CSV file.
    bool save(const std::string &path, std::string &error) const;

    SearchResult searchId(int id) const { return waiting_.searchById(id); }
    SearchResult searchName(const std::string &name) const { return waiting_.searchByName(name); }

    const TicketQueue &waiting() const { return waiting_; }
    const TicketQueue &served() const { return served_; }
    int nextId() const { return nextId_; }
    int historySize() const { return history_.size(); }
};
