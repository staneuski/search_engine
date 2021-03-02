#pragma once
#include <stack>
#include <vector>
#include <utility>

#include "search_server.h"

template <typename InputIt>
class IteratorRange {
public:
    IteratorRange(const InputIt begin, const InputIt end)
        : first_(begin), last_(end), size_(distance(first_, last_))
    {
    }

    inline InputIt begin() const noexcept {
        return first_;
    }

    inline InputIt end() const noexcept {
        return last_;
    }

private:
    InputIt first_, last_;
    size_t size_;
};

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server)
        : search_server_(search_server)
    {
    }

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query,
                                         DocumentPredicate document_predicate) {
        const std::vector<Document>& found_documents = search_server_.FindTopDocuments(
            raw_query,
            document_predicate
        );
        UpdateRequestQueue(found_documents.empty());
        return found_documents;
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query,
                                         DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    inline int GetNoResultRequests() const noexcept;

private:
    const static int sec_in_day_ = 1440;
    const SearchServer& search_server_;
    std::deque<bool> requests_;
    int no_result_count_ = 0;

    void UpdateRequestQueue(const bool is_empty);
};

//Не шаблонные функции лучше размещать в теле класа. Их обычно не выносят. Выносят только шаблонные.
int RequestQueue::GetNoResultRequests() const noexcept {
    return no_result_count_;
}