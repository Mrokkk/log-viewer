#include "file_load.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <thread>

#include "core/context.hpp"
#include "core/file.hpp"

namespace core
{

bool asyncLoadFile(std::string path, Context& context)
{
    if (not std::filesystem::exists(path))
    {
        *context.ui << error << path << ": no such file";
        return false;
    }

    auto view = context.ui->createView(path, context);

    auto& newFile = context.files.emplace_back(nullptr);

    std::thread(
        [path = std::move(path), &newFile, view, &context]
        {
            newFile = std::make_unique<File>(path);
            context.ui->attachFileToView(*newFile, view, context);
        }).detach();

    return true;
}

}  // namespace core
