// This file is part of Noggit3, licensed under GNU General Public License (version 3).

#include "ChunkClipboard.hpp"

#include <noggit/Action.hpp>
#include <noggit/ActionManager.hpp>
#include <noggit/ChunkWater.hpp>
#include <noggit/MapChunk.h>
#include <noggit/MapTile.h>
#include <noggit/Model.h>
#include <noggit/ModelInstance.h>
#include <noggit/WMO.h>
#include <noggit/WMOInstance.h>
#include <noggit/World.h>
#include <noggit/World.inl>
#include <noggit/map_index.hpp>
#include <noggit/scoped_blp_texture_reference.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>

using namespace Noggit::Ui::Tools::ChunkManipulator;

namespace
{
  constexpr int chunks_per_map = 64 * 16;

  SelectedChunkIndex index_for(MapChunk const* chunk)
  {
    return {TileIndex{glm::vec3{chunk->xbase, 0.f, chunk->zbase}},
            static_cast<unsigned>(chunk->px), static_cast<unsigned>(chunk->py)};
  }

  bool position_is_in_chunk(glm::vec3 const& position, MapChunk const* chunk)
  {
    return position.x >= chunk->xbase && position.x < chunk->xbase + CHUNKSIZE
        && position.z >= chunk->zbase && position.z < chunk->zbase + CHUNKSIZE;
  }

  std::array<int, mapbufsize> make_vertex_rotation_lookup()
  {
    std::array<int, mapbufsize> lookup{};

    for (int x = 0; x < 9; ++x)
    {
      for (int z = 0; z < 9; ++z)
      {
        int const new_x = z;
        int const new_z = 8 - x;
        lookup[z * 17 + x] = new_z * 17 + new_x;

        if (x < 8 && z < 8)
        {
          int const old_inner = (z + 1) * 9 + z * 8 + x;
          int const new_inner_x = z;
          int const new_inner_z = 7 - x;
          lookup[old_inner] = (new_inner_z + 1) * 9 + new_inner_z * 8 + new_inner_x;
        }
      }
    }

    return lookup;
  }

  std::array<int, mapbufsize> make_vertex_mirror_lookup(bool horizontal)
  {
    std::array<int, mapbufsize> lookup{};

    for (int x = 0; x < 9; ++x)
    {
      for (int z = 0; z < 9; ++z)
      {
        int const new_x = horizontal ? 8 - x : x;
        int const new_z = horizontal ? z : 8 - z;
        lookup[z * 17 + x] = new_z * 17 + new_x;

        if (x < 8 && z < 8)
        {
          int const old_inner = (z + 1) * 9 + z * 8 + x;
          int const new_inner_x = horizontal ? 7 - x : x;
          int const new_inner_z = horizontal ? z : 7 - z;
          lookup[old_inner] = (new_inner_z + 1) * 9 + new_inner_z * 8 + new_inner_x;
        }
      }
    }

    return lookup;
  }

  template<typename T, std::size_t N>
  void reorder(std::array<T, N>& values, std::array<int, N> const& lookup)
  {
    auto const original = values;
    for (std::size_t i = 0; i < N; ++i)
      values[lookup[i]] = original[i];
  }

  template<int Size>
  std::uint64_t rotate_mask(std::uint64_t source)
  {
    std::uint64_t result = 0;
    for (int x = 0; x < Size; ++x)
    {
      for (int z = 0; z < Size; ++z)
      {
        if ((source >> (z * Size + x)) & std::uint64_t(1))
        {
          int const new_x = z;
          int const new_z = Size - 1 - x;
          result |= std::uint64_t(1) << (new_z * Size + new_x);
        }
      }
    }
    return result;
  }

  template<int Size>
  std::uint64_t mirror_mask(std::uint64_t source, bool horizontal)
  {
    std::uint64_t result = 0;
    for (int x = 0; x < Size; ++x)
    {
      for (int z = 0; z < Size; ++z)
      {
        if ((source >> (z * Size + x)) & std::uint64_t(1))
        {
          int const new_x = horizontal ? Size - 1 - x : x;
          int const new_z = horizontal ? z : Size - 1 - z;
          result |= std::uint64_t(1) << (new_z * Size + new_x);
        }
      }
    }
    return result;
  }

  template<int Size>
  void rotate_bytes(std::array<std::uint8_t, Size * Size>& values)
  {
    auto const original = values;
    for (int x = 0; x < Size; ++x)
      for (int z = 0; z < Size; ++z)
        values[(Size - 1 - x) * Size + z] = original[z * Size + x];
  }

  template<int Size>
  void mirror_bytes(std::array<std::uint8_t, Size * Size>& values, bool horizontal)
  {
    auto const original = values;
    for (int x = 0; x < Size; ++x)
    {
      for (int z = 0; z < Size; ++z)
      {
        int const new_x = horizontal ? Size - 1 - x : x;
        int const new_z = horizontal ? z : Size - 1 - z;
        values[new_z * Size + new_x] = original[z * Size + x];
      }
    }
  }

  void rotate_alphamap(Alphamap& alphamap)
  {
    Alphamap const original = alphamap;
    for (int x = 0; x < 64; ++x)
      for (int z = 0; z < 64; ++z)
        alphamap.setAlpha((63 - x) * 64 + z, original.getAlpha(z * 64 + x));
  }

  void mirror_alphamap(Alphamap& alphamap, bool horizontal)
  {
    Alphamap const original = alphamap;
    for (int x = 0; x < 64; ++x)
    {
      for (int z = 0; z < 64; ++z)
      {
        int const new_x = horizontal ? 63 - x : x;
        int const new_z = horizontal ? z : 63 - z;
        alphamap.setAlpha(new_z * 64 + new_x, original.getAlpha(z * 64 + x));
      }
    }
  }

  std::array<std::uint16_t, 8> rotate_doodad_mapping(std::array<std::uint16_t, 8> const& source)
  {
    std::array<std::uint16_t, 8> result{};
    for (int x = 0; x < 8; ++x)
    {
      for (int z = 0; z < 8; ++z)
      {
        std::uint16_t const layer = (source[z] >> (x * 2)) & 0x3;
        int const new_x = z;
        int const new_z = 7 - x;
        result[new_z] |= layer << (new_x * 2);
      }
    }
    return result;
  }

  std::array<std::uint16_t, 8> mirror_doodad_mapping(std::array<std::uint16_t, 8> const& source,
                                                      bool horizontal)
  {
    std::array<std::uint16_t, 8> result{};
    for (int x = 0; x < 8; ++x)
    {
      for (int z = 0; z < 8; ++z)
      {
        std::uint16_t const layer = (source[z] >> (x * 2)) & 0x3;
        int const new_x = horizontal ? 7 - x : x;
        int const new_z = horizontal ? z : 7 - z;
        result[new_z] |= layer << (new_x * 2);
      }
    }
    return result;
  }

  std::array<std::uint8_t, 8> rotate_doodad_exclusion(std::array<std::uint8_t, 8> const& source)
  {
    std::array<std::uint8_t, 8> result{};
    for (int x = 0; x < 8; ++x)
    {
      for (int z = 0; z < 8; ++z)
      {
        if ((source[z] >> x) & 0x1)
          result[7 - x] |= std::uint8_t(1) << z;
      }
    }
    return result;
  }

  std::array<std::uint8_t, 8> mirror_doodad_exclusion(std::array<std::uint8_t, 8> const& source,
                                                       bool horizontal)
  {
    std::array<std::uint8_t, 8> result{};
    for (int x = 0; x < 8; ++x)
    {
      for (int z = 0; z < 8; ++z)
      {
        if ((source[z] >> x) & 0x1)
        {
          int const new_x = horizontal ? 7 - x : x;
          int const new_z = horizontal ? z : 7 - z;
          result[new_z] |= std::uint8_t(1) << new_x;
        }
      }
    }
    return result;
  }

  void rotate_texture_animation(layer_info& info)
  {
    if (!(info.flags & FLAG_ANIMATE))
      return;

    int rotation = static_cast<int>(info.flags & 0x7) - 2;
    if (rotation < 0)
      rotation += 8;
    info.flags = (info.flags & ~std::uint32_t(0x7)) | static_cast<std::uint32_t>(rotation);
  }

  void mirror_texture_animation(layer_info& info, bool horizontal)
  {
    if (!(info.flags & FLAG_ANIMATE))
      return;

    int rotation = static_cast<int>(info.flags & 0x7);
    rotation = horizontal ? 8 - rotation : 4 - rotation;
    rotation %= 8;
    if (rotation < 0)
      rotation += 8;
    info.flags = (info.flags & ~std::uint32_t(0x7)) | static_cast<std::uint32_t>(rotation);
  }
}

ChunkClipboard::ChunkClipboard(World* world, QObject* parent)
  : QObject(parent)
  , _world(world)
  , _copy_flags(ChunkCopyFlags::NONE)
{
}

bool ChunkClipboard::hasFlag(ChunkCopyFlags value, ChunkCopyFlags flag)
{
  return (static_cast<std::uint32_t>(value) & static_cast<std::uint32_t>(flag)) != 0;
}

bool ChunkClipboard::hasFlag(ChunkPasteFlags value, ChunkPasteFlags flag)
{
  return (static_cast<std::uint32_t>(value) & static_cast<std::uint32_t>(flag)) != 0;
}

MapChunk* ChunkClipboard::chunkAt(SelectedChunkIndex const& index, bool load) const
{
  if (!index.tile_index.is_valid() || index.x >= 16 || index.z >= 16
      || !_world->mapIndex.hasTile(index.tile_index))
    return nullptr;

  MapTile* tile = load ? _world->mapIndex.loadTile(index.tile_index)
                       : _world->mapIndex.getTile(index.tile_index);
  if (!tile)
    return nullptr;

  if (load)
    tile->wait_until_loaded();
  else if (!tile->finishedLoading())
    return nullptr;

  return tile->getChunk(index.x, index.z);
}

MapChunk* ChunkClipboard::chunkAtGlobal(int global_x, int global_z, bool load) const
{
  if (global_x < 0 || global_z < 0 || global_x >= chunks_per_map || global_z >= chunks_per_map)
    return nullptr;

  SelectedChunkIndex const index{
    TileIndex{static_cast<std::size_t>(global_x / 16), static_cast<std::size_t>(global_z / 16)},
    static_cast<unsigned>(global_x % 16), static_cast<unsigned>(global_z % 16)};
  return chunkAt(index, load);
}

void ChunkClipboard::setSourceOverlay(SelectedChunkIndex const& index, bool enabled)
{
  if (MapChunk* chunk = chunkAt(index, false))
  {
    if (_target_chunks.contains(index))
      chunk->setChunkTransferOverlay(ChunkTransferOverlay::TARGET);
    else
      chunk->setChunkTransferOverlay(enabled ? ChunkTransferOverlay::SOURCE : ChunkTransferOverlay::NONE);
  }
}

void ChunkClipboard::restoreOverlay(SelectedChunkIndex const& index)
{
  if (MapChunk* chunk = chunkAt(index, false))
  {
    if (_target_chunks.contains(index))
      chunk->setChunkTransferOverlay(ChunkTransferOverlay::TARGET);
    else
      chunk->setChunkTransferOverlay(_selected_chunks.contains(index)
                                       ? ChunkTransferOverlay::SOURCE
                                       : ChunkTransferOverlay::NONE);
  }
}

void ChunkClipboard::selectRange(glm::vec3 const& cursor_pos, float radius, bool square,
                                 ChunkSelectionMode mode)
{
  std::vector<SelectedChunkIndex> affected;
  auto collect = [&affected](MapChunk* chunk)
  {
    affected.emplace_back(index_for(chunk));
    return false;
  };

  if (square)
    _world->for_all_chunks_in_rect(cursor_pos, radius, collect);
  else
    _world->for_all_chunks_in_range(cursor_pos, radius, collect);

  bool changed = false;
  for (auto const& index : affected)
  {
    if (mode == ChunkSelectionMode::SELECT)
    {
      if (_selected_chunks.emplace(index).second)
      {
        changed = true;
        setSourceOverlay(index, true);
      }
    }
    else if (_selected_chunks.erase(index))
    {
      changed = true;
      restoreOverlay(index);
    }
  }

  if (changed)
    emit selectionChanged(_selected_chunks);
}

void ChunkClipboard::selectChunk(glm::vec3 const& pos, ChunkSelectionMode mode)
{
  if (MapChunk* chunk = _world->getChunkAt(pos))
  {
    auto const index = index_for(chunk);
    selectChunk(index.tile_index, index.x, index.z, mode);
  }
}

void ChunkClipboard::selectChunk(TileIndex const& tile_index, unsigned x, unsigned z,
                                 ChunkSelectionMode mode)
{
  SelectedChunkIndex const index{tile_index, x, z};
  if (!index.tile_index.is_valid() || x >= 16 || z >= 16 || !_world->mapIndex.hasTile(tile_index))
    return;

  bool changed = false;
  if (mode == ChunkSelectionMode::SELECT)
  {
    changed = _selected_chunks.emplace(index).second;
    if (changed)
      setSourceOverlay(index, true);
  }
  else
  {
    changed = _selected_chunks.erase(index) != 0;
    if (changed)
      restoreOverlay(index);
  }

  if (changed)
    emit selectionChanged(_selected_chunks);
}

void ChunkClipboard::copySelected(glm::vec3 const&, ChunkCopyFlags flags)
{
  clearTarget();
  _cached_chunks.clear();
  _copy_flags = flags;
  _source_metadata.reset();
  if (_saved_asset_mode)
  {
    _saved_asset_mode = false;
    emit savedAssetStateChanged(false);
  }

  if (_selected_chunks.empty() || flags == ChunkCopyFlags::NONE)
  {
    emit clipboardChanged(0);
    return;
  }

  int min_x = chunks_per_map;
  int min_z = chunks_per_map;
  int max_x = -1;
  int max_z = -1;

  for (auto const& index : _selected_chunks)
  {
    int const global_x = static_cast<int>(index.tile_index.x * 16 + index.x);
    int const global_z = static_cast<int>(index.tile_index.z * 16 + index.z);
    min_x = std::min(min_x, global_x);
    min_z = std::min(min_z, global_z);
    max_x = std::max(max_x, global_x);
    max_z = std::max(max_z, global_z);
  }

  int const pivot_x = (min_x + max_x + 1) / 2;
  int const pivot_z = (min_z + max_z + 1) / 2;

  for (auto const& index : _selected_chunks)
  {
    MapChunk* chunk = chunkAt(index, true);
    if (!chunk)
      continue;

    if (!_source_metadata)
    {
      _source_metadata = ChunkSourceMetadata{
        static_cast<std::uint32_t>(_world->getMapID()),
        _world->basename,
        static_cast<std::uint32_t>(chunk->getAreaID()),
        index.tile_index,
        index.x,
        index.z
      };
    }

    ChunkCache cache;

    if (hasFlag(flags, ChunkCopyFlags::TERRAIN))
    {
      cache.terrain_height.emplace();
      cache.terrain_normals.emplace();
      std::copy(std::begin(chunk->mVertices), std::end(chunk->mVertices), cache.terrain_height->begin());
      std::copy(std::begin(chunk->mNormals), std::end(chunk->mNormals), cache.terrain_normals->begin());
    }

    if (hasFlag(flags, ChunkCopyFlags::VERTEX_COLORS))
    {
      cache.vertex_colors.emplace();
      std::copy(std::begin(chunk->mccv), std::end(chunk->mccv), cache.vertex_colors->begin());
    }

    if (hasFlag(flags, ChunkCopyFlags::SHADOWS))
    {
      cache.shadows.emplace();
      std::copy(std::begin(chunk->_shadow_map), std::end(chunk->_shadow_map), cache.shadows->begin());
    }

    if (hasFlag(flags, ChunkCopyFlags::LIQUID))
    {
      cache.liquid_layers = *chunk->liquid_chunk()->getLayers();
      cache.liquid_attributes = chunk->liquid_chunk()->getAttributes();
    }

    if (hasFlag(flags, ChunkCopyFlags::TEXTURES))
    {
      ChunkTextureCache texture_cache;
      TextureSet* texture_set = chunk->getTextureSet();
      texture_set->apply_alpha_changes();
      texture_cache.texture_count = texture_set->num();

      for (std::size_t i = 0; i < texture_cache.texture_count; ++i)
        texture_cache.textures.emplace_back(texture_set->filename(i));

      auto const& source_alphamaps = *texture_set->getAlphamaps();
      for (std::size_t i = 0; i < MAX_ALPHAMAPS; ++i)
      {
        if (source_alphamaps[i])
          texture_cache.alphamaps[i] = std::make_unique<Alphamap>(*source_alphamaps[i]);
      }

      std::copy_n(texture_set->getMCLYEntries(), 4, texture_cache.layer_info_entries.begin());
      texture_cache.doodad_mapping = texture_set->getDoodadMapping();
      std::copy_n(texture_set->getDoodadStencilBase(), 8, texture_cache.doodad_exclusion.begin());
      cache.textures = std::move(texture_cache);
    }

    if (hasFlag(flags, ChunkCopyFlags::WMOS) || hasFlag(flags, ChunkCopyFlags::MODELS))
    {
      cache.objects.emplace();
      glm::vec3 const center{chunk->xbase + CHUNKSIZE * .5f, 0.f, chunk->zbase + CHUNKSIZE * .5f};
      auto objects = _world->getObjectsInRange(center, static_cast<float>(MAPCHUNK_RADIUS), true,
                                               hasFlag(flags, ChunkCopyFlags::WMOS),
                                               hasFlag(flags, ChunkCopyFlags::MODELS));

      for (SceneObject* object : objects)
      {
        if (!position_is_in_chunk(object->pos, chunk))
          continue;

        glm::vec3 local_position = object->pos;
        local_position.x -= center.x;
        local_position.z -= center.z;

        if (object->which() == eWMO)
        {
          auto* wmo = static_cast<WMOInstance*>(object);
          cache.objects->push_back({wmo->wmo->file_key(), ChunkManipulatorObjectTypes::WMO,
                                    local_position, object->dir, object->scale,
                                    wmo->mNameset, wmo->doodadset()});
        }
        else
        {
          auto* model = static_cast<ModelInstance*>(object);
          cache.objects->push_back({model->model->file_key(), ChunkManipulatorObjectTypes::M2,
                                    local_position, object->dir, object->scale, 0, 0});
        }
      }
    }

    if (hasFlag(flags, ChunkCopyFlags::SOUND_EMITTERS))
    {
      cache.sound_emitters = chunk->sound_emitters;
      float const center_x = chunk->xbase + CHUNKSIZE * .5f;
      float const center_z = chunk->zbase + CHUNKSIZE * .5f;
      for (auto& emitter : *cache.sound_emitters)
      {
        emitter.pos[0] -= center_x;
        emitter.pos[2] -= center_z;
      }
    }

    if (hasFlag(flags, ChunkCopyFlags::HOLES))
      cache.holes = static_cast<unsigned>(chunk->holes);
    if (hasFlag(flags, ChunkCopyFlags::FLAGS))
      cache.flags = chunk->header_flags;
    if (hasFlag(flags, ChunkCopyFlags::AREA_ID))
      cache.area_id = chunk->areaID;

    int const global_x = static_cast<int>(index.tile_index.x * 16 + index.x);
    int const global_z = static_cast<int>(index.tile_index.z * 16 + index.z);
    SelectedChunkIndexRelative relative{index, global_x - pivot_x, global_z - pivot_z};
    _cached_chunks.emplace_back(std::move(relative), std::move(cache));
  }

  emit clipboardChanged(_cached_chunks.size());
}

void ChunkClipboard::clearSelection()
{
  clearTarget();
  for (auto const& index : _selected_chunks)
    setSourceOverlay(index, false);

  _selected_chunks.clear();
  _cached_chunks.clear();
  _copy_flags = ChunkCopyFlags::NONE;
  _source_metadata.reset();
  if (_saved_asset_mode)
  {
    _saved_asset_mode = false;
    emit savedAssetStateChanged(false);
  }
  emit selectionCleared();
  emit clipboardChanged(0);
  emit selectionChanged(_selected_chunks);
}

void ChunkClipboard::transformCache90(ChunkCache& cache)
{
  static auto const vertex_lookup = make_vertex_rotation_lookup();

  if (cache.terrain_height)
    reorder(*cache.terrain_height, vertex_lookup);
  if (cache.vertex_colors)
    reorder(*cache.vertex_colors, vertex_lookup);
  if (cache.terrain_normals)
  {
    reorder(*cache.terrain_normals, vertex_lookup);
    for (auto& normal : *cache.terrain_normals)
      normal = {normal.z, normal.y, -normal.x};
  }
  if (cache.shadows)
    rotate_bytes<64>(*cache.shadows);
  if (cache.holes)
    cache.holes = static_cast<unsigned>(rotate_mask<4>(*cache.holes));

  if (cache.liquid_layers)
    for (auto& layer : *cache.liquid_layers)
      layer.rotate_90_degrees();
  if (cache.liquid_attributes)
  {
    cache.liquid_attributes->fatigue = rotate_mask<8>(cache.liquid_attributes->fatigue);
    cache.liquid_attributes->fishable = rotate_mask<8>(cache.liquid_attributes->fishable);
  }

  if (cache.textures)
  {
    for (auto& alphamap : cache.textures->alphamaps)
      if (alphamap)
        rotate_alphamap(*alphamap);
    for (auto& layer : cache.textures->layer_info_entries)
      rotate_texture_animation(layer);
    cache.textures->doodad_mapping = rotate_doodad_mapping(cache.textures->doodad_mapping);
    cache.textures->doodad_exclusion = rotate_doodad_exclusion(cache.textures->doodad_exclusion);
  }

  if (cache.objects)
  {
    for (auto& object : *cache.objects)
    {
      float const old_x = object.local_position.x;
      object.local_position.x = object.local_position.z;
      object.local_position.z = -old_x;
      object.rotation.y += 90.f;
    }
  }

  if (cache.sound_emitters)
  {
    for (auto& emitter : *cache.sound_emitters)
    {
      float const old_x = emitter.pos[0];
      emitter.pos[0] = emitter.pos[2];
      emitter.pos[2] = -old_x;
    }
  }
}

void ChunkClipboard::transformCacheMirror(ChunkCache& cache, bool horizontal)
{
  auto const vertex_lookup = make_vertex_mirror_lookup(horizontal);

  if (cache.terrain_height)
    reorder(*cache.terrain_height, vertex_lookup);
  if (cache.vertex_colors)
    reorder(*cache.vertex_colors, vertex_lookup);
  if (cache.terrain_normals)
  {
    reorder(*cache.terrain_normals, vertex_lookup);
    for (auto& normal : *cache.terrain_normals)
    {
      if (horizontal)
        normal.x = -normal.x;
      else
        normal.z = -normal.z;
    }
  }
  if (cache.shadows)
    mirror_bytes<64>(*cache.shadows, horizontal);
  if (cache.holes)
    cache.holes = static_cast<unsigned>(mirror_mask<4>(*cache.holes, horizontal));

  if (cache.liquid_layers)
    for (auto& layer : *cache.liquid_layers)
      layer.mirror(horizontal);
  if (cache.liquid_attributes)
  {
    cache.liquid_attributes->fatigue = mirror_mask<8>(cache.liquid_attributes->fatigue, horizontal);
    cache.liquid_attributes->fishable = mirror_mask<8>(cache.liquid_attributes->fishable, horizontal);
  }

  if (cache.textures)
  {
    for (auto& alphamap : cache.textures->alphamaps)
      if (alphamap)
        mirror_alphamap(*alphamap, horizontal);
    for (auto& layer : cache.textures->layer_info_entries)
      mirror_texture_animation(layer, horizontal);
    cache.textures->doodad_mapping = mirror_doodad_mapping(cache.textures->doodad_mapping, horizontal);
    cache.textures->doodad_exclusion = mirror_doodad_exclusion(cache.textures->doodad_exclusion, horizontal);
  }

  if (cache.objects)
  {
    for (auto& object : *cache.objects)
    {
      if (horizontal)
      {
        object.local_position.x = -object.local_position.x;
        object.rotation.y = 180.f - object.rotation.y;
      }
      else
      {
        object.local_position.z = -object.local_position.z;
        object.rotation.y = -object.rotation.y;
      }
    }
  }

  if (cache.sound_emitters)
  {
    for (auto& emitter : *cache.sound_emitters)
    {
      if (horizontal)
        emitter.pos[0] = -emitter.pos[0];
      else
        emitter.pos[2] = -emitter.pos[2];
    }
  }
}

void ChunkClipboard::rotate90Degrees()
{
  if (_cached_chunks.empty())
    return;

  auto const previous_target = _last_target_position;
  clearTarget();
  for (auto& [relative, cache] : _cached_chunks)
  {
    int const old_x = relative.rel_x;
    relative.rel_x = relative.rel_z;
    relative.rel_z = -old_x;
    transformCache90(cache);
  }

  if (previous_target)
    updateTarget(*previous_target);
  emit clipboardChanged(_cached_chunks.size());
}

void ChunkClipboard::rotateLeft90Degrees()
{
  if (_cached_chunks.empty())
    return;

  auto const previous_target = _last_target_position;
  clearTarget();

  // Counter-clockwise quarter turn = three clockwise quarter turns. Reuse the
  // exact already-proven cache transform rather than introducing a second path.
  for (int turn = 0; turn < 3; ++turn)
  {
    for (auto& [relative, cache] : _cached_chunks)
    {
      int const old_x = relative.rel_x;
      relative.rel_x = relative.rel_z;
      relative.rel_z = -old_x;
      transformCache90(cache);
    }
  }

  if (previous_target)
    updateTarget(*previous_target);
  emit clipboardChanged(_cached_chunks.size());
}

void ChunkClipboard::mirror(bool horizontal)
{
  if (_cached_chunks.empty())
    return;

  auto const previous_target = _last_target_position;
  clearTarget();
  for (auto& [relative, cache] : _cached_chunks)
  {
    if (horizontal)
      relative.rel_x = -relative.rel_x;
    else
      relative.rel_z = -relative.rel_z;
    transformCacheMirror(cache, horizontal);
  }

  if (previous_target)
    updateTarget(*previous_target);
  emit clipboardChanged(_cached_chunks.size());
}

void ChunkClipboard::updateTarget(glm::vec3 const& pos)
{
  clearTarget();
  if (_cached_chunks.empty())
    return;

  MapChunk* anchor = _world->getChunkAt(pos);
  if (!anchor)
    return;

  auto const anchor_index = index_for(anchor);
  int const anchor_x = static_cast<int>(anchor_index.tile_index.x * 16 + anchor_index.x);
  int const anchor_z = static_cast<int>(anchor_index.tile_index.z * 16 + anchor_index.z);

  for (auto const& [relative, cache] : _cached_chunks)
  {
    (void)cache;
    MapChunk* target = chunkAtGlobal(anchor_x + relative.rel_x, anchor_z + relative.rel_z, false);
    if (!target)
      continue;

    auto const target_index = index_for(target);
    _target_chunks.emplace(target_index);
    target->setChunkTransferOverlay(ChunkTransferOverlay::TARGET);
  }

  _last_target_position = pos;
}

void ChunkClipboard::clearTarget()
{
  auto const target_chunks = std::move(_target_chunks);
  _target_chunks.clear();
  for (auto const& index : target_chunks)
    restoreOverlay(index);
  _last_target_position.reset();
}

void ChunkClipboard::applyCache(MapChunk* target, ChunkCache const& cache,
                                ChunkPasteFlags flags, float height_offset)
{
  if (NOGGIT_CUR_ACTION)
    NOGGIT_CUR_ACTION->registerAllChunkChanges(target);

  if (cache.terrain_height)
  {
    for (std::size_t i = 0; i < cache.terrain_height->size(); ++i)
      target->mVertices[i].y = (*cache.terrain_height)[i].y + height_offset;
    if (cache.terrain_normals)
      std::copy(cache.terrain_normals->begin(), cache.terrain_normals->end(), std::begin(target->mNormals));
    target->recalcExtents();
    target->registerChunkUpdate(ChunkUpdateFlags::VERTEX | ChunkUpdateFlags::NORMALS);
  }

  if (cache.vertex_colors)
  {
    target->initMCCV();
    std::copy(cache.vertex_colors->begin(), cache.vertex_colors->end(), std::begin(target->mccv));
    target->registerChunkUpdate(ChunkUpdateFlags::MCCV);
  }

  if (cache.shadows)
  {
    std::copy(cache.shadows->begin(), cache.shadows->end(), std::begin(target->_shadow_map));
    target->registerChunkUpdate(ChunkUpdateFlags::SHADOW);
  }

  if (cache.holes)
  {
    target->holes = static_cast<int>(*cache.holes);
    target->registerChunkUpdate(ChunkUpdateFlags::HOLES);
  }
  if (cache.flags)
  {
    target->header_flags = *cache.flags;
    target->registerChunkUpdate(ChunkUpdateFlags::FLAGS);
  }
  if (cache.area_id)
  {
    target->areaID = *cache.area_id;
    target->registerChunkUpdate(ChunkUpdateFlags::AREA_ID);
  }

  if (cache.textures)
  {
    TextureSet* texture_set = target->getTextureSet();
    auto* target_textures = texture_set->getTextures();
    target_textures->clear();
    target_textures->reserve(cache.textures->textures.size());
    for (auto const& texture : cache.textures->textures)
      target_textures->emplace_back(texture, _world->getRenderContext());

    texture_set->setNTextures(cache.textures->texture_count);
    texture_set->setAlphamaps(cache.textures->alphamaps);
    texture_set->getTempAlphamaps().reset();
    std::copy(cache.textures->layer_info_entries.begin(), cache.textures->layer_info_entries.end(),
              texture_set->getMCLYEntries());
    std::copy(cache.textures->doodad_mapping.begin(), cache.textures->doodad_mapping.end(),
              texture_set->getDoodadMappingBase());
    std::copy(cache.textures->doodad_exclusion.begin(), cache.textures->doodad_exclusion.end(),
              texture_set->getDoodadStencilBase());
    texture_set->markDirty();
    target->registerChunkUpdate(ChunkUpdateFlags::ALPHAMAP | ChunkUpdateFlags::FLAGS
                              | ChunkUpdateFlags::GROUND_EFFECT
                              | ChunkUpdateFlags::DETAILDOODADS_EXCLUSION);
  }

  if (cache.liquid_layers && cache.liquid_attributes)
  {
    target->liquid_chunk()->replace_layers(*cache.liquid_layers, *cache.liquid_attributes,
                                           {target->xbase, target->ybase, target->zbase}, height_offset);
  }

  if (cache.sound_emitters)
  {
    target->sound_emitters = *cache.sound_emitters;
    float const center_x = target->xbase + CHUNKSIZE * .5f;
    float const center_z = target->zbase + CHUNKSIZE * .5f;
    for (auto& emitter : target->sound_emitters)
    {
      emitter.pos[0] += center_x;
      emitter.pos[1] += height_offset;
      emitter.pos[2] += center_z;
    }
  }

  if (cache.objects)
  {
    glm::vec3 const center{target->xbase + CHUNKSIZE * .5f, 0.f, target->zbase + CHUNKSIZE * .5f};

    if (hasFlag(flags, ChunkPasteFlags::REPLACE_OBJECTS))
    {
      auto objects = _world->getObjectsInRange(center, static_cast<float>(MAPCHUNK_RADIUS), true,
                                               hasFlag(_copy_flags, ChunkCopyFlags::WMOS),
                                               hasFlag(_copy_flags, ChunkCopyFlags::MODELS));
      objects.erase(std::remove_if(objects.begin(), objects.end(),
                                   [target](SceneObject const* object)
                                   {
                                     return !position_is_in_chunk(object->pos, target);
                                   }), objects.end());
      if (!objects.empty())
        _world->deleteObjects(objects, true);
    }

    for (auto const& object : *cache.objects)
    {
      glm::vec3 position = object.local_position;
      position.x += center.x;
      position.y += height_offset;
      position.z += center.z;

      if (object.type == ChunkManipulatorObjectTypes::WMO)
      {
        if (auto* instance = _world->addWMOAndGetInstance(object.file_key, position,
                                                          object.rotation, object.scale, true))
        {
          instance->change_nameset(object.wmo_nameset);
          instance->change_doodadset(object.wmo_doodadset);
        }
      }
      else
      {
        _world->addM2AndGetInstance(object.file_key, position, object.scale,
                                    object.rotation, nullptr, true, true);
      }
    }
  }

  _world->mapIndex.setChanged(target->mt);
}

void ChunkClipboard::repairTargetEdges(std::vector<MapChunk*> const& targets)
{
  for (MapChunk* target : targets)
  {
    MapChunk* left = _world->getChunkAt({target->xbase - CHUNKSIZE * .5f, 0.f,
                                        target->zbase + CHUNKSIZE * .5f});
    MapChunk* above = _world->getChunkAt({target->xbase + CHUNKSIZE * .5f, 0.f,
                                         target->zbase - CHUNKSIZE * .5f});
    MapChunk* right = _world->getChunkAt({target->xbase + CHUNKSIZE * 1.5f, 0.f,
                                         target->zbase + CHUNKSIZE * .5f});
    MapChunk* below = _world->getChunkAt({target->xbase + CHUNKSIZE * .5f, 0.f,
                                         target->zbase + CHUNKSIZE * 1.5f});

    if (left && NOGGIT_CUR_ACTION)
      NOGGIT_CUR_ACTION->registerAllChunkChanges(target);
    if (left && target->fixGapLeft(left))
      _world->mapIndex.setChanged(target->mt);

    if (above && NOGGIT_CUR_ACTION)
      NOGGIT_CUR_ACTION->registerAllChunkChanges(target);
    if (above && target->fixGapAbove(above))
      _world->mapIndex.setChanged(target->mt);

    if (right && NOGGIT_CUR_ACTION)
      NOGGIT_CUR_ACTION->registerAllChunkChanges(right);
    if (right && right->fixGapLeft(target))
      _world->mapIndex.setChanged(right->mt);

    if (below && NOGGIT_CUR_ACTION)
      NOGGIT_CUR_ACTION->registerAllChunkChanges(below);
    if (below && below->fixGapAbove(target))
      _world->mapIndex.setChanged(below->mt);

    _world->recalc_norms(target);
    if (left) _world->recalc_norms(left);
    if (above) _world->recalc_norms(above);
    if (right) _world->recalc_norms(right);
    if (below) _world->recalc_norms(below);
  }
}

bool ChunkClipboard::pasteSelection(glm::vec3 const& pos, ChunkPasteFlags flags, float height_offset)
{
  if (_cached_chunks.empty())
    return false;

  MapChunk* anchor = _world->getChunkAt(pos);
  if (!anchor)
    return false;

  auto const anchor_index = index_for(anchor);
  int const anchor_x = static_cast<int>(anchor_index.tile_index.x * 16 + anchor_index.x);
  int const anchor_z = static_cast<int>(anchor_index.tile_index.z * 16 + anchor_index.z);

  std::vector<MapChunk*> targets;
  targets.reserve(_cached_chunks.size());
  for (auto const& [relative, cache] : _cached_chunks)
  {
    (void)cache;
    MapChunk* target = chunkAtGlobal(anchor_x + relative.rel_x, anchor_z + relative.rel_z, true);
    if (!target)
      return false;
    targets.emplace_back(target);
  }

  clearTarget();
  for (std::size_t i = 0; i < targets.size(); ++i)
    applyCache(targets[i], _cached_chunks[i].second, flags, height_offset);

  if (hasFlag(flags, ChunkPasteFlags::FIX_GAPS))
    repairTargetEdges(targets);
  else
    for (MapChunk* target : targets)
      _world->recalc_norms(target);

  updateTarget(pos);
  emit pasted();
  return true;
}

ChunkCopyFlags ChunkClipboard::copyParams() const
{
  return _copy_flags;
}

void ChunkClipboard::setCopyParams(ChunkCopyFlags flags)
{
  _copy_flags = flags;
}

std::set<SelectedChunkIndex> const& ChunkClipboard::selectedChunks() const
{
  return _selected_chunks;
}

std::size_t ChunkClipboard::selectedCount() const
{
  return _selected_chunks.size();
}

bool ChunkClipboard::hasCopiedData() const
{
  return !_cached_chunks.empty();
}

bool ChunkClipboard::isSavedAsset() const
{
  return _saved_asset_mode;
}

std::vector<CachedChunkEntry> const& ChunkClipboard::cachedChunks() const
{
  return _cached_chunks;
}

std::optional<ChunkSourceMetadata> const& ChunkClipboard::sourceMetadata() const
{
  return _source_metadata;
}

void ChunkClipboard::replaceClipboard(std::vector<CachedChunkEntry>&& chunks, ChunkCopyFlags flags,
                                      std::optional<ChunkSourceMetadata> source, bool saved_asset)
{
  clearTarget();

  // Loading a persistent asset changes the clipboard authority. Drop any old live
  // source selection overlays without clearing the incoming clipboard data.
  for (auto const& index : _selected_chunks)
    setSourceOverlay(index, false);
  _selected_chunks.clear();
  emit selectionCleared();
  emit selectionChanged(_selected_chunks);

  _cached_chunks = std::move(chunks);
  _copy_flags = flags;
  _source_metadata = std::move(source);

  if (_saved_asset_mode != saved_asset)
  {
    _saved_asset_mode = saved_asset;
    emit savedAssetStateChanged(_saved_asset_mode);
  }

  emit clipboardChanged(_cached_chunks.size());
}
