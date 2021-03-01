#pragma once
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "../document/document.h"
#include "../request_queue/request_queue.h"

//Не используйте в названии файлов слово functions, лучше utils или helpers, т.к. functions звучит неоднозначно
//либо функциональность, либо функции.

void PrintMatchDocumentResult(
    int document_id,
    const std::vector<std::string>& words,
    DocumentStatus status
);

std::ostream& operator<<(std::ostream& out, const Document& document);

std::ostream& operator<<(std::ostream& out, const std::vector<Document>& documents);

template <typename InputIt>
std::ostream& operator<<(std::ostream& out,
                         const IteratorRange<InputIt> iterator_range) {
    for (auto it = iterator_range.begin(); it != iterator_range.end(); ++it) {
        out << *it;
    }
    return out;
}

template <typename T>
std::ostream& Print(std::ostream& out, const T& container,
                    const std::string& delimeter = ", ") {
    bool is_first = true;
    for (const auto& element : container) {
        if (is_first) {
            out << element;
            is_first = false;
        } else {
            out << delimeter << element;
        }
    }
    return out;
}

template <typename T, typename S>
std::ostream& operator<<(std::ostream& out, const std::pair<T, S>& p) {
    out << p.first << ": " << p.second;
    return out;
}

template <typename T>
std::ostream& operator<<(std::ostream& out, const std::vector<T>& container) {
    out << '[';
    Print(out, container);
    return out << ']';
}

template <typename T>
std::ostream& operator<<(std::ostream& out, const std::set<T>& container) {
    out << '{';
    Print(out, container);
    return out << '}';
}

template <typename Key, typename Value>
std::ostream& operator<<(std::ostream& out, const std::map<Key, Value>& container) {
    out << '{';
    Print(out, container);
    return out << '}';
}
