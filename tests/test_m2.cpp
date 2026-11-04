// M2 tests: the HelpDesk commands (add, serve, print, search, cancel) and the command parser.
// Checkpoint: cancel works at the front, middle and rear, and the order of the others is unchanged.
#include <sstream>
#include <string>
#include "HelpDesk.h"
#include "CommandLine.h"
#include "check.h"

static std::string ids(const TicketQueue &q)
{
    std::string s;
    Ticket t;
    for (int p = 1; p <= q.size(); p++)
    {
        q.get(p, t);
        if (!s.empty())
            s += " ";
        s += std::to_string(t.id);
    }
    return s;
}

// desk with tickets 1..5 needing 5, 10, 15, 20, 25 minutes
static void fill(HelpDesk &desk)
{
    Ticket t;
    const char *names[5] = {"Ana", "Ben", "Chen", "Dina", "Eli"};
    for (int i = 0; i < 5; i++)
        desk.add(names[i], "wifi", (i + 1) * 5, t);
}

// run several command lines, return everything they printed
static std::string run(HelpDesk &desk, const std::string &script)
{
    std::istringstream in(script);
    std::ostringstream out;
    std::string line;
    while (std::getline(in, line))
        runCommand(desk, line, out);
    return out.str();
}

static bool contains(const std::string &text, const std::string &part)
{
    return text.find(part) != std::string::npos;
}

static void testAdd()
{
    HelpDesk desk;
    Ticket t;
    CHECK(desk.nextId() == 1);
    CHECK(desk.add("Ana", "wifi", 5, t) && t.id == 1);
    CHECK(desk.add("Ben", "printer", 10, t) && t.id == 2);
    CHECK(ids(desk.waiting()) == "1 2");
    CHECK(desk.nextId() == 3);
    CHECK(desk.historySize() == 2);

    // invalid input is refused and does not use up an id
    CHECK(!desk.add("", "wifi", 5, t));
    CHECK(!desk.add("Cy", "", 5, t));
    CHECK(!desk.add("Cy", "wifi", 0, t));
    CHECK(!desk.add("Cy", "wifi", -3, t));
    CHECK(desk.nextId() == 3);
    CHECK(desk.historySize() == 2);
}

static void testServe()
{
    HelpDesk desk;
    Ticket t;
    CHECK(!desk.serve(t)); // empty: nothing to serve
    CHECK(desk.historySize() == 0);

    fill(desk);
    CHECK(desk.serve(t) && t.id == 1); // arrival order
    CHECK(desk.serve(t) && t.id == 2);
    CHECK(ids(desk.waiting()) == "3 4 5");
    CHECK(ids(desk.served()) == "1 2");
    CHECK(desk.historySize() == 7); // 5 adds + 2 serves

    // ids keep counting after serves; they are never reused
    CHECK(desk.add("Fay", "wifi", 5, t) && t.id == 6);
}

static void testSearch()
{
    HelpDesk desk;
    fill(desk);
    SearchResult r = desk.searchId(3);
    CHECK(r.position == 3 && r.waitMinutes == 5 + 10);
    r = desk.searchName("Eli");
    CHECK(r.position == 5 && r.waitMinutes == 50);
    CHECK(desk.searchId(42).position == -1);
    CHECK(desk.searchName("Zed").position == -1);

    Ticket t;
    desk.serve(t); // the wait of everyone behind drops by 5
    r = desk.searchId(3);
    CHECK(r.position == 2 && r.waitMinutes == 10);
    CHECK(desk.searchId(1).position == -1); // served tickets are not waiting
}

static void testCancel()
{
    HelpDesk desk;
    fill(desk);
    Ticket t;
    int pos = 0;

    CHECK(desk.cancel(3, t, pos) && t.id == 3 && pos == 3); // middle
    CHECK(ids(desk.waiting()) == "1 2 4 5");
    CHECK(desk.cancel(1, t, pos) && pos == 1);              // front
    CHECK(ids(desk.waiting()) == "2 4 5");
    CHECK(desk.cancel(5, t, pos) && pos == 3);              // rear
    CHECK(ids(desk.waiting()) == "2 4");

    // the queue still works normally after cancels at both ends
    CHECK(desk.add("Fay", "wifi", 5, t) && t.id == 6);
    CHECK(ids(desk.waiting()) == "2 4 6");
    CHECK(desk.serve(t) && t.id == 2);
    CHECK(ids(desk.waiting()) == "4 6");

    int before = desk.historySize();
    CHECK(!desk.cancel(3, t, pos));  // already cancelled
    CHECK(!desk.cancel(2, t, pos));  // already served
    CHECK(!desk.cancel(99, t, pos)); // never existed
    CHECK(desk.historySize() == before); // failed commands are not recorded

    // cancel everything, then the empty queue accepts new tickets
    CHECK(desk.cancel(4, t, pos) && desk.cancel(6, t, pos));
    CHECK(desk.waiting().isEmpty());
    CHECK(desk.add("Gus", "wifi", 5, t) && ids(desk.waiting()) == "7");
}

static void testCommandLine()
{
    HelpDesk desk;
    std::string out = run(desk,
                          "add Ayham Makhamra wifi 10\n"
                          "add Ben printer 20\n"
                          "add Chen password 5\n");
    CHECK(contains(out, "added #1 Ayham Makhamra (wifi, 10 min): position 1 of 1, estimated wait 0 min"));
    CHECK(contains(out, "added #3 Chen (password, 5 min): position 3 of 3, estimated wait 30 min"));

    out = run(desk, "search name Ayham Makhamra\n");
    CHECK(contains(out, "position 1 of 3"));
    out = run(desk, "search id 3\n");
    CHECK(contains(out, "#3 Chen") && contains(out, "estimated wait 30 min"));

    out = run(desk, "print\n");
    CHECK(contains(out, "3 ticket(s) waiting, total 35 min"));

    out = run(desk, "cancel 2\nprint\n");
    CHECK(contains(out, "cancelled #2 Ben (printer, 20 min) (was at position 2)"));
    CHECK(contains(out, "2 ticket(s) waiting, total 15 min"));

    out = run(desk, "serve\nserved\n");
    CHECK(contains(out, "served #1 Ayham Makhamra"));
    CHECK(contains(out, "1 ticket(s) served, total 10 min"));

    out = run(desk, "serve\nserve\n");
    CHECK(contains(out, "no tickets waiting"));
    CHECK(ids(desk.waiting()).empty());
}

static void testBadInput()
{
    HelpDesk desk;
    std::string out = run(desk,
                          "add Ana wifi\n"        // too few words
                          "add Ana wifi ten\n"    // not a number
                          "add Ana wifi 10x\n"
                          "add Ana wifi 0\n"
                          "cancel\n"
                          "cancel abc\n"
                          "search\n"
                          "search id\n"
                          "search id 1 2\n"
                          "search foo bar\n"
                          "fly away\n"
                          "\n"
                          "   \n");
    CHECK(desk.waiting().isEmpty());
    CHECK(desk.historySize() == 0);
    CHECK(desk.nextId() == 1);
    CHECK(contains(out, "usage: add"));
    CHECK(contains(out, "minutes must be a positive whole number"));
    CHECK(contains(out, "usage: cancel"));
    CHECK(contains(out, "usage: search"));
    CHECK(contains(out, "unknown command 'fly'"));

    std::ostringstream sink;
    CHECK(runCommand(desk, "help", sink));
    CHECK(!runCommand(desk, "quit", sink));
    CHECK(!runCommand(desk, "exit", sink));
    CHECK(runCommand(desk, "add Ana wifi 10\r", sink)); // Windows line ending
    CHECK(desk.waiting().size() == 1);
}

int main()
{
    testAdd();
    testServe();
    testSearch();
    testCancel();
    testCommandLine();
    testBadInput();

    std::cout << (checks - failures) << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
