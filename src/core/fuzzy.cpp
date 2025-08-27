#include "fuzzy.hpp"

#include <exception>
#include <expected>
#include <queue>
#include <string>

#include <rapidfuzz/fuzz.hpp>

namespace core
{

struct ScoredString
{
    std::string* string;
    double       score;
};

bool operator<(const ScoredString& lhs, const ScoredString& rhs)
{
    return lhs.score < rhs.score;
}

using PriorityQueueOrError = std::expected<std::priority_queue<ScoredString>, std::string>;

static PriorityQueueOrError extract(
    const std::string& query,
    const utils::Strings& choices,
    const double score_cutoff = 0.0)
{
    std::priority_queue<ScoredString> results;

    try
    {
        rapidfuzz::fuzz::CachedTokenRatio<std::string::value_type> scorer(query);

        for (const auto& choice : choices)
        {
            double score = scorer.similarity(choice, score_cutoff);

            if (score >= score_cutoff)
            {
                results.emplace(ScoredString{.string = (std::string*)&choice, .score = score});
            }
        }
    }
    catch (const std::exception& e)
    {
        return std::unexpected(e.what());
    }

    return results;
}

StringRefsOrError fuzzyFilter(const utils::Strings& strings, const std::string& pattern)
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

    auto values = extract(pattern, strings, 2);

    if (not values) [[unlikely]]
    {
        return std::unexpected(std::move(values.error()));
    }

    while (not values->empty())
    {
        refs.push_back(values->top().string);
        values->pop();
    }

    return refs;
}

}  // namespace core
