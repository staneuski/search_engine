#pragma once
#include <vector>

#include "../request_queue/request_queue.h"

template <typename InputIt>
class Paginator {
public:
    Paginator(const InputIt begin, const InputIt end, const size_t page_size) {
        InputIt page_end = begin;
        while (distance(page_end, end) > 0) {
            InputIt page_begin = page_end;
            advance(page_end, page_size);
            pages_.push_back({page_begin, page_end});
        }
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

private:
    std::vector<IteratorRange<InputIt>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}