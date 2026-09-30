#pragma once

#include "vm/Program.hpp"

#include <filesystem>
#include <string>
#include <string_view>

// Operacje na tekście pliku .swap. Boot wkleja i wycina procedury,
// zachowując resztę pliku (komentarze, formatowanie) bez zmian.
namespace PimpEx::boot {

// Tekst procedury (od linii PROC do END włącznie).
std::string extract_proc_source(std::string_view text, const vm::Proc &proc);

// Dokleja procedurę na koniec pliku, oddzieloną pustą linią.
std::string append_proc(std::string_view text, std::string_view procSource);

// Wycina procedurę (linie PROC..END) i jedną pustą linię po niej.
std::string remove_proc(std::string_view text, const vm::Proc &proc);

// Zapis „wszystko albo nic”: najpierw plik tymczasowy, potem rename.
// Edytor ani watcher nie zobaczą nigdy połowy pliku.
bool write_file_atomic(const std::filesystem::path &path, std::string_view content);

// Szybki hash treści – watcher porównuje go, żeby nie wczytywać
// zmian, które zapisał sam silnik.
std::size_t content_hash(std::string_view content);

} // namespace PimpEx::boot
