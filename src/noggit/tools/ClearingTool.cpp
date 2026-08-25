// This file is part of Noggit3, licensed under GNU General Public License (version 3).

#include "ClearingTool.hpp"

#include <noggit/ActionManager.hpp>
#include <noggit/Input.hpp>
#include <noggit/MapView.h>
#include <noggit/World.h>
#include <noggit/ui/tools/ClearingTool/ClearingToolPanel.hpp>
#include <noggit/ui/tools/ToolPanel/ToolPanel.hpp>

namespace Noggit
{
  ClearingTool::ClearingTool(MapView* mapView)
    : Tool{mapView}
  {
  }

  ClearingTool::~ClearingTool()
  {
    delete _panel;
  }

  char const* ClearingTool::name() const
  {
    return "Eraser / Clear Tool";
  }

  editing_mode ClearingTool::editingMode() const
  {
    return editing_mode::clearing;
  }

  Ui::FontNoggit::Icons ClearingTool::icon() const
  {
    // Red's bundled icon font has no dedicated eraser glyph.
    return Ui::FontNoggit::TOOL_HOLE_CUTTER;
  }

  ToolDrawParameters ClearingTool::drawParameters() const
  {
    return {
      .radius = _panel ? _panel->radius() : 0.0f,
      .cursor_type = CursorType::CIRCLE,
    };
  }

  void ClearingTool::setupUi(Ui::Tools::ToolPanel* toolPanel)
  {
    _panel = new Ui::Tools::ClearingTool::ClearingToolPanel(mapView());
    toolPanel->registerTool(this, _panel);
  }

  ClearingOptions ClearingTool::options() const
  {
    ClearingOptions result;
    if (!_panel)
      return result;

    result.height = _panel->clearHeight();
    result.textures = _panel->clearTextures();
    result.duplicate_textures = _panel->clearDuplicateTextures();
    result.textures_below_threshold = _panel->clearTexturesBelowThreshold();
    result.alpha_threshold = _panel->alphaThreshold();
    result.texture_flags = _panel->clearTextureFlags();
    result.liquids = _panel->clearLiquids();
    result.m2s = _panel->clearM2s();
    result.wmos = _panel->clearWMOs();
    result.shadows = _panel->clearShadows();
    result.vertex_colors = _panel->clearVertexColors();
    result.impassible_flag = _panel->clearImpassibleFlag();
    result.holes = _panel->clearHoles();
    return result;
  }

  void ClearingTool::onTick(float, TickParameters const& params)
  {
    if (!_panel || params.underMap || !params.left_mouse || !params.mod_shift_down
        || params.mod_ctrl_down || params.mod_alt_down || !_panel->hasOperations())
    {
      return;
    }

    auto* mv = mapView();
    auto const clear_options = options();

    NOGGIT_ACTION_MGR->beginAction(
      mv,
      ActionFlags::eNO_FLAG,
      ActionModalityControllers::eSHIFT | ActionModalityControllers::eLMB);

    if (_panel->adtMode())
      mv->getWorld()->clearOnTiles(mv->cursorPosition(), _panel->radius(), clear_options);
    else
      mv->getWorld()->clearOnChunks(mv->cursorPosition(), _panel->radius(), clear_options);
  }

  void ClearingTool::onMouseMove(MouseMoveParameters const& params)
  {
    if (_panel && params.left_mouse && params.mod_alt_down
        && !params.mod_shift_down && !params.mod_ctrl_down)
    {
      _panel->changeRadius(static_cast<float>(params.relative_movement.dx() / XSENS));
    }
  }
}
