#include "TicketFile.h"
#include <fstream>
#include <string>
#include <vector>

namespace
{
// Splits one CSV line into fields, handling "quoted, fields" and "" inside quotes.
bool splitCsv(const std::string &line, std::vector<std::string> &fields)
{
    fields.clear();
    std::string field;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); i++)
    {
        char c = line[i];
        if (inQuotes)
        {
            if (c == '"' && i + 1 < line.size() && line[i + 1] == '"')
            {
                field += '"';
                i++;
            }
            else if (c == '"')
                inQuotes = false;
            else
                field += c;
        }
        else if (c == '"')
            inQuotes = true;
        else if (c == ',')
        {
            fields.push_back(field);
            field.clear();
        }
        else if (c != '\r') // ignore Windows line endings
            field += c;
    }
    fields.push_back(field);
    return !inQuotes; // an unclosed quote is an error
}

std::string trim(const std::string &s)
{
    size_t b = s.find_first_not_of(" \t\r");
    if (b == std::string::npos)
        return "";
    size_t e = s.find_last_not_of(" \t\r");
    return s.substr(b, e - b + 1);
}

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

std::string quoteIfNeeded(const std::string &s)
{
    if (s.find_first_of(",\"") == std::string::npos)
        return s;
    std::string q = "\"";
    for (char c : s)
    {
        if (c == '"')
            q += "\"\"";
        else
            q += c;
    }
    return q + "\"";
}

// true if a ticket with this id is already in q
bool hasId(const TicketQueue &q, int id) { return q.searchById(id).position != -1; }
} // namespace

bool loadTickets(const std::string &path, TicketQueue &out, std::string &error)
{
    error.clear();
    std::ifstream in(path);
    if (!in)
    {
        error = "cannot open " + path;
        return false;
    }

    TicketQueue result; // build here first, so a bad file changes nothing
    std::string line;
    std::vector<std::string> f;
    int lineNo = 0;
    bool firstDataLine = true;

    while (std::getline(in, line))
    {
        lineNo++;
        if (trim(line).empty())
            continue;

        std::string where = path + " line " + std::to_string(lineNo) + ": ";
        if (!splitCsv(line, f))
        {
            error = where + "unclosed quote";
            return false;
        }
        for (std::string &s : f)
            s = trim(s);

        if (firstDataLine && f[0] == "id") // header
        {
            firstDataLine = false;
            continue;
        }
        firstDataLine = false;

        if (f.size() != 4 && f.size() != 5)
        {
            error = where + "expected 4 or 5 fields (id,name,category,minutes[,arrival])";
            return false;
        }

        Ticket t;
        t.name = f[1];
        t.category = f[2];
        if (!parseInt(f[0], t.id) || t.id <= 0)
            error = where + "id must be a positive whole number";
        else if (t.name.empty() || t.category.empty())
            error = where + "name and category cannot be empty";
        else if (!parseInt(f[3], t.minutes) || t.minutes <= 0)
            error = where + "minutes must be a positive whole number";
        else if (f.size() == 5 && (!parseInt(f[4], t.arrival) || t.arrival < 0))
            error = where + "arrival must be a whole number >= 0";
        else if (hasId(result, t.id))
            error = where + "duplicate id " + f[0];
        if (!error.empty())
            return false;

        result.enqueue(t);
    }

    out = result;
    return true;
}

bool saveTickets(const std::string &path, const TicketQueue &q, std::string &error)
{
    std::ofstream file(path);
    if (!file)
    {
        error = "cannot write " + path;
        return false;
    }

    file << "id,name,category,minutes,arrival\n";
    TicketQueue copy = q; // walk a copy so the real queue is untouched
    Ticket t;
    while (copy.dequeue(t))
        file << t.id << "," << quoteIfNeeded(t.name) << "," << quoteIfNeeded(t.category) << ","
             << t.minutes << "," << t.arrival << "\n";

    if (!file)
    {
        error = "error while writing " + path;
        return false;
    }
    return true;
}
