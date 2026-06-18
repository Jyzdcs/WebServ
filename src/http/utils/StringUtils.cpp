#include "../../../include/http/utils/StringUtils.hpp"
#include <cstdlib>

std::string urlDecode(const std::string& encoded)
{
    std::string decoded;

    for (std::size_t pos = 0; pos < encoded.size(); ++pos)
    {
        bool isPercentSign   = encoded[pos] == '%';
        bool hasTwoCharAfter = pos + 2 < encoded.size();

        if (!isPercentSign || !hasTwoCharAfter)
        {
            decoded += encoded[pos];
            continue;
        }

        char hexSequence[3] = { encoded[pos + 1], encoded[pos + 2], '\0' };
        char* parseEnd;
        int   decodedChar = std::strtol(hexSequence, &parseEnd, 16);

        bool validHex = (parseEnd == hexSequence + 2);
        if (!validHex)
        {
            decoded += encoded[pos];
            continue;
        }

        decoded += static_cast<char>(decodedChar);
        pos += 2;
    }

    return decoded;
}

bool hasPathTraversal(const std::string& uri)
{
    std::string decoded = urlDecode(uri);

    if (decoded.find("/../") != std::string::npos)
        return true;
    if (decoded.size() >= 3 && decoded.substr(decoded.size() - 3) == "/..")
        return true;
    if (decoded.size() >= 3 && decoded.substr(0, 3) == "../")
        return true;
    if (decoded == "..")
        return true;
    return false;
}

std::string extractQueryString(const std::string& uri)
{
    std::size_t queryStart = uri.find('?');
    if (queryStart == std::string::npos)
        return "";
    return uri.substr(queryStart + 1);
}

std::string extractFilename(const std::string& uri)
{
    std::size_t lastSlashPosition = uri.rfind('/');
    if (lastSlashPosition == std::string::npos || lastSlashPosition + 1 >= uri.size())
        return "";
    return uri.substr(lastSlashPosition + 1);
}
