#include "file_load.hpp"

#include <filesystem>
#include <string>
#include <thread>

#include "core/context.hpp"
#include "core/file.hpp"
#include "core/files.hpp"
#include "core/user_interface.hpp"

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

    auto& newFile = context.files.emplaceBack(std::move(path));

    std::thread(
        [&newFile, view, &context]
        {
            newFile.load();
            context.ui->attachFileToView(newFile, view, context);
        }).detach();

    return true;
}

}  // namespace core
