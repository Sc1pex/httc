#include "httc/utils/mime.h"
#include <unordered_map>

namespace httc {
namespace utils {

std::optional<std::string_view> mime_type(const std::filesystem::path& path) {
    static const std::unordered_map<std::string_view, std::string_view> mime_types = {
        // Text files
        { ".html", "text/html" },
        { ".htm", "text/html" },
        { ".css", "text/css" },
        { ".js", "text/javascript" },
        { ".json", "application/json" },
        { ".xml", "application/xml" },
        { ".txt", "text/plain" },
        { ".csv", "text/csv" },

        // Images
        { ".jpg", "image/jpeg" },
        { ".jpeg", "image/jpeg" },
        { ".png", "image/png" },
        { ".gif", "image/gif" },
        { ".svg", "image/svg+xml" },
        { ".bmp", "image/bmp" },
        { ".webp", "image/webp" },
        { ".ico", "image/x-icon" },

        // Audio
        { ".mp3", "audio/mpeg" },
        { ".wav", "audio/wav" },
        { ".ogg", "audio/ogg" },

        // Video
        { ".mp4", "video/mp4" },
        { ".webm", "video/webm" },
        { ".avi", "video/x-msvideo" },

        // Documents
        { ".pdf", "application/pdf" },
        { ".zip", "application/zip" },
        { ".tar", "application/x-tar" },
        { ".gz", "application/gzip" }
    };

    std::string extension = path.extension().string();

    // Convert to lowercase
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    auto it = mime_types.find(extension);
    if (it != mime_types.end()) {
        return it->second;
    }

    return std::nullopt;
}

}
}
