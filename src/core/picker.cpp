#include "picker.hpp"

#include "core/fuzzy.hpp"
#include "utils/math.hpp"
#include "utils/string.hpp"

namespace core
{

Picker::Picker(Feeder feeder)
    : mFeeder(feeder)
    , mCursor(0)
{
}

Picker::~Picker() = default;

const std::string* Picker::atCursor() const
{
    if (mCursor < mFiltered.size()) [[likely]]
    {
        return mFiltered[mCursor];
    }
    return nullptr;
}

size_t Picker::cursor() const
{
    return mCursor;
}

const utils::StringRefs& Picker::filtered() const
{
    return mFiltered;
}

void Picker::load(Context& context)
{
    mData = mFeeder(context);
    mFiltered = fuzzyFilter(mData, "").value_or({});
    mCursor = 0;
}

void Picker::clear()
{
    mData.clear();
    mFiltered.clear();
    mCursor = 0;
}

void Picker::move(long offset)
{
    long size = mFiltered.size();

    if (size > 0)
    {
        mCursor = utils::clamp(static_cast<long>(mCursor) + offset, 0l, size - 1);
    }
}

void Picker::filter(const std::string& pattern)
{
    mFiltered = fuzzyFilter(mData, pattern).value_or({});
    mCursor = 0;
}

}  // namespace core
