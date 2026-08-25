// This file is part of Noggit3, licensed under GNU General Public License (version 3).

#include "ChunkTool.hpp"

#include <noggit/ActionManager.hpp>
#include <noggit/Input.hpp>
#include <noggit/MapView.h>
#include <noggit/World.h>
#include <noggit/rendering/WorldRender.hpp>
#include <noggit/ui/tools/ChunkManipulator/ChunkClipboard.hpp>
#include <noggit/ui/tools/ChunkManipulator/ChunkManipulatorPanel.hpp>
#include <noggit/ui/tools/ChunkManipulator/SavedChunkAsset.hpp>
#include <noggit/ui/tools/ToolPanel/ToolPanel.hpp>

#include <QMessageBox>

#include <filesystem>

#include <QInputDialog>
#include <QLineEdit>

namespace Noggit
{
  ChunkTool::ChunkTool(MapView* mapView)
    : Tool{mapView}
    , _clipboard(new Ui::Tools::ChunkManipulator::ChunkClipboard(mapView->getWorld(), mapView))
  {
    addHotkey("copySelection"_hash, Hotkey{
      .onPress = [this] { copySelection(); },
      .condition = [this]
      {
        return this->mapView()->get_editing_mode() == editing_mode::chunk
            && _chunkManipulator && _clipboard->selectedCount() > 0 && !NOGGIT_CUR_ACTION;
      },
    });

    addHotkey("paste"_hash, Hotkey{
      .onPress = [this] { pasteAtCursor(); },
      .condition = [this]
      {
        return this->mapView()->get_editing_mode() == editing_mode::chunk
            && _chunkManipulator && _clipboard->hasCopiedData() && !NOGGIT_CUR_ACTION;
      },
    });
  }

  ChunkTool::~ChunkTool()
  {
    delete _chunkManipulator;
  }

  char const* ChunkTool::name() const
  {
    return "Chunk Manipulator";
  }

  editing_mode ChunkTool::editingMode() const
  {
    return editing_mode::chunk;
  }

  Ui::FontNoggit::Icons ChunkTool::icon() const
  {
    return Ui::FontNoggit::INFO;
  }

  ToolDrawParameters ChunkTool::drawParameters() const
  {
    return {
      .radius = _chunkManipulator ? _chunkManipulator->radius() : 0.f,
      .cursor_type = CursorType::CIRCLE,
      .cursor_color = {0.25f, 0.85f, 1.f, 1.f},
    };
  }

  void ChunkTool::setupUi(Ui::Tools::ToolPanel* toolPanel)
  {
    _chunkManipulator = new Ui::Tools::ChunkManipulator::ChunkManipulatorPanel(
      mapView(), _clipboard, mapView());
    toolPanel->registerTool(this, _chunkManipulator);

    QObject::connect(_chunkManipulator,
                     &Ui::Tools::ChunkManipulator::ChunkManipulatorPanel::copyRequested,
                     [this] { copySelection(); });
    QObject::connect(_chunkManipulator,
                     &Ui::Tools::ChunkManipulator::ChunkManipulatorPanel::pasteRequested,
                     [this] { pasteAtCursor(); });
    QObject::connect(_chunkManipulator,
                     &Ui::Tools::ChunkManipulator::ChunkManipulatorPanel::clearRequested,
                     [this]
                     {
                       _loaded_saved_chunk_path.clear();
                       _clipboard->clearSelection();
                     });
    QObject::connect(_chunkManipulator,
                     &Ui::Tools::ChunkManipulator::ChunkManipulatorPanel::rotateRequested,
                     [this]
                     {
                       _clipboard->rotate90Degrees();
                       updateTarget();
                     });
    QObject::connect(_chunkManipulator,
                     &Ui::Tools::ChunkManipulator::ChunkManipulatorPanel::rotateLeftRequested,
                     [this]
                     {
                       _clipboard->rotateLeft90Degrees();
                       updateTarget();
                     });
    QObject::connect(_chunkManipulator,
                     &Ui::Tools::ChunkManipulator::ChunkManipulatorPanel::mirrorHorizontalRequested,
                     [this]
                     {
                       if (_clipboard->isSavedAsset())
                         return;
                       _clipboard->mirror(true);
                       updateTarget();
                     });
    QObject::connect(_chunkManipulator,
                     &Ui::Tools::ChunkManipulator::ChunkManipulatorPanel::mirrorVerticalRequested,
                     [this]
                     {
                       if (_clipboard->isSavedAsset())
                         return;
                       _clipboard->mirror(false);
                       updateTarget();
                     });
    QObject::connect(_chunkManipulator,
                     &Ui::Tools::ChunkManipulator::ChunkManipulatorPanel::saveSavedChunkRequested,
                     [this] { saveCurrentChunkAsset(); });
    QObject::connect(_chunkManipulator,
                     &Ui::Tools::ChunkManipulator::ChunkManipulatorPanel::loadSavedChunkRequested,
                     [this](QString const& path) { loadChunkAsset(path.toStdString()); });
    QObject::connect(_chunkManipulator,
                     &Ui::Tools::ChunkManipulator::ChunkManipulatorPanel::deleteSavedChunkRequested,
                     [this](QString const& path) { deleteChunkAsset(path.toStdString()); });
  }

  void ChunkTool::onSelected()
  {
    mapView()->getWorld()->renderer()->getTerrainParamsUniformBlock()->draw_selection_overlay = true;
    updateTarget();
  }

  void ChunkTool::onDeselected()
  {
    _clipboard->clearTarget();
    mapView()->getWorld()->renderer()->getTerrainParamsUniformBlock()->draw_selection_overlay = false;
  }

  void ChunkTool::onTick(float, TickParameters const& params)
  {
    if (!_chunkManipulator || params.underMap)
      return;

    if (params.left_mouse && params.mod_shift_down != params.mod_ctrl_down)
    {
      _clipboard->selectRange(mapView()->cursorPosition(), _chunkManipulator->radius(),
                              _chunkManipulator->squareSelection(),
                              params.mod_shift_down
                                ? Ui::Tools::ChunkManipulator::ChunkSelectionMode::SELECT
                                : Ui::Tools::ChunkManipulator::ChunkSelectionMode::DESELECT);
      return;
    }

    if (_clipboard->hasCopiedData())
      updateTarget();
  }

  void ChunkTool::onMouseMove(MouseMoveParameters const& params)
  {
    if (_chunkManipulator && params.left_mouse && params.mod_alt_down
        && !params.mod_shift_down && !params.mod_ctrl_down)
    {
      _chunkManipulator->changeRadius(static_cast<float>(params.relative_movement.dx() / XSENS));
    }
  }

  void ChunkTool::copySelection()
  {
    if (!_chunkManipulator || _clipboard->selectedCount() == 0)
      return;

    _loaded_saved_chunk_path.clear();
    _clipboard->copySelected(mapView()->cursorPosition(), _chunkManipulator->copyFlags());
    updateTarget();
  }

  void ChunkTool::pasteAtCursor()
  {
    if (!_chunkManipulator || !_clipboard->hasCopiedData()
        || !mapView()->getWorld()->getChunkAt(mapView()->cursorPosition()))
      return;

    NOGGIT_ACTION_MGR->beginAction(mapView(), ActionFlags::eNO_FLAG);
    _clipboard->pasteSelection(mapView()->cursorPosition(), _chunkManipulator->pasteFlags(),
                               _chunkManipulator->heightOffset());
    NOGGIT_ACTION_MGR->endAction();
  }

  void ChunkTool::updateTarget()
  {
    if (_clipboard->hasCopiedData())
      _clipboard->updateTarget(mapView()->cursorPosition());
    else
      _clipboard->clearTarget();
  }

  void ChunkTool::saveCurrentChunkAsset()
  {
    if (!_chunkManipulator || !_clipboard->hasCopiedData())
    {
      QMessageBox::information(mapView(), QStringLiteral("Saved Chunks"),
                               QStringLiteral("Copy one or more chunks before saving a persistent chunk asset."));
      return;
    }

    std::string const suggestion = Ui::Tools::ChunkManipulator::SavedChunkAsset::suggestedName(*_clipboard);
    bool accepted = false;
    QString const requested = QInputDialog::getText(mapView(), QStringLiteral("Save Chunk Asset"),
                                                     QStringLiteral("Save name"), QLineEdit::Normal,
                                                     QString::fromStdString(suggestion), &accepted);
    if (!accepted || requested.trimmed().isEmpty())
      return;

    std::filesystem::path saved_path;
    std::string error;
    if (!Ui::Tools::ChunkManipulator::SavedChunkAsset::save(requested.toStdString(), *_clipboard, saved_path, error))
    {
      QMessageBox::critical(mapView(), QStringLiteral("Save Chunk Asset"), QString::fromStdString(error));
      return;
    }

    _chunkManipulator->refreshSavedChunks();
    QMessageBox::information(mapView(), QStringLiteral("Save Chunk Asset"),
                             QStringLiteral("Saved %1").arg(QString::fromStdString(saved_path.filename().string())));
  }

  void ChunkTool::loadChunkAsset(std::string const& path)
  {
    if (path.empty())
      return;

    std::string display_name;
    std::string error;
    if (!Ui::Tools::ChunkManipulator::SavedChunkAsset::load(path, *_clipboard, display_name, error))
    {
      QMessageBox::critical(mapView(), QStringLiteral("Load Saved Chunk"), QString::fromStdString(error));
      return;
    }

    _loaded_saved_chunk_path = path;
    updateTarget();
  }

  void ChunkTool::deleteChunkAsset(std::string const& path)
  {
    if (path.empty())
      return;

    QString const name = QString::fromStdString(std::filesystem::path(path).stem().string());
    auto const answer = QMessageBox::question(mapView(), QStringLiteral("Delete Saved Chunk"),
      QStringLiteral("Delete saved chunk '%1'?\n\nThis removes only the reusable library file. Terrain already pasted into maps is not changed.").arg(name),
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
      return;

    std::string error;
    if (!Ui::Tools::ChunkManipulator::SavedChunkAsset::remove(path, error))
    {
      QMessageBox::critical(mapView(), QStringLiteral("Delete Saved Chunk"), QString::fromStdString(error));
      return;
    }

    if (_loaded_saved_chunk_path == path)
    {
      _loaded_saved_chunk_path.clear();
      _clipboard->clearSelection();
    }
    _chunkManipulator->refreshSavedChunks();
  }
}
