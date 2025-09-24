#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "core/fwd.hpp"
#include "utils/immobile.hpp"
#include "utils/string.hpp"

namespace core
{

struct Readline;

struct Picker : utils::Immobile
{
    using Feeder = std::move_only_function<utils::Strings(Context&)>;

    enum class Orientation : uint8_t
    {
        topDown,
        downTop,
    };

    Picker(Orientation orientation, Feeder feeder);
    ~Picker();

    constexpr void setHeight(uint16_t height)
    {
        mHeight = height;
    }

    constexpr uint16_t height() const
    {
        return mHeight;
    }

    const std::string* atCursor() const;
    size_t cursor() const;
    const utils::Strings& data() const;
    const utils::StringRefs& filtered() const;

private:
    friend struct Readline;

    void load(Context& context);
    void clear();
    void move(long offset);
    void movePage(long offset);
    void filter(const std::string& pattern);

    Orientation       mOrientation;
    uint16_t          mHeight;
    Feeder            mFeeder;
    utils::Strings    mData;
    utils::StringRefs mFiltered;
    size_t            mCursor;
};

}  // namespace core
