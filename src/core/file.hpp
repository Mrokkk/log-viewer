#pragma once

#include <string>
#include <vector>

#include "sys/file.hpp"
#include "sys/mapping.hpp"
#include "utils/immobile.hpp"

namespace core
{

struct Line final
{
    const size_t start;
    const size_t len;
};

using Lines = std::vector<Line>;
using LineRefs = std::vector<size_t>;

#define BLOCK_SIZE static_cast<size_t>(16 * 1024 * 1024)

struct File final : utils::Immobile
{
    File(std::string path);
    ~File();

    bool load();
    size_t lineCount() const;
    float loadTime() const;
    const std::string& path() const;
    std::string operator[](unsigned long i);
    bool isLoaded() const;

private:
    bool         loaded_;
    float        loadTime_;
    sys::File    file_;
    Lines        lines_;
    sys::Mapping mapping_;
};

}  // namespace core
