// This file is part of Noggit3, licensed under GNU General Public License (version 3).

#include "ChunkManipulatorPanel.hpp"

#include "ChunkClipboard.hpp"
#include "SavedChunkAsset.hpp"

#include <noggit/MapHeaders.h>

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>
#include <array>

using namespace Noggit::Ui::Tools::ChunkManipulator;

namespace
{
  template<typename Flag>
  void add_flag(std::uint32_t& flags, QCheckBox const* checkbox, Flag flag)
  {
    if (checkbox->isChecked())
      flags |= static_cast<std::uint32_t>(flag);
  }

  QCheckBox* checked_box(QString const& text, QWidget* parent)
  {
    auto* checkbox = new QCheckBox(text, parent);
    checkbox->setChecked(true);
    return checkbox;
  }
}

ChunkManipulatorPanel::ChunkManipulatorPanel(MapView* map_view, ChunkClipboard* clipboard, QWidget* parent)
  : QWidget(parent)
{
  (void)map_view;
  auto* layout = new QVBoxLayout(this);

  auto* selection_group = new QGroupBox(tr("Selection"), this);
  auto* selection_layout = new QGridLayout(selection_group);
  _radius = new QDoubleSpinBox(selection_group);
  _radius->setRange(1.0, TILESIZE);
  _radius->setDecimals(1);
  _radius->setSingleStep(CHUNKSIZE * .25f);
  _radius->setValue(CHUNKSIZE * .45f);
  _radius->setSuffix(tr(" units"));
  _square_selection = new QCheckBox(tr("Square brush"), selection_group);
  _selection_status = new QLabel(tr("Selected: 0 chunks"), selection_group);
  _clipboard_status = new QLabel(tr("Clipboard: empty"), selection_group);
  selection_layout->addWidget(new QLabel(tr("Radius"), selection_group), 0, 0);
  selection_layout->addWidget(_radius, 0, 1);
  selection_layout->addWidget(_square_selection, 1, 0, 1, 2);
  selection_layout->addWidget(_selection_status, 2, 0, 1, 2);
  selection_layout->addWidget(_clipboard_status, 3, 0, 1, 2);
  layout->addWidget(selection_group);

  auto* components_group = new QGroupBox(tr("Copy components"), this);
  auto* components_layout = new QGridLayout(components_group);
  _terrain = checked_box(tr("Terrain + normals"), components_group);
  _textures = checked_box(tr("Textures + alpha"), components_group);
  _liquid = checked_box(tr("Liquids"), components_group);
  _vertex_colors = checked_box(tr("Vertex colors"), components_group);
  _shadows = checked_box(tr("Shadows"), components_group);
  _holes = checked_box(tr("Holes"), components_group);
  _flags = checked_box(tr("Chunk flags"), components_group);
  _area_id = checked_box(tr("Area ID"), components_group);
  _models = checked_box(tr("M2 models"), components_group);
  _wmos = checked_box(tr("WMOs"), components_group);
  _sound_emitters = checked_box(tr("Sound emitters"), components_group);

  std::array<QCheckBox*, 11> const components{
    _terrain, _textures, _liquid, _vertex_colors, _shadows, _holes,
    _flags, _area_id, _models, _wmos, _sound_emitters};
  for (std::size_t i = 0; i < components.size(); ++i)
    components_layout->addWidget(components[i], static_cast<int>(i / 2), static_cast<int>(i % 2));
  layout->addWidget(components_group);

  auto* paste_group = new QGroupBox(tr("Paste options"), this);
  auto* paste_layout = new QGridLayout(paste_group);
  _height_offset = new QDoubleSpinBox(paste_group);
  _height_offset->setRange(-10000.0, 10000.0);
  _height_offset->setDecimals(2);
  _height_offset->setSingleStep(1.0);
  _height_offset->setSuffix(tr(" units"));
  _replace_objects = checked_box(tr("Replace target objects"), paste_group);
  _fix_gaps = checked_box(tr("Fix terrain seams"), paste_group);
  paste_layout->addWidget(new QLabel(tr("Height offset"), paste_group), 0, 0);
  paste_layout->addWidget(_height_offset, 0, 1);
  paste_layout->addWidget(_replace_objects, 1, 0, 1, 2);
  paste_layout->addWidget(_fix_gaps, 2, 0, 1, 2);
  layout->addWidget(paste_group);

  auto* copy_paste_layout = new QHBoxLayout;
  auto* copy_button = new QPushButton(tr("Copy selected"), this);
  auto* paste_button = new QPushButton(tr("Paste at cursor"), this);
  copy_button->setToolTip(tr("Copy selection (Ctrl+C)"));
  paste_button->setToolTip(tr("Paste at cursor (Ctrl+V)"));
  copy_paste_layout->addWidget(copy_button);
  copy_paste_layout->addWidget(paste_button);
  layout->addLayout(copy_paste_layout);

  auto* transform_layout = new QGridLayout;
  auto* rotate_left_button = new QPushButton(tr("Rotate Left 90°"), this);
  auto* rotate_right_button = new QPushButton(tr("Rotate Right 90°"), this);
  _mirror_horizontal_button = new QPushButton(tr("Mirror X"), this);
  _mirror_vertical_button = new QPushButton(tr("Mirror Z"), this);
  auto* clear_button = new QPushButton(tr("Clear selection"), this);
  transform_layout->addWidget(rotate_left_button, 0, 0);
  transform_layout->addWidget(rotate_right_button, 0, 1);
  transform_layout->addWidget(_mirror_horizontal_button, 1, 0);
  transform_layout->addWidget(_mirror_vertical_button, 1, 1);
  transform_layout->addWidget(clear_button, 2, 0, 1, 2);
  layout->addLayout(transform_layout);

  auto* saved_group = new QGroupBox(tr("Saved Chunks"), this);
  auto* saved_layout = new QVBoxLayout(saved_group);
  _saved_chunk_filter = new QLineEdit(saved_group);
  _saved_chunk_filter->setPlaceholderText(tr("Filter saved chunks..."));
  saved_layout->addWidget(_saved_chunk_filter);
  _saved_chunks = new QListWidget(saved_group);
  _saved_chunks->setToolTip(tr("Persistent chunk assets stored in this Noggit project. Selecting one loads it into the placement clipboard."));
  saved_layout->addWidget(_saved_chunks);

  auto* saved_buttons = new QGridLayout;
  auto* save_saved_button = new QPushButton(tr("Save current copy"), saved_group);
  auto* delete_saved_button = new QPushButton(tr("Delete saved chunk"), saved_group);
  auto* refresh_saved_button = new QPushButton(tr("Refresh"), saved_group);
  saved_buttons->addWidget(save_saved_button, 0, 0);
  saved_buttons->addWidget(delete_saved_button, 0, 1);
  saved_buttons->addWidget(refresh_saved_button, 1, 0, 1, 2);
  saved_layout->addLayout(saved_buttons);
  layout->addWidget(saved_group);
  layout->addStretch();

  connect(copy_button, &QPushButton::clicked, this, &ChunkManipulatorPanel::copyRequested);
  connect(paste_button, &QPushButton::clicked, this, &ChunkManipulatorPanel::pasteRequested);
  connect(clear_button, &QPushButton::clicked, this, &ChunkManipulatorPanel::clearRequested);
  connect(rotate_right_button, &QPushButton::clicked, this, &ChunkManipulatorPanel::rotateRequested);
  connect(rotate_left_button, &QPushButton::clicked, this, &ChunkManipulatorPanel::rotateLeftRequested);
  connect(_mirror_horizontal_button, &QPushButton::clicked,
          this, &ChunkManipulatorPanel::mirrorHorizontalRequested);
  connect(_mirror_vertical_button, &QPushButton::clicked,
          this, &ChunkManipulatorPanel::mirrorVerticalRequested);
  connect(save_saved_button, &QPushButton::clicked, this, &ChunkManipulatorPanel::saveSavedChunkRequested);
  connect(delete_saved_button, &QPushButton::clicked, this, [this]
  {
    if (auto* item = _saved_chunks->currentItem())
      emit deleteSavedChunkRequested(item->data(Qt::UserRole).toString());
  });
  connect(refresh_saved_button, &QPushButton::clicked, this, &ChunkManipulatorPanel::refreshSavedChunks);
  connect(_saved_chunk_filter, &QLineEdit::textChanged, this, &ChunkManipulatorPanel::applySavedChunkFilter);
  connect(_saved_chunks, &QListWidget::currentItemChanged, this,
          [this](QListWidgetItem* current, QListWidgetItem*)
          {
            if (current)
              emit loadSavedChunkRequested(current->data(Qt::UserRole).toString());
          });

  connect(clipboard, &ChunkClipboard::selectionChanged, this,
          [this](std::set<SelectedChunkIndex> const& selected)
          {
            _selection_status->setText(tr("Selected: %1 chunks").arg(
              static_cast<qulonglong>(selected.size())));
          });
  connect(clipboard, &ChunkClipboard::clipboardChanged, this,
          [this](std::size_t chunks)
          {
            _clipboard_status->setText(chunks == 0
              ? tr("Clipboard: empty")
              : tr("Clipboard: %1 chunks").arg(static_cast<qulonglong>(chunks)));
          });
  connect(clipboard, &ChunkClipboard::savedAssetStateChanged, this,
          &ChunkManipulatorPanel::setSavedAssetMode);

  refreshSavedChunks();
  setSavedAssetMode(clipboard->isSavedAsset());
}

float ChunkManipulatorPanel::radius() const
{
  return static_cast<float>(_radius->value());
}

bool ChunkManipulatorPanel::squareSelection() const
{
  return _square_selection->isChecked();
}

float ChunkManipulatorPanel::heightOffset() const
{
  return static_cast<float>(_height_offset->value());
}

ChunkCopyFlags ChunkManipulatorPanel::copyFlags() const
{
  std::uint32_t flags = 0;
  add_flag(flags, _terrain, ChunkCopyFlags::TERRAIN);
  add_flag(flags, _liquid, ChunkCopyFlags::LIQUID);
  add_flag(flags, _wmos, ChunkCopyFlags::WMOS);
  add_flag(flags, _models, ChunkCopyFlags::MODELS);
  add_flag(flags, _shadows, ChunkCopyFlags::SHADOWS);
  add_flag(flags, _textures, ChunkCopyFlags::TEXTURES);
  add_flag(flags, _vertex_colors, ChunkCopyFlags::VERTEX_COLORS);
  add_flag(flags, _holes, ChunkCopyFlags::HOLES);
  add_flag(flags, _flags, ChunkCopyFlags::FLAGS);
  add_flag(flags, _area_id, ChunkCopyFlags::AREA_ID);
  add_flag(flags, _sound_emitters, ChunkCopyFlags::SOUND_EMITTERS);
  return static_cast<ChunkCopyFlags>(flags);
}

ChunkPasteFlags ChunkManipulatorPanel::pasteFlags() const
{
  std::uint32_t flags = 0;
  add_flag(flags, _replace_objects, ChunkPasteFlags::REPLACE_OBJECTS);
  add_flag(flags, _fix_gaps, ChunkPasteFlags::FIX_GAPS);
  return static_cast<ChunkPasteFlags>(flags);
}

void ChunkManipulatorPanel::changeRadius(float delta)
{
  _radius->setValue(std::clamp(_radius->value() + delta,
                               _radius->minimum(), _radius->maximum()));
}

void ChunkManipulatorPanel::refreshSavedChunks()
{
  if (!_saved_chunks)
    return;

  QString const previous_path = _saved_chunks->currentItem()
    ? _saved_chunks->currentItem()->data(Qt::UserRole).toString()
    : QString{};

  QSignalBlocker const blocker(_saved_chunks);
  _saved_chunks->clear();
  int restore_row = -1;
  auto const records = SavedChunkAsset::list();
  for (auto const& record : records)
  {
    auto* item = new QListWidgetItem(QString::fromStdString(record.display_name), _saved_chunks);
    QString const path = QString::fromStdString(record.path.string());
    item->setData(Qt::UserRole, path);
    item->setToolTip(path);
    if (path == previous_path)
      restore_row = _saved_chunks->row(item);
  }

  if (restore_row >= 0)
    _saved_chunks->setCurrentRow(restore_row);

  applySavedChunkFilter(_saved_chunk_filter ? _saved_chunk_filter->text() : QString{});
}

void ChunkManipulatorPanel::applySavedChunkFilter(QString const& text)
{
  if (!_saved_chunks)
    return;

  QString const needle = text.trimmed();
  for (int row = 0; row < _saved_chunks->count(); ++row)
  {
    auto* item = _saved_chunks->item(row);
    item->setHidden(!needle.isEmpty() && !item->text().contains(needle, Qt::CaseInsensitive));
  }
}

void ChunkManipulatorPanel::setSavedAssetMode(bool enabled)
{
  // Persistent saved assets deliberately support quarter-turn rotation only.
  // The live clipboard still retains its already-proven mirror controls.
  if (_mirror_horizontal_button)
    _mirror_horizontal_button->setEnabled(!enabled);
  if (_mirror_vertical_button)
    _mirror_vertical_button->setEnabled(!enabled);
}
