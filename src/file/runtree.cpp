#include "mutils/logger/logger.h"

#include "mutils/file/runtree.h"
#include "mutils/file/file.h"

namespace MUtils
{
fs::path runtreePath;

void runtree_init(const fs::path& appRoot)
{
    fs::path basePath = appRoot;
    basePath = basePath / "run_tree";
    runtreePath = fs::canonical(fs::absolute(basePath));
    LOG(DBG, "runtree Path: " << runtreePath.string());
}

void runtree_destroy()
{
    runtreePath.clear();
}

fs::path runtree_find_asset_internal(const fs::path& searchPath)
{
    fs::path found(runtreePath / searchPath);
    if (fs::exists(found))
    {
        //LOG(DEBUG) << "Found file: " << found.string();
        return fs::canonical(fs::absolute(found));
    }

    LOG(DBG, "** File not found: " << searchPath.string());
    return fs::path();
}

fs::path runtree_find_asset(const fs::path& p)
{
    return runtree_find_asset_internal(p);
}

std::string runtree_load_asset(const fs::path& p)
{
    auto path = runtree_find_asset(p);
    return file_read(path);
}

fs::path runtree_path()
{
    return runtreePath;
}

} // MUtils
