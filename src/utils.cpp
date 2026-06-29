#include <cstdio>
#include <iostream>
#include <ranges>
#include <string_view>

#include <sys/iosupport.h>

#include "utils.h"

using std::cout;
using std::cerr;
using std::endl;

bool CreateParentDirectories(const std::filesystem::path& inputPath) {
    std::filesystem::path fullPath = inputPath;
    std::filesystem::path parentPath = fullPath.parent_path();

    if (!(std::filesystem::create_directories(parentPath))) {
        return false;
    }

    return true;
}

void DeletePath(const std::filesystem::path& inputPath) {
    if (inputPath.empty()) {
        cerr << "attempting to delete empty path!" << endl;
        return;
    }
    if (!exists(inputPath)) {
        cerr << inputPath << " could not be found!" << endl;
        return;
    }

    // TODO: why not use remove_all()?
    if (is_directory(inputPath)) {
        for (auto& entry : std::filesystem::directory_iterator(inputPath)) {
            DeletePath(entry.path());
            cout << "Successfully deleted " << entry.path() << endl;
        }
    }
    else {
        cout << "Deleting " << inputPath << "..." << endl;
    }

    try {
        remove(inputPath);
    }
    catch (const std::exception& e) {
        cerr << "Error deleting: " << e.what() << endl;
        return;
    }
}

static std::u32string format_codepoint(char32_t c)
{
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "U+%04X", c);
    std::u32string result;
    for (unsigned char c : std::string_view{buffer})
        result += char32_t{c};
    return result;
}

std::filesystem::path sanitize_element(const std::filesystem::path& input)
{
    std::u32string output_str;
    std::u32string input_str = input.u32string();
    for (auto c : input_str) {
        switch (c) {
            case U'/':
            case U'\\':
            case U'<':
            case U'>':
            case U':':
            case U'"':
            case U'|':
            case U'?':
            case U'*':
            case 127: // "DEL"
                output_str += U'_';
                break;
            default:
                // non-printable ASCII are forbidden
                if (c < 32)
                    output_str += U'_';
                // WORKAROUND: the Wii U needs filenames encoded in Shift-JIS, but wut
                // does not implement any conversion. So we'll enforce ASCII-only for now.
                else if (c > 127)
#if 0
                    output_str += format_codepoint(c);
#else
                    (void)format_codepoint; // do nothing
#endif
                else
                    output_str += c;
        }
    }
    // Do not allow it to end with '.'
    while (output_str.ends_with(U'.'))
        output_str.pop_back();
    return output_str;
}

std::filesystem::path sanitize(const std::filesystem::path& input) {
    std::filesystem::path output;
    auto is_newlib_root = [](const std::filesystem::path& name) -> bool
    {
        return GetDeviceOpTab(name.c_str()) != nullptr;
    };

    for (auto [index, element] : input | std::views::enumerate) {
        if (index == 0 && is_newlib_root(element))
            output /= element; // do not sanitize root element
        else
            output /= sanitize_element(element);
    }
    return output;
}
