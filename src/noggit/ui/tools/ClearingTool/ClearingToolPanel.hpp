// This file is part of Noggit3, licensed under GNU General Public License (version 3).

#pragma once

#include <QWidget>

class QCheckBox;
class QDoubleSpinBox;
class QRadioButton;

namespace Noggit::Ui::Tools::ClearingTool
{
  class ClearingToolPanel : public QWidget
  {
  public:
    explicit ClearingToolPanel(QWidget* parent = nullptr);

    [[nodiscard]] float radius() const;
    [[nodiscard]] float alphaThreshold() const;
    [[nodiscard]] bool adtMode() const;
    [[nodiscard]] bool hasOperations() const;

    [[nodiscard]] bool clearHeight() const;
    [[nodiscard]] bool clearTextures() const;
    [[nodiscard]] bool clearDuplicateTextures() const;
    [[nodiscard]] bool clearTexturesBelowThreshold() const;
    [[nodiscard]] bool clearTextureFlags() const;
    [[nodiscard]] bool clearLiquids() const;
    [[nodiscard]] bool clearM2s() const;
    [[nodiscard]] bool clearWMOs() const;
    [[nodiscard]] bool clearShadows() const;
    [[nodiscard]] bool clearVertexColors() const;
    [[nodiscard]] bool clearImpassibleFlag() const;
    [[nodiscard]] bool clearHoles() const;

    void changeRadius(float delta);

  private:
    QDoubleSpinBox* _radius = nullptr;
    QDoubleSpinBox* _alpha_threshold = nullptr;
    QRadioButton* _chunk_mode = nullptr;
    QRadioButton* _adt_mode = nullptr;

    QCheckBox* _height = nullptr;
    QCheckBox* _textures = nullptr;
    QCheckBox* _duplicate_textures = nullptr;
    QCheckBox* _textures_below_threshold = nullptr;
    QCheckBox* _texture_flags = nullptr;
    QCheckBox* _liquids = nullptr;
    QCheckBox* _m2s = nullptr;
    QCheckBox* _wmos = nullptr;
    QCheckBox* _shadows = nullptr;
    QCheckBox* _vertex_colors = nullptr;
    QCheckBox* _impassible_flag = nullptr;
    QCheckBox* _holes = nullptr;
  };
}
