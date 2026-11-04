// M3 tests: undo for every command, and CSV load / save.
// Checkpoint: three undos in a row restore the earlier queue exactly.
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include "HelpDesk.h"
#include "CommandLine.h"
#include "TicketFile.h"
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

static void fill(HelpDesk &desk)
{
    Ticket t;
    const char *names[5] = {"Ana", "Ben", "Chen", "Dina", "Eli"};
    for (int i = 0; i < 5; i++)
        desk.add(names[i], "wifi", (i + 1) * 5, t);
}

static void writeFile(const std::string &path, const std::string &text)
{
    std::ofstream f(path);
    f << text;
}

static std::string readFile(const std::string &path)
{
    std::ifstream f(path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static void testUndoEach()
{
    HelpDesk desk;
    Action a;
    Ticket t;
    int pos = 0;
    CHECK(!desk.undo(a)); // nothing to undo

    // undo add: ticket gone, id given back
    fill(desk);
    CHECK(desk.undo(a) && a.type == ActionType::Add && a.ticket.id == 5);
    CHECK(ids(desk.waiting()) == "1 2 3 4");
    CHECK(desk.nextId() == 5);
    CHECK(desk.add("Eve", "wifi", 5, t) && t.id == 5);

    // undo serve: back at the front, off the served log
    desk.serve(t);
    CHECK(ids(desk.served()) == "1");
    CHECK(desk.undo(a) && a.type == ActionType::Serve && a.ticket.id == 1);
    CHECK(ids(desk.waiting()) == "1 2 3 4 5");
    CHECK(desk.served().isEmpty());

    // undo cancel at the front, middle and rear: back at the old position
    int cases[3] = {1, 3, 5};
    for (int id : cases)
    {
        CHECK(desk.cancel(id, t, pos));
        CHECK(desk.undo(a) && a.type == ActionType::Cancel && a.position == id);
        CHECK(ids(desk.waiting()) == "1 2 3 4 5");
    }

    // undo everything that is left: 5 adds (the cancel/undo pairs and serve/undo removed themselves)
    int n = 0;
    while (desk.undo(a))
        n++;
    CHECK(n == 5);
    CHECK(desk.waiting().isEmpty() && desk.nextId() == 1);
}

static void testThreeUndos()
{
    HelpDesk desk;
    fill(desk);
    Ticket t;
    int pos = 0;
    desk.serve(t);

    // snapshot of the earlier state
    TicketQueue waitingBefore = desk.waiting();
    TicketQueue servedBefore = desk.served();
    int nextIdBefore = desk.nextId();
    int historyBefore = desk.historySize();

    // three different commands...
    desk.add("Fay", "printer", 30, t); // 2 3 4 5 6
    desk.cancel(3, t, pos);            // 2 4 5 6
    desk.serve(t);                     // 4 5 6, served 1 2
    CHECK(ids(desk.waiting()) == "4 5 6");

    // ...and three undos in a row
    Action a;
    CHECK(desk.undo(a) && a.type == ActionType::Serve);
    CHECK(desk.undo(a) && a.type == ActionType::Cancel);
    CHECK(desk.undo(a) && a.type == ActionType::Add);

    CHECK(desk.waiting().equals(waitingBefore)); // every field, every position
    CHECK(desk.served().equals(servedBefore));
    CHECK(desk.nextId() == nextIdBefore);
    CHECK(desk.historySize() == historyBefore);

    // the same with two cancels and an add, in another order
    waitingBefore = desk.waiting();
    desk.cancel(5, t, pos);
    desk.add("Gus", "email", 8, t);
    desk.cancel(2, t, pos);
    desk.undo(a);
    desk.undo(a);
    desk.undo(a);
    CHECK(desk.waiting().equals(waitingBefore));
}

static void testSaveLoad()
{
    const std::string path = "test_m3_tmp.csv";
    HelpDesk desk;
    Ticket t;
    desk.add("Ayham Makhamra", "wifi", 10, t, 3);
    desk.add("O\"Neil, Pat", "printer", 20, t, 7); // comma and quote in the name
    desk.add("Chen", "password", 5, t, 9);

    std::string error;
    CHECK(desk.save(path, error));
    CHECK(readFile(path) == "id,name,category,minutes,arrival\n"
                            "1,Ayham Makhamra,wifi,10,3\n"
                            "2,\"O\"\"Neil, Pat\",printer,20,7\n"
                            "3,Chen,password,5,9\n");

    HelpDesk other;
    other.add("Zed", "wifi", 1, t);
    other.serve(t);
    int count = 0;
    CHECK(other.load(path, count, error) && count == 3);
    CHECK(other.waiting().equals(desk.waiting())); // round trip is exact
    CHECK(other.served().isEmpty());
    CHECK(other.historySize() == 0);
    Action a;
    CHECK(!other.undo(a));
    CHECK(other.add("New", "wifi", 5, t) && t.id == 4); // ids continue after the file

    // header optional, arrival optional, blank lines and Windows endings ok, ids need not be in order
    writeFile(path, "7,Ana,wifi,5\r\n\r\n3,Ben,email,8,12\r\n");
    CHECK(other.load(path, count, error) && count == 2);
    CHECK(ids(other.waiting()) == "7 3");
    CHECK(other.add("Cy", "wifi", 5, t) && t.id == 8);

    std::remove(path.c_str());
}

static void testLoadErrors()
{
    const std::string path = "test_m3_tmp.csv";
    HelpDesk desk;
    fill(desk);
    TicketQueue before = desk.waiting();
    int count = 0;
    std::string error;

    const char *bad[] = {
        "1,Ana,wifi\n",               // too few fields
        "1,Ana,wifi,5,0,extra\n",     // too many
        "x,Ana,wifi,5\n",             // id not a number
        "0,Ana,wifi,5\n",             // id not positive
        "1,,wifi,5\n",                // empty name
        "1,Ana,wifi,0\n",             // minutes not positive
        "1,Ana,wifi,5,-2\n",          // negative arrival
        "1,Ana,wifi,5\n1,Ben,wifi,5\n", // duplicate id
        "1,\"Ana,wifi,5\n",           // unclosed quote
    };
    for (const char *text : bad)
    {
        writeFile(path, text);
        CHECK(!desk.load(path, count, error));
        CHECK(!error.empty());
        CHECK(desk.waiting().equals(before)); // a bad file changes nothing
    }
    writeFile(path, "id,name,category,minutes\n1,Ana,wifi,5\n2,Ben,wifi,abc\n");
    CHECK(!desk.load(path, count, error));
    CHECK(error.find("line 3") != std::string::npos); // error says where

    CHECK(!desk.load("no_such_file.csv", count, error));
    CHECK(desk.waiting().equals(before));
    CHECK(desk.historySize() == 5); // history kept after a failed load

    std::remove(path.c_str());
}

static void testCommandLine()
{
    HelpDesk desk;
    std::ostringstream out;
    std::string script = "add Ana wifi 5\nadd Ben printer 10\nadd Chen email 8\n"
                         "cancel 2\nserve\nundo\nundo\nundo\nundo\nundo\nundo\nundo\n"
                         "add Dina wifi 5\nsave test_m3_cli.csv\nserve\nload test_m3_cli.csv\n"
                         "load nope.csv\n";
    std::istringstream in(script);
    std::string line;
    while (std::getline(in, line))
        runCommand(desk, line, out);
    std::string s = out.str();

    CHECK(s.find("undid serve: #1 Ana (wifi, 5 min) is back at the front") != std::string::npos);
    CHECK(s.find("undid cancel: #2 Ben (printer, 10 min) is back at position 2") != std::string::npos);
    CHECK(s.find("undid add: #3 Chen") != std::string::npos);
    CHECK(s.find("nothing to undo") != std::string::npos);
    CHECK(s.find("added #1 Dina") != std::string::npos); // ids were given back
    CHECK(s.find("saved 1 waiting ticket(s) to test_m3_cli.csv") != std::string::npos);
    CHECK(s.find("loaded 1 ticket(s)") != std::string::npos);
    CHECK(s.find("error: cannot open nope.csv (nothing changed)") != std::string::npos);
    CHECK(ids(desk.waiting()) == "1");
    std::remove("test_m3_cli.csv");
}

int main()
{
    testUndoEach();
    testThreeUndos();
    testSaveLoad();
    testLoadErrors();
    testCommandLine();

    std::cout << (checks - failures) << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
