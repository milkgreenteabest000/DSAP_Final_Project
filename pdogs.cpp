#include <array>
#include <cassert>
#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <string>

namespace Feis {
    struct GameManagerConfig {
        static constexpr int kBoardWidth = 62;
        static constexpr int kBoardHeight = 36;
        static constexpr std::size_t kGoalSize = 4;
        static constexpr std::size_t kConveyorBufferSize = 10;
        static constexpr int kNumberOfWalls = 100;
        static constexpr std::size_t kEndTime = 9000;
    };

    struct CellPosition {
        int row;
        int col;
        CellPosition &operator+=(const CellPosition &other) {
            row += other.row;
            col += other.col;
            return *this;
        }
    };

    inline bool operator==(const CellPosition &lhs, const CellPosition &rhs) {
        return lhs.row == rhs.row && lhs.col == rhs.col;
    }

    inline bool operator!=(const CellPosition &lhs, const CellPosition &rhs) { return !(lhs == rhs); }

    inline CellPosition operator+(const CellPosition &lhs, const CellPosition &rhs) {
        return {lhs.row + rhs.row, lhs.col + rhs.col};
    }

    enum class Direction : int { kTop = 0, kRight = 1, kBottom = 2, kLeft = 3 };

    class GameBoard;

    class LayeredCell;

    class IGameInfo {
    public:
        virtual ~IGameInfo() = default;
        [[nodiscard]] virtual std::string GetLevelInfo() const = 0;
        [[nodiscard]] virtual const LayeredCell &GetLayeredCell(CellPosition cellPosition) const = 0;
        [[nodiscard]] virtual bool IsScoredProduct(int number) const = 0;
        [[nodiscard]] virtual int GetScores() const = 0;
        [[nodiscard]] virtual int GetEndTime() const = 0;
        [[nodiscard]] virtual int GetElapsedTime() const = 0;
        [[nodiscard]] virtual bool IsGameOver() const = 0;
    };

    class IGameManager : public IGameInfo {
    public:
        ~IGameManager() override = default;
        [[nodiscard]] std::string GetLevelInfo() const override = 0;
        [[nodiscard]] const LayeredCell &GetLayeredCell(CellPosition cellPosition) const override = 0;
        [[nodiscard]] bool IsScoredProduct(int number) const override = 0;
        [[nodiscard]] int GetScores() const override = 0;
        [[nodiscard]] int GetEndTime() const override = 0;
        [[nodiscard]] int GetElapsedTime() const override = 0;
        [[nodiscard]] bool IsGameOver() const override = 0;
        virtual void OnProductReceived(int number) = 0;
    };

    class Cell;
    class NumberCell;
    class CollectionCenterCell;
    class MiningMachineCell;
    class ConveyorCell;
    class CombinerCell;
    class WallCell;

    // ReSharper disable once CppClassCanBeFinal
    class CellVisitor {
    public:
        virtual ~CellVisitor() = default;
        virtual void Visit(const NumberCell *cell) const {}
        virtual void Visit(const CollectionCenterCell *cell) const {}
        virtual void Visit(const MiningMachineCell *cell) const {}
        virtual void Visit(const ConveyorCell *cell) const {}
        virtual void Visit(const CombinerCell *cell) const {}
        virtual void Visit(const WallCell *cell) const {}
    };

    class Cell {
    public:
        virtual void Accept(const CellVisitor *visitor) const = 0;
        virtual ~Cell() = default;
    };

    class IBackgroundCell : public Cell {
    public:
        [[nodiscard]] virtual bool CanBuild() const = 0;
        ~IBackgroundCell() override = default;
    };

    class ForegroundCell : public Cell {
    public:
        explicit ForegroundCell(const CellPosition topLeftCellPosition) : topLeftCellPosition_(topLeftCellPosition) {}

        [[nodiscard]] virtual std::size_t GetWidth() const { return 1; }

        [[nodiscard]] virtual std::size_t GetHeight() const { return 1; }

        [[nodiscard]] virtual CellPosition GetTopLeftCellPosition() const { return topLeftCellPosition_; }

        [[nodiscard]] virtual bool CanRemove() const { return false; }

        [[nodiscard]] virtual std::size_t GetCapacity(CellPosition cellPosition) const { return 0; }

        virtual void ReceiveProduct(CellPosition cellPosition, int number) {}

        virtual void UpdatePassOne(CellPosition cellPosition, GameBoard &board) {}

        virtual void UpdatePassTwo(CellPosition cellPosition, GameBoard &board) {}

        ~ForegroundCell() override = default;

    protected:
        CellPosition topLeftCellPosition_;
    };

    class ICellRenderer {
    public:
        virtual ~ICellRenderer() = default;
        virtual void RenderPassOne(CellPosition position) const = 0;
        virtual void RenderPassTwo(CellPosition position) const = 0;
    };

    inline CellPosition GetNeighborCellPosition(const CellPosition cellPosition, const Direction direction) {
        switch (direction) {
            case Direction::kTop:
                return cellPosition + CellPosition{-1, 0};
            case Direction::kRight:
                return cellPosition + CellPosition{0, 1};
            case Direction::kBottom:
                return cellPosition + CellPosition{1, 0};
            case Direction::kLeft:
                return cellPosition + CellPosition{0, -1};
        }
        assert(false);
    }

    std::size_t GetNeighborCapacity(const GameBoard &board, CellPosition cellPosition, Direction direction);

    void SendProduct(const GameBoard &board, CellPosition cellPosition, Direction direction, int product);

    class ConveyorCell final : public ForegroundCell {
    public:
        ConveyorCell(const CellPosition topLeftCellPosition, const Direction direction) :
            ForegroundCell(topLeftCellPosition), products_{}, direction_{direction} {}

        [[nodiscard]] int GetProduct(const std::size_t i) const { return products_[i]; }

        [[nodiscard]] std::size_t GetProductCount() const { return products_.size(); }

        [[nodiscard]] Direction GetDirection() const { return direction_; }

        void Accept(const CellVisitor *visitor) const override { visitor->Visit(this); }

        [[nodiscard]] bool CanRemove() const override { return true; }

        [[nodiscard]] std::size_t GetCapacity(CellPosition cellPosition) const override {
            for (std::size_t i = 0; i < products_.size(); ++i) {
                if (products_[products_.size() - 1 - i] != 0) {
                    return i;
                }
            }
            return products_.size();
        }

        void ReceiveProduct(CellPosition cellPosition, const int number) override {
            assert(number != 0);
            assert(products_.back() == 0);
            products_.back() = number;
        }

        void UpdatePassOne(const CellPosition cellPosition, GameBoard &board) override {
            const std::size_t capacity = GetNeighborCapacity(board, cellPosition, direction_);

            if (capacity >= 3) {
                if (products_[0] != 0) {
                    SendProduct(board, cellPosition, direction_, products_[0]);
                    products_[0] = 0;
                }
            }

            if (capacity >= 2) {
                if (products_[0] == 0 && products_[1] != 0) {
                    std::swap(products_[0], products_[1]);
                }
            }

            if (capacity >= 1) {
                if (products_[0] == 0 && products_[1] == 0 && products_[2] != 0) {
                    std::swap(products_[1], products_[2]);
                }
            }
        }

        void UpdatePassTwo(CellPosition cellPosition, GameBoard &board) override {
            for (std::size_t k = 3; k < products_.size(); ++k) {
                if (products_[k] != 0 && products_[k - 1] == 0 && products_[k - 2] == 0 && products_[k - 3] == 0) {
                    std::swap(products_[k], products_[k - 1]);
                }
            }
        }

    protected:
        std::array<int, GameManagerConfig::kConveyorBufferSize> products_;

    private:
        Direction direction_;
    };

    class CombinerCell final : public ForegroundCell {
    public:
        CombinerCell(const CellPosition topLeft, const Direction direction) :
            ForegroundCell(topLeft), direction_{direction}, firstSlotProduct_{}, secondSlotProduct_{} {}

        [[nodiscard]] Direction GetDirection() const { return direction_; }

        [[nodiscard]] int GetFirstSlotProduct() const { return firstSlotProduct_; }

        [[nodiscard]] int GetSecondSlotProduct() const { return secondSlotProduct_; }

        void Accept(const CellVisitor *visitor) const override { visitor->Visit(this); }

        [[nodiscard]] std::size_t GetWidth() const override {
            return direction_ == Direction::kTop || direction_ == Direction::kBottom ? 2 : 1;
        }

        [[nodiscard]] std::size_t GetHeight() const override {
            return direction_ == Direction::kTop || direction_ == Direction::kBottom ? 1 : 2;
        }

        [[nodiscard]] bool CanRemove() const override { return true; }

        [[nodiscard]] bool IsMainCell(const CellPosition cellPosition) const {
            switch (direction_) {
                case Direction::kTop:
                case Direction::kRight:
                    return cellPosition != topLeftCellPosition_;
                case Direction::kBottom:
                case Direction::kLeft:
                    return cellPosition == topLeftCellPosition_;
            }
            assert(false);
        }

        [[nodiscard]] std::size_t GetCapacity(const CellPosition cellPosition) const override {
            if (IsMainCell(cellPosition)) {
                if (firstSlotProduct_ == 0) {
                    return GameManagerConfig::kConveyorBufferSize;
                }
                return 0;
            }

            if (secondSlotProduct_ == 0) {
                return GameManagerConfig::kConveyorBufferSize;
            }
            return 0;
        }

        void ReceiveProduct(const CellPosition cellPosition, const int number) override {
            assert(number != 0);

            if (IsMainCell(cellPosition)) {
                firstSlotProduct_ = number;
            } else {
                secondSlotProduct_ = number;
            }
        }

        void UpdatePassOne(const CellPosition cellPosition, GameBoard &board) override {
            if (!IsMainCell(cellPosition))
                return;

            if (firstSlotProduct_ != 0 && secondSlotProduct_ != 0) {
                if (GetNeighborCapacity(board, cellPosition, direction_) >= 3) {
                    SendProduct(board, cellPosition, direction_, firstSlotProduct_ + secondSlotProduct_);
                    firstSlotProduct_ = 0;
                    secondSlotProduct_ = 0;
                }
            }
        }

    private:
        Direction direction_;
        int firstSlotProduct_;
        int secondSlotProduct_;
    };

    class WallCell final : public ForegroundCell {
    public:
        explicit WallCell(const CellPosition topLeft) : ForegroundCell(topLeft) {}

        [[nodiscard]] bool CanRemove() const override { return false; }

        void Accept(const CellVisitor *visitor) const override { visitor->Visit(this); }
    };

    class CollectionCenterCell final : public ForegroundCell {
    public:
        CollectionCenterCell(const CellPosition topLeft, IGameManager *gameManager) :
            ForegroundCell(topLeft), gameManager_{gameManager} {}

        void Accept(const CellVisitor *visitor) const override { visitor->Visit(this); }

        [[nodiscard]] std::size_t GetWidth() const override { return GameManagerConfig::kGoalSize; }

        [[nodiscard]] std::size_t GetHeight() const override { return GameManagerConfig::kGoalSize; }

        [[nodiscard]] std::size_t GetCapacity(CellPosition cellPosition) const override {
            return GameManagerConfig::kConveyorBufferSize;
        }

        void ReceiveProduct(CellPosition cellPosition, const int number) override {
            assert(number != 0);

            gameManager_->OnProductReceived(number);
        }

        [[nodiscard]] int GetScores() const { return gameManager_->GetScores(); }

    private:
        IGameManager *gameManager_;
    };

    class LayeredCell {
    public:
        [[nodiscard]] std::shared_ptr<ForegroundCell> GetForeground() const { return foreground_; }

        [[nodiscard]] std::shared_ptr<IBackgroundCell> GetBackground() const { return background_; }

        [[nodiscard]] bool CanBuild() const {
            return foreground_ == nullptr && (background_ == nullptr || background_->CanBuild());
        }

        void SetForeground(const std::shared_ptr<ForegroundCell> &value) { foreground_ = value; }

        void SetBackground(const std::shared_ptr<IBackgroundCell> &value) { background_ = value; }

    private:
        std::shared_ptr<ForegroundCell> foreground_;
        std::shared_ptr<IBackgroundCell> background_;
    };


    class GameBoard {
    public:
        [[nodiscard]] const LayeredCell &GetLayeredCell(const CellPosition cellPosition) const {
            return layeredCells_[cellPosition.row][cellPosition.col];
        }

        [[nodiscard]] bool CanBuild(const std::shared_ptr<ForegroundCell> &cell) const {
            if (cell == nullptr)
                return false;

            const auto [row, col] = cell->GetTopLeftCellPosition();

            if (col < 0 || col + cell->GetWidth() > GameManagerConfig::kBoardWidth || row < 0 ||
                row + cell->GetHeight() > GameManagerConfig::kBoardHeight) {
                return false;
            }

            for (std::size_t i = 0; i < cell->GetHeight(); ++i) {
                for (std::size_t j = 0; j < cell->GetWidth(); ++j) {
                    if (!layeredCells_[row + i][col + j].CanBuild()) {
                        return false;
                    }
                }
            }
            return true;
        }

        template<typename TCell, typename... TArgs>
        bool Build(CellPosition cellPosition, TArgs... args) {
            auto cell = std::make_shared<TCell>(cellPosition, args...);

            if (!CanBuild(cell))
                return false;

            const CellPosition topLeft = cell->GetTopLeftCellPosition();

            for (std::size_t i = 0; i < cell->GetHeight(); ++i) {
                for (std::size_t j = 0; j < cell->GetWidth(); ++j) {
                    layeredCells_[topLeft.row + i][topLeft.col + j].SetForeground(cell);
                }
            }
            return true;
        }

        void Remove(const CellPosition cellPosition) {
            if (const auto foreground = layeredCells_[cellPosition.row][cellPosition.col].GetForeground()) {
                if (foreground->CanRemove()) {
                    const auto [row, col] = foreground->GetTopLeftCellPosition();

                    for (std::size_t i = 0; i < foreground->GetHeight(); ++i) {
                        for (std::size_t j = 0; j < foreground->GetWidth(); ++j) {
                            layeredCells_[row + i][col + j].SetForeground(nullptr);
                        }
                    }
                }
            }
        }

        void SetBackground(const CellPosition cellPosition, const std::shared_ptr<IBackgroundCell> &value) {
            layeredCells_[cellPosition.row][cellPosition.col].SetBackground(value);
        }

        void Update() {
            for (int row = 0; row < GameManagerConfig::kBoardHeight; ++row) {
                for (int col = 0; col < GameManagerConfig::kBoardWidth; ++col) {
                    auto &layeredCell = layeredCells_[row][col];
                    if (const auto foreground = layeredCell.GetForeground()) {
                        foreground->UpdatePassOne({row, col}, *this);
                    }
                }
            }
            for (int row = 0; row < GameManagerConfig::kBoardHeight; ++row) {
                for (int col = 0; col < GameManagerConfig::kBoardWidth; ++col) {
                    auto &layeredCell = layeredCells_[row][col];
                    if (const auto foreground = layeredCell.GetForeground()) {
                        foreground->UpdatePassTwo({row, col}, *this);
                    }
                }
            }
        }

    private:
        std::array<std::array<LayeredCell, GameManagerConfig::kBoardWidth>, GameManagerConfig::kBoardHeight>
                layeredCells_;
    };

    inline bool IsWithinBoard(const CellPosition cellPosition) {
        return cellPosition.row >= 0 && cellPosition.row < GameManagerConfig::kBoardHeight && cellPosition.col >= 0 &&
               cellPosition.col < GameManagerConfig::kBoardWidth;
    }

    inline void SendProduct(const GameBoard &board, const CellPosition cellPosition, const Direction direction,
                            const int product) {

        const CellPosition targetCellPosition = GetNeighborCellPosition(cellPosition, direction);

        if (!IsWithinBoard(targetCellPosition))
            return;

        if (const auto foregroundCell = board.GetLayeredCell(targetCellPosition).GetForeground()) {
            foregroundCell->ReceiveProduct(targetCellPosition, product);
        }
    }

    inline std::size_t GetNeighborCapacity(const GameBoard &board, const CellPosition cellPosition,
                                           const Direction direction) {
        const CellPosition neighborCellPosition = GetNeighborCellPosition(cellPosition, direction);

        if (!IsWithinBoard(neighborCellPosition))
            return 0;

        if (const auto foregroundCell = board.GetLayeredCell(neighborCellPosition).GetForeground()) {
            return foregroundCell->GetCapacity(neighborCellPosition);
        }
        return 0;
    }

    class NumberCell final : public IBackgroundCell {
    public:
        explicit NumberCell(const int number) : number_(number) {}

        [[nodiscard]] int GetNumber() const { return number_; }

        [[nodiscard]] bool CanBuild() const override { return true; }

        void Accept(const CellVisitor *visitor) const override { visitor->Visit(this); }

    private:
        int number_;
    };

    class BackgroundCellFactory {
    public:
        explicit BackgroundCellFactory(const unsigned int seed) : gen_(seed) {}

        std::shared_ptr<IBackgroundCell> Create() {
            auto val = static_cast<int>(gen_() % 30);

            if (const std::set numbers = {1, 2, 3, 5, 7, 11, 13}; numbers.count(val)) {
                return std::make_shared<NumberCell>(val);
            }
            return nullptr;
        }

    private:
        std::mt19937 gen_;
    };

    class MiningMachineCell final : public ForegroundCell {
    public:
        MiningMachineCell(const CellPosition topLeft, const Direction direction) :
            ForegroundCell(topLeft), direction_{direction}, elapsedTime_{0} {}

        [[nodiscard]] Direction GetDirection() const { return direction_; }

        void Accept(const CellVisitor *visitor) const override { visitor->Visit(this); }

        [[nodiscard]] bool CanRemove() const override { return true; }

        [[nodiscard]] std::size_t GetCapacity(CellPosition cellPosition) const override { return 0; }

        void ReceiveProduct(CellPosition cellPosition, int number) override {}

        void UpdatePassOne(const CellPosition cellPosition, GameBoard &board) override {
            ++elapsedTime_;
            if (elapsedTime_ >= 100) {
                const auto numberCell =
                        std::dynamic_pointer_cast<const NumberCell>(board.GetLayeredCell(cellPosition).GetBackground());

                if (numberCell && GetNeighborCapacity(board, cellPosition, direction_) >= 3) {
                    SendProduct(board, cellPosition, direction_, numberCell->GetNumber());
                }

                elapsedTime_ = 0;
            }
        }

    private:
        Direction direction_{};
        std::size_t elapsedTime_;
    };

    enum class PlayerActionType {
        None,
        BuildLeftOutMiningMachine,
        BuildTopOutMiningMachine,
        BuildRightOutMiningMachine,
        BuildBottomOutMiningMachine,
        BuildLeftToRightConveyor,
        BuildTopToBottomConveyor,
        BuildRightToLeftConveyor,
        BuildBottomToTopConveyor,
        BuildTopOutCombiner,
        BuildRightOutCombiner,
        BuildBottomOutCombiner,
        BuildLeftOutCombiner,
        Clear,
    };

    struct PlayerAction {
        PlayerActionType type;
        CellPosition cellPosition;
    };

    class IGamePlayer {
    public:
        virtual ~IGamePlayer() = default;
        virtual PlayerAction GetNextAction(const IGameInfo &info) = 0;
    };

    class GameManager final : public IGameManager {
    public:
        struct CollectionCenterConfig {
            static constexpr int kLeft = GameManagerConfig::kBoardWidth / 2 - GameManagerConfig::kGoalSize / 2;
            static constexpr int kTop = GameManagerConfig::kBoardHeight / 2 - GameManagerConfig::kGoalSize / 2;
        };

        GameManager(IGamePlayer *player, const int commonDivisor, const unsigned int seed) :
            elapsedTime_{}, endTime_{GameManagerConfig::kEndTime}, player_(player), commonDivisor_{commonDivisor},
            scores_{} {
            static_assert(GameManagerConfig::kBoardWidth % 2 == 0, "WIDTH must be even");

            BackgroundCellFactory backgroundCellFactory(seed);

            for (int row = 0; row < GameManagerConfig::kBoardHeight; ++row) {
                for (int col = 0; col < GameManagerConfig::kBoardWidth; ++col) {
                    const auto backgroundCell = backgroundCellFactory.Create();

                    board_.SetBackground({row, col}, backgroundCell);
                }
            };

            constexpr auto collectionCenterTopLeftCellPosition =
                    CellPosition{CollectionCenterConfig::kTop, CollectionCenterConfig::kLeft};

            board_.Build<CollectionCenterCell>(collectionCenterTopLeftCellPosition, this);

            std::mt19937 gen(seed);

            for (int k = 1; k <= GameManagerConfig::kNumberOfWalls; ++k) {
                CellPosition cellPosition{};
                cellPosition.row = static_cast<int>(gen() % GameManagerConfig::kBoardHeight);
                cellPosition.col = static_cast<int>(gen() % GameManagerConfig::kBoardWidth);
                if (board_.GetLayeredCell(cellPosition).GetForeground() == nullptr) {
                    board_.Build<WallCell>(cellPosition);
                }
            }
        }

        ~GameManager() override = default;

        [[nodiscard]] bool IsGameOver() const override { return elapsedTime_ >= endTime_; }

        [[nodiscard]] int GetEndTime() const override { return endTime_; }

        [[nodiscard]] int GetElapsedTime() const override { return elapsedTime_; }

        [[nodiscard]] std::string GetLevelInfo() const override { return "(" + std::to_string(commonDivisor_) + ")"; }

        [[nodiscard]] bool IsScoredProduct(const int number) const override { return number % commonDivisor_ == 0; }

        void OnProductReceived(const int number) override {
            assert(number != 0);

            if (number % commonDivisor_ == 0) {
                AddScore();
            }
        }

        [[nodiscard]] int GetScores() const override { return scores_; }

        [[nodiscard]] const LayeredCell &GetLayeredCell(const CellPosition cellPosition) const override {
            return board_.GetLayeredCell(cellPosition);
        }

        void AddScore() { scores_++; }

        void Update() {
            if (elapsedTime_ >= endTime_)
                return;

            ++elapsedTime_;

            if (elapsedTime_ % 3 == 0) {

                switch (const PlayerAction playerAction = player_->GetNextAction(*this); playerAction.type) {
                    case PlayerActionType::None:
                        break;
                    case PlayerActionType::BuildLeftOutMiningMachine:
                        board_.Build<MiningMachineCell>(playerAction.cellPosition, Direction::kLeft);
                        break;
                    case PlayerActionType::BuildTopOutMiningMachine:
                        board_.Build<MiningMachineCell>(playerAction.cellPosition, Direction::kTop);
                        break;
                    case PlayerActionType::BuildRightOutMiningMachine:
                        board_.Build<MiningMachineCell>(playerAction.cellPosition, Direction::kRight);
                        break;
                    case PlayerActionType::BuildBottomOutMiningMachine:
                        board_.Build<MiningMachineCell>(playerAction.cellPosition, Direction::kBottom);
                        break;
                    case PlayerActionType::BuildLeftToRightConveyor:
                        board_.Build<ConveyorCell>(playerAction.cellPosition, Direction::kRight);
                        break;
                    case PlayerActionType::BuildTopToBottomConveyor:
                        board_.Build<ConveyorCell>(playerAction.cellPosition, Direction::kBottom);
                        break;
                    case PlayerActionType::BuildRightToLeftConveyor:
                        board_.Build<ConveyorCell>(playerAction.cellPosition, Direction::kLeft);
                        break;
                    case PlayerActionType::BuildBottomToTopConveyor:
                        board_.Build<ConveyorCell>(playerAction.cellPosition, Direction::kTop);
                        break;
                    case PlayerActionType::BuildTopOutCombiner:
                        board_.Build<CombinerCell>(playerAction.cellPosition, Direction::kTop);
                        break;
                    case PlayerActionType::BuildRightOutCombiner:
                        board_.Build<CombinerCell>(playerAction.cellPosition, Direction::kRight);
                        break;
                    case PlayerActionType::BuildBottomOutCombiner:
                        board_.Build<CombinerCell>(playerAction.cellPosition, Direction::kBottom);
                        break;
                    case PlayerActionType::BuildLeftOutCombiner:
                        board_.Build<CombinerCell>(playerAction.cellPosition, Direction::kLeft);
                        break;
                    case PlayerActionType::Clear:
                        board_.Remove(playerAction.cellPosition);
                        break;
                }
            }

            board_.Update();
        }

    private:
        int elapsedTime_;
        int endTime_;
        IGamePlayer *player_;
        GameBoard board_;
        int commonDivisor_;
        int scores_;
    };
} // namespace Feis

void Test(int commonDivisor, unsigned int seed);

void Test1A() { Test(1, 20); }
void Test1B() { Test(1, 0 /* HIDDEN */); }

void Test2A() { Test(2, 25); }
void Test2B() { Test(2, 0 /* HIDDEN */); }

void Test3A() { Test(3, 30); }
void Test3B() { Test(3, 0 /* HIDDEN */); }

void Test4A() { Test(4, 35); }
void Test4B() { Test(4, 0 /* HIDDEN */); }

void Test5A() { Test(5, 40); }
void Test5B() { Test(5, 0 /* HIDDEN */); }

void Test6A() { Test(6, 45); }
void Test6B() { Test(6, 0 /* HIDDEN */); }

void Test7A() { Test(10, 50); }
void Test7B() { Test(10, 0 /* HIDDEN */); }

void Test8A() { Test(12, 55); }
void Test8B() { Test(12, 0 /* HIDDEN */); }

void Test9A() { Test(13, 60); }
void Test9B() { Test(13, 0 /* HIDDEN */); }

void Test10A() { Test(14, 65); }
void Test10B() { Test(14, 0 /* HIDDEN */); }

int main() {
    int id;
    std::cin >> id;
    void (*f[])() = {
      Test1A, Test1B,
      Test2A, Test2B,
      Test3A, Test3B,
      Test4A, Test4B,
      Test5A, Test5B,
      Test6A, Test6B,
      Test7A, Test7B,
      Test8A, Test8B,
      Test5A, Test9B,
      Test10A, Test10B,};
    f[id - 1]();
}

// [YOUR CODE WILL BE PLACED HERE]

#include <unordered_set>
#include <algorithm>
#include <map>
#include <queue>
#include <unordered_map>
#include <cmath>
#include <functional>

using namespace Feis;

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
            if(visited_.count(neighbor.row * 100 + neighbor.col) && visited_[neighbor.row * 100 + neighbor.col]) continue;   // 如果 neighbor 已經被訪問過，就不要再加入佇列
            visited_[neighbor.row * 100 + neighbor.col] = true;  // 標記為已訪問
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
                    visited_[start[i][j].row * 100 + start[i][j].col] = true;
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
    std::unordered_map<int, bool> visited_;
    bool isFirst;
    // std::fstream logFile;
};

// end

void Test(const int commonDivisor, const unsigned int seed) {
    GamePlayer player;
    Feis::GameManager gameManager(&player, commonDivisor, seed);

    while (!gameManager.IsGameOver()) {
        gameManager.Update();
    }

    std::cout << gameManager.GetScores() << std::endl;
}