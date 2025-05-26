#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <map>
#include <vector>
#include <queue>
#include <cstdlib>
#include <ctime>
#include <cmath>
using namespace std;
using namespace sf;

const float CELL_SIZE = 60.0f;
const int GRID_SIZE = 10;
const int BOARD_SIZE = 100;
const int WINDOW_SIZE = CELL_SIZE * GRID_SIZE;

vector<Color> PLAYER_COLORS = {Color::Blue, Color::Red, Color::Green, Color::Magenta};

class PlayerSelector
{
public:
    int selectPlayers(RenderWindow &window, Font &font)
    {
        int selected = -1;
        while (window.isOpen() && selected == -1)
        {
            Event event;
            while (window.pollEvent(event))
            {
                if (event.type == Event::Closed)
                    window.close();
                if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left)
                {
                    Vector2i pos = Mouse::getPosition(window);
                    for (int i = 0; i < 3; ++i)
                    {
                        FloatRect box(150 + i * 120, 250, 100, 120);
                        if (box.contains(pos.x, pos.y))
                            selected = 2 + i;
                    }
                }
            }
            window.clear(Color(135, 206, 250));

            Text title("<<Welcome to Snake And Ladder>>", font, 28);
            title.setFillColor(Color(40, 30, 60));
            title.setStyle(Text::Bold);
            FloatRect titleRect = title.getLocalBounds();
            title.setOrigin(titleRect.left + titleRect.width / 2.0f, 0);
            title.setPosition(WINDOW_SIZE / 2, 38);
            window.draw(title);

            Text heading("Select Number of Players", font, 32);
            heading.setFillColor(Color(40, 30, 60));
            heading.setPosition(WINDOW_SIZE / 2 - 180, 100);
            window.draw(heading);

            for (int i = 0; i < 3; ++i)
            {
                RectangleShape btn(Vector2f(100, 120));
                btn.setPosition(150 + i * 120, 250);
                btn.setFillColor(Color(245, 245, 245));
                btn.setOutlineThickness(4);
                btn.setOutlineColor(Color(100, 100, 100));
                window.draw(btn);

                Text num(to_string(2 + i), font, 36);
                num.setFillColor(Color(40, 30, 60));
                num.setStyle(Text::Bold);

                FloatRect numRect = num.getLocalBounds();
                num.setOrigin(numRect.left + numRect.width / 2.0f, 0);
                num.setPosition(150 + i * 120 + 50, 260);
                window.draw(num);

                int playerCount = 2 + i;
                float startY = 290;
                float gap = 30;
                float x = 150 + i * 120 + 50;
                for (int j = 0; j < playerCount; ++j)
                {
                    CircleShape circle(15);
                    circle.setFillColor(PLAYER_COLORS[j]);
                    circle.setOutlineThickness(2);
                    circle.setOutlineColor(Color(70, 70, 70));
                    circle.setOrigin(15, 15);
                    circle.setPosition(x, startY + j * gap);
                    window.draw(circle);
                }
            }

            Text clickNote("Click a box to continue", font, 22);
            clickNote.setFillColor(Color(80, 80, 80));
            clickNote.setPosition(WINDOW_SIZE / 2 - 120, WINDOW_SIZE - 60);
            window.draw(clickNote);

            window.display();
        }
        return selected;
    }
};

class SnakeAndLadder
{
private:
    map<int, int> snakes;
    map<int, int> ladders;
    vector<int> players;
    queue<int> playerQueue;
    int numPlayers;
    RenderWindow &window;
    vector<Color> playerColors;
    Font &font;
    bool gameOver = false;
    int winner = -1, lastDice = 1;

    SoundBuffer moveBuffer;
    Sound moveSound;
    bool soundLoaded = false;

public:
    SnakeAndLadder(int numPlayers, RenderWindow &window, Font &font)
        : numPlayers(numPlayers), window(window), font(font)
    {
        initializeBoard();
        initializePlayers();
        loadAssets();
        srand(static_cast<unsigned>(time(0)));
    }
    void run()
    {
        while (window.isOpen())
        {
            Event event;
            while (window.pollEvent(event))
            {
                if (event.type == Event::Closed)
                    window.close();
                if (!gameOver && event.type == Event::KeyPressed && event.key.code == Keyboard::Space)
                {
                    playTurn();
                }
                if (gameOver && event.type == Event::KeyPressed && event.key.code == Keyboard::Enter)
                {
                    window.close();
                }
            }
            render();
        }
    }

private:
    void initializeBoard()
    {
        snakes = {{16, 6}, {47, 26}, {49, 11}, {56, 53}, {62, 19}, {64, 60}, {87, 24}, {93, 73}, {95, 75}, {98, 78}};
        ladders = {{2, 38}, {4, 14}, {9, 31}, {21, 42}, {28, 84}, {36, 44}, {51, 67}, {71, 91}, {80, 100}};
    }
    void initializePlayers()
    {
        playerColors = PLAYER_COLORS;
        players.assign(numPlayers, 1);
        for (int i = 0; i < numPlayers; i++)
            playerQueue.push(i);
    }
    void loadAssets()
    {
        string assetDir = "./assets/";
        soundLoaded = moveBuffer.loadFromFile(assetDir + "move.wav");
        if (soundLoaded)
            moveSound.setBuffer(moveBuffer);
    }

    void playTurn()
    {
        int current = playerQueue.front();
        playerQueue.pop();
        int dice = (rand() % 6) + 1;
        lastDice = dice;
        int oldPos = players[current];
        int newPos = oldPos + dice;
        std::cout << "Player " << (current + 1) << " rolled a " << dice << " and moved from " << oldPos;
        if (newPos <= 100)
        {
            players[current] = newPos;
            std::cout << " to " << newPos;
            if (snakes.count(newPos))
            {
                players[current] = snakes[newPos];
                std::cout << " and was beaten by a snake to " << players[current] << "!\n";
            }
            else if (ladders.count(newPos))
            {
                players[current] = ladders[newPos];
                std::cout << " and climbed a ladder to " << players[current] << "!\n";
            }
            else
            {
                std::cout << ".\n";
            }
        }
        else
        {
            std::cout << " but cannot move (must land exactly on 100).\n";
        }

        if (soundLoaded)
        {
            moveSound.play();
        }
        if (players[current] == 100)
        {
            winner = current;
            gameOver = true;
            std::cout << "Player " << (winner + 1) << " wins the game!\n";
        }
        else
        {
            playerQueue.push(current);
        }
    }

    // Offset tokens to avoid complete overlap when on same cell
    std::pair<float, float> getCellCoordinates(int pos, int playerIdx = 0, int nOnCell = 1, int tokenOrder = 0)
    {
        // Convert linear cell number to grid position
        int row = (pos - 1) / GRID_SIZE;
        int col = (pos - 1) % GRID_SIZE;

        // Snake pattern: reverse direction every other row
        if (row % 2 == 1)
        {
            col = GRID_SIZE - 1 - col;
        }

        // Flip vertically so row 0 is bottom of board
        row = GRID_SIZE - 1 - row;

        // Compute center coordinates of the cell
        float x = col * CELL_SIZE + CELL_SIZE / 2.0f;
        float y = row * CELL_SIZE + CELL_SIZE / 2.0f;

        // Offset for visual spacing when multiple tokens on one cell
        if (nOnCell > 1)
        {
            float angle = 2 * 3.14159f * tokenOrder / nOnCell;
            float radius = 12.0f; // Distance from center for token separation
            x += radius * std::cos(angle);
            y += radius * std::sin(angle);
        }

        return {x, y};
    }

    void render()
    {
        window.clear(Color(255, 255, 240)); // Ivory
        // Draw checkerboard, blue/white
        for (int i = 0; i < GRID_SIZE; ++i)
        {
            for (int j = 0; j < GRID_SIZE; ++j)
            {
                RectangleShape cell(Vector2f(CELL_SIZE, CELL_SIZE));
                cell.setPosition(j * CELL_SIZE, i * CELL_SIZE);
                cell.setFillColor(((i + j) % 2 == 0) ? Color(145, 197, 255) : Color::White); // light blue and white
                window.draw(cell);
            }
        }
        // Draw ladders as orange lines with rungs
        for (auto &l : ladders)
            drawConnection(l.first, l.second, true);
        // Draw snakes as green wavy lines
        for (auto &s : snakes)
            drawConnection(s.first, s.second, false);
        // Compute number of tokens at each cell
        map<int, vector<int>> cellTokens;
        for (int i = 0; i < numPlayers; ++i)
            cellTokens[players[i]].push_back(i);
        // Draw players (with offsets if >1 at a cell)
        for (auto &[pos, indices] : cellTokens)
        {
            for (size_t k = 0; k < indices.size(); ++k)
            {
                int i = indices[k];
                auto [x, y] = getCellCoordinates(players[i], i, indices.size(), k);
                CircleShape token(15);
                token.setFillColor(playerColors[i]);
                token.setOutlineThickness(2);
                token.setOutlineColor(Color::Black);
                token.setPosition(x - 15, y - 15);
                window.draw(token);
            }
        }

        float infoHeight = 46;
        RectangleShape infoBox(Vector2f(WINDOW_SIZE * 0.8f, infoHeight));
        infoBox.setFillColor(Color(30, 30, 30, 230));
        infoBox.setPosition(WINDOW_SIZE * 0.1f, 8);
        window.draw(infoBox);

        if (!gameOver)
        {
            int turn = playerQueue.front();
            RectangleShape turnBox(Vector2f(32, 32));
            turnBox.setPosition(WINDOW_SIZE * 0.12f, 14);
            turnBox.setFillColor(playerColors[turn]);
            turnBox.setOutlineThickness(2);
            turnBox.setOutlineColor(Color::White);
            window.draw(turnBox);

            Text turnText("Player " + to_string(turn + 1) + "'s turn. Dice: " + to_string(lastDice) + "  (Press SPACE)", font, 24);
            turnText.setFillColor(Color::White);
            turnText.setStyle(Text::Bold);
            turnText.setPosition(WINDOW_SIZE * 0.12f + 40, 18);
            window.draw(turnText);
        }

        else
        {
            RectangleShape overlay(Vector2f(GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE));
            overlay.setFillColor(Color(0, 0, 0, 180));
            window.draw(overlay);
            Text winText("Player " + to_string(winner + 1) + " Wins!", font, 44);
            winText.setFillColor(playerColors[winner]);
            winText.setOutlineThickness(4);
            winText.setOutlineColor(Color::White);
            winText.setPosition((GRID_SIZE * CELL_SIZE) / 2 - 180, (GRID_SIZE * CELL_SIZE) / 2 - 40);
            window.draw(winText);
            Text pressEnter("Press ENTER to exit...", font, 28);
            pressEnter.setFillColor(Color::Yellow);
            pressEnter.setPosition((GRID_SIZE * CELL_SIZE) / 2 - 130, (GRID_SIZE * CELL_SIZE) / 2 + 30);
            window.draw(pressEnter);
        }
        window.display();
    }
    void drawConnection(int from, int to, bool isLadder)
    {
        auto [x1, y1] = getCellCoordinates(from);
        auto [x2, y2] = getCellCoordinates(to);

        float dx = x2 - x1;
        float dy = y2 - y1;
        float len = std::sqrt(dx * dx + dy * dy);
        float nx = -dy / len;
        float ny = dx / len;

        if (!isLadder)
        {
            const int points = 80;
            sf::VertexArray snake(sf::TriangleStrip, points * 2);

            for (int i = 0; i < points; ++i)
            {
                float t = i / static_cast<float>(points - 1);
                float px = x1 * (1 - t) + x2 * t;
                float py = y1 * (1 - t) + y2 * t;
                float wave = 10 * std::sin(t * 6.2831f * 3 + from + to);

                float ox = wave * nx;
                float oy = wave * ny;

                float thickness = 5;

                snake[2 * i] = sf::Vertex(sf::Vector2f(px + ox + nx * thickness, py + oy + ny * thickness), sf::Color(10, 180, 10));
                snake[2 * i + 1] = sf::Vertex(sf::Vector2f(px + ox - nx * thickness, py + oy - ny * thickness), sf::Color(10, 150, 10));
            }

            window.draw(snake);
        }
        else
        {
            float railThickness = 6.0f;
            sf::Vector2f dir = sf::Vector2f(x2 - x1, y2 - y1);
            float angle = std::atan2(dir.y, dir.x) * 180 / 3.14159f;
            float railLength = len;

            for (int side = -1; side <= 1; side += 2)
            {
                sf::RectangleShape rail(sf::Vector2f(railLength, railThickness));
                rail.setFillColor(sf::Color(160, 82, 45)); 
                rail.setOrigin(0, railThickness / 2.0f);
                rail.setRotation(angle);
                rail.setPosition(x1 + side * nx * 10, y1 + side * ny * 10);
                window.draw(rail);
            }

            int numRungs = 6;
            float rungThickness = 3.0f;
            for (int r = 1; r < numRungs; ++r)
            {
                float t = r / static_cast<float>(numRungs);
                float px = x1 * (1 - t) + x2 * t;
                float py = y1 * (1 - t) + y2 * t;

                float rungLen = CELL_SIZE * 0.4f;
                sf::RectangleShape rung(sf::Vector2f(rungLen * 2, rungThickness));
                rung.setFillColor(sf::Color(210, 140, 60)); 
                rung.setOrigin(rungLen, rungThickness / 2.0f);
                rung.setRotation(angle);
                rung.setPosition(px, py);
                window.draw(rung);
            }
        }
    }
};

int main()
{
    RenderWindow window(VideoMode(WINDOW_SIZE, WINDOW_SIZE), "Snake and Ladder");
    Font font;
    if (!font.loadFromFile("./assets/DejaVuSans.ttf"))
    {
        std::cerr << "Failed to load font! Make sure DejaVuSans.ttf is in assets folder.\n";
        return 1;
    }
    PlayerSelector selector;
    int numPlayers = selector.selectPlayers(window, font);
    if (numPlayers < 2 || numPlayers > 4)
        return 0;
    SnakeAndLadder game(numPlayers, window, font);
    game.run();
    return 0;
}