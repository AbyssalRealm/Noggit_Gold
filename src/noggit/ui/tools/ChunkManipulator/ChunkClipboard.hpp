// This file is part of Noggit3, licensed under GNU General Public License (version 3).

#pragma once

#include <noggit/Alphamap.hpp>
#include <noggit/MapHeaders.h>
#include <noggit/TileIndex.hpp>
#include <noggit/liquid_layer.hpp>
#include <noggit/texture_set.hpp>

#include <blizzard-archive-library/include/Listfile.hpp>

#include <QObject>
#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>

class MapChunk;
class World;

namespace Noggit::Ui::Tools::ChunkManipulator
{
  enum class ChunkCopyFlags : std::uint32_t
  {
    NONE = 0,
    TERRAIN = 0x1,
    LIQUID = 0x2,
    WMOS = 0x4,
    MODELS = 0x8,
    SHADOWS = 0x10,
    TEXTURES = 0x20,
    VERTEX_COLORS = 0x40,
    HOLES = 0x80,
    FLAGS = 0x100,
    AREA_ID = 0x200,
    SOUND_EMITTERS = 0x400
  };

  enum class ChunkPasteFlags : std::uint32_t
  {
    NONE = 0,
    REPLACE_OBJECTS = 0x1,
    FIX_GAPS = 0x2
  };

  enum class ChunkSelectionMode
  {
    SELECT,
    DESELECT
  };

  struct ChunkTextureCache
  {
    std::size_t texture_count = 0;
    std::vector<std::string> textures;
    std::array<std::unique_ptr<Alphamap>, MAX_ALPHAMAPS> alphamaps;
    std::array<layer_info, 4> layer_info_entries{};
    std::array<std::uint16_t, 8> doodad_mapping{};
    std::array<std::uint8_t, 8> doodad_exclusion{};
  };

  enum class ChunkManipulatorObjectTypes
  {
    WMO,
    M2
  };

  struct ChunkObjectCacheEntry
  {
    BlizzardArchive::Listfile::FileKey file_key;
    ChunkManipulatorObjectTypes type;
    glm::vec3 local_position;
    glm::vec3 rotation;
    float scale = 1.f;
    std::uint16_t wmo_nameset = 0;
    std::uint16_t wmo_doodadset = 0;
  };

  struct SelectedChunkIndex
  {
    TileIndex tile_index;
    unsigned x;
    unsigned z;

    friend bool operator<(SelectedChunkIndex const& lhs, SelectedChunkIndex const& rhs)
    {
      return std::tie(lhs.tile_index, lhs.x, lhs.z) < std::tie(rhs.tile_index, rhs.x, rhs.z);
    }
  };

  struct SelectedChunkIndexRelative
  {
    SelectedChunkIndex source;
    int rel_x = 0;
    int rel_z = 0;
  };

  struct ChunkSourceMetadata
  {
    std::uint32_t map_id = 0;
    std::string map_name;
    std::uint32_t area_id = 0;
    TileIndex tile_index;
    unsigned chunk_x = 0;
    unsigned chunk_z = 0;
  };

  struct ChunkCache
  {
    std::optional<std::array<glm::vec3, 145>> terrain_height;
    std::optional<std::array<glm::vec3, 145>> terrain_normals;
    std::optional<std::array<glm::vec3, 145>> vertex_colors;
    std::optional<std::array<std::uint8_t, 64 * 64>> shadows;
    std::optional<std::vector<liquid_layer>> liquid_layers;
    std::optional<MH2O_Attributes> liquid_attributes;
    std::optional<ChunkTextureCache> textures;
    std::optional<std::vector<ChunkObjectCacheEntry>> objects;
    std::optional<std::vector<ENTRY_MCSE>> sound_emitters;
    std::optional<unsigned> holes;
    std::optional<mcnk_flags> flags;
    std::optional<unsigned> area_id;
  };

  using CachedChunkEntry = std::pair<SelectedChunkIndexRelative, ChunkCache>;

  class ChunkClipboard : public QObject
  {
    Q_OBJECT

  public:
    explicit ChunkClipboard(World* world, QObject* parent = nullptr);

    void selectRange(glm::vec3 const& cursor_pos, float radius, bool square, ChunkSelectionMode mode);
    void selectChunk(glm::vec3 const& pos, ChunkSelectionMode mode);
    void selectChunk(TileIndex const& tile_index, unsigned x, unsigned z, ChunkSelectionMode mode);
    void copySelected(glm::vec3 const& pos, ChunkCopyFlags flags);
    void clearSelection();
    bool pasteSelection(glm::vec3 const& pos, ChunkPasteFlags flags, float height_offset);

    void rotate90Degrees();
    void rotateLeft90Degrees();
    void mirror(bool horizontal);
    void updateTarget(glm::vec3 const& pos);
    void clearTarget();

    [[nodiscard]] ChunkCopyFlags copyParams() const;
    void setCopyParams(ChunkCopyFlags flags);

    [[nodiscard]] std::set<SelectedChunkIndex> const& selectedChunks() const;
    [[nodiscard]] std::size_t selectedCount() const;
    [[nodiscard]] bool hasCopiedData() const;
    [[nodiscard]] bool isSavedAsset() const;
    [[nodiscard]] std::vector<CachedChunkEntry> const& cachedChunks() const;
    [[nodiscard]] std::optional<ChunkSourceMetadata> const& sourceMetadata() const;

    void replaceClipboard(std::vector<CachedChunkEntry>&& chunks, ChunkCopyFlags flags,
                          std::optional<ChunkSourceMetadata> source, bool saved_asset);

  signals:
    void selectionChanged(std::set<SelectedChunkIndex> const& selected_chunks);
    void selectionCleared();
    void clipboardChanged(std::size_t chunk_count);
    void savedAssetStateChanged(bool saved_asset);
    void pasted();

  private:
    static bool hasFlag(ChunkCopyFlags value, ChunkCopyFlags flag);
    static bool hasFlag(ChunkPasteFlags value, ChunkPasteFlags flag);

    MapChunk* chunkAt(SelectedChunkIndex const& index, bool load) const;
    MapChunk* chunkAtGlobal(int global_x, int global_z, bool load) const;
    void setSourceOverlay(SelectedChunkIndex const& index, bool enabled);
    void restoreOverlay(SelectedChunkIndex const& index);
    void transformCache90(ChunkCache& cache);
    void transformCacheMirror(ChunkCache& cache, bool horizontal);
    void applyCache(MapChunk* target, ChunkCache const& cache, ChunkPasteFlags flags, float height_offset);
    void repairTargetEdges(std::vector<MapChunk*> const& targets);

    std::set<SelectedChunkIndex> _selected_chunks;
    std::vector<CachedChunkEntry> _cached_chunks;
    std::set<SelectedChunkIndex> _target_chunks;
    World* _world;
    ChunkCopyFlags _copy_flags;
    std::optional<ChunkSourceMetadata> _source_metadata;
    bool _saved_asset_mode = false;
    std::optional<glm::vec3> _last_target_position;
  };
}
