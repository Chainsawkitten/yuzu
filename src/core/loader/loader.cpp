// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include <string>
#include "common/logging/log.h"
#include "common/string_util.h"
#include "core/hle/kernel/process.h"
#include "core/loader/elf.h"
#include "core/loader/nro.h"
#include "core/loader/nso.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Loader {

const std::initializer_list<Kernel::AddressMapping> default_address_mappings = {
    {0x1FF50000, 0x8000, true},    // part of DSP RAM
    {0x1FF70000, 0x8000, true},    // part of DSP RAM
    {0x1F000000, 0x600000, false}, // entire VRAM
};

FileType IdentifyFile(FileUtil::IOFile& file) {
    FileType type;

#define CHECK_TYPE(loader)                                                                         \
    type = AppLoader_##loader::IdentifyType(file);                                                 \
    if (FileType::Error != type)                                                                   \
        return type;

    CHECK_TYPE(ELF)
    CHECK_TYPE(NSO)
    CHECK_TYPE(NRO)

#undef CHECK_TYPE

    return FileType::Unknown;
}

FileType IdentifyFile(const std::string& file_name) {
    FileUtil::IOFile file(file_name, "rb");
    if (!file.IsOpen()) {
        LOG_ERROR(Loader, "Failed to load file %s", file_name.c_str());
        return FileType::Unknown;
    }

    return IdentifyFile(file);
}

FileType GuessFromExtension(const std::string& extension_) {
    std::string extension = Common::ToLower(extension_);

    if (extension == ".elf" || extension == ".axf")
        return FileType::ELF;
    else if (extension == ".nro")
        return FileType::NRO;
    else if (extension == ".nso")
        return FileType::NSO;

    return FileType::Unknown;
}

const char* GetFileTypeString(FileType type) {
    switch (type) {
    case FileType::ELF:
        return "ELF";
    case FileType::NRO:
        return "NRO";
    case FileType::NSO:
        return "NSO";
    case FileType::Error:
    case FileType::Unknown:
        break;
    }

    return "unknown";
}

/**
 * Get a loader for a file with a specific type
 * @param file The file to load
 * @param type The type of the file
 * @param filename the file name (without path)
 * @param filepath the file full path (with name)
 * @return std::unique_ptr<AppLoader> a pointer to a loader object;  nullptr for unsupported type
 */
static std::unique_ptr<AppLoader> GetFileLoader(FileUtil::IOFile&& file, FileType type,
                                                const std::string& filename,
                                                const std::string& filepath) {
    switch (type) {

    // Standard ELF file format.
    case FileType::ELF:
        return std::make_unique<AppLoader_ELF>(std::move(file), filename);

    // NX NSO file format.
    case FileType::NSO:
        return std::make_unique<AppLoader_NSO>(std::move(file), filepath);

    // NX NRO file format.
    case FileType::NRO:
        return std::make_unique<AppLoader_NRO>(std::move(file), filepath);

    default:
        return nullptr;
    }
}

std::unique_ptr<AppLoader> GetLoader(const std::string& filename) {
    FileUtil::IOFile file(filename, "rb");
    if (!file.IsOpen()) {
        LOG_ERROR(Loader, "Failed to load file %s", filename.c_str());
        return nullptr;
    }

    std::string filename_filename, filename_extension;
    Common::SplitPath(filename, nullptr, &filename_filename, &filename_extension);

    FileType type = IdentifyFile(file);
    FileType filename_type = GuessFromExtension(filename_extension);

    if (type != filename_type) {
        LOG_WARNING(Loader, "File %s has a different type than its extension.", filename.c_str());
        if (FileType::Unknown == type)
            type = filename_type;
    }

    LOG_DEBUG(Loader, "Loading file %s as %s...", filename.c_str(), GetFileTypeString(type));

    return GetFileLoader(std::move(file), type, filename_filename, filename);
}

} // namespace Loader
