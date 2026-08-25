// This file is part of Noggit3, licensed under GNU General Public License (version 3).
// Noggit_Gold persistent saved-chunk library.

#include "SavedChunkAsset.hpp"

#include <noggit/DBC.h>
#include <noggit/project/CurrentProject.hpp>

#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QString>

#include <algorithm>
#include <array>
#include <cctype>
#include <limits>
#include <system_error>
#include <utility>

using namespace Noggit::Ui::Tools::ChunkManipulator;

namespace
{
  constexpr char MAGIC[8] = {'N', 'G', 'C', 'H', 'U', 'N', 'K', '\0'};
  constexpr quint32 FORMAT_VERSION = 1;

  enum CachePresence : quint32
  {
    P_TERRAIN = 1u << 0,
    P_NORMALS = 1u << 1,
    P_VERTEX_COLORS = 1u << 2,
    P_SHADOWS = 1u << 3,
    P_LIQUID = 1u << 4,
    P_TEXTURES = 1u << 5,
    P_OBJECTS = 1u << 6,
    P_SOUND = 1u << 7,
    P_HOLES = 1u << 8,
    P_FLAGS = 1u << 9,
    P_AREA_ID = 1u << 10
  };

  void configure(QDataStream& stream)
  {
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
  }

  void writeVec2(QDataStream& out, glm::vec2 const& v)
  {
    out << v.x << v.y;
  }

  void writeVec3(QDataStream& out, glm::vec3 const& v)
  {
    out << v.x << v.y << v.z;
  }

  glm::vec2 readVec2(QDataStream& in)
  {
    glm::vec2 v{};
    in >> v.x >> v.y;
    return v;
  }

  glm::vec3 readVec3(QDataStream& in)
  {
    glm::vec3 v{};
    in >> v.x >> v.y >> v.z;
    return v;
  }

  template<std::size_t N>
  void writeVec3Array(QDataStream& out, std::array<glm::vec3, N> const& values)
  {
    for (auto const& value : values)
      writeVec3(out, value);
  }

  template<std::size_t N>
  std::array<glm::vec3, N> readVec3Array(QDataStream& in)
  {
    std::array<glm::vec3, N> values{};
    for (auto& value : values)
      value = readVec3(in);
    return values;
  }

  quint32 presenceMask(ChunkCache const& cache)
  {
    quint32 mask = 0;
    if (cache.terrain_height) mask |= P_TERRAIN;
    if (cache.terrain_normals) mask |= P_NORMALS;
    if (cache.vertex_colors) mask |= P_VERTEX_COLORS;
    if (cache.shadows) mask |= P_SHADOWS;
    if (cache.liquid_layers && cache.liquid_attributes) mask |= P_LIQUID;
    if (cache.textures) mask |= P_TEXTURES;
    if (cache.objects) mask |= P_OBJECTS;
    if (cache.sound_emitters) mask |= P_SOUND;
    if (cache.holes) mask |= P_HOLES;
    if (cache.flags) mask |= P_FLAGS;
    if (cache.area_id) mask |= P_AREA_ID;
    return mask;
  }

  void writeTextureCache(QDataStream& out, ChunkTextureCache const& textures)
  {
    out << static_cast<quint32>(textures.texture_count);
    out << static_cast<quint32>(textures.textures.size());
    for (auto const& texture : textures.textures)
      out << QString::fromStdString(texture);

    for (auto const& alphamap : textures.alphamaps)
    {
      out << static_cast<quint8>(alphamap ? 1 : 0);
      if (!alphamap)
        continue;

      QByteArray bytes;
      bytes.resize(64 * 64);
      for (int i = 0; i < 64 * 64; ++i)
        bytes[i] = static_cast<char>(alphamap->getAlpha(static_cast<std::size_t>(i)));
      out.writeRawData(bytes.constData(), bytes.size());
    }

    for (auto const& layer : textures.layer_info_entries)
      out << static_cast<quint32>(layer.flags) << static_cast<quint32>(layer.effectID);

    for (auto value : textures.doodad_mapping)
      out << static_cast<quint16>(value);
    for (auto value : textures.doodad_exclusion)
      out << static_cast<quint8>(value);
  }

  ChunkTextureCache readTextureCache(QDataStream& in)
  {
    ChunkTextureCache textures;
    quint32 texture_count = 0;
    quint32 path_count = 0;
    in >> texture_count >> path_count;
    textures.texture_count = texture_count;
    textures.textures.reserve(path_count);
    for (quint32 i = 0; i < path_count; ++i)
    {
      QString path;
      in >> path;
      textures.textures.emplace_back(path.toStdString());
    }

    for (auto& alphamap : textures.alphamaps)
    {
      quint8 present = 0;
      in >> present;
      if (!present)
        continue;

      QByteArray bytes(64 * 64, 0);
      if (in.readRawData(bytes.data(), bytes.size()) != bytes.size())
      {
        in.setStatus(QDataStream::ReadPastEnd);
        return textures;
      }

      alphamap = std::make_unique<Alphamap>();
      for (int i = 0; i < bytes.size(); ++i)
        alphamap->setAlpha(static_cast<std::size_t>(i), static_cast<unsigned char>(bytes.at(i)));
    }

    for (auto& layer : textures.layer_info_entries)
    {
      quint32 flags = 0;
      quint32 effect = 0;
      in >> flags >> effect;
      layer.flags = flags;
      layer.effectID = effect;
    }

    for (auto& value : textures.doodad_mapping)
    {
      quint16 v = 0;
      in >> v;
      value = v;
    }
    for (auto& value : textures.doodad_exclusion)
    {
      quint8 v = 0;
      in >> v;
      value = v;
    }

    return textures;
  }

  void writeLiquid(QDataStream& out, ChunkCache const& cache)
  {
    out << static_cast<quint64>(cache.liquid_attributes->fishable)
        << static_cast<quint64>(cache.liquid_attributes->fatigue);
    out << static_cast<quint32>(cache.liquid_layers->size());

    for (auto const& layer : *cache.liquid_layers)
    {
      out << static_cast<qint32>(layer.liquidID());
      out << static_cast<quint64>(layer.getSubchunks());
      for (auto const& vertex : layer.getVertices())
      {
        writeVec3(out, vertex.position);
        writeVec2(out, vertex.uv);
        out << vertex.depth;
      }
    }
  }

  void readLiquid(QDataStream& in, ChunkCache& cache)
  {
    quint64 fishable = 0;
    quint64 fatigue = 0;
    quint32 layer_count = 0;
    in >> fishable >> fatigue >> layer_count;

    cache.liquid_attributes = MH2O_Attributes{fishable, fatigue};
    cache.liquid_layers.emplace();
    cache.liquid_layers->reserve(layer_count);

    for (quint32 layer_index = 0; layer_index < layer_count; ++layer_index)
    {
      qint32 liquid_id = 0;
      quint64 subchunks = 0;
      in >> liquid_id >> subchunks;

      struct PortableLiquidVertex
      {
        glm::vec3 position;
        glm::vec2 uv;
        float depth = 0.f;
      };
      std::array<PortableLiquidVertex, 9 * 9> vertices{};
      for (auto& vertex : vertices)
      {
        vertex.position = readVec3(in);
        vertex.uv = readVec2(in);
        in >> vertex.depth;
      }

      glm::vec3 const base{vertices[0].position.x, 0.f, vertices[0].position.z};
      liquid_layer layer(nullptr, base, vertices[0].position.y, liquid_id);
      for (int z = 0; z < 8; ++z)
        for (int x = 0; x < 8; ++x)
          layer.setSubchunk(x, z, ((subchunks >> (z * 8 + x)) & quint64(1)) != 0);

      auto& restored_vertices = layer.getVertices();
      for (std::size_t i = 0; i < restored_vertices.size(); ++i)
      {
        restored_vertices[i].position = vertices[i].position;
        restored_vertices[i].uv = vertices[i].uv;
        restored_vertices[i].depth = vertices[i].depth;
      }
      // A zero-delta relocate recalculates min/max/fatigue from the restored vertices.
      layer.relocate(nullptr, base, 0.f);
      cache.liquid_layers->emplace_back(std::move(layer));
    }
  }

  void writeCache(QDataStream& out, CachedChunkEntry const& entry)
  {
    auto const& relative = entry.first;
    auto const& cache = entry.second;

    out << static_cast<quint32>(relative.source.tile_index.x)
        << static_cast<quint32>(relative.source.tile_index.z)
        << static_cast<quint32>(relative.source.x)
        << static_cast<quint32>(relative.source.z)
        << static_cast<qint32>(relative.rel_x)
        << static_cast<qint32>(relative.rel_z);

    quint32 const presence = presenceMask(cache);
    out << presence;

    if (cache.terrain_height) writeVec3Array(out, *cache.terrain_height);
    if (cache.terrain_normals) writeVec3Array(out, *cache.terrain_normals);
    if (cache.vertex_colors) writeVec3Array(out, *cache.vertex_colors);

    if (cache.shadows)
    {
      QByteArray bytes(reinterpret_cast<char const*>(cache.shadows->data()), static_cast<int>(cache.shadows->size()));
      out.writeRawData(bytes.constData(), bytes.size());
    }

    if (presence & P_LIQUID) writeLiquid(out, cache);
    if (cache.textures) writeTextureCache(out, *cache.textures);

    if (cache.objects)
    {
      out << static_cast<quint32>(cache.objects->size());
      for (auto const& object : *cache.objects)
      {
        out << QString::fromStdString(object.file_key.hasFilepath() ? object.file_key.filepath() : std::string{});
        out << static_cast<quint32>(object.file_key.hasFileDataID() ? object.file_key.fileDataID() : 0);
        out << static_cast<quint8>(object.type == ChunkManipulatorObjectTypes::WMO ? 0 : 1);
        writeVec3(out, object.local_position);
        writeVec3(out, object.rotation);
        out << object.scale
            << static_cast<quint16>(object.wmo_nameset)
            << static_cast<quint16>(object.wmo_doodadset);
      }
    }

    if (cache.sound_emitters)
    {
      out << static_cast<quint32>(cache.sound_emitters->size());
      for (auto const& emitter : *cache.sound_emitters)
      {
        out << static_cast<quint32>(emitter.soundId);
        for (float value : emitter.pos) out << value;
        for (float value : emitter.size) out << value;
      }
    }

    if (cache.holes) out << static_cast<quint32>(*cache.holes);
    if (cache.flags) out << static_cast<quint32>(cache.flags->value);
    if (cache.area_id) out << static_cast<quint32>(*cache.area_id);
  }

  CachedChunkEntry readCache(QDataStream& in)
  {
    quint32 tile_x = 0, tile_z = 0, chunk_x = 0, chunk_z = 0;
    qint32 rel_x = 0, rel_z = 0;
    quint32 presence = 0;
    in >> tile_x >> tile_z >> chunk_x >> chunk_z >> rel_x >> rel_z >> presence;

    SelectedChunkIndexRelative relative{
      SelectedChunkIndex{TileIndex{tile_x, tile_z}, chunk_x, chunk_z}, rel_x, rel_z};
    ChunkCache cache;

    if (presence & P_TERRAIN) cache.terrain_height = readVec3Array<145>(in);
    if (presence & P_NORMALS) cache.terrain_normals = readVec3Array<145>(in);
    if (presence & P_VERTEX_COLORS) cache.vertex_colors = readVec3Array<145>(in);

    if (presence & P_SHADOWS)
    {
      cache.shadows.emplace();
      if (in.readRawData(reinterpret_cast<char*>(cache.shadows->data()), static_cast<int>(cache.shadows->size()))
          != static_cast<int>(cache.shadows->size()))
        in.setStatus(QDataStream::ReadPastEnd);
    }

    if (presence & P_LIQUID) readLiquid(in, cache);
    if (presence & P_TEXTURES) cache.textures = readTextureCache(in);

    if (presence & P_OBJECTS)
    {
      quint32 count = 0;
      in >> count;
      cache.objects.emplace();
      cache.objects->reserve(count);
      for (quint32 i = 0; i < count; ++i)
      {
        QString path;
        quint32 file_data_id = 0;
        quint8 type = 0;
        in >> path >> file_data_id >> type;
        glm::vec3 const local_position = readVec3(in);
        glm::vec3 const rotation = readVec3(in);
        float scale = 1.f;
        quint16 nameset = 0, doodadset = 0;
        in >> scale >> nameset >> doodadset;

        BlizzardArchive::Listfile::FileKey key;
        if (!path.isEmpty())
          key = BlizzardArchive::Listfile::FileKey(path.toStdString(), file_data_id);
        else
          key = BlizzardArchive::Listfile::FileKey(file_data_id);

        cache.objects->push_back({std::move(key),
          type == 0 ? ChunkManipulatorObjectTypes::WMO : ChunkManipulatorObjectTypes::M2,
          local_position, rotation, scale, nameset, doodadset});
      }
    }

    if (presence & P_SOUND)
    {
      quint32 count = 0;
      in >> count;
      cache.sound_emitters.emplace();
      cache.sound_emitters->resize(count);
      for (auto& emitter : *cache.sound_emitters)
      {
        quint32 sound_id = 0;
        in >> sound_id;
        emitter.soundId = sound_id;
        for (float& value : emitter.pos) in >> value;
        for (float& value : emitter.size) in >> value;
      }
    }

    if (presence & P_HOLES)
    {
      quint32 value = 0;
      in >> value;
      cache.holes = value;
    }
    if (presence & P_FLAGS)
    {
      quint32 value = 0;
      in >> value;
      mcnk_flags flags{};
      flags.value = value;
      cache.flags = flags;
    }
    if (presence & P_AREA_ID)
    {
      quint32 value = 0;
      in >> value;
      cache.area_id = value;
    }

    return {std::move(relative), std::move(cache)};
  }

  std::string zoneName(ChunkSourceMetadata const& source)
  {
    std::string name = gAreaDB.getAreaFullName(static_cast<int>(source.area_id));
    auto const colon = name.find(':');
    if (colon != std::string::npos)
      name = name.substr(0, colon);
    if (name.empty() || name == "Unknown location")
      name = source.map_name.empty() ? "Unknown" : source.map_name;
    return name;
  }

  bool resolveLibraryMember(std::filesystem::path const& path,
                            std::filesystem::path& candidate,
                            std::string& error)
  {
    std::error_code ec;
    auto const directory = std::filesystem::weakly_canonical(SavedChunkAsset::libraryDirectory(), ec);
    if (ec)
    {
      error = "Unable to resolve the saved_chunks library: " + ec.message();
      return false;
    }

    ec.clear();
    candidate = std::filesystem::weakly_canonical(path, ec);
    if (ec)
    {
      error = "Unable to resolve the saved chunk path: " + ec.message();
      return false;
    }

    if (candidate.parent_path() != directory || candidate.extension() != ".ngchunk")
    {
      error = "Refusing to access a file outside the Noggit_Gold saved_chunks library.";
      return false;
    }
    return true;
  }
}

std::filesystem::path SavedChunkAsset::libraryDirectory()
{
  std::filesystem::path directory = Noggit::Project::CurrentProject::get()->ProjectPath;
  directory /= "saved_chunks";
  return directory;
}

std::vector<SavedChunkAssetRecord> SavedChunkAsset::list()
{
  std::vector<SavedChunkAssetRecord> records;
  std::error_code ec;
  auto const directory = libraryDirectory();
  if (!std::filesystem::exists(directory, ec))
    return records;

  for (auto const& entry : std::filesystem::directory_iterator(directory, ec))
  {
    if (ec || !entry.is_regular_file())
      continue;
    if (entry.path().extension() != ".ngchunk")
      continue;
    records.push_back({entry.path(), entry.path().stem().string()});
  }

  std::sort(records.begin(), records.end(), [](auto const& lhs, auto const& rhs)
  {
    return lhs.display_name < rhs.display_name;
  });
  return records;
}

std::string SavedChunkAsset::sanitizeName(std::string name)
{
  std::string clean;
  clean.reserve(name.size());
  bool last_separator = false;
  for (unsigned char c : name)
  {
    if (std::isalnum(c))
    {
      clean.push_back(static_cast<char>(c));
      last_separator = false;
    }
    else if (c == '-' || c == '_')
    {
      clean.push_back(static_cast<char>(c));
      last_separator = false;
    }
    else if (!last_separator)
    {
      clean.push_back('_');
      last_separator = true;
    }
  }

  while (!clean.empty() && clean.back() == '_') clean.pop_back();
  while (!clean.empty() && clean.front() == '_') clean.erase(clean.begin());
  return clean.empty() ? "SavedChunk" : clean;
}

std::string SavedChunkAsset::suggestedName(ChunkClipboard const& clipboard)
{
  auto const& source = clipboard.sourceMetadata();
  if (!source)
    return "SavedChunk";

  std::string name = sanitizeName(zoneName(*source));
  name += "-" + std::to_string(source->tile_index.x) + "_" + std::to_string(source->tile_index.z);
  name += "-c" + (source->chunk_x < 10 ? std::string("0") : std::string()) + std::to_string(source->chunk_x);
  name += "_" + (source->chunk_z < 10 ? std::string("0") : std::string()) + std::to_string(source->chunk_z);
  if (clipboard.cachedChunks().size() > 1)
    name += "-" + std::to_string(clipboard.cachedChunks().size()) + "chunks";
  return name;
}

bool SavedChunkAsset::save(std::string const& requested_name,
                           ChunkClipboard const& clipboard,
                           std::filesystem::path& saved_path,
                           std::string& error)
{
  if (!clipboard.hasCopiedData() || !clipboard.sourceMetadata())
  {
    error = "Copy one or more chunks before saving a persistent chunk asset.";
    return false;
  }

  std::error_code ec;
  auto const directory = libraryDirectory();
  std::filesystem::create_directories(directory, ec);
  if (ec)
  {
    error = "Unable to create saved_chunks directory: " + ec.message();
    return false;
  }

  std::string const base_name = sanitizeName(requested_name.empty() ? suggestedName(clipboard) : requested_name);
  saved_path = directory / (base_name + ".ngchunk");
  int suffix = 2;
  while (std::filesystem::exists(saved_path))
    saved_path = directory / (base_name + "_" + std::to_string(suffix++) + ".ngchunk");

  QSaveFile file(QString::fromStdString(saved_path.string()));
  if (!file.open(QIODevice::WriteOnly))
  {
    error = file.errorString().toStdString();
    return false;
  }

  QDataStream out(&file);
  configure(out);
  out.writeRawData(MAGIC, sizeof(MAGIC));
  out << FORMAT_VERSION;
  out << QString::fromStdString(saved_path.stem().string());

  auto const& source = *clipboard.sourceMetadata();
  out << static_cast<quint32>(source.map_id)
      << QString::fromStdString(source.map_name)
      << static_cast<quint32>(source.area_id)
      << static_cast<quint32>(source.tile_index.x)
      << static_cast<quint32>(source.tile_index.z)
      << static_cast<quint32>(source.chunk_x)
      << static_cast<quint32>(source.chunk_z)
      << static_cast<quint32>(clipboard.copyParams())
      << static_cast<quint32>(clipboard.cachedChunks().size());

  for (auto const& entry : clipboard.cachedChunks())
    writeCache(out, entry);

  if (out.status() != QDataStream::Ok || !file.commit())
  {
    error = file.errorString().toStdString();
    if (error.empty()) error = "Failed writing saved chunk asset.";
    return false;
  }

  return true;
}

bool SavedChunkAsset::load(std::filesystem::path const& path,
                           ChunkClipboard& clipboard,
                           std::string& display_name,
                           std::string& error)
{
  std::filesystem::path candidate;
  if (!resolveLibraryMember(path, candidate, error))
    return false;

  QFile file(QString::fromStdString(candidate.string()));
  if (!file.open(QIODevice::ReadOnly))
  {
    error = file.errorString().toStdString();
    return false;
  }

  QDataStream in(&file);
  configure(in);
  char magic[sizeof(MAGIC)]{};
  if (in.readRawData(magic, sizeof(magic)) != sizeof(magic) || !std::equal(std::begin(MAGIC), std::end(MAGIC), std::begin(magic)))
  {
    error = "Not a Noggit_Gold .ngchunk file.";
    return false;
  }

  quint32 version = 0;
  QString name;
  in >> version >> name;
  if (version != FORMAT_VERSION)
  {
    error = "Unsupported .ngchunk format version " + std::to_string(version) + ".";
    return false;
  }

  quint32 map_id = 0, area_id = 0, tile_x = 0, tile_z = 0, chunk_x = 0, chunk_z = 0;
  quint32 copy_flags = 0, chunk_count = 0;
  QString map_name;
  in >> map_id >> map_name >> area_id >> tile_x >> tile_z >> chunk_x >> chunk_z >> copy_flags >> chunk_count;

  if (chunk_count == 0 || chunk_count > 4096)
  {
    error = "Saved chunk asset has an invalid chunk count.";
    return false;
  }

  std::vector<CachedChunkEntry> chunks;
  chunks.reserve(chunk_count);
  for (quint32 i = 0; i < chunk_count; ++i)
    chunks.emplace_back(readCache(in));

  if (in.status() != QDataStream::Ok)
  {
    error = "Saved chunk asset is truncated or corrupt.";
    return false;
  }

  ChunkSourceMetadata source{
    map_id, map_name.toStdString(), area_id, TileIndex{tile_x, tile_z}, chunk_x, chunk_z};
  clipboard.replaceClipboard(std::move(chunks), static_cast<ChunkCopyFlags>(copy_flags), source, true);
  display_name = name.toStdString();
  return true;
}

bool SavedChunkAsset::remove(std::filesystem::path const& path, std::string& error)
{
  std::filesystem::path candidate;
  if (!resolveLibraryMember(path, candidate, error))
    return false;

  std::error_code ec;
  if (!std::filesystem::remove(candidate, ec))
  {
    error = ec ? ec.message() : "Saved chunk file was not found.";
    return false;
  }
  return true;
}
