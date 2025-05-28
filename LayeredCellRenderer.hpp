#ifndef LAYERED_CELL_RENDERER_HPP
#define LAYERED_CELL_RENDERER_HPP
#include "CellRendererFirstPassVisitor.hpp"
#include "CellRendererSecondPassVisitor.hpp"
#include "CellRendererThirdPassVisitor.hpp"
#include "Drawer.hpp"

template<typename TGameRendererConfig>
class LayeredCellRenderer {
public:
    using IGameInfo = Feis::IGameInfo;
    using CellPosition = Feis::CellPosition;

    void RenderPassOne(const IGameInfo &info, Drawer<TGameRendererConfig> &renderer, CellPosition position) const {
        auto &layeredCell = info.GetLayeredCell(position);

        const auto foreground = layeredCell.GetForeground();
        const auto background = layeredCell.GetBackground();

        if (foreground) {
            CellRendererFirstPassVisitor<TGameRendererConfig> cellRenderer(&info, &renderer, position,
                                                                           background.get());
            foreground->Accept(&cellRenderer);
            return;
        }

        if (background) {
            CellRendererFirstPassVisitor<TGameRendererConfig> cellRenderer(&info, &renderer, position,
                                                                           background.get());
            background->Accept(&cellRenderer);
            return;
        }

        renderer.DrawBorder(position);
    }

    void RenderPassTwo(const IGameInfo &info, Drawer<TGameRendererConfig> &drawer, CellPosition cellPosition) const {
        auto &layeredCell = info.GetLayeredCell(cellPosition);

        if (const auto foreground = layeredCell.GetForeground()) {
            CellRendererSecondPassVisitor<TGameRendererConfig> cellRenderer(&info, &drawer, cellPosition);
            foreground->Accept(&cellRenderer);
        }
    }

    void RenderPassThree(const IGameInfo &info, Drawer<TGameRendererConfig> &drawer, CellPosition cellPosition) const {
        auto &layeredCell = info.GetLayeredCell(cellPosition);

        if (const auto foreground = layeredCell.GetForeground()) {
            CellRendererThirdPassVisitor<TGameRendererConfig> cellRenderer(&info, &drawer, cellPosition);
            foreground->Accept(&cellRenderer);
        }
    }
};
#endif
