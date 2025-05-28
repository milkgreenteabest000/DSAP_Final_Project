#ifndef CELL_RENDERER_THIRD_PASS_VISITOR_HPP
#define CELL_RENDERER_THIRD_PASS_VISITOR_HPP
#include "PDOGS.hpp"

template<typename TGameRendererConfig>
class CellRendererThirdPassVisitor final : public Feis::CellVisitor {
public:
    using Direction = Feis::Direction;
    using CellPosition = Feis::CellPosition;

    CellRendererThirdPassVisitor(const Feis::IGameInfo *info, Drawer<TGameRendererConfig> *drawer,
                                 const CellPosition cellPosition) :
        info_(info), drawer_(drawer), cellPosition_(cellPosition) {}

    void Visit(const Feis::CollectionCenterCell *cell) const override {
        if (cellPosition_ != cell->GetTopLeftCellPosition())
            return;

        sf::RectangleShape rectangle(sf::Vector2f(TGameRendererConfig::kCellSize * cell->GetWidth(),
                                                  TGameRendererConfig::kCellSize * cell->GetHeight()));

        rectangle.setFillColor(sf::Color(0, 0, 180));
        rectangle.setPosition(drawer_->GetCellTopLeft(cellPosition_));
        drawer_->DrawShape(rectangle);

        sf::Vector2f scoreTextPosition =
                drawer_->GetCellTopLeft(cell->GetTopLeftCellPosition()) +
                sf::Vector2f(static_cast<float>(cell->GetWidth()), static_cast<float>(cell->GetHeight())) * 0.5f *
                        static_cast<float>(TGameRendererConfig::kCellSize) +
                sf::Vector2f(0, -10);

        drawer_->DrawText(std::to_string(cell->GetScores()), 20, sf::Color::White, scoreTextPosition);

        drawer_->DrawText(info_->GetLevelInfo(), 16, sf::Color(0, 255, 0), scoreTextPosition + sf::Vector2f(0, 30));
    }

    void Visit(const Feis::CombinerCell *cell) const override {
        auto direction = cell->GetDirection();

        if (cell->IsMainCell(cellPosition_)) {
            drawer_->DrawRectangle(cellPosition_,
                                   cell->GetFirstSlotProduct() != 0 ? sf::Color::Yellow : sf::Color(200, 200, 200));
            drawer_->DrawArrow(cellPosition_, direction);
        } else {
            drawer_->DrawTriangle(cellPosition_, direction,
                                  cell->GetSecondSlotProduct() != 0 ? sf::Color::Yellow : sf::Color(200, 200, 200));
        }
    }

    void Visit(const Feis::WallCell *cell) const override {
        drawer_->DrawRectangle(cellPosition_, sf::Color(60, 60, 60));
    }

private:
    const Feis::IGameInfo *info_;
    Drawer<TGameRendererConfig> *const drawer_;
    CellPosition cellPosition_;
};
#endif
