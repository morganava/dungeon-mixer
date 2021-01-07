#include "utilities.hpp"
#include "utf8_string.hpp"
#include "file_dialog.hpp"

namespace dm
{
    static void nfdchar_free(nfdchar_t* buffer)
    {
        ::free(buffer);
    }

    typedef std::unique_ptr<nfdchar_t, decltype(&nfdchar_free)> unique_nfdbuffer;
    static auto make_unique_nfdbuffer()
    {
        return unique_nfdbuffer{nullptr, &nfdchar_free};
    }

    static std::optional<utf8_string> make_filter_list(std::initializer_list<utf8_string> filters)
    {
        if (filters.size() > 0)
        {
            utf8_stringbuffer sb;

            auto it = filters.begin();
            sb.push_back(*it);
            for(++it; it < filters.end(); ++it)
            {
                sb.push_back(static_cast<utf8_codepoint>(';'));
                sb.push_back(*it);
            }
            return sb.str();
        }
        return {};
    }


    static std::optional<fs::path> make_path(nfdresult_t result, const unique_nfdbuffer& path)
    {
        switch(result)
        {
            case NFD_ERROR: MX_THROW_MSG("{}", NFD_GetError()); break;
            case NFD_OKAY: return std::make_optional<fs::path>(path.get());
            case NFD_CANCEL: return std::nullopt;
            default: MX_THROW_MSG("Unknown nfdresult_t returned from NFD API: {}", (int)result);
        }
    }

    static std::string make_default_path_string(const fs::path& path)
    {
        // xxx: zenity implementation needs trailing slash on directories to work properly
        auto defaultPathStr = (path / ".").lexically_normal();
        return defaultPathStr;
    }

    std::optional<fs::path> file_dialog::open(std::initializer_list<utf8_string> filters, fs::path workingDirectory)
    {
        auto path = make_unique_nfdbuffer();
        auto filterString = make_filter_list(filters);

        const nfdchar_t* filterList = (filterString ? filterString->c_str() : nullptr);
        auto defaultPathString = make_default_path_string(workingDirectory);
        const nfdchar_t* defaultPath = defaultPathString.c_str();

        auto result = NFD_OpenDialog(filterList, defaultPath, out(path));
        return make_path(result, path);
    }

    std::optional<fs::path> file_dialog::save(std::initializer_list<utf8_string> filters, fs::path workingDirectory)
    {
        auto path = make_unique_nfdbuffer();
        auto filterString = make_filter_list(filters);

        const nfdchar_t* filterList = (filterString ? filterString->c_str() : nullptr);
        auto defaultPathString = make_default_path_string(workingDirectory);
        const nfdchar_t* defaultPath = defaultPathString.c_str();

        auto result = NFD_SaveDialog(filterList, defaultPath, out(path));
        return make_path(result, path);
    }
}
