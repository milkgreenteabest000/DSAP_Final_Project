#ifndef GAME_RENDERER_HPP
#define GAME_RENDERER_HPP
#include "Drawer.hpp"
#include "LayeredCellRenderer.hpp"
#include "PDOGS.hpp"

template<typename TGameRendererConfig>
class GameRenderer {
public:
    using GameManagerConfig = Feis::GameManagerConfig;

    explicit GameRenderer(sf::RenderWindow *window) : renderer_(window) {}

    void Render(const Feis::IGameInfo &gameManagerInfo) {
        renderer_.Clear();

        for (int row = 0; row < GameManagerConfig::kBoardHeight; ++row) {
            for (int col = 0; col < GameManagerConfig::kBoardWidth; ++col) {
                layeredCellRenderer_.RenderPassOne(gameManagerInfo, renderer_, {row, col});
            }
        }

        for (int row = 0; row < GameManagerConfig::kBoardHeight; ++row) {
            for (int col = 0; col < GameManagerConfig::kBoardWidth; ++col) {
                layeredCellRenderer_.RenderPassTwo(gameManagerInfo, renderer_, {row, col});
            }
        }

        for (int row = 0; row < GameManagerConfig::kBoardHeight; ++row) {
            for (int col = 0; col < GameManagerConfig::kBoardWidth; ++col) {
                layeredCellRenderer_.RenderPassThree(gameManagerInfo, renderer_, {row, col});
            }
        }

        int timeLeft = gameManagerInfo.GetEndTime() - gameManagerInfo.GetElapsedTime();

        renderer_.DrawText(std::to_string(timeLeft / (TGameRendererConfig::kFPS * 60) / 10) +
                                   std::to_string(timeLeft / (TGameRendererConfig::kFPS * 60) % 10) + ":" +
                                   std::to_string(timeLeft / TGameRendererConfig::kFPS % 60 / 10) +
                                   std::to_string(timeLeft / TGameRendererConfig::kFPS % 10),
                           20, sf::Color::White, sf::Vector2f(50, 30));


        renderer_.Display();
    }

private:
    Drawer<TGameRendererConfig> renderer_;
    LayeredCellRenderer<TGameRendererConfig> layeredCellRenderer_;
};
#endif
