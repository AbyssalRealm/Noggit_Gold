// This file is part of Noggit3, licensed under GNU General Public License (version 3).

#pragma once

#include <noggit/TileIndex.hpp>

#include <QWidget>

#include <array>
#include <optional>
#include <string>
#include <vector>

class QLabel;
class QListWidget;
class QDoubleSpinBox;
class QSpinBox;
class QPushButton;
class MapView;

namespace Noggit::Ui
{
  class current_texture;
}

namespace Noggit::Ui::Tools
{
  class AutoTextureWidget : public QWidget
  {
  public:
    AutoTextureWidget(MapView* map_view, Noggit::Ui::current_texture* current_texture, QWidget* parent = nullptr);

  private:
    struct TextureSlot
    {
      Noggit::Ui::current_texture* preview = nullptr;
      QPushButton* filename_button = nullptr;
      std::string filename;
    };

    static constexpr std::size_t MAX_SELECTED_ADTS = 8;

    MapView* _map_view;
    Noggit::Ui::current_texture* _current_texture;

    std::vector<TileIndex> _selected_adts;
    QListWidget* _selected_adts_list = nullptr;
    QLabel* _selection_status = nullptr;
    QLabel* _terrain_height_range = nullptr;
    QLabel* _height_band_status = nullptr;
    QSpinBox* _adt_x = nullptr;
    QSpinBox* _adt_z = nullptr;

    std::array<TextureSlot, 4> _texture_slots;
    std::optional<std::size_t> _pending_texture_slot;
    bool _texture_browser_close_guard_connected = false;

    QDoubleSpinBox* _low_full = nullptr;
    QDoubleSpinBox* _low_fade_end = nullptr;
    QDoubleSpinBox* _high_fade_start = nullptr;
    QDoubleSpinBox* _high_full = nullptr;
    QDoubleSpinBox* _cliff_start = nullptr;
    QDoubleSpinBox* _cliff_full = nullptr;
    QDoubleSpinBox* _noise_amount = nullptr;
    QDoubleSpinBox* _noise_scale = nullptr;

    QPushButton* _apply_button = nullptr;

    void add_adt(TileIndex const& index);
    void remove_selected_adt();
    void clear_adts();
    void refresh_adt_list();
    void choose_texture_for_slot(std::size_t slot);
    void set_texture_slot(std::size_t slot, std::string const& filename);
    bool scan_selected_height_range(float& min_height, float& max_height) const;
    void update_height_feedback();
    void fit_height_ranges();
    void apply_auto_texture();
  };
}
