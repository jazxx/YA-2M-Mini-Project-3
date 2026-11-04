#pragma once
#include <string>
#include <utility>

// One help desk request.
class Ticket
{
public:
    int id = 0;
    std::string name;     // student name
    std::string category; // e.g. wifi, password, printer
    int minutes = 0;      // minutes the agent needs to solve it
    int arrival = 0;      // minutes after opening (used by the M4 simulation)

    Ticket() = default;
    Ticket(int id, std::string name, std::string category, int minutes, int arrival = 0)
        : id(id), name(std::move(name)), category(std::move(category)),
          minutes(minutes), arrival(arrival) {}

    bool operator==(const Ticket &o) const
    {
        return id == o.id && name == o.name && category == o.category &&
               minutes == o.minutes && arrival == o.arrival;
    }
    bool operator!=(const Ticket &o) const { return !(*this == o); }
};
