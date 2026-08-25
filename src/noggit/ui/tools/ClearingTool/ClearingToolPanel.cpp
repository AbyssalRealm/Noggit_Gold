// This file is part of Noggit3, licensed under GNU General Public License (version 3).

#include "ClearingToolPanel.hpp"

#include <noggit/MapHeaders.h>

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

#include <algorithm>
#include <array>

using namespace Noggit::Ui::Tools::ClearingTool;

ClearingToolPanel::ClearingToolPanel(QWidget* parent)
  : QWidget(parent)
{
  auto* layout = new QVBoxLayout(this);

  auto* help = new QLabel(
    tr("Shift + LMB: clear selected data\nAlt + LMB drag: change radius\nCtrl+Z / Ctrl+Y: undo / redo"), this);
  help->setWordWrap(true);
  layout->addWidget(help);

  auto* clear_group = new QGroupBox(tr("Clear"), this);
  auto* clear_layout = new QGridLayout(clear_group);

  _height = new QCheckBox(tr("Height"), clear_group);
  _textures = new QCheckBox(tr("Textures"), clear_group);
  _duplicate_textures = new QCheckBox(tr("Texture duplicates"), clear_group);
  _textures_below_threshold = new QCheckBox(tr("Textures below threshold"), clear_group);
  _texture_flags = new QCheckBox(tr("Texture flags"), clear_group);
  _liquids = new QCheckBox(tr("Liquids"), clear_group);
  _m2s = new QCheckBox(tr("M2s"), clear_group);
  _wmos = new QCheckBox(tr("WMOs"), clear_group);
  _shadows = new QCheckBox(tr("Shadows"), clear_group);
  _vertex_colors = new QCheckBox(tr("Vertex colors"), clear_group);
  _impassible_flag = new QCheckBox(tr("Impassible flag"), clear_group);
  _holes = new QCheckBox(tr("Holes"), clear_group);

  std::array<QCheckBox*, 12> const options{
    _height, _textures, _duplicate_textures, _textures_below_threshold,
    _texture_flags, _liquids, _m2s, _wmos, _shadows, _vertex_colors,
    _impassible_flag, _holes};

  for (std::size_t i = 0; i < options.size(); ++i)
  {
    clear_layout->addWidget(options[i], static_cast<int>(i / 2), static_cast<int>(i % 2));
  }
  layout->addWidget(clear_group);

  auto* mode_group = new QGroupBox(tr("Mode"), this);
  auto* mode_layout = new QHBoxLayout(mode_group);
  _chunk_mode = new QRadioButton(tr("Chunk"), mode_group);
  _adt_mode = new QRadioButton(tr("ADT"), mode_group);
  _chunk_mode->setChecked(true);
  mode_layout->addWidget(_chunk_mode);
  mode_layout->addWidget(_adt_mode);
  layout->addWidget(mode_group);

  auto* parameters_group = new QGroupBox(tr("Parameters"), this);
  auto* parameters_layout = new QGridLayout(parameters_group);

  _radius = new QDoubleSpinBox(parameters_group);
  _radius->setRange(0.0, 1000.0);
  _radius->setDecimals(1);
  _radius->setSingleStep(CHUNKSIZE * 0.25f);
  _radius->setValue(15.0);
  _radius->setSuffix(tr(" units"));

  _alpha_threshold = new QDoubleSpinBox(parameters_group);
  _alpha_threshold->setRange(0.0, 255.0);
  _alpha_threshold->setDecimals(0);
  _alpha_threshold->setSingleStep(1.0);
  _alpha_threshold->setValue(1.0);

  parameters_layout->addWidget(new QLabel(tr("Radius"), parameters_group), 0, 0);
  parameters_layout->addWidget(_radius, 0, 1);
  parameters_layout->addWidget(new QLabel(tr("Texture alpha threshold"), parameters_group), 1, 0);
  parameters_layout->addWidget(_alpha_threshold, 1, 1);
  layout->addWidget(parameters_group);

  auto* safety = new QLabel(
    tr("No clear option is enabled by default. Each Shift+LMB stroke is recorded in Red's action history."), this);
  safety->setWordWrap(true);
  layout->addWidget(safety);
  layout->addStretch();
}

float ClearingToolPanel::radius() const { return static_cast<float>(_radius->value()); }
float ClearingToolPanel::alphaThreshold() const { return static_cast<float>(_alpha_threshold->value()); }
bool ClearingToolPanel::adtMode() const { return _adt_mode->isChecked(); }

bool ClearingToolPanel::hasOperations() const
{
  return clearHeight() || clearTextures() || clearDuplicateTextures()
      || clearTexturesBelowThreshold() || clearTextureFlags() || clearLiquids()
      || clearM2s() || clearWMOs() || clearShadows() || clearVertexColors()
      || clearImpassibleFlag() || clearHoles();
}

bool ClearingToolPanel::clearHeight() const { return _height->isChecked(); }
bool ClearingToolPanel::clearTextures() const { return _textures->isChecked(); }
bool ClearingToolPanel::clearDuplicateTextures() const { return _duplicate_textures->isChecked(); }
bool ClearingToolPanel::clearTexturesBelowThreshold() const { return _textures_below_threshold->isChecked(); }
bool ClearingToolPanel::clearTextureFlags() const { return _texture_flags->isChecked(); }
bool ClearingToolPanel::clearLiquids() const { return _liquids->isChecked(); }
bool ClearingToolPanel::clearM2s() const { return _m2s->isChecked(); }
bool ClearingToolPanel::clearWMOs() const { return _wmos->isChecked(); }
bool ClearingToolPanel::clearShadows() const { return _shadows->isChecked(); }
bool ClearingToolPanel::clearVertexColors() const { return _vertex_colors->isChecked(); }
bool ClearingToolPanel::clearImpassibleFlag() const { return _impassible_flag->isChecked(); }
bool ClearingToolPanel::clearHoles() const { return _holes->isChecked(); }

void ClearingToolPanel::changeRadius(float delta)
{
  _radius->setValue(std::clamp(_radius->value() + delta, _radius->minimum(), _radius->maximum()));
}
