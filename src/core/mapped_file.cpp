#include "mapped_file.hpp"

#include <stdexcept>
#include <string.h>

#include "core/logger.hpp"
#include "sys/system.hpp"
#include "utils/time.hpp"

namespace core
{

MappedFile::MappedFile(std::string path)
    : file_(sys::fileOpen(std::move(path)))
    , mapping_{.ptr = nullptr, .offset = 0, .len = 0}
{
    loadTime_ = utils::measureTime(
        [&]
        {
            auto sizeLeft = file_.size;
            size_t offset{0};
            size_t lineStart{0};

            while (sizeLeft)
            {
                auto toRead = std::min(sizeLeft, BLOCK_SIZE);

                if (sys::remap(file_, mapping_, offset, toRead))
                {
                    logger << error << "cannot map " << strerror(errno);
                    throw std::runtime_error("Cannot map");
                }

                const auto text = mapping_.ptrAt<const char*>(offset);

                for (size_t i = 0; i < toRead; ++i)
                {
                    if (text[i] == '\n')
                    {
                        lines_.emplace_back(Line{.start = lineStart, .len = offset + i - lineStart});
                        lineStart = offset + i + 1;
                    }
                }

                sizeLeft -= toRead;
                offset += toRead;
            }
        });
}

MappedFile::~MappedFile()
{
    sys::unmap(mapping_);
    sys::fileClose(file_);
}

size_t MappedFile::lineCount() const
{
    return lines_.size();
}

float MappedFile::loadTime() const
{
    return loadTime_;
}

const std::string& MappedFile::path() const
{
    return file_.path;
}

std::string MappedFile::operator[](unsigned long i)
{
    const auto& line = lines_[i];

    if (line.len == 0)
    {
        return "";
    }

    if (line.start > mapping_.offset and line.len + line.start < mapping_.len + mapping_.offset)
    {
        return std::string(mapping_.ptrAt<const char*>(line.start), line.len);
    }
    else
    {
        if (sys::remap(file_, mapping_, line.start, std::min(BLOCK_SIZE, file_.size - line.start)))
        {
            logger << error << "cannot map " << strerror(errno);
            throw std::runtime_error("Cannot map");
        }

        return std::string(mapping_.ptrAt<const char*>(line.start), line.len);
    }
}

}  // namespace core
