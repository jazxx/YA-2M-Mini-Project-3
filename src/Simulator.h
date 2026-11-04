#pragma once
#include "TicketQueue.h"

// Result of simulating one day.
struct SimResult
{
    int agents = 0;
    int tickets = 0;
    int totalWait = 0;      // minutes, summed over all tickets
    double averageWait = 0; // minutes
    int longestWait = 0;    // minutes
    int longestWaitId = -1; // ticket that waited longest (-1 if there were no tickets)
    int lastFinish = 0;     // minute (after opening) when the last ticket is done
};

const int MAX_AGENTS = 16;

// Serves the day's tickets in line order with the given number of agents.
// Each ticket goes to the agent who is free first; it starts at
//   start = max(its arrival, when that agent is free)
// and waits (start - arrival) minutes. The queue passed in is not changed.
// Returns false if agents is not in 1..MAX_AGENTS.
bool simulate(const TicketQueue &day, int agents, SimResult &result);
