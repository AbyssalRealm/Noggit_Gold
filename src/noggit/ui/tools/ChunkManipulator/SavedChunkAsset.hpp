// This file is part of Noggit3, licensed under GNU General Public License (version 3).
// Noggit_Gold persistent saved-chunk library.

#pragma once

#include "ChunkClipboard.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace Noggit::Ui::Tools::ChunkManipulator
{
  struct SavedChunkAssetRecord
  {
    std::filesystem::path path;
    std::string display_name;
  };

  class SavedChunkAsset
  {
  public:
    static std::filesystem::path libraryDirectory();
    static std::vector<SavedChunkAssetRecord> list();

    static std::string suggestedName(ChunkClipboard const& clipboard);

    static bool save(std::string const& requested_name,
                     ChunkClipboard const& clipboard,
                     std::filesystem::path& saved_path,
                     std::string& error);

    static bool load(std::filesystem::path const& path,
                     ChunkClipboard& clipboard,
                     std::string& display_name,
                     std::string& error);

    static bool remove(std::filesystem::path const& path, std::string& error);

  private:
    static std::string sanitizeName(std::string name);
  };
}
