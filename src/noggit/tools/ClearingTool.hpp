// This file is part of Noggit3, licensed under GNU General Public License (version 3).

#pragma once

#include <noggit/Tool.hpp>

struct ClearingOptions;

namespace Noggit
{
  namespace Ui::Tools::ClearingTool
  {
    class ClearingToolPanel;
  }

  class ClearingTool final : public Tool
  {
  public:
    explicit ClearingTool(MapView* mapView);
    ~ClearingTool() override;

    [[nodiscard]] char const* name() const override;
    [[nodiscard]] editing_mode editingMode() const override;
    [[nodiscard]] Ui::FontNoggit::Icons icon() const override;
    [[nodiscard]] ToolDrawParameters drawParameters() const override;

    void setupUi(Ui::Tools::ToolPanel* toolPanel) override;
    void onTick(float deltaTime, TickParameters const& params) override;
    void onMouseMove(MouseMoveParameters const& params) override;

  private:
    [[nodiscard]] ClearingOptions options() const;

    Ui::Tools::ClearingTool::ClearingToolPanel* _panel = nullptr;
  };
}
