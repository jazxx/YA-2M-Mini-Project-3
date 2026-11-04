#include "Simulator.h"

bool simulate(const TicketQueue &day, int agents, SimResult &result)
{
    if (agents < 1 || agents > MAX_AGENTS)
        return false;

    result = SimResult();
    result.agents = agents;

    int freeAt[MAX_AGENTS] = {0}; // minute at which each agent is next free
    TicketQueue line = day;       // work on a copy: the waiting line of the day
    Ticket t;

    while (line.dequeue(t)) // FIFO: tickets are served in arrival order
    {
        int a = 0; // the agent who is free first
        for (int i = 1; i < agents; i++)
            if (freeAt[i] < freeAt[a])
                a = i;

        int start = t.arrival > freeAt[a] ? t.arrival : freeAt[a];
        int wait = start - t.arrival;
        freeAt[a] = start + t.minutes;

        result.tickets++;
        result.totalWait += wait;
        if (wait > result.longestWait || result.longestWaitId == -1)
        {
            result.longestWait = wait;
            result.longestWaitId = t.id;
        }
        if (freeAt[a] > result.lastFinish)
            result.lastFinish = freeAt[a];
    }

    if (result.tickets > 0)
        result.averageWait = static_cast<double>(result.totalWait) / result.tickets;
    return true;
}
