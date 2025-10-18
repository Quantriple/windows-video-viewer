#include "VideoFile.h"
#include "Utils.h"

namespace VideoViewer {

std::string VideoFile::getFormattedSize() const {
    return Utils::FormatFileSize(fileSize);
}

std::string VideoFile::getFormattedTime() const {
    return Utils::FormatFileTime(lastModified);
}

} // namespace VideoViewer