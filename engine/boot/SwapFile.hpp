#pragma once

#include "vm/Program.hpp"

#include <filesystem>
#include <string>
#include <string_view>

// Text operations on .swap files. Boot pastes and cuts procedures while
// keeping the rest of the file (comments, formatting) untouched.
namespace PimpEx::boot {

// From the PROC line to END, inclusive.
std::string extract_proc_source(std::string_view text, const vm::Proc &proc);

// Appends after an empty line.
std::string append_proc(std::string_view text, std::string_view procSource);

// Cuts PROC..END and one empty line after it.
std::string remove_proc(std::string_view text, const vm::Proc &proc);

// All-or-nothing: a temp file, then rename, so nobody ever sees half a file.
bool write_file_atomic(const std::filesystem::path &path, std::string_view content);

// Lets the watcher ignore the engine's own writes.
std::size_t content_hash(std::string_view content);

} // namespace PimpEx::boot
