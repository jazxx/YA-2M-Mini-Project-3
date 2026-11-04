# Team: YA^2M: Yasmein Asfour & Ayham Makhamra

# helpdesk-queue-cpp

A help desk ticket system in C++17, built on my own linked **queue** and **stack** classes
(no `std::queue` or `std::stack`). Mini project for EECE 7205 Fundamentals of Computer
Engineering, Northeastern University.

## The problem

A university IT help desk receives requests all day: password resets, broken printers, Wi-Fi
problems. Tickets must be served in **arrival order**, agents must be able to **undo** their
last action, and the manager wants to **search**, **cancel** and **print** tickets at any time.
At closing time the waiting line is saved to a file, and a whole day can be replayed to see how
long students wait with one or two agents.

## Class design

| Class | Structure | Role |
|---|---|---|
| `Ticket` | plain class | id, student name, category, minutes needed, arrival minute |
| `TicketQueue` | linked FIFO queue (front and rear pointers) | the waiting line, and the served log |
| `ActionStack` | linked LIFO stack (head = top) | the undo history |
| `HelpDesk` | owns one waiting queue, one served queue, one stack | the commands |
| `CommandLine` | | parses one line of input and prints the result |
| `TicketFile` | | CSV load and save |
| `Simulator` | uses a `TicketQueue` | replays one day with *k* agents |

### Which operation uses the queue and which uses the stack

| Command | Queue (waiting line) | Stack (history) | Cost |
|---|---|---|---|
| `add` | `enqueue` at the rear | push `Add` | O(1) |
| `serve` | `dequeue` from the front, `enqueue` on the served log | push `Serve` | O(1) |
| `print` | walk front to rear | | O(n) |
| `search id` / `search name` | walk from the front, summing minutes | | O(n) |
| `cancel id` | unlink the node, remember its position | push `Cancel` (ticket and position) | O(n) |
| `undo` | reverse the action (below) | **pop** | O(1) or O(n) |
| `load` / `save` | rebuild / walk the line | load clears it | O(n) |
| `simulate` | dequeue the day's tickets in order | | O(n·k) |

Undo works because the stack gives actions back in reverse order (LIFO), so each action is
undone on exactly the state it produced:

| Undone action | What undo does | Queue operation |
|---|---|---|
| add | take the ticket off the rear and give its id back | `removeRear` |
| serve | take it off the served log and put it back at the front | `pushFront` |
| cancel | put the ticket back at the position it had | `insertAt(position)` |

**Why a linked queue and not a circular array?** Cancel deletes from the middle and undo
inserts into the middle, at the front, or removes the rear. In a linked list these are pointer
changes; in a circular array every one of them would shift elements across the wrap-around
point. Add and serve stay O(1) either way. The stack is linked too, so the undo history has no
fixed capacity.

## File format

`data/sample_day.csv` holds one day (42 tickets). Columns: `id,name,category,minutes,arrival`,
where `arrival` is the minute after opening (the desk opens at 09:00). The header and the
`arrival` column are optional; fields with a comma or a quote are written in quotes.

`load` replaces the waiting line, clears the served log and the undo history (only add, serve
and cancel are undoable), and continues new ids after the largest id in the file. A file with
an error is rejected as a whole, with its line number, and nothing changes.

## Build and run

Requirements: a C++17 compiler (`g++` or `clang++`). `make` is optional. Run every command
from the project folder (the one with this README), because the program and the M4 tests read
files from `data/`.

### With make

```
make            # builds the main program ./helpdesk
make test       # builds and runs the four test programs, one per milestone
make clean      # removes the built programs
```

### Without make (e.g. Windows)

The main program:

```
g++ -std=c++17 -Wall -Wextra -pedantic -Isrc src/*.cpp -o helpdesk
```

Each test program is its test file plus every source file except `main.cpp`:

```
g++ -std=c++17 -Wall -Wextra -pedantic -Isrc tests/test_m1.cpp src/TicketQueue.cpp src/ActionStack.cpp src/HelpDesk.cpp src/CommandLine.cpp src/TicketFile.cpp src/Simulator.cpp -o test_m1
./test_m1
```

Repeat with `test_m2`, `test_m3` and `test_m4`. On Windows PowerShell, run the programs as
`.\test_m1.exe` and `.\helpdesk.exe`.

### Running the tests

Each test program prints how many of its checks passed. A check is one `CHECK(condition)` in the
test file: one expected result. Passing output:

```
./test_m1
89/89 checks passed
./test_m2
67/67 checks passed
./test_m3
81/81 checks passed
./test_m4
26/26 checks passed
```

A failing check prints its file and line, e.g. `FAIL tests/test_m2.cpp:97: ...`, and the
program exits with code 1, so `make test` stops at the first milestone that fails.

### Running the main program

```
./helpdesk
```

It prints a `>` prompt; type commands, or `help` to list them:

| Command | What it does |
|---|---|
| `add <name> <category> <minutes>` | add a ticket at the rear (the name may have spaces) |
| `serve` | serve the front ticket |
| `print` | list the waiting tickets, front to rear, with the total minutes |
| `served` | list the served tickets |
| `search id <id>` / `search name <name>` | position in the line and estimated wait |
| `cancel <id>` | remove a waiting ticket; the others keep their order |
| `undo` | reverse the last add, serve or cancel |
| `load <file.csv>` / `save <file.csv>` | read a day of tickets / save the waiting line |
| `simulate <file.csv> [agents]` | waiting times for a day (default: 1 and 2 agents) |
| `quit` | exit |

To try it quickly:

```
load data/demo_day.csv
print
add Ayham Makhamra wifi 10
cancel 3
undo
simulate data/sample_day.csv
quit
```

To replay the sample session below from a file:

```
./helpdesk < data/sample_session.txt                       # macOS, Linux, Git Bash
Get-Content data\sample_session.txt | .\helpdesk.exe      # Windows PowerShell
```

The session ends with `save closing.csv`, so it also writes that file.

## Sample session

Using `data/demo_day.csv` (the first 8 tickets of the sample day):

```
Help desk ready. Type 'help' for commands.
> load data/demo_day.csv
loaded 8 ticket(s) from data/demo_day.csv (served log and undo history cleared)
> search name Kofi Kim
#4 Kofi Kim (printer, 18 min): position 4 of 8, estimated wait 34 min
> serve
served #1 Rosa Walsh (account, 14 min)
> serve
served #2 Jade Okafor (printer, 11 min)
> add Ayham Makhamra wifi 10
added #9 Ayham Makhamra (wifi, 10 min): position 7 of 7, estimated wait 78 min
> cancel 5
cancelled #5 Ximena Chang (account, 14 min) (was at position 3)
> print
  pos    id  name                category      min
    1     3  Dina Patel          wifi            9
    2     4  Kofi Kim            printer        18
    3     6  Wen Okafor          account        10
    4     7  Nora Huang          software       21
    5     8  Ivan Garcia         email           6
    6     9  Ayham Makhamra      wifi           10
  6 ticket(s) waiting, total 74 min
> undo
undid cancel: #5 Ximena Chang (account, 14 min) is back at position 3
> undo
undid add: #9 Ayham Makhamra (wifi, 10 min) removed from the line
> undo
undid serve: #2 Jade Okafor (printer, 11 min) is back at the front
> search id 5
#5 Ximena Chang (account, 14 min): position 4 of 7, estimated wait 38 min
> save closing.csv
saved 7 waiting ticket(s) to closing.csv
bye
```

The three undos in a row restore the line exactly as it was after the first `serve`.

## Waiting-time results (M4)

```
> simulate data/sample_day.csv
simulation of data/sample_day.csv: 42 ticket(s), 509 min of work, opening at 09:00
  agents  avg wait (min)  longest (min)   ticket  last done
       1           57.0           116      #36   544 (18:04)
       2            6.5            28      #11   465 (16:45)
```

| Agents | Average wait | Longest wait | Last ticket done |
|---|---|---|---|
| 1 | 57.0 min | 116 min (ticket #36) | 18:04 |
| 2 | 6.5 min | 28 min (ticket #11) | 16:45 |

**Method.** Tickets are served in line order. Each ticket goes to the agent who is free
first and starts at `max(arrival, time that agent is free)`; its wait is `start − arrival`.

**Reading the results.** The day has 509 minutes of work in a 480-minute day, so one agent is
overloaded: the line keeps growing through the busy morning and after lunch, and the last
ticket finishes an hour after closing. A second agent cuts the average wait by almost 90%,
because two agents have 960 minutes of capacity for the same 509 minutes of work, so the line
rarely builds up.

## Tests

| File | Milestone | Checks |
|---|---|---|
| `tests/test_m1.cpp` | queue and stack: empty, one element, many, 10,000 elements (linked, so never "full"), deep copy | 89 |
| `tests/test_m2.cpp` | commands; cancel at front, middle and rear keeps the others in order; bad input | 67 |
| `tests/test_m3.cpp` | undo for each command; three undos in a row restore the queue exactly; CSV round trip and bad files | 81 |
| `tests/test_m4.cpp` | simulation against hand-calculated waits; the sample day | 26 |

All tests pass, the build has no warnings with `-Wall -Wextra -pedantic`, and AddressSanitizer
reports no leaks or memory errors.

## Project structure

```
src/    Ticket.h  TicketQueue.h/.cpp  ActionStack.h/.cpp  HelpDesk.h/.cpp
        CommandLine.h/.cpp  TicketFile.h/.cpp  Simulator.h/.cpp  main.cpp
tests/  check.h  test_m1.cpp  test_m2.cpp  test_m3.cpp  test_m4.cpp
data/   sample_day.csv  demo_day.csv  sample_session.txt
```
