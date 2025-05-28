#ifndef CELL_RENDERER_SECOND_PASS_VISITOR_HPP
#define CELL_RENDERER_SECOND_PASS_VISITOR_HPP
#include "PDOGS.hpp"

template<typename TGameRendererConfig>
class CellRendererSecondPassVisitor final : public Feis::CellVisitor {
public:
    using Direction = Feis::Direction;
    using CellPosition = Feis::CellPosition;

    CellRendererSecondPassVisitor(const Feis::IGameInfo *info, Drawer<TGameRendererConfig> *drawer,
                                  const CellPosition cellPosition) :
        info(info), drawer_(drawer), cellPosition_(cellPosition) {}
    void Visit(const Feis::ConveyorCell *cell) const override {
        std::size_t productCount = cell->GetProductCount();

        for (std::size_t k = 0; k < productCount; ++k) {
            int product;
            sf::Vector2f offset;

            switch (cell->GetDirection()) {
                case Direction::kTop:
                    product = cell->GetProduct(k);
                    offset = sf::Vector2f(
                            0, TGameRendererConfig::kCellSize *
                                       (-1.0f + static_cast<float>(k + 1) / static_cast<float>(productCount)));
                    break;
                case Direction::kRight:
                    product = cell->GetProduct(productCount - 1 - k);
                    offset = sf::Vector2f(TGameRendererConfig::kCellSize * static_cast<float>(k) / productCount, 0);
                    break;
                case Direction::kBottom:
                    product = cell->GetProduct(productCount - 1 - k);
                    offset = sf::Vector2f(0, TGameRendererConfig::kCellSize * static_cast<float>(k) / productCount);
                    break;
                case Direction::kLeft:
                    product = cell->GetProduct(k);
                    offset =
                            sf::Vector2f(TGameRendererConfig::kCellSize *
                                                 (-1.0f + static_cast<float>(k + 1) / static_cast<float>(productCount)),
                                         0);
                    break;
                default:
                    assert(0);
                    break;
            }

            if (product != 0) {
                auto center = drawer_->GetCellTopLeft(cellPosition_) + offset +
                              sf::Vector2f(TGameRendererConfig::kCellSize / 2, TGameRendererConfig::kCellSize / 2);

                drawer_->DrawCircle(center, TGameRendererConfig::kCellSize * 0.6,
                                    info->IsScoredProduct(product) ? sf::Color(30, 60, 30) : sf::Color(30, 30, 30));

                drawer_->DrawText(std::to_string(product), TGameRendererConfig::kCellSize * 0.7, sf::Color::White,
                                  center);
            }
        }
    }

private:
    const Feis::IGameInfo *info;
    Drawer<TGameRendererConfig> *drawer_;
    CellPosition cellPosition_;
};
#endif
