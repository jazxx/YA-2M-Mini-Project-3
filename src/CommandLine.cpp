#include "CommandLine.h"
#include "Simulator.h"
#include "TicketFile.h"
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace
{
std::vector<std::string> split(const std::string &line)
{
    std::istringstream in(line); // also drops a trailing '\r' from Windows files
    std::vector<std::string> words;
    std::string w;
    while (in >> w)
        words.push_back(w);
    return words;
}

// words[from..to) joined with single spaces, e.g. a name with several words
std::string join(const std::vector<std::string> &words, size_t from, size_t to)
{
    std::string s;
    for (size_t i = from; i < to; i++)
    {
        if (!s.empty())
            s += " ";
        s += words[i];
    }
    return s;
}

// strict: "12" is fine, "12x" and "" are not
bool parseInt(const std::string &s, int &value)
{
    if (s.empty())
        return false;
    try
    {
        size_t used = 0;
        value = std::stoi(s, &used);
        return used == s.size();
    }
    catch (...)
    {
        return false;
    }
}

std::string describe(const Ticket &t)
{
    return "#" + std::to_string(t.id) + " " + t.name + " (" + t.category + ", " +
           std::to_string(t.minutes) + " min)";
}

// minute after opening -> clock time, opening at 9:00
std::string clock(int minute)
{
    int h = 9 + minute / 60, m = minute % 60;
    char buf[8];
    std::snprintf(buf, sizeof buf, "%02d:%02d", h % 24, m);
    return buf;
}

void reportSimulation(const SimResult &r, std::ostream &out)
{
    out << "  " << std::setw(6) << r.agents
        << "  " << std::setw(13) << std::fixed << std::setprecision(1) << r.averageWait
        << "  " << std::setw(12) << r.longestWait
        << "  " << std::setw(7);
    if (r.longestWaitId == -1)
        out << "-";
    else
        out << ("#" + std::to_string(r.longestWaitId));
    out << "  " << std::setw(4) << r.lastFinish << " (" << clock(r.lastFinish) << ")\n";
    out.unsetf(std::ios::fixed);
}

void reportPosition(const HelpDesk &desk, const SearchResult &r, std::ostream &out)
{
    Ticket t;
    desk.waiting().get(r.position, t);
    out << describe(t) << ": position " << r.position << " of " << desk.waiting().size()
        << ", estimated wait " << r.waitMinutes << " min\n";
}
} // namespace

void printHelp(std::ostream &out)
{
    out << "commands:\n"
           "  add <name> <category> <minutes>   e.g. add Ayham Makhamra wifi 10\n"
           "  serve                             serve the front ticket\n"
           "  print                             list waiting tickets, front to rear\n"
           "  served                            list served tickets\n"
           "  search id <id>                    position and estimated wait\n"
           "  search name <name>\n"
           "  cancel <id>                       remove a waiting ticket\n"
           "  undo                              reverse the last add, serve or cancel\n"
           "  load <file.csv>                   replace the line with a day of tickets\n"
           "  save <file.csv>                   save the waiting line\n"
           "  simulate <file.csv> [agents]      waiting times for a day (default: 1 and 2)\n"
           "  help\n"
           "  quit\n";
}

bool runCommand(HelpDesk &desk, const std::string &line, std::ostream &out)
{
    std::vector<std::string> w = split(line);
    if (w.empty())
        return true;
    const std::string &cmd = w[0];

    if (cmd == "quit" || cmd == "exit")
        return false;

    if (cmd == "help")
    {
        printHelp(out);
    }
    else if (cmd == "add")
    {
        // the name may have spaces: the last word is minutes, the one before is the category
        int minutes = 0;
        if (w.size() < 4)
            out << "usage: add <name> <category> <minutes>\n";
        else if (!parseInt(w.back(), minutes) || minutes <= 0)
            out << "error: minutes must be a positive whole number\n";
        else
        {
            Ticket t;
            desk.add(join(w, 1, w.size() - 2), w[w.size() - 2], minutes, t);
            out << "added ";
            reportPosition(desk, desk.searchId(t.id), out);
        }
    }
    else if (cmd == "serve")
    {
        Ticket t;
        if (desk.serve(t))
            out << "served " << describe(t) << "\n";
        else
            out << "no tickets waiting\n";
    }
    else if (cmd == "print")
    {
        desk.waiting().print(out, "waiting");
    }
    else if (cmd == "served")
    {
        desk.served().print(out, "served");
    }
    else if (cmd == "search")
    {
        int id = 0;
        if (w.size() >= 3 && w[1] == "id")
        {
            if (!parseInt(w[2], id) || w.size() != 3)
                out << "usage: search id <id>\n";
            else
            {
                SearchResult r = desk.searchId(id);
                if (r.position == -1)
                    out << "no waiting ticket with id " << id << "\n";
                else
                    reportPosition(desk, r, out);
            }
        }
        else if (w.size() >= 3 && w[1] == "name")
        {
            std::string name = join(w, 2, w.size());
            SearchResult r = desk.searchName(name);
            if (r.position == -1)
                out << "no waiting ticket for " << name << "\n";
            else
                reportPosition(desk, r, out);
        }
        else
            out << "usage: search id <id>  or  search name <name>\n";
    }
    else if (cmd == "cancel")
    {
        int id = 0;
        if (w.size() != 2 || !parseInt(w[1], id))
            out << "usage: cancel <id>\n";
        else
        {
            Ticket t;
            int position = 0;
            if (desk.cancel(id, t, position))
                out << "cancelled " << describe(t) << " (was at position " << position << ")\n";
            else
                out << "no waiting ticket with id " << id << "\n";
        }
    }
    else if (cmd == "undo")
    {
        Action a;
        if (!desk.undo(a))
            out << "nothing to undo\n";
        else if (a.type == ActionType::Add)
            out << "undid add: " << describe(a.ticket) << " removed from the line\n";
        else if (a.type == ActionType::Serve)
            out << "undid serve: " << describe(a.ticket) << " is back at the front\n";
        else
            out << "undid cancel: " << describe(a.ticket) << " is back at position "
                << a.position << "\n";
    }
    else if (cmd == "load")
    {
        std::string path = join(w, 1, w.size()), error;
        int count = 0;
        if (path.empty())
            out << "usage: load <file.csv>\n";
        else if (desk.load(path, count, error))
            out << "loaded " << count << " ticket(s) from " << path
                << " (served log and undo history cleared)\n";
        else
            out << "error: " << error << " (nothing changed)\n";
    }
    else if (cmd == "save")
    {
        std::string path = join(w, 1, w.size()), error;
        if (path.empty())
            out << "usage: save <file.csv>\n";
        else if (desk.save(path, error))
            out << "saved " << desk.waiting().size() << " waiting ticket(s) to " << path << "\n";
        else
            out << "error: " << error << "\n";
    }
    else if (cmd == "simulate")
    {
        // simulate <file> [agents]; without agents, compare 1 and 2
        int agents = 0;
        size_t pathEnd = w.size();
        if (w.size() >= 3 && parseInt(w.back(), agents))
            pathEnd--;
        std::string path = join(w, 1, pathEnd), error;
        TicketQueue day;
        if (path.empty())
            out << "usage: simulate <file.csv> [agents]\n";
        else if (pathEnd < w.size() && (agents < 1 || agents > MAX_AGENTS))
            out << "error: agents must be 1.." << MAX_AGENTS << "\n";
        else if (!loadTickets(path, day, error))
            out << "error: " << error << "\n";
        else
        {
            out << "simulation of " << path << ": " << day.size() << " ticket(s), "
                << day.totalMinutes() << " min of work, opening at 09:00\n"
                << "  agents  avg wait (min)  longest (min)   ticket  last done\n";
            int from = agents ? agents : 1, to = agents ? agents : 2;
            for (int k = from; k <= to; k++)
            {
                SimResult r;
                simulate(day, k, r);
                reportSimulation(r, out);
            }
        }
    }
    else
    {
        out << "unknown command '" << cmd << "' (type help)\n";
    }
    return true;
}
