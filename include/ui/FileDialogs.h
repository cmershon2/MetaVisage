#ifndef METAVISAGE_UI_FILEDIALOGS_H
#define METAVISAGE_UI_FILEDIALOGS_H

#include <optional>
#include <string>
#include <vector>

namespace MetaVisage::FileDialogs {

struct Filter {
    std::string name;        // "FBX mesh"
    std::string extensions;  // comma-separated, no dots: "fbx"
};

// Blocking open/save dialogs backed by nativefiledialog-extended. Return
// std::nullopt on cancel. Optional `defaultName` is only used by SaveFile.
std::optional<std::string> OpenFile(const std::vector<Filter>& filters);
std::optional<std::string> SaveFile(const std::vector<Filter>& filters,
                                    const std::string& defaultName);

} // namespace MetaVisage::FileDialogs

#endif
