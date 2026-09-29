#pragma once

// Duplicates everything written to stdout/stderr (printf, std::cout, std::cerr, ...) to both the
// original console and a log file:
//   Windows: %LOCALAPPDATA%/EWOCvj2/EWOCvj2.log
//   macOS:   ~/Library/Logs/EWOCvj2/EWOCvj2.log
//   Linux:   ~/.ewocvj2/EWOCvj2.log
// "Console" is wherever the output went before: a terminal, an IDE run window or a redirect. On Windows
// (GUI subsystem) it is the inherited std handles, else the console of the launching terminal, else none.

// Call once, as early as possible in main/WinMain.
void logtee_start();

// Waits (up to timeout_ms) until everything written so far has reached console and log file.
// flushstreams = false from crash handlers: don't touch the CRT stream locks of a crashing process.
void logtee_flush(int timeout_ms = 500, bool flushstreams = true);
