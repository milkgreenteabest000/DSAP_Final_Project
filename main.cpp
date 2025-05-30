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
#include <stdlib.h> /* 亂數相關函數 */
#include <time.h>   /* 時間相關函數 */
using namespace Feis;

constexpr Feis::CellPosition start[4][4] = {
    {{15,29},{15,30},{15,31},{15,32}},  //top
    {{20,29},{20,30},{20,31},{20,32}},  //bottom
    {{16,28},{17,28},{18,28},{19,28}},  //left
    {{16,33},{17,33},{18,33},{19,33}}}; //right
constexpr Feis::PlayerActionType act[3][4] = { // 0: conveyor, 1: mining, dir: 0 toBottom, 1 toTop, 2 toRight, 3 toLeft
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
    {{ 8, 29}, {15, 32}}, // top
    {{ 3, 30}, { 7, 31}}, // top
    {{20, 29}, {27, 32}}, // bottom
    {{28, 30}, {32, 31}}, // bottom
    {{16, 19}, {19, 28}}, // left
    {{17, 13}, {18, 18}}, // left
    {{16, 33}, {19, 42}}, // right
    {{17, 43}, {18, 48}}  // right
};

bool isOpsiteDirection(Feis::PlayerActionType act1, Feis::PlayerActionType act2) {
    if (act1 == Feis::PlayerActionType::BuildTopToBottomConveyor && act2 == Feis::PlayerActionType::BuildBottomToTopConveyor) return true;
    if (act1 == Feis::PlayerActionType::BuildBottomToTopConveyor && act2 == Feis::PlayerActionType::BuildTopToBottomConveyor) return true;
    if (act1 == Feis::PlayerActionType::BuildLeftToRightConveyor && act2 == Feis::PlayerActionType::BuildRightToLeftConveyor) return true;
    if (act1 == Feis::PlayerActionType::BuildRightToLeftConveyor && act2 == Feis::PlayerActionType::BuildLeftToRightConveyor) return true;
    return false;
}

bool isInExcludedSquare(const Feis::CellPosition& pos) {
    for (const auto& square : excludedSquare) {
        if (pos.row >= square.first.row && pos.row <= square.second.row &&
            pos.col >= square.first.col && pos.col <= square.second.col) {
            return true;
        }
    }
    return false;
}

std::string actionType2str(const Feis::PlayerActionType& type) {
    switch (type) {
        case Feis::PlayerActionType::BuildLeftOutMiningMachine: return "BuildLeftOutMiningMachine";
        case Feis::PlayerActionType::BuildTopOutMiningMachine: return "BuildTopOutMiningMachine";
        case Feis::PlayerActionType::BuildRightOutMiningMachine: return "BuildRightOutMiningMachine";
        case Feis::PlayerActionType::BuildBottomOutMiningMachine: return "BuildBottomOutMiningMachine";
        case Feis::PlayerActionType::BuildLeftToRightConveyor: return "BuildLeftToRightConveyor";
        case Feis::PlayerActionType::BuildTopToBottomConveyor: return "BuildTopToBottomConveyor";
        case Feis::PlayerActionType::BuildRightToLeftConveyor: return "BuildRightToLeftConveyor";
        case Feis::PlayerActionType::BuildBottomToTopConveyor: return "BuildBottomToTopConveyor";
        case Feis::PlayerActionType::BuildTopOutCombiner: return "BuildTopOutCombiner";
        case Feis::PlayerActionType::BuildRightOutCombiner: return "BuildRightOutCombiner";
        case Feis::PlayerActionType::BuildBottomOutCombiner: return "BuildBottomOutCombiner";
        case Feis::PlayerActionType::BuildLeftOutCombiner: return "BuildLeftOutCombiner";
        default: return "None";
    }
}

class GamePlayer final : public Feis::IGamePlayer
{
public:
    GamePlayer() {
        srand( time(NULL) );
        /*
        for(int i = 0; i < 4; i++) {
            for(int j = 0; j < 4; j++){
                pushOperation({act[0][i], start[i][j]});
            }
        }
        */
    }

    void initializer(const Feis::IGameInfo& info){

        int dividendNum = 0;
        for(auto i : {1, 2, 3, 5, 7, 11, 13}){
            if(info.IsScoredProduct(i)) dividendNum++;
        }
        if(dividendNum <= 0){
            isRequiredCombiner = 1;
        }

        if(isRequiredCombiner){
            for(int i = 0; i < 4; i++) {
                for(int j = 0; j < 4; j++){
                    pushOperation({act[2][i], start[i][j]});
                }
            }

            return;
        }

        for(int i = 0; i < 4; i++) {
            for(int j = 0; j < 4; j++){
                pushOperation({act[0][i], start[i][j]});
            }
        }
    }

    void pushOperation(const Feis::PlayerAction& action) {
        actions_.push(action);
        visited_[action.cellPosition.row * 100 + action.cellPosition.col] = action.type;
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
        for (int i = 0; i < 4; ++i) {
            auto neighbor = GetNeighborCellPosition(pos, dirs[i^1]);
            if(visited_.count(neighbor.row * 100 + neighbor.col) && visited_[neighbor.row * 100 + neighbor.col] != PlayerActionType::None) continue;
            if(!Feis::IsWithinBoard(neighbor)) continue;
            if(IsBlockingCell(neighbor, info)) continue;
            auto lc = info.GetLayeredCell(neighbor);
            auto bg = lc.GetBackground();
            auto mining = std::dynamic_pointer_cast<Feis::NumberCell>(bg);
            if(isInExcludedSquare(neighbor))
                pushOperation({ act[0][i], neighbor});
            else if (mining && info.IsScoredProduct(mining->GetNumber()))
                pushOperation({ act[1][i], neighbor });
            else {
                bool nearMain = false;
                for(int j = 0; j < 4; j++) {
                    auto near = GetNeighborCellPosition(neighbor, dirs[j]);
                    if(!Feis::IsWithinBoard(near)) continue;
                    if (isInExcludedSquare(near) && !IsBlockingCell(near, info) && !isOpsiteDirection(act[0][j], visited_[near.row * 100 + near.col])) {
                        if(isRequiredCombiner && (rand()%100 < 80)) pushOperation({ act[2][j], neighbor });
                        pushOperation({ act[0][j], neighbor });
                        nearMain = true;
                        break;
                    }
                }
                if(!nearMain)
                    pushOperation({ act[0][i], neighbor });
            }
        }
    }

    Feis::PlayerAction GetNextAction(const Feis::IGameInfo& info) override {
        if(isFirst) {initializer(info); isFirst = 0;}

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
    std::unordered_map<int, Feis::PlayerActionType> visited_;
    int isFirst = 1;
    bool isRequiredCombiner = 0;
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
