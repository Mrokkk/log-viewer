#include "picker.hpp"

#include <algorithm>

#include "core/fuzzy.hpp"
#include "utils/math.hpp"
#include "utils/string.hpp"

namespace core
{

Picker::Picker(Orientation orientation, Feeder feeder)
    : mOrientation(orientation)
    , mHeight(0)
    , mFeeder(std::move(feeder))
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

const utils::Strings& Picker::data() const
{
    return mData;
}

const utils::StringRefs& Picker::filtered() const
{
    return mFiltered;
}

void Picker::load(Context& context)
{
    mData = mFeeder(context);

    if (mOrientation == Orientation::downTop)
    {
        std::reverse(mData.begin(), mData.end());
        mCursor = mData.size() - 1;
    }
    else
    {
        mCursor = 0;
    }

    mFiltered = fuzzyFilter(mData, "").value_or({});
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

void Picker::movePage(long offset)
{
    if (not mHeight) [[unlikely]]
    {
        return;
    }
    move(mHeight * offset);
}

void Picker::filter(const std::string& pattern)
{
    mFiltered = fuzzyFilter(mData, pattern, mOrientation == Orientation::downTop).value_or({});

    if (mOrientation == Orientation::downTop)
    {
        mCursor = mFiltered.size() - 1;
    }
    else
    {
        mCursor = 0;
    }
}

}  // namespace core
