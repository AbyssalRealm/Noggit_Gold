// This file is part of Noggit3, licensed under GNU General Public License (version 3).

#pragma once

#include <QWidget>
#include <QString>

#include <cstdint>

class MapView;
class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QListWidget;
class QLineEdit;
class QPushButton;

namespace Noggit::Ui::Tools::ChunkManipulator
{
  class ChunkClipboard;
  enum class ChunkCopyFlags : std::uint32_t;
  enum class ChunkPasteFlags : std::uint32_t;

  class ChunkManipulatorPanel : public QWidget
  {
    Q_OBJECT

  public:
    ChunkManipulatorPanel(MapView* map_view, ChunkClipboard* clipboard, QWidget* parent = nullptr);

    [[nodiscard]] float radius() const;
    [[nodiscard]] bool squareSelection() const;
    [[nodiscard]] float heightOffset() const;
    [[nodiscard]] ChunkCopyFlags copyFlags() const;
    [[nodiscard]] ChunkPasteFlags pasteFlags() const;
    void changeRadius(float delta);
    void refreshSavedChunks();
    void setSavedAssetMode(bool enabled);
    void applySavedChunkFilter(QString const& text);

  signals:
    void copyRequested();
    void pasteRequested();
    void clearRequested();
    void rotateRequested();
    void rotateLeftRequested();
    void mirrorHorizontalRequested();
    void mirrorVerticalRequested();
    void saveSavedChunkRequested();
    void loadSavedChunkRequested(QString const& path);
    void deleteSavedChunkRequested(QString const& path);

  private:
    QDoubleSpinBox* _radius;
    QCheckBox* _square_selection;
    QDoubleSpinBox* _height_offset;
    QLabel* _selection_status;
    QLabel* _clipboard_status;
    QListWidget* _saved_chunks;
    QLineEdit* _saved_chunk_filter;
    QPushButton* _mirror_horizontal_button;
    QPushButton* _mirror_vertical_button;

    QCheckBox* _terrain;
    QCheckBox* _liquid;
    QCheckBox* _wmos;
    QCheckBox* _models;
    QCheckBox* _shadows;
    QCheckBox* _textures;
    QCheckBox* _vertex_colors;
    QCheckBox* _holes;
    QCheckBox* _flags;
    QCheckBox* _area_id;
    QCheckBox* _sound_emitters;
    QCheckBox* _replace_objects;
    QCheckBox* _fix_gaps;
  };
}
