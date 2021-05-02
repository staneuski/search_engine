#pragma once
#include <map>
#include <mutex>
#include <string>
#include <vector>

using namespace std::string_literals;

const size_t DEFAULT_SIZE = 50;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    ConcurrentMap() = default;

    explicit ConcurrentMap(const size_t& bucket_count)
        : buckets_(bucket_count)
    {
    }

    struct Bucket {
        std::mutex m;
        std::map<Key, Value> dict;
    };

    struct Access {
        explicit Access(Bucket& bucket, const Key& key)
            : lock(bucket.m)
            , ref_to_value(bucket.dict[key])
        {
        }

        operator Value&();

        std::lock_guard<std::mutex> lock;
        Value& ref_to_value;
    };

    inline Access operator[](const Key& key) {
        return Access(GetBucket(key), key);
    }

    void erase(const Key& key) {
        Bucket& bucket = GetBucket(key);

        std::lock_guard<std::mutex> lock(bucket.m);
        bucket.dict.erase(key);
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> ordinary_map;
        for (Bucket& bucket : buckets_) {
            std::lock_guard<std::mutex> lock(bucket.m);
            ordinary_map.insert(bucket.dict.begin(), bucket.dict.end());
        }
        return ordinary_map;
    }

private:
    std::vector<Bucket> buckets_{DEFAULT_SIZE};

    inline Bucket& GetBucket(const Key& key) {
        return buckets_[static_cast<uint64_t>(key) % buckets_.size()];
    }
};