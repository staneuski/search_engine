#pragma once
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "document.h"
#include "request_queue.h"

/* --------------------------------- Input --------------------------------- */

std::string ReadLine() {
    std::string s;
    std::getline(std::cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    std::cin >> result;
    ReadLine();
    return result;
}


/* --------------------------------- Output -------------------------------- */

void PrintMatchDocumentResult(
    int document_id,
    const std::vector<std::string>& words,
    DocumentStatus status
)
{
    std::cout << "{ "
              << "document_id = " << document_id << ", "
              << "status = " << static_cast<int>(status) << ", "
              << "words =";
    for (const std::string& word : words)
        std::cout << ' ' << word;
    std::cout << '}' << std::endl;
}

std::ostream& operator<<(std::ostream& out, const Document& document) {
    out << "{ document_id = " << document.id
        << ", relevance = " << document.relevance
        << ", rating = " << document.rating
        << " }";
    return out;
}

template <typename T, typename S>
std::ostream& operator<<(std::ostream& out, const std::pair<T, S>& p) {
    out << p.first << ": " << p.second;
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

std::ostream& operator<<(std::ostream& out,
                         const std::vector<Document>& documents) {
    using namespace std::string_literals;
    return Print(out, documents, "\n"s);
}

template <typename InputIt>
std::ostream& operator<<(std::ostream& out,
                         const IteratorRange<InputIt> iterator_range) {
    for (auto it = iterator_range.begin(); it != iterator_range.end(); ++it)
        out << *it;
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
