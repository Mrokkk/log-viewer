#include "fuzzy.hpp"

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

std::priority_queue<ScoredString> extract(
    const std::string& query,
    const utils::Strings& choices,
    const double score_cutoff = 0.0)
{
    std::priority_queue<ScoredString> results;

    rapidfuzz::fuzz::CachedTokenRatio<std::string::value_type> scorer(query);

    for (const auto& choice : choices)
    {
        double score = scorer.similarity(choice, score_cutoff);

        if (score >= score_cutoff)
        {
            results.emplace(ScoredString{.string = (std::string*)&choice, .score = score});
        }
    }

    return results;
}

utils::StringRefs fuzzyFilter(const utils::Strings& strings, const std::string& pattern)
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

    while (not values.empty())
    {
        refs.push_back(values.top().string);
        values.pop();
    }

    return refs;
}

}  // namespace core
