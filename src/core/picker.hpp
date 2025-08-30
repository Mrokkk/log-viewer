#pragma once

#include <functional>
#include <string>

#include "core/fwd.hpp"
#include "utils/string.hpp"

namespace core
{

struct Readline;

struct Picker
{
    using Feeder = std::function<utils::Strings(Context&)>;

    Picker(Feeder feeder);
    ~Picker();

    const std::string* atCursor() const;
    size_t cursor() const;
    const utils::StringRefs& filtered() const;

private:
    friend struct Readline;

    void load(Context& context);
    void clear();
    void move(long offset);
    void filter(const std::string& pattern);

    Feeder            mFeeder;
    utils::Strings    mData;
    utils::StringRefs mFiltered;
    size_t            mCursor;
};

}  // namespace core
