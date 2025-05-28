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

using namespace std;
#include <unordered_set>
#include <algorithm>
#include <map>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <cmath>
#include <functional>

Feis::CellPosition centerPosition[4][4] = {
    {{16,29},{16,30},{16,31},{16,32}},
    {{17,29},{17,30},{17,31},{17,32}},
    {{18,29},{18,30},{18,31},{18,32}},
    {{19,29},{19,30},{19,31},{19,32}}
};

struct DirectionInfo {
    Feis::Direction outputDir; // conveyor 方向
    Feis::Direction neighborDir; // 從哪邊來
    PlayerActionType conveyorAction;
    PlayerActionType miningMachineAction;
};

std::unordered_map<int, DirectionInfo> directionMap = {
    {0, {Feis::Direction::kBottom, Feis::Direction::kTop, PlayerActionType::BuildTopToBottomConveyor, PlayerActionType::BuildBottomOutMiningMachine}},
    {1, {Feis::Direction::kLeft,   Feis::Direction::kRight, PlayerActionType::BuildRightToLeftConveyor, PlayerActionType::BuildLeftOutMiningMachine}},
    {2, {Feis::Direction::kTop,    Feis::Direction::kBottom, PlayerActionType::BuildBottomToTopConveyor, PlayerActionType::BuildTopOutMiningMachine}},
    {3, {Feis::Direction::kRight,  Feis::Direction::kLeft, PlayerActionType::BuildLeftToRightConveyor, PlayerActionType::BuildRightOutMiningMachine}},
};


int countDistance(const Feis::CellPosition x, const Feis::CellPosition y){
    return abs(x.col - y.col) + abs(x.row - y.row);
}

int countDistanceWithCenter(const Feis::CellPosition x){
    int minDistance = 100;
    for(int i = 0; i < 16; i++){
        int r = i / 4, c = i % 4;
        if(r == 1 || r == 2) continue;
        if(c == 1 || c == 2) continue;

        minDistance = min(minDistance, countDistance(x, centerPosition[r][c]));
    }
    return minDistance;
}

int getNextDirection(const Feis::CellPosition x){
    if(x.col < 29) return 3;
    if(x.col > 32) return 1;
    if(x.row < 16) return 0;
    return 2;
}

struct CellPositionHasher {
    std::size_t operator()(const Feis::CellPosition& pos) const {
        return std::hash<int>()(pos.row) ^ (std::hash<int>()(pos.col) << 1);
    }
};

class GamePlayer final : public Feis::IGamePlayer
{
public:
    GamePlayer(){
        Feis::CellPosition start = origin_;
        actions_.emplace(Feis::PlayerAction{Feis::PlayerActionType::None, start});
        visited_.insert(start);
    }

    void pushNeighbot(const Feis::CellPosition pos, const Feis::IGameInfo& info){
        for (auto dir : {Feis::Direction::kTop, Feis::Direction::kBottom,
                         Feis::Direction::kLeft, Feis::Direction::kRight}) {

            auto neighbor = GetNeighborCellPosition(pos, dir);
            if (!Feis::IsWithinBoard(neighbor)) continue;
            if (visited_.count(neighbor)) continue;

            auto neighborCell = info.GetLayeredCell(neighbor);
            auto fg = neighborCell.GetForeground();
            auto bg = neighborCell.GetBackground();

            if (std::dynamic_pointer_cast<Feis::WallCell>(bg) ||
                std::dynamic_pointer_cast<Feis::CollectionCenterCell>(bg) ||
                std::dynamic_pointer_cast<Feis::MiningMachineCell>(fg) ||
                std::dynamic_pointer_cast<Feis::ConveyorCell>(fg) ||
                std::dynamic_pointer_cast<Feis::CombinerCell>(fg)) {
                continue;
            }

            visited_.insert(neighbor);
            actions_.emplace(Feis::PlayerAction{Feis::PlayerActionType::None, neighbor});
        }
    }

    Feis::PlayerAction GetNextAction(const Feis::IGameInfo& info) override {
        while (!actions_.empty()) {
            Feis::PlayerAction curr = actions_.top();
            actions_.pop();

            Feis::CellPosition pos = curr.cellPosition;

            Feis::LayeredCell cell = info.GetLayeredCell(pos);
            auto bg = cell.GetBackground();
            auto fg = cell.GetForeground();

            if (auto mining = std::dynamic_pointer_cast<Feis::NumberCell>(bg)) {
                if (info.IsScoredProduct(mining->GetNumber())) {
                    for (auto dir : {Feis::Direction::kTop, Feis::Direction::kBottom,
                                     Feis::Direction::kLeft, Feis::Direction::kRight}) {
                        auto neighbor = GetNeighborCellPosition(pos, dir);
                        if (!Feis::IsWithinBoard(neighbor)) continue;

                        auto neighborCell = info.GetLayeredCell(neighbor);
                        auto neighborFg = neighborCell.GetForeground();

                        if (std::dynamic_pointer_cast<Feis::CollectionCenterCell>(neighborFg) ||
                            std::dynamic_pointer_cast<Feis::ConveyorCell>(neighborFg)) {

                            curr.type = GetMiningAction(dir);
                            pushNeighbot(pos, info);
                            return curr;
                        }
                    }
                }
            }

            for (auto dir : {Feis::Direction::kTop, Feis::Direction::kBottom,
                             Feis::Direction::kLeft, Feis::Direction::kRight}) {

                auto neighbor = GetNeighborCellPosition(pos, dir);
                if (!Feis::IsWithinBoard(neighbor)) continue;

                auto neighborCell = info.GetLayeredCell(neighbor);
                auto neighborFg = neighborCell.GetForeground();

                if (std::dynamic_pointer_cast<Feis::CollectionCenterCell>(neighborFg) ||
                    std::dynamic_pointer_cast<Feis::ConveyorCell>(neighborFg)) {

                    curr.type = GetConveyorAction(dir);
                    pushNeighbot(pos, info);
                    return curr;
                }
            }
        }

        return {Feis::PlayerActionType::None, {0, 0}};
    }

    void EnqueueAction(const PlayerAction action) { actions_.emplace(action); }

private:
    Feis::CellPosition origin_ = {15, 29};

    struct Compare {
        Feis::CellPosition origin;
        Compare(Feis::CellPosition o) : origin(o) {}

        bool operator()(const Feis::PlayerAction& a, const Feis::PlayerAction& b) const {
            return countDistanceWithCenter(a.cellPosition) > countDistanceWithCenter(b.cellPosition);  // Min-heap
        }
    };

    std::priority_queue<Feis::PlayerAction, std::vector<Feis::PlayerAction>, Compare> actions_{Compare(origin_)};
    std::unordered_set<Feis::CellPosition, CellPositionHasher> visited_;

    Feis::PlayerActionType GetMiningAction(Feis::Direction dir) {
        switch (dir) {
            case Feis::Direction::kRight: return Feis::PlayerActionType::BuildRightOutMiningMachine;
            case Feis::Direction::kLeft: return Feis::PlayerActionType::BuildLeftOutMiningMachine;
            case Feis::Direction::kTop: return Feis::PlayerActionType::BuildTopOutMiningMachine;
            case Feis::Direction::kBottom: return Feis::PlayerActionType::BuildBottomOutMiningMachine;
        }
        return Feis::PlayerActionType::None;
    }

    Feis::PlayerActionType GetConveyorAction(Feis::Direction dir) {
        switch (dir) {
            case Feis::Direction::kRight: return Feis::PlayerActionType::BuildLeftToRightConveyor;
            case Feis::Direction::kLeft: return Feis::PlayerActionType::BuildRightToLeftConveyor;
            case Feis::Direction::kTop: return Feis::PlayerActionType::BuildBottomToTopConveyor;
            case Feis::Direction::kBottom: return Feis::PlayerActionType::BuildTopToBottomConveyor;
        }
        return Feis::PlayerActionType::None;
    }
};







/*
class GamePlayerWithHistory final : public IGamePlayer {
public:
    PlayerAction GetNextAction(const IGameInfo &info) override {
        // static 狀態變數，模擬我們在走路
        static int spiralLayer = 1;
        static vector<Feis::CellPosition> spiralPath = generateSpiralPath(spiralLayer);
        static int pathIndex = 0;

        // 下一步要去哪
        Feis::CellPosition nextMove = spiralPath[pathIndex];

        // 更新狀態
        pathIndex++;
        if (pathIndex >= spiralPath.size()) {
            spiralLayer++; // 完成一圈，往外擴
            spiralPath = generateSpiralPath(spiralLayer);
            pathIndex = 0;
        }

        Feis::LayeredCell currLayeredCell = info.GetLayeredCell(nextMove);
        auto currBg = currLayeredCell.GetBackground();
        auto currFg = currLayeredCell.GetForeground();

        int nextDirection = getNextDirection(nextMove); // 0: 上, 1: 右, 2: 下, 3: 左

        if (nextDirection == 3) { // ← 要從左邊放輸出物件

            // 1. 是牆壁就跳過
            if (std::dynamic_pointer_cast<WallCell>(currFg)) {
                return GetNextAction(info);
            }

            // 2. 是礦區就建右輸出的礦機
            if (std::dynamic_pointer_cast<NumberCell>(currBg)) {
                return {PlayerActionType::BuildRightOutMiningMachine, nextMove};
            }

            // 3. 準備建 conveyor
            Feis::CellPosition rightPos = GetNeighborCellPosition(nextMove, Direction::kRight);

            Feis::LayeredCell rightCell = info.GetLayeredCell(rightPos);
            auto rightFg = rightCell.GetForeground();
            auto rightbg = rightCell.GetBackground();

            bool rightCanReceive = false;
            if(auto center = std::dynamic_pointer_cast<CollectionCenterCell>(rightFg)){
                rightCanReceive = true;
            }
            else if (auto conveyor = std::dynamic_pointer_cast<ConveyorCell>(rightFg)) {
                // 向左的 conveyor 會從右邊接收
                if (conveyor->GetDirection() != Direction::kLeft) {
                    rightCanReceive = true;
                }
            }

            if (rightCanReceive) {
                return {PlayerActionType::BuildLeftToRightConveyor, nextMove};
            } else {
                return {PlayerActionType::BuildTopToBottomConveyor, nextMove};
            }
        }
        if(nextDirection == 1){
            // 1. 是牆壁就跳過
            if (std::dynamic_pointer_cast<WallCell>(currFg)) {
                return GetNextAction(info);
            }

            // 2. 是礦區就建右輸出的礦機
            if (std::dynamic_pointer_cast<NumberCell>(currBg)) {
                return {PlayerActionType::BuildLeftOutMiningMachine, nextMove};
            }

            // 3. 準備建 conveyor
            Feis::CellPosition leftPos = GetNeighborCellPosition(nextMove, Direction::kLeft);

            Feis::LayeredCell leftCell = info.GetLayeredCell(leftPos);
            auto leftFg = leftCell.GetForeground();
            auto leftbg = leftCell.GetBackground();

            bool leftCanReceive = false;
            if(auto center = std::dynamic_pointer_cast<CollectionCenterCell>(leftFg)){
                leftCanReceive = true;
            }
            else if (auto conveyor = std::dynamic_pointer_cast<ConveyorCell>(leftFg)) {
                // 向左的 conveyor 會從右邊接收
                if (conveyor->GetDirection() != Direction::kLeft) {
                    leftCanReceive = true;
                }
            }

            if (leftCanReceive) {
                return {PlayerActionType::BuildRightToLeftConveyor, nextMove};
            } else {
                return {PlayerActionType::BuildTopToBottomConveyor, nextMove};
            }
        }
        if (nextDirection == 0) { // ↑ 向下輸出

            // 1. 是牆壁就跳過
            if (std::dynamic_pointer_cast<WallCell>(currFg)) {
                return GetNextAction(info);
            }

            // 2. 是礦區就建下輸出的礦機
            if (std::dynamic_pointer_cast<NumberCell>(currBg)) {
                return {PlayerActionType::BuildBottomOutMiningMachine, nextMove};
            }

            // 3. 否則要考慮下方的格子
            Feis::CellPosition belowPos = GetNeighborCellPosition(nextMove, Direction::kBottom);
            if (belowPos.row > 61) {
                // 邊界保護（根據你地圖大小調整）
                return {PlayerActionType::BuildLeftToRightConveyor, nextMove};
            }

            Feis::LayeredCell belowCell = info.GetLayeredCell(belowPos);
            auto belowFg = belowCell.GetForeground();

            bool belowCanReceive = false;
            if(auto center = std::dynamic_pointer_cast<CollectionCenterCell>(belowFg)){
                belowCanReceive = true;
            }
            else if (auto conveyor = std::dynamic_pointer_cast<ConveyorCell>(belowFg)) {
                // 若方向是向上，就可以從上接收
                if (conveyor->GetDirection() == Direction::kTop) {
                    belowCanReceive = true;
                }
            }

            if (belowCanReceive) {
                return {PlayerActionType::BuildTopToBottomConveyor, nextMove};
            } else {
                return {PlayerActionType::BuildLeftToRightConveyor, nextMove};
            }
        }

        if(nextDirection == 2){

            // 1. 是牆壁就跳過
            if (std::dynamic_pointer_cast<WallCell>(currFg)) {
                return GetNextAction(info);
            }

            // 2. 是礦區就建下輸出的礦機
            if (std::dynamic_pointer_cast<NumberCell>(currBg)) {
                return {PlayerActionType::BuildTopOutMiningMachine, nextMove};
            }

            // 3. 否則要考慮下方的格子
            Feis::CellPosition abovePos = GetNeighborCellPosition(nextMove, Direction::kTop);
            if (abovePos.row > 61) {
                // 邊界保護（根據你地圖大小調整）
                return {PlayerActionType::BuildLeftToRightConveyor, nextMove};
            }

            Feis::LayeredCell aboveCell = info.GetLayeredCell(abovePos);
            auto aboveFg = aboveCell.GetForeground();

            bool aboveCanReceive = false;
            if(auto center = std::dynamic_pointer_cast<CollectionCenterCell>(aboveFg)){
                aboveCanReceive = true;
            }
            else if (auto conveyor = std::dynamic_pointer_cast<ConveyorCell>(aboveFg)) {
                // 若方向是向上，就可以從上接收
                if (conveyor->GetDirection() == Direction::kTop) {
                    aboveCanReceive = true;
                }
            }

            if (aboveCanReceive) {
                return {PlayerActionType::BuildBottomToTopConveyor, nextMove};
            } else {
                return {PlayerActionType::BuildLeftToRightConveyor, nextMove};
            }
        }
    }

    void EnqueueAction(const PlayerAction action) { actions_.push(action); }

private:
    std::queue<PlayerAction> actions_;
};
*/






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
