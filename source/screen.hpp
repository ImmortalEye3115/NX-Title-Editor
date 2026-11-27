// NX Title Editor
// Copyright (C) 2026 ImmortalEye
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
// The Switch-side terminal screen (80x45 cells of 16x16 px).
//
//  rows 1-3  [bar] NX Title Editor v0.6.3                    Ready  |
//  row 6+    [QR code]   A title editor powered by icon sysmodules
//
//                        Network connected
//
//                        76 games (from cache)   <- progress bar while scanning
//
//                        Activity
//                        [band] Time   Action    Title             Item
//                               00:05  wrote     Balatro           Home
//            Open on your phone or PC            (one entry every other row)
//            http://192.168.1.42:8420
//  rows 43-45 [bar] [A] Full rescan   [-] Reboot   [+] Exit
//
// No ASCII lines: the console font is heavy and its line characters don't
// join, so bars and bands are solid background colour.
// Pressing - swaps in a full-screen reboot confirmation (header kept).
// The spinner only moves while the main loop runs, so a frozen spinner next
// to a job label means "busy with this", and a frozen "Ready" means a hang.

#include <string>

namespace screen {

enum class LogKind { Ok, Error, Info };
enum class Mode { Normal, ConfirmReboot, Rebooting };

void init(const char* version);
void showAddress(const std::string& url, const std::string& note);
void showError(const std::string& line1, const std::string& line2 = "");

// Status next to the spinner. busy=true (orange) for jobs, false (white)
// for states waiting on you.
void setStatus(const std::string& status, bool busy);
// Back to the resting status for the current mode: "Ready" / "Confirm reboot".
void setIdle();
// Advances the spinner if enough time has passed. Call often.
void tick();

void setGamesLine(const std::string& line);
void progress(int done, int total);  // draws the bar on the games line and ticks

// A table row: Time | Action | Title | Item (Home, Apps, Both or Info)
void logAction(LogKind kind, const std::string& action, const std::string& title, const std::string& item,
               const std::string& titleId = "");
// A row whose message spans the Title and Item columns (rescan, errors...).
// Every row is also written to sdmc:/config/nx-title-editor/log.txt; error rows
// show "Check log" on screen and the full message only in the file.
void logMessage(LogKind kind, const std::string& action, const std::string& message);
// Writes a row to log.txt only (the screen row was merged with the one before).
void logFileOnly(const std::string& action, const std::string& title, const std::string& item,
                 const std::string& titleId = "");
// Changes the Item column of the newest row (used to merge Home + Apps into Both).
void setLastIcon(const std::string& item);
// Increases by one every time a row is added.
unsigned logCount();

// "Network connected" (green) or "Waiting for network..." (orange).
void setNetwork(bool ok);

Mode mode();
void showRebootConfirm();
void showNormal();
void showRebooting();

// Keeps printable ASCII only (the console font can't show ™, ®, CJK...).
std::string ascii(const std::string& s);

}  // namespace screen
