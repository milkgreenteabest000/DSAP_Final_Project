#ifndef CELL_RENDERER_FIRST_PASS_VISITOR_HPP
#define CELL_RENDERER_FIRST_PASS_VISITOR_HPP
#include "PDOGS.hpp"
#include "Drawer.hpp"

template <typename TGameRendererConfig>
class CellRendererFirstPassVisitor final : public Feis::CellVisitor
{
public:
    using CellPosition = Feis::CellPosition;
    using Direction = Feis::Direction;
    using IBackgroundCell = Feis::IBackgroundCell;

    CellRendererFirstPassVisitor(
        const Feis::IGameInfo *info,
        Drawer<TGameRendererConfig> *drawer,
                                 const CellPosition cellPosition,
        IBackgroundCell *backgroundCell)
        : info{info}, drawer_(drawer), cellPosition_(cellPosition), backgroundCell_(backgroundCell)
    {
    }

    void Visit(const Feis::Cell *cell) const
    {
        cell->Accept(this);
    }

    void Visit(const Feis::NumberCell *cell) const override
    {
        drawer_->DrawBorder(cellPosition_);

        const int number = cell->GetNumber();
        std::mt19937 gen(number);
        std::uniform_int_distribution dis(0, 50);
        const int r = dis(gen) + 128;
        const int g = dis(gen) + 128;
        const int b = dis(gen) + 128;
        sf::Color color(r, g, b);

        drawer_->DrawText(
            std::to_string(number),
            TGameRendererConfig::kCellSize * 0.75f,
            color,
            cellPosition_);
    }

    void Visit(const Feis::CollectionCenterCell *cell) const override
    {
    }

    void Visit(const Feis::MiningMachineCell *cell) const override
    {
        drawer_->DrawRectangle(cellPosition_, sf::Color(128, 0, 0));

        const auto numberCell =
            dynamic_cast<const Feis::NumberCell *>(backgroundCell_);

        if (numberCell)
        {
            drawer_->DrawText(
                std::to_string(numberCell->GetNumber()),
                TGameRendererConfig::kCellSize * 0.8,
                sf::Color::White,
                cellPosition_,
                static_cast<Direction>((static_cast<int>(cell->GetDirection()) + 1) % 4));
        }
    }

    void Visit(const Feis::ConveyorCell *cell) const override
    {
        drawer_->DrawBorder(cellPosition_);
        drawer_->DrawRectangle(cellPosition_, sf::Color(128, 128, 128));
        drawer_->DrawArrow(cellPosition_, cell->GetDirection());
    }

    void Visit(const Feis::CombinerCell *cell) const override
    {
        drawer_->DrawBorder(cellPosition_);
    }

    void Visit(const Feis::WallCell *cell) const override
    {
        drawer_->DrawBorder(cellPosition_);
    }
private:
    const Feis::IGameInfo *info;
    Drawer<TGameRendererConfig> *drawer_;
    CellPosition cellPosition_;
    IBackgroundCell *backgroundCell_;
};
#endif