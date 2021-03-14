#pragma once
#include <chrono>
#include <iostream>

#define PROFILE_CONCAT_INTERNAL(X, Y) X##Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(x, stdout) LogDuration UNIQUE_VAR_NAME_PROFILE(x, stdout)
#define LOG_DURATION_STDERR(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)
#define LOG_DURATION_DEFAULT LogDuration UNIQUE_VAR_NAME_PROFILE

class LogDuration {
public:
    using Clock = std::chrono::steady_clock;

    LogDuration() = default;

    explicit LogDuration(const std::string& query) {
        out_ << "Documents matching for the query: " << query << std::endl;
    }

    explicit LogDuration(const std::string& query, std::ostream& out)
        : out_(out)
    {
        out_ << "Documents matching for the query: " << query << std::endl;
    }

    ~LogDuration() {
        using namespace std::chrono;
        using namespace std::literals;

        out_ << "Operation time: "
             << duration_cast<milliseconds>(Clock::now() - start_time_).count() << " ms"
             << std::endl;
    }

private:
    const Clock::time_point start_time_ = Clock::now();
    std::ostream& out_ = std::cerr;
};