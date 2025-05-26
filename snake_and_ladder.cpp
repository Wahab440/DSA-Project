#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <map>
#include <vector>
#include <queue>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <limits.h>
#include <cmath>

using namespace std;
using namespace sf;

const float CELL_SIZE = 60.0f;

class SnakeAndLadder {
private:
    static const int BOARD_SIZE = 100;
    static const int GRID_SIZE = 10;

    map<int, int> snakes;
    map<int, int> ladders;
    vector<int> players;
    queue<int> playerQueue;
    int numPlayers;
    RenderWindow window;
    vector<Color> playerColors;
    Font font;
    bool gameOver = false;
    int winner = -1;

    Texture snakeTexture;
    Texture ladderTexture;
    SoundBuffer moveBuffer;
    Sound moveSound;

public:
    SnakeAndLadder(int numPlayers) : numPlayers(numPlayers),
        window(VideoMode(GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE), "Snake and Ladder") {
        initializeBoard();
        initializePlayers();
        loadAssets();
        srand(static_cast<unsigned>(time(0)));
    }

    void run() {
        while (window.isOpen()) {
            Event event;
            while (window.pollEvent(event)) {
                if (event.type == Event::Closed)
                    window.close();
                if (!gameOver && event.type == Event::KeyPressed && event.key.code == Keyboard::Space) {
                    playTurn();
                }
                if (gameOver && event.type == Event::KeyPressed && event.key.code == Keyboard::Enter) {
                    window.close();
                }
            }
            render();
        }
    }

private:
    void initializeBoard() {
        snakes = {{16, 6}, {47, 26}, {49, 11}, {56, 53}, {62, 19},
                  {64, 60}, {87, 24}, {93, 73}, {95, 75}, {98, 78}};
        ladders = {{1, 38}, {4, 14}, {9, 31}, {21, 42}, {28, 84},
                   {36, 44}, {51, 67}, {71, 91}, {80, 100}};
    }

    void initializePlayers() {
        playerColors = {Color::Blue, Color::Green, Color::Yellow, Color::Magenta};
        for (int i = 0; i < numPlayers; i++) {
            players.push_back(1);
            playerQueue.push(i);
        }
    }

    void loadAssets() {
        // Show working directory for debugging
        char cwd[PATH_MAX];
        getcwd(cwd, sizeof(cwd));
        cout << "Current working dir: " << cwd << endl;

        // Load font
        if (!font.loadFromFile("assets/DejaVuSans.ttf")) {
            cerr << "Failed to load font! Check assets/DejaVuSans.ttf\n";
        }
        // Load images
        if (!snakeTexture.loadFromFile("assets/snake.png")) {
            cerr << "Failed to load assets/snake.png!\n";
        }
        if (!ladderTexture.loadFromFile("assets/ladder.png")) {
            cerr << "Failed to load assets/ladder.png!\n";
        }
        // Load sound
        if (!moveBuffer.loadFromFile("assets/move.wav")) {
            cerr << "Failed to load assets/move.wav!\n";
        }
        moveSound.setBuffer(moveBuffer);
    }

    void playTurn() {
        int current = playerQueue.front();
        playerQueue.pop();
        int dice = (rand() % 6) + 1;
        int newPos = players[current] + dice;
        if (newPos <= 100) {
            players[current] = newPos;
            if (snakes.count(newPos)) players[current] = snakes[newPos];
            else if (ladders.count(newPos)) players[current] = ladders[newPos];
        }
        moveSound.play();
        if (players[current] == 100) {
            winner = current;
            gameOver = true;
        } else {
            playerQueue.push(current);
        }
    }

    pair<float, float> getCellCoordinates(int pos) {
        int row = (pos - 1) / GRID_SIZE;
        int col = (pos - 1) % GRID_SIZE;
        if (row % 2 == 1) col = GRID_SIZE - 1 - col;
        row = GRID_SIZE - 1 - row;
        return {col * CELL_SIZE + CELL_SIZE / 2, row * CELL_SIZE + CELL_SIZE / 2};
    }

    void render() {
        window.clear(Color::White);

        // Draw checkerboard
        for (int i = 0; i < GRID_SIZE; ++i) {
            for (int j = 0; j < GRID_SIZE; ++j) {
                RectangleShape cell(Vector2f(CELL_SIZE, CELL_SIZE));
                cell.setPosition(j * CELL_SIZE, i * CELL_SIZE);
                cell.setFillColor(((i + j) % 2 == 0) ? Color(230, 170, 120) : Color(200, 110, 70));
                window.draw(cell);
            }
        }

        // Draw snakes
        for (auto& s : snakes) drawConnection(s.first, s.second, snakeTexture);
        // Draw ladders
        for (auto& l : ladders) drawConnection(l.first, l.second, ladderTexture);

        // Draw players
        for (int i = 0; i < numPlayers; i++) {
            auto [x, y] = getCellCoordinates(players[i]);
            CircleShape token(15);
            token.setFillColor(playerColors[i]);
            token.setPosition(x - 15, y - 15);
            window.draw(token);
        }

        // Draw winner or turn text
        Text text;
        text.setFont(font);
        text.setCharacterSize(20);
        text.setPosition(10, 10);
        if (gameOver) {
            text.setString("Player " + to_string(winner + 1) + " wins! Press Enter.");
            text.setFillColor(Color::Black);
        } else {
            int turn = playerQueue.front();
            text.setString("Player " + to_string(turn + 1) + "'s turn. Press Space.");
            text.setFillColor(Color::Black);
        }
        window.draw(text);

        window.display();
    }

    void drawConnection(int from, int to, Texture& texture) {
        auto [x1, y1] = getCellCoordinates(from);
        auto [x2, y2] = getCellCoordinates(to);
        Sprite sprite(texture);
        float dx = x2 - x1, dy = y2 - y1;
        float len = sqrt(dx * dx + dy * dy);
        if (texture.getSize().x > 0 && texture.getSize().y > 0) {
            sprite.setScale(CELL_SIZE / texture.getSize().x, len / texture.getSize().y);
            sprite.setPosition(x1 - CELL_SIZE / 2, y1 - CELL_SIZE / 2);
            sprite.setRotation(atan2(dy, dx) * 180.f / 3.14159f);
            window.draw(sprite);
        }
    }
};

int main() {
    int players = 2; // Or 3 or 4
    SnakeAndLadder game(players);
    game.run();
    return 0;
}
