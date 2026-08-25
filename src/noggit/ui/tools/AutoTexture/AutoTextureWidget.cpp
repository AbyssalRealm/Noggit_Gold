// This file is part of Noggit3, licensed under GNU General Public License (version 3).

#include <noggit/ui/tools/AutoTexture/AutoTextureWidget.hpp>

#include <noggit/ActionManager.hpp>
#include <noggit/MapChunk.h>
#include <noggit/MapHeaders.h>
#include <noggit/MapTile.h>
#include <noggit/MapView.h>
#include <noggit/scoped_blp_texture_reference.hpp>
#include <noggit/World.h>
#include <noggit/texture_set.hpp>
#include <noggit/ui/CurrentTexture.h>

#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QMetaObject>
#include <QPushButton>
#include <QProgressDialog>
#include <QSpinBox>
#include <QStringList>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <glm/geometric.hpp>
#include <set>
#include <sstream>

namespace
{
  struct TerrainTriangle
  {
    glm::vec3 a{};
    glm::vec3 normal{};
    float slope_degrees = 0.0f;
  };

  struct ChunkTerrainSampler
  {
    std::array<TerrainTriangle, 8 * 8 * 4> triangles;

    explicit ChunkTerrainSampler(MapChunk const* chunk)
    {
      constexpr float RAD_TO_DEG = 57.295779513082320876f;

      for (int unit_z = 0; unit_z < 8; ++unit_z)
      {
        for (int unit_x = 0; unit_x < 8; ++unit_x)
        {
          int const top_left = 17 * unit_z + unit_x;
          int const top_right = top_left + 1;
          int const center = 17 * unit_z + 9 + unit_x;
          int const bottom_left = 17 * (unit_z + 1) + unit_x;
          int const bottom_right = bottom_left + 1;

          std::array<std::array<int, 3>, 4> const indices = {{
            {{top_left, top_right, center}},       // top
            {{top_right, bottom_right, center}},  // right
            {{bottom_right, bottom_left, center}},// bottom
            {{bottom_left, top_left, center}}     // left
          }};

          for (int tri = 0; tri < 4; ++tri)
          {
            glm::vec3 const& a = chunk->mVertices[indices[tri][0]];
            glm::vec3 const& b = chunk->mVertices[indices[tri][1]];
            glm::vec3 const& c = chunk->mVertices[indices[tri][2]];

            glm::vec3 normal = glm::cross(b - a, c - a);
            float const len = glm::length(normal);
            if (len > 0.000001f)
              normal /= len;
            else
              normal = glm::vec3(0.f, 1.f, 0.f);

            float const up_dot = std::clamp(std::abs(normal.y), 0.0f, 1.0f);
            TerrainTriangle& out = triangles[((unit_z * 8 + unit_x) * 4) + tri];
            out.a = a;
            out.normal = normal;
            out.slope_degrees = std::acos(up_dot) * RAD_TO_DEG;
          }
        }
      }
    }

    std::pair<float, float> sample(MapChunk const* chunk, float world_x, float world_z) const
    {
      float const local_x = std::clamp(world_x - chunk->xbase, 0.0f, CHUNKSIZE - 0.0001f);
      float const local_z = std::clamp(world_z - chunk->zbase, 0.0f, CHUNKSIZE - 0.0001f);

      int const unit_x = std::clamp(static_cast<int>(local_x / UNITSIZE), 0, 7);
      int const unit_z = std::clamp(static_cast<int>(local_z / UNITSIZE), 0, 7);

      float const tx = (local_x - unit_x * UNITSIZE) / UNITSIZE;
      float const tz = (local_z - unit_z * UNITSIZE) / UNITSIZE;
      float const dx = tx - 0.5f;
      float const dz = tz - 0.5f;

      int tri = 0;
      if (std::abs(dx) > std::abs(dz))
        tri = dx > 0.0f ? 1 : 3;
      else
        tri = dz > 0.0f ? 2 : 0;

      TerrainTriangle const& triangle = triangles[((unit_z * 8 + unit_x) * 4) + tri];
      glm::vec3 const& n = triangle.normal;

      float height = triangle.a.y;
      if (std::abs(n.y) > 0.000001f)
      {
        height = triangle.a.y - (n.x * (world_x - triangle.a.x) + n.z * (world_z - triangle.a.z)) / n.y;
      }

      return {height, triangle.slope_degrees};
    }
  };

  float smoothstep01(float t)
  {
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
  }

  std::uint32_t hash_coords(std::int32_t x, std::int32_t z)
  {
    std::uint32_t h = static_cast<std::uint32_t>(x) * 0x8da6b343u;
    h ^= static_cast<std::uint32_t>(z) * 0xd8163841u;
    h ^= h >> 13;
    h *= 0xcb1ab31fu;
    h ^= h >> 16;
    return h;
  }

  float hash_value(std::int32_t x, std::int32_t z)
  {
    return (static_cast<float>(hash_coords(x, z) & 0x00FFFFFFu) / static_cast<float>(0x00FFFFFFu)) * 2.0f - 1.0f;
  }

  float world_noise(float world_x, float world_z, float scale)
  {
    scale = std::max(scale, 0.001f);
    float const fx = world_x / scale;
    float const fz = world_z / scale;
    auto const ix = static_cast<std::int32_t>(std::floor(fx));
    auto const iz = static_cast<std::int32_t>(std::floor(fz));
    float const tx = smoothstep01(fx - std::floor(fx));
    float const tz = smoothstep01(fz - std::floor(fz));

    float const n00 = hash_value(ix, iz);
    float const n10 = hash_value(ix + 1, iz);
    float const n01 = hash_value(ix, iz + 1);
    float const n11 = hash_value(ix + 1, iz + 1);

    float const nx0 = n00 + (n10 - n00) * tx;
    float const nx1 = n01 + (n11 - n01) * tx;
    return nx0 + (nx1 - nx0) * tz;
  }

  float descending_weight(float value, float full_at_or_below, float zero_at_or_above)
  {
    if (value <= full_at_or_below)
      return 1.0f;
    if (value >= zero_at_or_above)
      return 0.0f;
    return 1.0f - smoothstep01((value - full_at_or_below) / (zero_at_or_above - full_at_or_below));
  }

  float ascending_weight(float value, float zero_at_or_below, float full_at_or_above)
  {
    if (value <= zero_at_or_below)
      return 0.0f;
    if (value >= full_at_or_above)
      return 1.0f;
    return smoothstep01((value - zero_at_or_below) / (full_at_or_above - zero_at_or_below));
  }
}

namespace Noggit::Ui::Tools
{
  AutoTextureWidget::AutoTextureWidget(MapView* map_view, Noggit::Ui::current_texture* current_texture, QWidget* parent)
    : QWidget(parent)
    , _map_view(map_view)
    , _current_texture(current_texture)
  {
    auto* root = new QVBoxLayout(this);
    root->setAlignment(Qt::AlignTop);

    auto* warning = new QLabel(
      "PASS1 safety boundary: only explicitly selected, currently loaded ADTs are processed. "
      "Auto Texture replaces the existing four-layer terrain palette on those ADTs.", this);
    warning->setWordWrap(true);
    warning->setStyleSheet("QLabel { color : orange; }");
    root->addWidget(warning);

    auto* adt_group = new QGroupBox("ADT Selection (maximum 8)", this);
    auto* adt_layout = new QVBoxLayout(adt_group);

    auto* current_row = new QHBoxLayout();
    auto* add_current = new QPushButton("Add Current ADT", adt_group);
    current_row->addWidget(add_current);
    adt_layout->addLayout(current_row);

    auto* coordinate_row = new QHBoxLayout();
    _adt_x = new QSpinBox(adt_group);
    _adt_z = new QSpinBox(adt_group);
    _adt_x->setRange(0, 63);
    _adt_z->setRange(0, 63);
    TileIndex const initial_tile(_map_view->getCamera()->position);
    if (initial_tile.is_valid())
    {
      _adt_x->setValue(static_cast<int>(initial_tile.x));
      _adt_z->setValue(static_cast<int>(initial_tile.z));
    }
    auto* add_coordinates = new QPushButton("Add X/Z", adt_group);
    coordinate_row->addWidget(new QLabel("X", adt_group));
    coordinate_row->addWidget(_adt_x);
    coordinate_row->addWidget(new QLabel("Z", adt_group));
    coordinate_row->addWidget(_adt_z);
    coordinate_row->addWidget(add_coordinates);
    adt_layout->addLayout(coordinate_row);

    _selected_adts_list = new QListWidget(adt_group);
    _selected_adts_list->setMaximumHeight(82);
    adt_layout->addWidget(_selected_adts_list);

    auto* selection_buttons = new QHBoxLayout();
    auto* remove_selected = new QPushButton("Remove Selected", adt_group);
    auto* clear_selection = new QPushButton("Clear", adt_group);
    selection_buttons->addWidget(remove_selected);
    selection_buttons->addWidget(clear_selection);
    adt_layout->addLayout(selection_buttons);

    _selection_status = new QLabel("Selected: 0 / 8", adt_group);
    adt_layout->addWidget(_selection_status);

    _terrain_height_range = new QLabel("<b>Selected Terrain Height</b><br>No ADTs selected.", adt_group);
    _terrain_height_range->setWordWrap(true);
    adt_layout->addWidget(_terrain_height_range);

    _height_band_status = new QLabel("<b>Height Bands</b><br>Select loaded ADTs to check Low/High coverage.", adt_group);
    _height_band_status->setWordWrap(true);
    adt_layout->addWidget(_height_band_status);

    auto* fit_heights = new QPushButton("Fit Heights", adt_group);
    fit_heights->setMinimumHeight(32);
    fit_heights->setStyleSheet("QPushButton { font-weight: bold; }");
    fit_heights->setToolTip("Fit the Low/High elevation blend bands to the selected terrain Min/Max height range.");
    adt_layout->addWidget(fit_heights);

    root->addWidget(adt_group);

    auto* textures_group = new QGroupBox("Four Texture Layers", this);
    auto* textures_layout = new QGridLayout(textures_group);
    textures_layout->setColumnStretch(1, 1);
    std::array<QString, 4> const names = {"Base", "Low Ground", "High Ground", "Cliff"};

    for (std::size_t i = 0; i < _texture_slots.size(); ++i)
    {
      auto* label = new QLabel(names[i], textures_group);
      label->setStyleSheet("QLabel { font-weight: bold; }");
      auto* preview = new Noggit::Ui::current_texture(true, textures_group);
      preview->setFixedSize(56, 56);
      preview->setScaledContents(true);
      auto* filename = new QPushButton("Choose Texture...", textures_group);
      filename->setFlat(true);
      filename->setCursor(Qt::PointingHandCursor);
      filename->setStyleSheet(
        "QPushButton { text-align: left; padding: 2px; border: none; }"
        "QPushButton:hover { text-decoration: underline; }");
      filename->setToolTip("Click to choose this Auto Texture layer from Noggit's Texture Browser.");
      auto* use_current = new QPushButton("Use Current", textures_group);
      use_current->setToolTip("Assign the Texture Painter's current texture to this Auto Texture layer.");

      preview->setCursor(Qt::PointingHandCursor);
      preview->setToolTip("Click to choose this Auto Texture layer from Noggit's Texture Browser, or drag/drop a texture here.");

      _texture_slots[i].preview = preview;
      _texture_slots[i].filename_button = filename;

      int const row = static_cast<int>(i) * 2;
      textures_layout->addWidget(label, row, 0, 1, 2);
      textures_layout->addWidget(use_current, row, 2);
      textures_layout->addWidget(preview, row + 1, 0, Qt::AlignTop | Qt::AlignHCenter);
      textures_layout->addWidget(filename, row + 1, 1, 1, 2, Qt::AlignVCenter);

      connect(use_current, &QPushButton::clicked, this, [this, i]()
      {
        if (!_current_texture)
          return;
        _pending_texture_slot.reset();
        set_texture_slot(i, _current_texture->filename());
      });

      connect(preview, &Noggit::Ui::current_texture::clicked, this, [this, i]()
      {
        choose_texture_for_slot(i);
      });

      connect(filename, &QPushButton::clicked, this, [this, i]()
      {
        choose_texture_for_slot(i);
      });

      connect(preview, &Noggit::Ui::current_texture::texture_dropped, this, [this, i](std::string const& dropped)
      {
        _pending_texture_slot.reset();
        set_texture_slot(i, dropped);
      });
    }
    root->addWidget(textures_group);

    auto* elevation_group = new QGroupBox("Elevation Blend", this);
    auto* elevation_layout = new QFormLayout(elevation_group);

    auto make_height_spin = [elevation_group](double value)
    {
      auto* spin = new QDoubleSpinBox(elevation_group);
      spin->setRange(-5000.0, 5000.0);
      spin->setDecimals(1);
      spin->setSingleStep(5.0);
      spin->setValue(value);
      return spin;
    };

    _low_full = make_height_spin(0.0);
    _low_fade_end = make_height_spin(20.0);
    _high_fade_start = make_height_spin(100.0);
    _high_full = make_height_spin(140.0);
    elevation_layout->addRow("Low full below:", _low_full);
    elevation_layout->addRow("Low fade ends:", _low_fade_end);
    elevation_layout->addRow("High fade starts:", _high_fade_start);
    elevation_layout->addRow("High full above:", _high_full);
    root->addWidget(elevation_group);

    auto* slope_group = new QGroupBox("Cliff Slope Blend", this);
    auto* slope_layout = new QFormLayout(slope_group);
    _cliff_start = new QDoubleSpinBox(slope_group);
    _cliff_full = new QDoubleSpinBox(slope_group);
    for (auto* spin : {_cliff_start, _cliff_full})
    {
      spin->setRange(0.0, 89.9);
      spin->setDecimals(1);
      spin->setSingleStep(1.0);
      spin->setSuffix(" deg");
    }
    _cliff_start->setValue(38.0);
    _cliff_full->setValue(55.0);
    slope_layout->addRow("Cliff starts:", _cliff_start);
    slope_layout->addRow("Cliff full:", _cliff_full);
    root->addWidget(slope_group);

    auto* variation_group = new QGroupBox("Boundary Variation", this);
    auto* variation_layout = new QFormLayout(variation_group);
    _noise_amount = new QDoubleSpinBox(variation_group);
    _noise_amount->setRange(0.0, 100.0);
    _noise_amount->setDecimals(1);
    _noise_amount->setSingleStep(1.0);
    _noise_amount->setValue(4.0);
    _noise_amount->setSuffix(" height");
    _noise_scale = new QDoubleSpinBox(variation_group);
    _noise_scale->setRange(1.0, 1000.0);
    _noise_scale->setDecimals(1);
    _noise_scale->setSingleStep(5.0);
    _noise_scale->setValue(24.0);
    _noise_scale->setSuffix(" world units");
    variation_layout->addRow("Noise amount:", _noise_amount);
    variation_layout->addRow("Noise scale:", _noise_scale);
    root->addWidget(variation_group);

    _apply_button = new QPushButton("Apply Auto Texture to Selected ADTs", this);
    _apply_button->setMinimumHeight(34);
    root->addWidget(_apply_button);

    connect(add_current, &QPushButton::clicked, this, [this]()
    {
      TileIndex const current(_map_view->getCamera()->position);
      _adt_x->setValue(static_cast<int>(current.x));
      _adt_z->setValue(static_cast<int>(current.z));
      add_adt(current);
    });

    connect(add_coordinates, &QPushButton::clicked, this, [this]()
    {
      add_adt(TileIndex(static_cast<std::size_t>(_adt_x->value()), static_cast<std::size_t>(_adt_z->value())));
    });
    connect(remove_selected, &QPushButton::clicked, this, [this]() { remove_selected_adt(); });
    connect(clear_selection, &QPushButton::clicked, this, [this]() { clear_adts(); });
    connect(fit_heights, &QPushButton::clicked, this, [this]() { fit_height_ranges(); });
    connect(_apply_button, &QPushButton::clicked, this, [this]() { apply_auto_texture(); });

    for (auto* spin : {_low_full, _low_fade_end, _high_fade_start, _high_full})
    {
      connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
        [this](double) { update_height_feedback(); });
    }

    if (_current_texture)
    {
      connect(_current_texture, &Noggit::Ui::current_texture::texture_updated, this, [this]()
      {
        if (!_pending_texture_slot.has_value())
          return;

        std::size_t const slot = *_pending_texture_slot;
        _pending_texture_slot.reset();
        set_texture_slot(slot, _current_texture->filename());
      });
    }

    update_height_feedback();
  }

  void AutoTextureWidget::add_adt(TileIndex const& index)
  {
    if (!index.is_valid())
      return;

    auto* world = _map_view->getWorld();
    if (!world->mapIndex.hasTile(index))
    {
      QMessageBox::warning(this, "ADT does not exist", "That X/Z coordinate is not an existing ADT on this map.");
      return;
    }

    if (std::find(_selected_adts.begin(), _selected_adts.end(), index) != _selected_adts.end())
      return;

    if (_selected_adts.size() >= MAX_SELECTED_ADTS)
    {
      QMessageBox::warning(this, "ADT limit reached", "Auto Texture PASS1 is limited to 8 explicitly selected ADTs per operation.");
      return;
    }

    _selected_adts.push_back(index);
    refresh_adt_list();
  }

  void AutoTextureWidget::remove_selected_adt()
  {
    int const row = _selected_adts_list->currentRow();
    if (row < 0 || static_cast<std::size_t>(row) >= _selected_adts.size())
      return;

    _selected_adts.erase(_selected_adts.begin() + row);
    refresh_adt_list();
  }

  void AutoTextureWidget::clear_adts()
  {
    _selected_adts.clear();
    refresh_adt_list();
  }

  void AutoTextureWidget::refresh_adt_list()
  {
    _selected_adts_list->clear();
    auto* world = _map_view->getWorld();
    for (TileIndex const& index : _selected_adts)
    {
      MapTile* tile = world->mapIndex.getTile(index);
      bool const loaded = tile && tile->finishedLoading();
      QString const label = QString("%1_%2_%3  [%4]")
        .arg(QString::fromStdString(world->basename))
        .arg(static_cast<qulonglong>(index.x))
        .arg(static_cast<qulonglong>(index.z))
        .arg(loaded ? "loaded" : "NOT loaded");
      _selected_adts_list->addItem(label);
    }
    _selection_status->setText(QString("Selected: %1 / %2")
      .arg(static_cast<qulonglong>(_selected_adts.size())).arg(static_cast<qulonglong>(MAX_SELECTED_ADTS)));
    update_height_feedback();
  }

  void AutoTextureWidget::choose_texture_for_slot(std::size_t slot)
  {
    if (slot >= _texture_slots.size() || !_current_texture || !_map_view)
      return;

    _pending_texture_slot = slot;

    QDockWidget* texture_browser = nullptr;
    if (QWidget* top_level = _map_view->window())
    {
      for (QDockWidget* dock : top_level->findChildren<QDockWidget*>())
      {
        if (dock && dock->windowTitle() == "Texture Browser")
        {
          texture_browser = dock;
          break;
        }
      }
    }

    if (texture_browser && !_texture_browser_close_guard_connected)
    {
      connect(texture_browser, &QDockWidget::visibilityChanged, this, [this](bool visible)
      {
        if (!visible)
          _pending_texture_slot.reset();
      });
      _texture_browser_close_guard_connected = true;
    }

    // Reuse the exact existing Texture Painter click path so the normal browser
    // toggle/property ownership stays with TexturingTool. Do not create a second picker.
    if (!texture_browser || !texture_browser->isVisible())
      QMetaObject::invokeMethod(_current_texture, "clicked", Qt::DirectConnection);

    if (!texture_browser && _map_view->window())
    {
      for (QDockWidget* dock : _map_view->window()->findChildren<QDockWidget*>())
      {
        if (dock && dock->windowTitle() == "Texture Browser")
        {
          texture_browser = dock;
          break;
        }
      }

      if (texture_browser && !_texture_browser_close_guard_connected)
      {
        connect(texture_browser, &QDockWidget::visibilityChanged, this, [this](bool visible)
        {
          if (!visible)
            _pending_texture_slot.reset();
        });
        _texture_browser_close_guard_connected = true;
      }
    }

    if (texture_browser)
    {
      texture_browser->raise();
      texture_browser->activateWindow();
    }
  }

  void AutoTextureWidget::set_texture_slot(std::size_t slot, std::string const& filename)
  {
    if (slot >= _texture_slots.size() || filename.empty())
      return;

    TextureSlot& target = _texture_slots[slot];
    target.filename = filename;
    target.preview->set_texture(filename);

    QString full_name = QString::fromStdString(filename);
    QString display_name = full_name;
    display_name.replace('\\', '/');
    display_name = display_name.section('/', -1);
    target.filename_button->setText(display_name);
    target.filename_button->setToolTip(QString("Click to choose this Auto Texture layer from Noggit\'s Texture Browser.\nCurrent: %1").arg(full_name));
    target.preview->setToolTip(QString("Click to choose this Auto Texture layer from Noggit\'s Texture Browser, or drag/drop a texture here.\nCurrent: %1").arg(full_name));
  }

  bool AutoTextureWidget::scan_selected_height_range(float& min_height, float& max_height) const
  {
    if (_selected_adts.empty())
      return false;

    min_height = std::numeric_limits<float>::max();
    max_height = std::numeric_limits<float>::lowest();

    for (TileIndex const& index : _selected_adts)
    {
      MapTile* tile = _map_view->getWorld()->mapIndex.getTile(index);
      if (!tile || !tile->finishedLoading())
        return false;

      for (unsigned chunk_z = 0; chunk_z < 16; ++chunk_z)
      {
        for (unsigned chunk_x = 0; chunk_x < 16; ++chunk_x)
        {
          MapChunk* chunk = tile->getChunk(chunk_x, chunk_z);
          if (!chunk)
            continue;

          for (glm::vec3 const& vertex : chunk->mVertices)
          {
            min_height = std::min(min_height, vertex.y);
            max_height = std::max(max_height, vertex.y);
          }
        }
      }
    }

    return min_height != std::numeric_limits<float>::max()
      && max_height != std::numeric_limits<float>::lowest();
  }

  void AutoTextureWidget::update_height_feedback()
  {
    if (!_terrain_height_range || !_height_band_status)
      return;

    if (_selected_adts.empty())
    {
      _terrain_height_range->setText("<b>Selected Terrain Height</b><br>No ADTs selected.");
      _height_band_status->setText("<b>Height Bands</b><br>Select loaded ADTs to check Low/High coverage.");
      _height_band_status->setStyleSheet(QString());
      return;
    }

    float min_height = 0.0f;
    float max_height = 0.0f;
    if (!scan_selected_height_range(min_height, max_height))
    {
      _terrain_height_range->setText(
        "<b>Selected Terrain Height</b><br>Range unavailable.<br>Every selected ADT must be loaded.");
      _height_band_status->setText(
        "<b>Height Bands</b><br>Load every selected ADT before fitting or checking Low/High bands.");
      _height_band_status->setStyleSheet("QLabel { color : orange; }");
      return;
    }

    _terrain_height_range->setText(QString(
      "<b>Selected Terrain Height</b><br>Minimum: %1<br>Maximum: %2<br>Span: %3")
      .arg(min_height, 0, 'f', 1)
      .arg(max_height, 0, 'f', 1)
      .arg(max_height - min_height, 0, 'f', 1));

    if (!(_low_full->value() < _low_fade_end->value()) || !(_high_fade_start->value() < _high_full->value()))
    {
      QStringList invalid_ranges;
      invalid_ranges << "<b>Height Bands</b>";
      if (!(_low_full->value() < _low_fade_end->value()))
        invalid_ranges << "Low range invalid:<br>Low Full must be below Low Fade End.";
      if (!(_high_fade_start->value() < _high_full->value()))
        invalid_ranges << "High range invalid:<br>High Fade Start must be below High Full.";
      _height_band_status->setText(invalid_ranges.join("<br>"));
      _height_band_status->setStyleSheet("QLabel { color : orange; font-weight: bold; }");
      return;
    }

    bool const low_intersects = min_height < static_cast<float>(_low_fade_end->value());
    bool const high_intersects = max_height > static_cast<float>(_high_fade_start->value());

    if (low_intersects && high_intersects)
    {
      _height_band_status->setText("<b>Height Bands</b><br>Low Ground: Active<br>High Ground: Active");
      _height_band_status->setStyleSheet(QString());
      return;
    }

    QStringList warnings;
    warnings << "<b>Height Bands</b>";
    if (low_intersects)
    {
      warnings << "Low Ground: Active";
    }
    else
    {
      warnings << QString(
        "Low Ground: OUTSIDE TERRAIN RANGE<br>Low band ends at %1<br>Terrain minimum is %2")
        .arg(_low_fade_end->value(), 0, 'f', 1)
        .arg(min_height, 0, 'f', 1);
    }

    if (high_intersects)
    {
      warnings << "High Ground: Active";
    }
    else
    {
      warnings << QString(
        "High Ground: OUTSIDE TERRAIN RANGE<br>High band starts at %1<br>Terrain maximum is %2")
        .arg(_high_fade_start->value(), 0, 'f', 1)
        .arg(max_height, 0, 'f', 1);
    }

    _height_band_status->setText(warnings.join("<br>"));
    _height_band_status->setStyleSheet("QLabel { color : orange; font-weight: bold; }");
  }

  void AutoTextureWidget::fit_height_ranges()
  {
    if (_selected_adts.empty())
    {
      QMessageBox::information(this, "No ADTs selected", "Add at least one loaded ADT first.");
      return;
    }

    float min_height = 0.0f;
    float max_height = 0.0f;
    if (!scan_selected_height_range(min_height, max_height))
    {
      QMessageBox::warning(this, "ADT not loaded", "Every selected ADT must be loaded before fitting height ranges.");
      update_height_feedback();
      return;
    }

    if (!(max_height > min_height))
      return;

    float const range = max_height - min_height;
    _low_full->setValue(min_height + range * 0.10f);
    _low_fade_end->setValue(min_height + range * 0.30f);
    _high_fade_start->setValue(min_height + range * 0.70f);
    _high_full->setValue(min_height + range * 0.90f);
    update_height_feedback();
  }

  void AutoTextureWidget::apply_auto_texture()
  {
    if (NOGGIT_CUR_ACTION)
    {
      QMessageBox::warning(this, "Action already active", "Finish the current edit action before running Auto Texture.");
      return;
    }

    if (_selected_adts.empty())
    {
      QMessageBox::warning(this, "No ADTs selected", "Select at least one ADT first.");
      return;
    }

    std::set<std::string> unique_textures;
    for (TextureSlot const& slot : _texture_slots)
    {
      if (slot.filename.empty())
      {
        QMessageBox::warning(this, "Texture missing", "Base, Low Ground, High Ground, and Cliff textures must all be assigned.");
        return;
      }
      unique_textures.insert(slot.filename);
    }
    if (unique_textures.size() != 4)
    {
      QMessageBox::warning(this, "Textures must be distinct", "Auto Texture PASS1 requires four different texture files.");
      return;
    }

    double const low_full = _low_full->value();
    double const low_fade_end = _low_fade_end->value();
    double const high_fade_start = _high_fade_start->value();
    double const high_full = _high_full->value();
    double const cliff_start = _cliff_start->value();
    double const cliff_full = _cliff_full->value();

    if (!(low_full < low_fade_end))
    {
      QMessageBox::warning(this, "Invalid low-ground range", "Low Full must be below Low Fade End.");
      return;
    }
    if (!(high_fade_start < high_full))
    {
      QMessageBox::warning(this, "Invalid high-ground range", "High Fade Start must be below High Full.");
      return;
    }
    if (!(cliff_start < cliff_full))
    {
      QMessageBox::warning(this, "Invalid cliff range", "Cliff Start must be below Cliff Full.");
      return;
    }

    auto* world = _map_view->getWorld();
    for (TileIndex const& index : _selected_adts)
    {
      MapTile* tile = world->mapIndex.getTile(index);
      if (!tile || !tile->finishedLoading())
      {
        QString const message = QString("%1_%2_%3 is selected but not currently loaded. Move/load that ADT before Apply; PASS1 will not auto-load tiles behind your back.")
          .arg(QString::fromStdString(world->basename))
          .arg(static_cast<qulonglong>(index.x))
          .arg(static_cast<qulonglong>(index.z));
        QMessageBox::warning(this, "Selected ADT not loaded", message);
        refresh_adt_list();
        return;
      }
    }

    QMessageBox::StandardButton const answer = QMessageBox::warning(
      this,
      "Apply Auto Texture",
      QString("Auto Texture will REPLACE the existing terrain texture layers on %1 selected ADT(s).\n\n"
              "The operation is recorded as one normal texture action and can be reverted with Ctrl+Z.\n\nContinue?")
        .arg(static_cast<qulonglong>(_selected_adts.size())),
      QMessageBox::Yes | QMessageBox::No,
      QMessageBox::No);
    if (answer != QMessageBox::Yes)
      return;

    auto* action = NOGGIT_ACTION_MGR->beginAction(_map_view, Noggit::ActionFlags::eCHUNKS_TEXTURE);

    float const noise_amount = static_cast<float>(_noise_amount->value());
    float const noise_scale = static_cast<float>(_noise_scale->value());

    std::size_t chunks_changed = 0;
    int const total_chunks = static_cast<int>(_selected_adts.size() * 16 * 16);
    QProgressDialog progress("Auto Texturing selected ADTs...", QString(), 0, total_chunks, this);
    progress.setWindowTitle("Noggit Gold Auto Texture");
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setValue(0);

    for (TileIndex const& index : _selected_adts)
    {
      MapTile* tile = world->mapIndex.getTile(index);
      if (!tile)
        continue;

      for (unsigned chunk_z = 0; chunk_z < 16; ++chunk_z)
      {
        for (unsigned chunk_x = 0; chunk_x < 16; ++chunk_x)
        {
          MapChunk* chunk = tile->getChunk(chunk_x, chunk_z);
          if (!chunk || !chunk->getTextureSet())
            continue;

          action->registerChunkTextureChange(chunk);

          TextureSet* texture_set = chunk->getTextureSet();
          texture_set->eraseTextures();
          for (TextureSlot const& slot : _texture_slots)
          {
            texture_set->addTexture(scoped_blp_texture_reference(slot.filename, world->getRenderContext()));
          }

          texture_set->create_temporary_alphamaps_if_needed();
          auto& temp_ptr = texture_set->getTempAlphamaps();
          if (!temp_ptr)
            continue;

          ChunkTerrainSampler const sampler(chunk);
          tmp_edit_alpha_values& alpha = *temp_ptr;

          constexpr int ALPHA_SIZE = 64;
          float const texel_size = CHUNKSIZE / static_cast<float>(ALPHA_SIZE);

          for (int az = 0; az < ALPHA_SIZE; ++az)
          {
            for (int ax = 0; ax < ALPHA_SIZE; ++ax)
            {
              int const offset = az * ALPHA_SIZE + ax;
              float const world_x = chunk->xbase + (static_cast<float>(ax) + 0.5f) * texel_size;
              float const world_z = chunk->zbase + (static_cast<float>(az) + 0.5f) * texel_size;

              auto const [terrain_height, slope] = sampler.sample(chunk, world_x, world_z);
              float const varied_height = terrain_height + world_noise(world_x, world_z, noise_scale) * noise_amount;

              float low = descending_weight(varied_height, static_cast<float>(low_full), static_cast<float>(low_fade_end));
              float high = ascending_weight(varied_height, static_cast<float>(high_fade_start), static_cast<float>(high_full));

              if (low + high > 1.0f)
              {
                float const inv = 1.0f / (low + high);
                low *= inv;
                high *= inv;
              }

              float base = std::max(0.0f, 1.0f - low - high);
              float const cliff = ascending_weight(slope, static_cast<float>(cliff_start), static_cast<float>(cliff_full));
              float const non_cliff = 1.0f - cliff;

              base *= non_cliff;
              low *= non_cliff;
              high *= non_cliff;

              float total = base + low + high + cliff;
              if (total <= 0.000001f)
              {
                base = 1.0f;
                low = high = 0.0f;
                total = 1.0f;
              }

              float const inv_total = 255.0f / total;
              alpha[0][offset] = base * inv_total;
              alpha[1][offset] = low * inv_total;
              alpha[2][offset] = high * inv_total;
              alpha[3][offset] = cliff * inv_total;
            }
          }

          texture_set->apply_alpha_changes();
          texture_set->updateDoodadMapping();
          chunk->registerChunkUpdate(ChunkUpdateFlags::ALPHAMAP | ChunkUpdateFlags::GROUND_EFFECT);
          ++chunks_changed;
          progress.setValue(static_cast<int>(chunks_changed));
        }
      }

      world->mapIndex.setChanged(tile);
    }

    NOGGIT_ACTION_MGR->endAction();
    progress.setValue(total_chunks);

    QMessageBox::information(this, "Auto Texture complete",
      QString("Processed %1 ADT(s), %2 terrain chunks.\nUse Ctrl+Z to revert this entire Auto Texture operation.")
        .arg(static_cast<qulonglong>(_selected_adts.size())).arg(static_cast<qulonglong>(chunks_changed)));
  }
}
