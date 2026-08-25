// This file is part of Noggit3, licensed under GNU General Public License (version 3).

#pragma once

#include <noggit/Tool.hpp>
#include <string>

namespace Noggit
{
  namespace Ui::Tools::ChunkManipulator
  {
    class ChunkClipboard;
    class ChunkManipulatorPanel;
  }

  class ChunkTool final : public Tool
  {
  public:
    explicit ChunkTool(MapView* mapView);
    ~ChunkTool() override;

    [[nodiscard]] char const* name() const override;
    [[nodiscard]] editing_mode editingMode() const override;
    [[nodiscard]] Ui::FontNoggit::Icons icon() const override;
    [[nodiscard]] ToolDrawParameters drawParameters() const override;

    void setupUi(Ui::Tools::ToolPanel* toolPanel) override;
    void onSelected() override;
    void onDeselected() override;
    void onTick(float deltaTime, TickParameters const& params) override;
    void onMouseMove(MouseMoveParameters const& params) override;

  private:
    void copySelection();
    void pasteAtCursor();
    void updateTarget();
    void saveCurrentChunkAsset();
    void loadChunkAsset(std::string const& path);
    void deleteChunkAsset(std::string const& path);

    Ui::Tools::ChunkManipulator::ChunkClipboard* _clipboard = nullptr;
    Ui::Tools::ChunkManipulator::ChunkManipulatorPanel* _chunkManipulator = nullptr;
    std::string _loaded_saved_chunk_path;
  };
}
