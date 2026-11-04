#include "HelpDesk.h"
#include "TicketFile.h"

bool HelpDesk::add(const std::string &name, const std::string &category, int minutes,
                   Ticket &out, int arrival)
{
    if (name.empty() || category.empty() || minutes <= 0 || arrival < 0)
        return false;

    out = Ticket(nextId_, name, category, minutes, arrival);
    nextId_++;
    waiting_.enqueue(out);                     // queue: join at the rear
    history_.push({ActionType::Add, out, 0});  // stack: remember for undo
    return true;
}

bool HelpDesk::serve(Ticket &out)
{
    if (!waiting_.dequeue(out))                // queue: leave from the front
        return false;
    served_.enqueue(out);
    history_.push({ActionType::Serve, out, 0});
    return true;
}

bool HelpDesk::cancel(int id, Ticket &out, int &position)
{
    if (!waiting_.removeById(id, out, position))
        return false;
    history_.push({ActionType::Cancel, out, position}); // keep the position to put it back
    return true;
}

bool HelpDesk::undo(Action &undone)
{
    if (!history_.pop(undone)) // stack: the last action comes back first
        return false;

    Ticket t;
    switch (undone.type)
    {
    case ActionType::Add:
        // Everything after this add has been undone already (LIFO), so it is the rear ticket.
        waiting_.removeRear(t);
        nextId_ = undone.ticket.id; // give the id back
        break;
    case ActionType::Serve:
        served_.removeRear(t);           // it was the last one served
        waiting_.pushFront(undone.ticket); // and it had been the front of the line
        break;
    case ActionType::Cancel:
        waiting_.insertAt(undone.position, undone.ticket);
        break;
    }
    return true;
}

bool HelpDesk::load(const std::string &path, int &count, std::string &error)
{
    TicketQueue loaded;
    if (!loadTickets(path, loaded, error))
        return false;

    // the next id continues after the largest id in the file
    int maxId = 0;
    TicketQueue copy = loaded;
    Ticket t;
    while (copy.dequeue(t))
        if (t.id > maxId)
            maxId = t.id;

    waiting_ = loaded;
    served_.clear();
    history_.clear();
    nextId_ = maxId + 1;
    count = waiting_.size();
    return true;
}

bool HelpDesk::save(const std::string &path, std::string &error) const
{
    return saveTickets(path, waiting_, error);
}
