#ifndef DRAWER_HPP
#define DRAWER_HPP
#include <iostream>
#include <SFML/Graphics.hpp>
#include "PDOGS.hpp"

template<typename TGameRendererConfig>
class Drawer {
public:
    using CellPosition = Feis::CellPosition;
    using Direction = Feis::Direction;

    explicit Drawer(sf::RenderWindow *window) : window_(window) {
        if (!font_.openFromFile("arial.ttf")) {
            std::cout << "Error loading font" << std::endl;
        }
    }

    void Clear() const { window_->clear(sf::Color::Black); }

    void Display() const { window_->display(); }

    void DrawBorder(const CellPosition cellPosition) {
        const sf::Vector2f topLeft = GetBorderTopLeft() + sf::Vector2f(static_cast<float>(cellPosition.col),
                                                                       static_cast<float>(cellPosition.row)) *
                                                                  static_cast<float>(TGameRendererConfig::kCellSize);

        sf::RectangleShape rectangle(
                sf::Vector2f(TGameRendererConfig::kCellSize - 2 * TGameRendererConfig::kBorderSize,
                             TGameRendererConfig::kCellSize - 2 * TGameRendererConfig::kBorderSize));

        rectangle.setOutlineColor(sf::Color(60, 60, 60));
        rectangle.setOutlineThickness(TGameRendererConfig::kBorderSize);
        rectangle.setFillColor(sf::Color::Black);
        rectangle.setPosition(topLeft +
                              sf::Vector2f(TGameRendererConfig::kBorderSize, TGameRendererConfig::kBorderSize));
        window_->draw(rectangle);
    }

    void DrawText(const std::string &str, const unsigned int characterSize, const sf::Color color,
                  const sf::Vector2f position, Direction direction = Direction::kTop) const {
        sf::Text text(font_, str, characterSize);

        text.setFillColor(color);

        const sf::FloatRect rect = text.getLocalBounds();
        text.setOrigin(sf::Vector2f(rect.position.x + rect.size.x / 2.0f, rect.position.y + rect.size.y / 2.0f));
        text.setPosition(position);
        text.setRotation(sf::degrees(static_cast<float>(static_cast<int>(direction)) * 90.f));
        window_->draw(text);
    }

    void DrawText(const std::string &str, const unsigned int characterSize, const sf::Color color,
                  const CellPosition cellPosition, const Direction direction = Direction::kTop) {
        DrawText(str, characterSize, color, GetCellCenter(cellPosition), direction);
    }

    void DrawRectangle(const CellPosition cellPosition, const sf::Color color) {
        sf::RectangleShape rectangle(sf::Vector2f(TGameRendererConfig::kCellSize, TGameRendererConfig::kCellSize));
        rectangle.setOrigin(sf::Vector2f(rectangle.getLocalBounds().size.x / 2, rectangle.getLocalBounds().size.y / 2));
        rectangle.setFillColor(color);
        rectangle.setPosition(GetCellCenter(cellPosition));
        DrawShape(rectangle);
    }

    void DrawTriangle(const sf::Vector2f center, Direction direction, const sf::Color color) {
        sf::ConvexShape triangle;
        triangle.setPointCount(3);
        triangle.setPoint(0, sf::Vector2f(0, 0));
        triangle.setPoint(1, sf::Vector2f(TGameRendererConfig::kCellSize, 0));
        triangle.setPoint(2, sf::Vector2f(TGameRendererConfig::kCellSize, TGameRendererConfig::kCellSize));
        triangle.setOrigin(sf::Vector2f(triangle.getLocalBounds().size.x / 2, triangle.getLocalBounds().size.y / 2));
        triangle.rotate(sf::degrees(static_cast<float>(static_cast<int>(direction) + 1)) * 90.f);
        triangle.setFillColor(color);
        triangle.setPosition(center);
        DrawShape(triangle);
    }

    void DrawTriangle(const CellPosition cellPosition, const Direction direction, const sf::Color color) {
        DrawTriangle(GetCellCenter(cellPosition), direction, color);
    }

    void DrawCircle(const sf::Vector2f center, const float radius, const sf::Color color) const {
        sf::CircleShape circle(radius);
        circle.setOrigin(sf::Vector2f(circle.getLocalBounds().size.x / 2, circle.getLocalBounds().size.y / 2));
        circle.setFillColor(color);
        circle.setPosition(center);
        circle.setOutlineColor(sf::Color(60, 60, 60));
        circle.setOutlineThickness(2);
        DrawShape(circle);
    }

    void DrawArrow(const CellPosition cellPosition, Feis::Direction direction) {
        int offset = 2;
        sf::ConvexShape arrow(6);
        arrow.setPoint(0, sf::Vector2f(0, 0));
        arrow.setPoint(1, sf::Vector2f(-2 * offset, offset - TGameRendererConfig::kCellSize / 2));
        arrow.setPoint(2, sf::Vector2f(0, offset - TGameRendererConfig::kCellSize / 2));
        arrow.setPoint(3, sf::Vector2f(2.f * static_cast<float>(offset), 0.f));
        arrow.setPoint(4, sf::Vector2f(0, TGameRendererConfig::kCellSize / 2 - offset));
        arrow.setPoint(5, sf::Vector2f(-2 * offset, TGameRendererConfig::kCellSize / 2 - offset));
        arrow.rotate(sf::degrees(static_cast<float>(static_cast<int>(direction) + 3) * 90.f));
        arrow.setFillColor(sf::Color(60, 60, 60));
        arrow.setPosition(GetCellCenter(cellPosition));
        DrawShape(arrow);
    }

    void DrawShape(const sf::Shape &s) const { window_->draw(s); }

    sf::Vector2f GetCellCenter(const CellPosition cellPosition) {
        return GetCellTopLeft(cellPosition) +
               sf::Vector2f(TGameRendererConfig::kCellSize / 2, TGameRendererConfig::kCellSize / 2);
    }
    sf::Vector2f GetCellTopLeft(const CellPosition cellPosition) {
        return GetBorderTopLeft() +
               sf::Vector2f(static_cast<float>(cellPosition.col), static_cast<float>(cellPosition.row)) *
                       static_cast<float>(TGameRendererConfig::kCellSize);
    }
    sf::Vector2f GetBorderTopLeft() {
        return sf::Vector2f(TGameRendererConfig::kBoardLeft, TGameRendererConfig::kBoardTop);
    }

private:
    sf::RenderWindow *window_;
    sf::Font font_;
};
#endif
