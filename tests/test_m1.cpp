// M1 unit tests: Ticket, TicketQueue and ActionStack on empty, one-element and many-element
// structures. Both structures are linked, so they are never "full"; instead we test a large
// size (10,000 elements) to show they keep working without a capacity limit.
#include <iostream>
#include <sstream>
#include <string>
#include "Ticket.h"
#include "TicketQueue.h"
#include "ActionStack.h"
#include "check.h"

static Ticket mk(int id, int minutes = 10)
{
    return Ticket(id, "Student" + std::to_string(id), "wifi", minutes);
}

// queue with ids 1..n and minutes 5, 10, 15, ...
static TicketQueue makeQueue(int n)
{
    TicketQueue q;
    for (int i = 1; i <= n; i++)
        q.enqueue(mk(i, i * 5));
    return q;
}

// "1 2 3" — the ids from front to rear
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

static void testQueueEmpty()
{
    TicketQueue q;
    Ticket t;
    int pos = 0;
    CHECK(q.isEmpty());
    CHECK(q.size() == 0);
    CHECK(!q.dequeue(t));   // underflow is refused, not a crash
    CHECK(!q.peek(t));
    CHECK(!q.removeRear(t));
    CHECK(!q.removeById(1, t, pos));
    CHECK(!q.get(1, t));
    CHECK(q.searchById(1).position == -1);
    CHECK(q.searchByName("Nobody").position == -1);
    CHECK(q.totalMinutes() == 0);
    CHECK(!q.insertAt(0, mk(1)));
    CHECK(!q.insertAt(2, mk(1)));
    CHECK(q.insertAt(1, mk(1))); // position 1 is valid on an empty queue
    CHECK(ids(q) == "1");
}

static void testQueueOne()
{
    TicketQueue q;
    Ticket t;
    int pos = 0;
    q.enqueue(mk(7, 15));
    CHECK(!q.isEmpty());
    CHECK(q.size() == 1);
    CHECK(q.peek(t) && t.id == 7);
    CHECK(q.searchById(7).position == 1);
    CHECK(q.searchById(7).waitMinutes == 0);
    CHECK(q.searchByName("Student7").position == 1);
    CHECK(q.totalMinutes() == 15);

    // cancel the only ticket: front and rear must both reset
    CHECK(q.removeById(7, t, pos) && t.id == 7 && pos == 1);
    CHECK(q.isEmpty());
    q.enqueue(mk(8));
    CHECK(ids(q) == "8");
    CHECK(q.dequeue(t) && t.id == 8 && q.isEmpty());

    // pushFront and removeRear on a one-element queue
    q.pushFront(mk(9));
    CHECK(ids(q) == "9");
    CHECK(q.removeRear(t) && t.id == 9 && q.isEmpty());
    q.enqueue(mk(10));
    CHECK(ids(q) == "10");
}

static void testQueueMany()
{
    TicketQueue q = makeQueue(5); // minutes 5 10 15 20 25
    Ticket t;
    int pos = 0;
    CHECK(q.size() == 5);
    CHECK(ids(q) == "1 2 3 4 5");
    CHECK(q.totalMinutes() == 75);

    // search: position and wait = sum of the minutes in front
    SearchResult r = q.searchById(4);
    CHECK(r.position == 4 && r.waitMinutes == 5 + 10 + 15);
    r = q.searchByName("Student5");
    CHECK(r.position == 5 && r.waitMinutes == 50);
    CHECK(q.searchById(99).position == -1);

    // cancel at the front, middle and rear; the others keep their order
    CHECK(q.removeById(1, t, pos) && pos == 1);
    CHECK(ids(q) == "2 3 4 5");
    CHECK(q.removeById(3, t, pos) && pos == 2);
    CHECK(ids(q) == "2 4 5");
    CHECK(q.removeById(5, t, pos) && pos == 3);
    CHECK(ids(q) == "2 4");
    q.enqueue(mk(6)); // rear pointer must have moved back correctly
    CHECK(ids(q) == "2 4 6");

    // insertAt puts tickets back exactly where they were
    CHECK(q.insertAt(2, mk(3)));
    CHECK(ids(q) == "2 3 4 6");
    CHECK(q.insertAt(1, mk(1)));
    CHECK(ids(q) == "1 2 3 4 6");
    CHECK(q.insertAt(5, mk(5)));
    CHECK(ids(q) == "1 2 3 4 5 6");
    CHECK(!q.insertAt(8, mk(8)));

    // removeRear and pushFront
    CHECK(q.removeRear(t) && t.id == 6);
    CHECK(ids(q) == "1 2 3 4 5");
    q.pushFront(mk(0));
    CHECK(ids(q) == "0 1 2 3 4 5");
    q.enqueue(mk(6)); // rear still correct after pushFront
    CHECK(ids(q) == "0 1 2 3 4 5 6");

    // FIFO order on dequeue
    std::string order;
    while (q.dequeue(t))
        order += std::to_string(t.id) + " ";
    CHECK(order == "0 1 2 3 4 5 6 ");
    CHECK(q.isEmpty());
}

static void testQueueLarge()
{
    const int N = 10000;
    TicketQueue q = makeQueue(N);
    Ticket t;
    CHECK(q.size() == N);
    CHECK(q.searchById(N).position == N);
    bool inOrder = true;
    for (int i = 1; i <= N; i++)
        if (!q.dequeue(t) || t.id != i)
            inOrder = false;
    CHECK(inOrder);
    CHECK(q.isEmpty());
}

static void testQueueCopy()
{
    TicketQueue a = makeQueue(3);
    TicketQueue b = a; // copy constructor
    CHECK(a.equals(b));
    Ticket t;
    a.dequeue(t);
    CHECK(ids(a) == "2 3");
    CHECK(ids(b) == "1 2 3"); // deep copy: b is unchanged
    CHECK(!a.equals(b));

    b = a; // copy assignment
    CHECK(a.equals(b));
    TicketQueue &alias = b;
    b = alias; // self-assignment is safe
    CHECK(ids(b) == "2 3");
}

static void testQueuePrint()
{
    std::ostringstream out;
    TicketQueue q;
    q.print(out);
    CHECK(out.str().find("no tickets") != std::string::npos);

    out.str("");
    q = makeQueue(5);
    q.print(out);
    CHECK(out.str().find("5 ticket(s) waiting, total 75 min") != std::string::npos);
    CHECK(ids(q) == "1 2 3 4 5"); // printing does not change the queue
}

static void testStackEmpty()
{
    ActionStack s;
    Action a;
    CHECK(s.isEmpty());
    CHECK(s.size() == 0);
    CHECK(!s.pop(a)); // underflow is refused
    CHECK(!s.peek(a));
}

static void testStackOne()
{
    ActionStack s;
    Action a;
    s.push({ActionType::Cancel, mk(3), 2});
    CHECK(!s.isEmpty() && s.size() == 1);
    CHECK(s.peek(a) && a.type == ActionType::Cancel && a.ticket.id == 3 && a.position == 2);
    CHECK(s.size() == 1); // peek does not remove
    CHECK(s.pop(a) && a.ticket.id == 3);
    CHECK(s.isEmpty());
    CHECK(!s.pop(a));
}

static void testStackMany()
{
    ActionStack s;
    Action a;
    s.push({ActionType::Add, mk(1), 0});
    s.push({ActionType::Serve, mk(2), 0});
    s.push({ActionType::Cancel, mk(3), 4});
    CHECK(s.size() == 3);

    // LIFO: last action comes back first
    CHECK(s.pop(a) && a.type == ActionType::Cancel && a.ticket.id == 3);
    CHECK(s.pop(a) && a.type == ActionType::Serve && a.ticket.id == 2);
    CHECK(s.pop(a) && a.type == ActionType::Add && a.ticket.id == 1);
    CHECK(s.isEmpty());

    s.push({ActionType::Add, mk(9), 0});
    s.clear();
    CHECK(s.isEmpty() && s.size() == 0);

    CHECK(toString(ActionType::Add) == "add");
    CHECK(toString(ActionType::Serve) == "serve");
    CHECK(toString(ActionType::Cancel) == "cancel");
}

static void testStackLarge()
{
    const int N = 10000;
    ActionStack s;
    for (int i = 1; i <= N; i++)
        s.push({ActionType::Add, mk(i), 0});
    CHECK(s.size() == N);
    Action a;
    bool reversed = true;
    for (int i = N; i >= 1; i--)
        if (!s.pop(a) || a.ticket.id != i)
            reversed = false;
    CHECK(reversed);
    CHECK(s.isEmpty());
}

int main()
{
    testQueueEmpty();
    testQueueOne();
    testQueueMany();
    testQueueLarge();
    testQueueCopy();
    testQueuePrint();
    testStackEmpty();
    testStackOne();
    testStackMany();
    testStackLarge();

    std::cout << (checks - failures) << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
