#include <fstream>
#include <map>
#include <queue>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include "GameRenderer.hpp"

using namespace Feis;

struct GameRendererConfig {
    static constexpr int kFPS = 30;
    static constexpr int kCellSize = 20;
    static constexpr int kBoardLeft = 20;
    static constexpr int kBoardTop = 60;
    static constexpr int kBorderSize = 1;
};

CellPosition GetMouseCellPosition(const sf::RenderWindow &window) {
    const sf::Vector2i mousePosition = sf::Mouse::getPosition(window);
    const sf::Vector2i relatedMousePosition =
            mousePosition - sf::Vector2i(GameRendererConfig::kBoardLeft, GameRendererConfig::kBoardTop);
    return {relatedMousePosition.y / GameRendererConfig::kCellSize,
            relatedMousePosition.x / GameRendererConfig::kCellSize};
}

#include <fstream>
#include <iostream>
#include <unordered_set>
#include <algorithm>
#include <map>
#include <queue>
#include <unordered_map>
#include <cmath>
#include <functional>

struct CellPositionHasher {std::size_t operator()(const Feis::CellPosition& pos) const {return std::hash<int>()(pos.row) ^ (std::hash<int>()(pos.col) << 1);}};

class GamePlayer final : public Feis::IGamePlayer
{
public:
    GamePlayer() : isFirst(1) {
        // logFile.open("run.log", std::ios::out | std::ios::trunc);
        // if (!logFile.is_open())
        //     std::cerr << "Failed to open log file." << std::endl;
    }

    // 判斷某個格子（背景或前景）是否為「阻擋方塊」
    bool IsBlockingCell(const Feis::CellPosition& pos, const Feis::IGameInfo& info) {
        if (pos.row > 15 && pos.row < 19 && pos.col > 28 && pos.col < 33)
            return true;
        auto lc = info.GetLayeredCell(pos);
        auto fg = lc.GetForeground();
        if (std::dynamic_pointer_cast<Feis::WallCell>(fg) ||
            std::dynamic_pointer_cast<Feis::MiningMachineCell>(fg) ||
            std::dynamic_pointer_cast<Feis::CombinerCell>(fg)) return true;
        return false;
    }

    void pushNeighbor(const Feis::CellPosition& pos, const Feis::IGameInfo& info) {
        constexpr Feis::Direction dirs[4] = {
            Feis::Direction::kTop, Feis::Direction::kBottom,
            Feis::Direction::kLeft, Feis::Direction::kRight
        };
        constexpr Feis::PlayerActionType act[4] = {
            Feis::PlayerActionType::BuildTopToBottomConveyor,
            Feis::PlayerActionType::BuildBottomToTopConveyor,
            Feis::PlayerActionType::BuildLeftToRightConveyor,
            Feis::PlayerActionType::BuildRightToLeftConveyor
        };
        // logFile << "Pushing neighbors for position: (" << pos.row << ", " << pos.col << ")" << std::endl;
        // logFile.flush();
        for (int i = 0; i < 4; ++i) {
            auto neighbor = GetNeighborCellPosition(pos, dirs[i]);
            if(visited_.count(neighbor) && visited_[neighbor]) continue;   // 如果 neighbor 已經被訪問過，就不要再加入佇列
            visited_[neighbor] = true;  // 標記為已訪問
            if(!Feis::IsWithinBoard(neighbor)) continue; // 如果 neighbor 不在棋盤內，就跳過 
            // log
            // logFile << "Checking direction: " << static_cast<int>(dirs[i]) << " from position: (" 
            //         << pos.row << ", " << pos.col << ") => " << "Position: ("
            //         << neighbor.row << ", " << neighbor.col << ")" << std::endl;
            // logFile << "    Check Wall:" << IsBlockingCell(neighbor, info) << std::endl;

            if(IsBlockingCell(neighbor, info)) continue;
            actions_.push({ act[i], neighbor });
        }
    }


    Feis::PlayerAction GetNextAction(const Feis::IGameInfo& info) override {

        if(isFirst){
            Feis::CellPosition start[4][4] = {{{15,29},{15,30},{15,31},{15,32}},
                                            {{16,28},{17,28},{18,28},{19,28}},
                                            {{16,33},{17,33},{18,33},{19,33}},
                                            {{20,29},{20,30},{20,31},{20,32}}};
            Feis::PlayerActionType machineDirection[4] = {Feis::PlayerActionType::BuildBottomOutMiningMachine, 
                                                        Feis::PlayerActionType::BuildRightOutMiningMachine,
                                                        Feis::PlayerActionType::BuildLeftOutMiningMachine,
                                                        Feis::PlayerActionType::BuildTopOutMiningMachine};
            Feis::PlayerActionType beltDirection[4] = {Feis::PlayerActionType::BuildTopToBottomConveyor, 
                                                        Feis::PlayerActionType::BuildLeftToRightConveyor,
                                                        Feis::PlayerActionType::BuildRightToLeftConveyor,
                                                        Feis::PlayerActionType::BuildBottomToTopConveyor};

            for(int i = 0; i < 4; i++)
                for(int j = 0; j < 4; j++){
                    if(std::dynamic_pointer_cast<Feis::NumberCell>(info.GetLayeredCell(start[i][j]).GetBackground()))
                        actions_.push({machineDirection[i], start[i][j]});
                    else
                        actions_.push({beltDirection[i], start[i][j]});
                    visited_[start[i][j]] = true;
                }
            isFirst = 0;
        }
        



        while (!actions_.empty()) {
            Feis::PlayerAction curr = actions_.front();
            actions_.pop();
            Feis::CellPosition pos = curr.cellPosition;

            Feis::LayeredCell cell = info.GetLayeredCell(pos);
            auto bg = cell.GetBackground();
            auto fg = cell.GetForeground();

            auto mining = std::dynamic_pointer_cast<Feis::NumberCell>(bg);
            if (mining && info.IsScoredProduct(mining->GetNumber())) {
                for (auto dir : {Feis::Direction::kTop, Feis::Direction::kBottom,
                                    Feis::Direction::kLeft, Feis::Direction::kRight}) {

                    auto neighbor = GetNeighborCellPosition(pos, dir);
                    if (!Feis::IsWithinBoard(neighbor)) continue;

                    auto neighborCell = info.GetLayeredCell(neighbor);
                    auto neighborFg = neighborCell.GetForeground();

                    if (std::dynamic_pointer_cast<Feis::CollectionCenterCell>(neighborFg) ||
                        std::dynamic_pointer_cast<Feis::ConveyorCell>(neighborFg)) {
                        switch (dir) {
                            case Feis::Direction::kRight:
                                curr = {Feis::PlayerActionType::BuildRightOutMiningMachine, pos};
                                break;
                            case Feis::Direction::kLeft:
                                curr = {Feis::PlayerActionType::BuildLeftOutMiningMachine, pos};
                                break;
                            case Feis::Direction::kTop:
                                curr = {Feis::PlayerActionType::BuildTopOutMiningMachine, pos};
                                break;
                            case Feis::Direction::kBottom:
                                curr = {Feis::PlayerActionType::BuildBottomOutMiningMachine, pos};
                                break;
                        }
                    }
                }
            }

            if(curr.type != Feis::PlayerActionType::None){
                if (curr.type == Feis::PlayerActionType::BuildLeftToRightConveyor ||
                    curr.type == Feis::PlayerActionType::BuildTopToBottomConveyor ||
                    curr.type == Feis::PlayerActionType::BuildRightToLeftConveyor ||
                    curr.type == Feis::PlayerActionType::BuildBottomToTopConveyor)
                    pushNeighbor(pos, info);
                return curr;
            }
        }

        return {Feis::PlayerActionType::None, {0, 0}};
    }

    void EnqueueAction(const PlayerAction& action) { actions_.push(action);}

private:
    std::queue<Feis::PlayerAction> actions_;
    std::unordered_map<Feis::CellPosition, bool, CellPositionHasher> visited_;
    bool isFirst;
    // std::fstream logFile;
};


// end

void Save(std::queue<PlayerAction> playerActionHistory, const std::string &filename) {
    std::ofstream outFile(filename);
    while (!playerActionHistory.empty()) {
        const auto &[type, cellPosition] = playerActionHistory.front();
        outFile << cellPosition.row << " " << cellPosition.col << " "
                << static_cast<int>(type) << std::endl;
        playerActionHistory.pop();
    }
}

int main(int, char **) {
    auto mode = sf::VideoMode(sf::Vector2u(1280, 1024));

    sf::RenderWindow window(mode, "DSAP Final Project", sf::Style::Close);

    window.setFramerateLimit(GameRendererConfig::kFPS);

    GamePlayer player;

    GameManager gameManager(&player, 1, 20);

    const std::map<sf::Keyboard::Key, PlayerActionType> playerActionKeyboardMap = {
            {sf::Keyboard::Key::J, PlayerActionType::BuildLeftOutMiningMachine},
            {sf::Keyboard::Key::I, PlayerActionType::BuildTopOutMiningMachine},
            {sf::Keyboard::Key::L, PlayerActionType::BuildRightOutMiningMachine},
            {sf::Keyboard::Key::K, PlayerActionType::BuildBottomOutMiningMachine},
            {sf::Keyboard::Key::D, PlayerActionType::BuildLeftToRightConveyor},
            {sf::Keyboard::Key::S, PlayerActionType::BuildTopToBottomConveyor},
            {sf::Keyboard::Key::A, PlayerActionType::BuildRightToLeftConveyor},
            {sf::Keyboard::Key::W, PlayerActionType::BuildBottomToTopConveyor},
            {sf::Keyboard::Key::Num1, PlayerActionType::BuildTopOutCombiner},
            {sf::Keyboard::Key::Num2, PlayerActionType::BuildRightOutCombiner},
            {sf::Keyboard::Key::Num3, PlayerActionType::BuildBottomOutCombiner},
            {sf::Keyboard::Key::Num4, PlayerActionType::BuildLeftOutCombiner},
            {sf::Keyboard::Key::Backspace, PlayerActionType::Clear},
    };

    auto playerActionType = PlayerActionType::BuildLeftToRightConveyor;

    GameRenderer<GameRendererConfig> gameRenderer(&window);

    std::queue<PlayerAction> playerActionHistory;

    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            if (const auto *mouseEvent = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (const CellPosition mouseCellPosition = GetMouseCellPosition(window);
                    IsWithinBoard(mouseCellPosition)) {
                    if (mouseEvent->button == sf::Mouse::Button::Left) {
                        auto playerAction = PlayerAction{playerActionType, mouseCellPosition};
                        player.EnqueueAction(playerAction);
                        playerActionHistory.push(playerAction);
                    }
                }
            }

            if (const auto *keyboardEvent = event->getIf<sf::Event::KeyReleased>()) {
                if (playerActionKeyboardMap.count(keyboardEvent->code)) {
                    playerActionType = playerActionKeyboardMap.at(keyboardEvent->code);
                } else if (keyboardEvent->code == sf::Keyboard::Key::F4) {
                    Save(playerActionHistory, "gameplay.txt");
                }
            }
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        gameManager.Update();

        gameRenderer.Render(gameManager);
    }
}
