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

// start

#include <unordered_set>
#include <algorithm>
#include <map>
#include <queue>
#include <unordered_map>
#include <cmath>
#include <vector>
#include <functional>
using namespace Feis;

constexpr Feis::CellPosition start[4][4] = {
    {{15,29},{15,30},{15,31},{15,32}},  //top
    {{20,29},{20,30},{20,31},{20,32}},  //bottom
    {{16,28},{17,28},{18,28},{19,28}},  //left
    {{16,33},{17,33},{18,33},{19,33}}}; //right
constexpr Feis::PlayerActionType act[3][4] = { // 0: conveyor, 1: mining, 2: combiner dir: 0 toBottom, 1 toTop, 2 toRight, 3 toLeft
    {Feis::PlayerActionType::BuildTopToBottomConveyor,
    Feis::PlayerActionType::BuildBottomToTopConveyor,
    Feis::PlayerActionType::BuildLeftToRightConveyor,
    Feis::PlayerActionType::BuildRightToLeftConveyor},
    {Feis::PlayerActionType::BuildBottomOutMiningMachine, 
    Feis::PlayerActionType::BuildTopOutMiningMachine,
    Feis::PlayerActionType::BuildRightOutMiningMachine,
    Feis::PlayerActionType::BuildLeftOutMiningMachine,},
    {Feis::PlayerActionType::BuildBottomOutCombiner, 
    Feis::PlayerActionType::BuildTopOutCombiner,
    Feis::PlayerActionType::BuildRightOutCombiner,
    Feis::PlayerActionType::BuildLeftOutCombiner}
};
constexpr Feis::Direction dirs[4] = {Feis::Direction::kBottom, Feis::Direction::kTop, Feis::Direction::kRight, Feis::Direction::kLeft};
std::vector<std::pair<Feis::CellPosition, Feis::CellPosition>> excludedSquare = {
    {{11, 29}, {15, 32}}, // top
    {{ 6, 30}, {10, 31}}, // top
    {{20, 29}, {24, 32}}, // bottom
    {{25, 30}, {29, 31}}, // bottom
    {{16, 24}, {19, 28}}, // left
    {{17, 19}, {18, 23}}, // left
    {{16, 33}, {19, 37}}, // right
    {{17, 38}, {18, 42}}  // right
};

bool isInExcludedSquare(const Feis::CellPosition& pos) {
    for (const auto& square : excludedSquare) {
        if (pos.row >= square.first.row && pos.row <= square.second.row &&
            pos.col >= square.first.col && pos.col <= square.second.col) {
            return true;
        }
    }
    return false;
}

Feis::PlayerActionType getCornerConveyorAction(int dir, int posIndex) {
    // dir: 0=top, 1=bottom, 2=left, 3=right
    // posIndex: 0 or 3 -> 判斷是左角或右角
    if (dir == 0) return (posIndex == 0) ? act[0][2] : act[0][3]; // top → 左用→，右用←
    if (dir == 1) return (posIndex == 0) ? act[0][2] : act[0][3]; // bottom → 同上
    if (dir == 2) return (posIndex == 0) ? act[0][0] : act[0][1]; // left → 上下
    if (dir == 3) return (posIndex == 0) ? act[0][0] : act[0][1]; // right → 上下
    return act[0][dir]; // fallback
}

class GamePlayer final : public Feis::IGamePlayer
{
public:
    GamePlayer() {
        // 建 Combiner（內圈）
        for (int dir = 0; dir < 4; dir++) {
            for (int j = 0; j < 4; j++) {
                Feis::CellPosition pos = start[dir][j];
                if (!IsWithinBoard(pos)) continue;
                pushOperation({act[0][dir], pos}); // 2 表 combiner
            }
        }
    }

    void pushOperation(const Feis::PlayerAction& action) {actions_.push(action); visited_[action.cellPosition.row * 100 + action.cellPosition.col] = true;}

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
        for (int i = 0; i < 4; ++i) {
            auto neighbor = GetNeighborCellPosition(pos, dirs[i^1]);
            if(visited_.count(neighbor.row * 100 + neighbor.col) && visited_[neighbor.row * 100 + neighbor.col]) continue;
            visited_[neighbor.row * 100 + neighbor.col] = true;  // 標記為已訪問
            if(!Feis::IsWithinBoard(neighbor)) continue;
            if(IsBlockingCell(neighbor, info)) continue;
            auto lc = info.GetLayeredCell(neighbor);
            auto bg = lc.GetBackground();
            auto mining = std::dynamic_pointer_cast<Feis::NumberCell>(bg);
            actions_.push({ act[!(isInExcludedSquare(neighbor)) && mining && info.IsScoredProduct(mining->GetNumber())][i], neighbor });
        }
    }

    PlayerAction findCombiner(PlayerAction curr, const Feis::IGameInfo& info){
        constexpr Feis::Direction dirs[4] = {
            Feis::Direction::kTop, Feis::Direction::kBottom,
            Feis::Direction::kLeft, Feis::Direction::kRight
        };
        constexpr Feis::Direction dirsCounterClock[4] = {
            Feis::Direction::kLeft, Feis::Direction::kRight,
            Feis::Direction::kBottom, Feis::Direction::kTop
        };
        constexpr Feis::Direction dirsClock[4] = {
            Feis::Direction::kRight, Feis::Direction::kLeft,
            Feis::Direction::kTop, Feis::Direction::kBottom
        };
        constexpr Feis::PlayerActionType miningMachineAct[4] = {
            Feis::PlayerActionType::BuildTopOutMiningMachine,
            Feis::PlayerActionType::BuildBottomOutMiningMachine,
            Feis::PlayerActionType::BuildLeftOutMiningMachine,
            Feis::PlayerActionType::BuildRightOutMiningMachine
        };
        constexpr Feis::PlayerActionType act[4] = {
            Feis::PlayerActionType::BuildTopOutCombiner,
            Feis::PlayerActionType::BuildBottomOutCombiner,
            Feis::PlayerActionType::BuildRightOutCombiner,
            Feis::PlayerActionType::BuildLeftOutCombiner
        };

        Feis::CellPosition pos = curr.cellPosition;

        auto tryFind = [&](const Feis::Direction neighborDir, const Feis::Direction outputDir, 
            Feis::PlayerActionType expectedType, Feis::PlayerActionType buildAction) -> std::optional<PlayerAction> {

            if (curr.type != expectedType) return std::nullopt;
            
            Feis::CellPosition neighbor = GetNeighborCellPosition(pos, neighborDir);
            auto neighborCell = info.GetLayeredCell(neighbor);
            auto neighborFg = neighborCell.GetForeground();
            auto neighborNumberCell = std::dynamic_pointer_cast<Feis::NumberCell>(neighborFg);
            if (!neighborNumberCell || !info.IsScoredProduct(neighborNumberCell->GetNumber()))
                return std::nullopt;

            Feis::CellPosition combinerPos = GetNeighborCellPosition(pos, outputDir);
            return PlayerAction{buildAction, combinerPos};
        };


        for(int i = 0; i < 4; ++i){
            if (auto result = tryFind(dirsCounterClock[i], dirs[i], miningMachineAct[i], act[i]))
                return *result;
        }
        for(int i = 0; i < 4; ++i){
            if (auto result = tryFind(dirsClock[i], dirs[i], miningMachineAct[i], act[i]))
                return *result;
        }


        return {Feis::PlayerActionType::None, {0, 0}};
    }

    void pushCombiner(PlayerAction combiner, const Feis::IGameInfo& info){

        struct CombinerInfo {
            Feis::PlayerActionType action;
            Feis::Direction attachedDir;
        };

        constexpr CombinerInfo combiners[4] = {
            {Feis::PlayerActionType::BuildBottomOutCombiner, Feis::Direction::kRight},
            {Feis::PlayerActionType::BuildTopOutCombiner, Feis::Direction::kLeft},
            {Feis::PlayerActionType::BuildRightOutCombiner, Feis::Direction::kTop},
            {Feis::PlayerActionType::BuildLeftOutCombiner, Feis::Direction::kBottom},
        };

        Feis::CellPosition pos = combiner.cellPosition;
        for (const auto& c : combiners) {
            if (combiner.type == c.action) {
                if (std::dynamic_pointer_cast<Feis::ConveyorCell>(info.GetLayeredCell(pos).GetForeground()));
                    actions_.push({Feis::PlayerActionType::Clear, pos});

                Feis::CellPosition attached = GetNeighborCellPosition(pos, c.attachedDir);
                if (std::dynamic_pointer_cast<Feis::ConveyorCell>(info.GetLayeredCell(attached).GetForeground()))
                    actions_.push({Feis::PlayerActionType::Clear, attached});

                actions_.push(combiner);
                return;
            }
        }

    }
    


    Feis::PlayerAction GetNextAction(const Feis::IGameInfo& info) override {
        while (!actions_.empty()) {
            Feis::PlayerAction curr = actions_.front();
            actions_.pop();
            Feis::CellPosition pos = curr.cellPosition;

            if(curr.type == Feis::PlayerActionType::None) return {Feis::PlayerActionType::None, {0, 0}};
            
            for(auto actConveyor : act[0]){
                if(curr.type == actConveyor){
                    pushNeighbor(pos, info);
                    return curr;
                }
            }
            for(auto actConveyor : act[2]){
                if(curr.type == actConveyor){
                    pushNeighbor(pos, info);
                    return curr;
                }
            }
            return curr;
        }
        return {Feis::PlayerActionType::None, {0, 0}};
    }

    void EnqueueAction(const PlayerAction& action) { actions_.push(action);}

private:
    std::queue<Feis::PlayerAction> actions_;
    std::unordered_map<int, bool> visited_;
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
