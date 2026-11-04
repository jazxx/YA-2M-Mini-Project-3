// M4 tests: the one-day simulation with 1 and 2 agents, checked against hand calculations.
#include <sstream>
#include <string>
#include "Simulator.h"
#include "TicketFile.h"
#include "CommandLine.h"
#include "check.h"

static TicketQueue day(const int (*rows)[2], int n) // rows of {arrival, minutes}
{
    TicketQueue q;
    for (int i = 0; i < n; i++)
        q.enqueue(Ticket(i + 1, "S" + std::to_string(i + 1), "wifi", rows[i][1], rows[i][0]));
    return q;
}

static bool near(double a, double b) { return a - b < 1e-9 && b - a < 1e-9; }

static void testHandExample()
{
    // three tickets: arrive 0, 0, 5; each needs 10 minutes
    const int rows[3][2] = {{0, 10}, {0, 10}, {5, 10}};
    TicketQueue q = day(rows, 3);
    SimResult r;

    // 1 agent: starts 0, 10, 20  -> waits 0, 10, 15
    CHECK(simulate(q, 1, r));
    CHECK(r.tickets == 3 && r.totalWait == 25);
    CHECK(near(r.averageWait, 25.0 / 3));
    CHECK(r.longestWait == 15 && r.longestWaitId == 3);
    CHECK(r.lastFinish == 30);

    // 2 agents: starts 0, 0, 10 -> waits 0, 0, 5
    CHECK(simulate(q, 2, r));
    CHECK(r.totalWait == 5 && near(r.averageWait, 5.0 / 3));
    CHECK(r.longestWait == 5 && r.longestWaitId == 3);
    CHECK(r.lastFinish == 20);

    CHECK(q.size() == 3); // the queue passed in is unchanged
}

static void testIdleAgent()
{
    // the agent is idle between tickets: nobody waits
    const int rows[2][2] = {{0, 5}, {20, 5}};
    SimResult r;
    CHECK(simulate(day(rows, 2), 1, r));
    CHECK(r.totalWait == 0 && r.longestWait == 0 && r.longestWaitId == 1);
    CHECK(r.lastFinish == 25);
}

static void testEdgeCases()
{
    SimResult r;
    TicketQueue empty;
    CHECK(simulate(empty, 1, r));
    CHECK(r.tickets == 0 && r.averageWait == 0 && r.longestWaitId == -1);

    const int rows[1][2] = {{0, 5}};
    CHECK(!simulate(day(rows, 1), 0, r));
    CHECK(!simulate(day(rows, 1), MAX_AGENTS + 1, r));
    CHECK(simulate(day(rows, 1), MAX_AGENTS, r));
}

static void testSampleDay()
{
    TicketQueue q;
    std::string error;
    CHECK(loadTickets("data/sample_day.csv", q, error)); // run the tests from the project folder
    CHECK(q.size() > 0);

    SimResult one, two;
    CHECK(simulate(q, 1, one) && simulate(q, 2, two));
    CHECK(one.tickets == q.size() && two.tickets == q.size());
    CHECK(two.averageWait <= one.averageWait); // more agents never makes FIFO waits longer
    CHECK(two.longestWait <= one.longestWait);

    HelpDesk desk;
    std::ostringstream out;
    runCommand(desk, "simulate data/sample_day.csv", out);
    CHECK(out.str().find("agents  avg wait") != std::string::npos);
    CHECK(desk.waiting().isEmpty()); // simulate does not touch the desk
}

int main()
{
    testHandExample();
    testIdleAgent();
    testEdgeCases();
    testSampleDay();

    std::cout << (checks - failures) << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
