#pragma once
#include <string>
#include "TicketQueue.h"

// CSV format, one ticket per line, in line order (front first):
//
//   id,name,category,minutes,arrival
//   1,Ayham Makhamra,wifi,10,0
//
// The header line is optional. arrival (minutes after opening) is optional and defaults to 0.
// A field that contains a comma or a quote is written in quotes, with "" for a quote.

// Reads the file into out. On any error, out is left unchanged and error says what and where.
bool loadTickets(const std::string &path, TicketQueue &out, std::string &error);

// Writes the queue front to rear, with a header line.
bool saveTickets(const std::string &path, const TicketQueue &q, std::string &error);
