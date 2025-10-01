#include "fuzzy.hpp"

#include <exception>
#include <expected>
#include <functional>
#include <queue>
#include <string>

#include <rapidfuzz/fuzz.hpp>

namespace core
{

struct ScoredString
{
    const std::string* string;
    double             score;
};

bool operator<(const ScoredString& lhs, const ScoredString& rhs)
{
    return lhs.score < rhs.score;
}

bool operator>(const ScoredString& lhs, const ScoredString& rhs)
{
    return lhs.score > rhs.score;
}

using PriorityQueue = std::priority_queue<ScoredString>;
using ReversedPriorityQueue = std::priority_queue<
    ScoredString,
    std::vector<ScoredString>,
    std::greater<ScoredString>>;

using PriorityQueueOrError = std::expected<PriorityQueue, std::string>;
using ReversedPriorityQueueOrError = std::expected<ReversedPriorityQueue, std::string>;

template <typename Queue>
static StringRefsOrError extract(
    const std::string& query,
    const utils::Strings& choices,
    const double score_cutoff = 0.0)
{
    Queue results;

    try
    {
        rapidfuzz::fuzz::CachedWRatio<std::string::value_type> scorer(query);

        for (const auto& choice : choices)
        {
            double score = scorer.similarity(choice, score_cutoff);

            if (score >= score_cutoff)
            {
                results.emplace(ScoredString{.string = &choice, .score = score});
            }
        }
    }
    catch (const std::exception& e)
    {
        return std::unexpected(e.what());
    }

    utils::StringRefs refs;

    while (not results.empty())
    {
        refs.push_back(results.top().string);
        results.pop();
    }

    return refs;
}

StringRefsOrError fuzzyFilter(const utils::Strings& strings, const std::string& pattern, bool reversed)
{
    utils::StringRefs refs;

    if (pattern.empty())
    {
        for (const auto& string : strings)
        {
            refs.push_back(&string);
        }
        return refs;
    }

    if (reversed)
    {
        return extract<ReversedPriorityQueue>(pattern, strings, 2);
    }
    else
    {
        return extract<PriorityQueue>(pattern, strings, 2);
    }
}

}  // namespace core
