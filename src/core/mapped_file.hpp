#pragma once

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "file.hpp"
#include "mapping.hpp"
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

struct MappedFile final : utils::Immobile
{
    MappedFile(std::string path);
    ~MappedFile();
    size_t lineCount() const;
    float loadTime() const;
    const std::string& path() const;
    std::string operator[](unsigned long i);

private:
    float       loadTime_;
    File        file_;
    Lines       lines_;
    Mapping     mapping_;
};

using MappedFiles = std::list<std::unique_ptr<MappedFile>>;

}  // namespace core
