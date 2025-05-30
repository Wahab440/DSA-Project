#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <map>
#include <queue>
#include <fstream>
#include <ctime>
#include <cmath>
#include <algorithm>
using namespace sf;
using namespace std;

const int NUM_CELLS = 100;
const int BOARD_DIM = 10;
const float TILE_SIZE = 60.f;
const int WIN_SIZE = TILE_SIZE * BOARD_DIM;
const vector<Color> TOKEN_COLORS = {Color::Blue, Color::Red, Color::Green, Color::Yellow};

int rollDice()
{
    return rand() % 6 + 1;
}

int choosePlayers(RenderWindow &win, Font &font)
{
    int choice = -1;
    while (win.isOpen() && choice == -1)
    {
        Event evt;
        while (win.pollEvent(evt))
        {
            if (evt.type == Event::Closed)
            {
                win.close();
            }
            if (evt.type == Event::MouseButtonPressed && evt.mouseButton.button == Mouse::Left)
            {
                Vector2i m = Mouse::getPosition(win);
                for (int idx = 0; idx < 3; ++idx)
                {
                    FloatRect rect(160 + idx * 140, 210, 110, 120);
                    if (rect.contains(m.x, m.y))
                    {
                        choice = 2 + idx;
                    }
                }
            }
        }
        win.clear(Color(120, 196, 240));
        Text msg("Snake & Ladder", font, 34);
        msg.setFillColor(Color(25, 20, 40));
        msg.setStyle(Text::Bold | Text::Underlined);
        msg.setPosition(WIN_SIZE / 2 - 140, 30);
        win.draw(msg);

        Text opt("Select Number of Players", font, 28);
        opt.setPosition(WIN_SIZE / 2 - 190, 100);
        win.draw(opt);

        for (int idx = 0; idx < 3; ++idx)
        {
            RectangleShape box(Vector2f(110, 120));
            box.setPosition(160 + idx * 140, 210);
            box.setFillColor(Color(250, 250, 250));
            box.setOutlineThickness(3);
            box.setOutlineColor(Color(90, 90, 90));
            win.draw(box);

            Text pNum(to_string(2 + idx), font, 36);
            pNum.setPosition(160 + idx * 140 + 35, 220);
            win.draw(pNum);

            for (int t = 0; t < 2 + idx; ++t)
            {
                CircleShape token(13);
                token.setFillColor(TOKEN_COLORS[t]);
                token.setOutlineThickness(2);
                token.setOutlineColor(Color(60, 60, 60));
                token.setOrigin(13, 13);
                token.setPosition(160 + idx * 140 + 55, 260 + t * 28);
                win.draw(token);
            }
        }
        Text note("Click a box to continue", font, 18);
        note.setPosition(WIN_SIZE / 2 - 95, WIN_SIZE - 50);
        win.draw(note);
        win.display();
    }
    return choice;
}

class SnakeLadderGame
{
    map<int, int> snakePos, ladderPos;
    vector<int> tokens;
    queue<int> order;
    int activePlayers;
    RenderWindow &win;
    vector<Color> tColors;
    Font &gameFont;
    bool gameEnded = false;
    int champ = -1, diceVal = 1;
    SoundBuffer buf;
    Sound moveFx;
    bool sfxReady = false;

public:
    SnakeLadderGame(int n, RenderWindow &w, Font &f)
        : activePlayers(n), win(w), gameFont(f)
    {
        srand(time(0));
        loadBoard();
        setupTokens();
        prepSound();
    }

    void start()
    {
        string fileOut = "results.txt";
        while (win.isOpen())
        {
            Event evt;
            while (win.pollEvent(evt))
            {
                if (evt.type == Event::Closed)
                {
                    win.close();
                }
                if (!gameEnded && evt.type == Event::KeyPressed && evt.key.code == Keyboard::Space)
                {
                    doTurn();
                }
                if (!gameEnded && evt.type == Event::KeyPressed && evt.key.code == Keyboard::P)
                {
                    showBFSPath();
                }
                if (!gameEnded && evt.type == Event::KeyPressed && evt.key.code == Keyboard::D)
                {
                    showDijkstraPath();
                }
                if (gameEnded && evt.type == Event::KeyPressed && evt.key.code == Keyboard::Enter)
                {
                    saveResults(fileOut);
                    displayResults(fileOut);
                    win.close();
                }
            }
            render();
        }
    }

private:
    void loadBoard()
    {
        snakePos = { {16, 4}, {47, 26}, {49, 11}, {62, 19}, {64, 60},
                     {87, 24}, {93, 73}, {95, 75}, {98, 78} };
        ladderPos = { {3, 38}, {6, 14}, {9, 31}, {21, 42}, {28, 84},
                      {36, 44}, {51, 67}, {71, 91}, {80, 100} };
    }

    void setupTokens()
    {
        tColors = TOKEN_COLORS;
        tokens.assign(activePlayers, 1);
        for (int idx = 0; idx < activePlayers; ++idx)
        {
            order.push(idx);
        }
    }

    void prepSound()
    {
        string folder = "./assets/";
        sfxReady = buf.loadFromFile(folder + "move.wav");
        if (sfxReady)
        {
            moveFx.setBuffer(buf);
        }
    }

    pair<float, float> locateCell(int cell, int idx = 0, int nT = 1, int pos = 0)
    {
        int r = (cell - 1) / BOARD_DIM, c = (cell - 1) % BOARD_DIM;
        if (r % 2 == 1)
        {
            c = BOARD_DIM - 1 - c;
        }
        r = BOARD_DIM - 1 - r;
        float cx = c * TILE_SIZE + TILE_SIZE / 2;
        float cy = r * TILE_SIZE + TILE_SIZE / 2;
        if (nT > 1)
        {
            float angle = 2 * M_PI * pos / nT;
            cx += 12 * cos(angle);
            cy += 12 * sin(angle);
        }
        return { cx, cy };
    }

    void doTurn()
    {
        int who = order.front();
        order.pop();
        int roll = rollDice();
        diceVal = roll;
        int prior = tokens[who];
        int next = prior + roll;
        cout << "Player " << (who + 1) << " rolled " << roll << " from " << prior;
        if (next <= 100)
        {
            tokens[who] = next;
            cout << " to " << next;
            if (snakePos.count(next))
            {
                tokens[who] = snakePos[next];
                cout << ", bitten to " << tokens[who];
            }
            else if (ladderPos.count(next))
            {
                tokens[who] = ladderPos[next];
                cout << ", climbed to " << tokens[who];
            }
            cout << endl;
        }
        else
        {
            cout << " but must land exactly on 100.\n";
        }
        if (sfxReady)
        {
            moveFx.play();
        }
        if (tokens[who] == 100)
        {
            champ = who;
            gameEnded = true;
            cout << "Player " << (champ + 1) << " wins!\n";
        }
        else
        {
            order.push(who);
        }
    }

    void render()
    {
        win.clear(Color(255, 255, 240));
        for (int i = 0; i < BOARD_DIM; ++i)
        {
            for (int j = 0; j < BOARD_DIM; ++j)
            {
                RectangleShape tile(Vector2f(TILE_SIZE, TILE_SIZE));
                tile.setPosition(j * TILE_SIZE, i * TILE_SIZE);
                tile.setFillColor(((i + j) % 2) ? Color::White : Color(120, 180, 255));
                win.draw(tile);
            }
        }
        for (auto &lad : ladderPos)
        {
            drawConnector(lad.first, lad.second, true);
        }
        for (auto &snk : snakePos)
        {
            drawConnector(snk.first, snk.second, false);
        }
        map<int, vector<int>> onCell;
        for (int idx = 0; idx < activePlayers; ++idx)
        {
            onCell[tokens[idx]].push_back(idx);
        }
        for (auto &[cell, ids] : onCell)
        {
            for (size_t k = 0; k < ids.size(); ++k)
            {
                int idx = ids[k];
                auto [x, y] = locateCell(tokens[idx], idx, ids.size(), k);
                CircleShape tok(15);
                tok.setFillColor(tColors[idx]);
                tok.setOutlineThickness(2);
                tok.setOutlineColor(Color::Black);
                tok.setPosition(x - 15, y - 15);
                win.draw(tok);
            }
        }
        float infoH = 36;
        RectangleShape bar(Vector2f(WIN_SIZE * 0.82f, infoH));
        bar.setFillColor(Color(30, 30, 30, 230));
        bar.setPosition(WIN_SIZE * 0.09f, 6);
        win.draw(bar);
        if (!gameEnded)
        {
            int turn = order.front();
            RectangleShape marker(Vector2f(24, 24));
            marker.setPosition(WIN_SIZE * 0.10f, 13);
            marker.setFillColor(tColors[turn]);
            marker.setOutlineThickness(2);
            marker.setOutlineColor(Color::White);
            win.draw(marker);
            Text t1("Player " + to_string(turn + 1) + "'s turn. Dice: " + to_string(diceVal) + " (SPACE)", gameFont, 16);
            t1.setFillColor(Color::White);
            t1.setPosition(WIN_SIZE * 0.10f + 34, 15);
            win.draw(t1);
            Text t2("P: BFS Path  |  D: Dijkstra Path (See terminal)", gameFont, 13);
            t2.setFillColor(Color(55, 55, 55));
            t2.setPosition(WIN_SIZE * 0.09f + 6, 6 + infoH + 2);
            win.draw(t2);
        }
        else
        {
            RectangleShape overlay(Vector2f(WIN_SIZE, WIN_SIZE));
            overlay.setFillColor(Color(0, 0, 0, 175));
            win.draw(overlay);
            Text winT("Player " + to_string(champ + 1) + " Wins!", gameFont, 42);
            winT.setFillColor(tColors[champ]);
            winT.setPosition(WIN_SIZE / 2 - 170, WIN_SIZE / 2 - 60);
            win.draw(winT);
            Text endT("Press ENTER to see scores...", gameFont, 26);
            endT.setFillColor(Color::Yellow);
            endT.setPosition(WIN_SIZE / 2 - 120, WIN_SIZE / 2 + 20);
            win.draw(endT);
        }
        win.display();
    }

    void drawConnector(int a, int b, bool ladder)
    {
        auto [x1, y1] = locateCell(a), [x2, y2] = locateCell(b);
        float dx = x2 - x1, dy = y2 - y1, len = sqrt(dx * dx + dy * dy);
        if (!ladder)
        {
            int N = 45;
            vector<Vector2f> pts(N);
            for (int i = 0; i < N; ++i)
            {
                float t = i / float(N - 1);
                float px = x1 * (1 - t) + x2 * t, py = y1 * (1 - t) + y2 * t;
                float sway = 13 * sin(t * 2 * M_PI * 2 + a + b);
                float nx = (y2 - y1) / len, ny = -(x2 - x1) / len;
                px += sway * nx;
                py += sway * ny;
                pts[i] = Vector2f(px, py);
            }
            float thick = 14.0f;
            VertexArray snake(TriangleStrip, N * 2);
            for (int i = 0; i < N; ++i)
            {
                float dx = 0, dy = 0;
                if (i > 0)
                {
                    dx = pts[i].x - pts[i - 1].x;
                    dy = pts[i].y - pts[i - 1].y;
                }
                else if (i < N - 1)
                {
                    dx = pts[i + 1].x - pts[i].x;
                    dy = pts[i + 1].y - pts[i].y;
                }
                float nrm = sqrt(dx * dx + dy * dy);
                if (nrm == 0)
                {
                    nrm = 1;
                }
                float nx = -dy / nrm, ny = dx / nrm;
                snake[2 * i].position = Vector2f(pts[i].x + nx * (thick / 2), pts[i].y + ny * (thick / 2));
                snake[2 * i + 1].position = Vector2f(pts[i].x - nx * (thick / 2), pts[i].y - ny * (thick / 2));
                snake[2 * i].color = snake[2 * i + 1].color = Color(40, 160, 40);
            }
            win.draw(snake);
        }
        else
        {
            float off = 10.0f, nx = -dy / len, ny = dx / len;
            for (int side = -1; side <= 1; side += 2)
            {
                VertexArray rail(Lines, 2);
                rail[0].position = Vector2f(x1 + side * nx * off, y1 + side * ny * off);
                rail[1].position = Vector2f(x2 + side * nx * off, y2 + side * ny * off);
                rail[0].color = rail[1].color = Color(230, 110, 40);
                win.draw(rail);
            }
            int rungs = 7;
            for (int r = 1; r < rungs; ++r)
            {
                float t = r / float(rungs);
                float px = x1 * (1 - t) + x2 * t, py = y1 * (1 - t) + y2 * t;
                VertexArray rung(Lines, 2);
                rung[0].position = Vector2f(px + nx * off, py + ny * off);
                rung[1].position = Vector2f(px - nx * off, py - ny * off);
                rung[0].color = rung[1].color = Color(230, 170, 80);
                win.draw(rung);
            }
        }
    }

    void saveResults(const string &fname)
    {
        ofstream f(fname);
        for (int i = 0; i < activePlayers; ++i)
        {
            f << "Player " << (i + 1) << ": " << tokens[i] << endl;
        }
        f.close();
    }

    vector<string> loadResults(const string &fname)
    {
        ifstream f(fname);
        vector<string> lines;
        string s;
        while (getline(f, s))
        {
            lines.push_back(s);
        }
        return lines;
    }

    void displayResults(const string &fname)
    {
        auto data = loadResults(fname);
        RenderWindow resWin(VideoMode(WIN_SIZE, WIN_SIZE), "Final Scores");
        while (resWin.isOpen())
        {
            Event evt;
            while (resWin.pollEvent(evt))
            {
                if (evt.type == Event::Closed || (evt.type == Event::KeyPressed && evt.key.code == Keyboard::Escape))
                {
                    resWin.close();
                }
            }
            resWin.clear(Color(120, 196, 240));
            Text hd("Final Scores", gameFont, 44);
            hd.setPosition(WIN_SIZE / 2 - 140, 60);
            hd.setFillColor(Color(20, 20, 80));
            resWin.draw(hd);
            float y = 160, h = 56;
            for (size_t i = 0; i < data.size(); ++i)
            {
                Text t(data[i], gameFont, 34);
                t.setFillColor(i == champ ? Color(0, 180, 0) : Color::Black);
                t.setStyle(i == champ ? Text::Bold : Text::Regular);
                t.setPosition(WIN_SIZE / 2 - 110, y + i * h);
                resWin.draw(t);
            }
            Text winnerT("Winner: Player " + to_string(champ + 1) + "!", gameFont, 32);
            winnerT.setFillColor(Color(50, 160, 40));
            winnerT.setStyle(Text::Bold);
            winnerT.setPosition(WIN_SIZE / 2 - 90, y + data.size() * h + 16);
            resWin.draw(winnerT);
            Text note("ESC: Exit", gameFont, 26);
            note.setPosition(WIN_SIZE / 2 - 50, WIN_SIZE - 80);
            note.setFillColor(Color(60, 60, 60));
            resWin.draw(note);
            resWin.display();
        }
    }

    void showBFSPath()
    {
        vector<int> jump(NUM_CELLS + 1);
        for (int i = 1; i <= NUM_CELLS; ++i)
        {
            jump[i] = i;
        }
        for (auto &p : ladderPos)
        {
            jump[p.first] = p.second;
        }
        for (auto &s : snakePos)
        {
            jump[s.first] = s.second;
        }
        vector<bool> seen(NUM_CELLS + 1, 0);
        vector<int> prev(NUM_CELLS + 1, -1);
        queue<int> q;
        q.push(1);
        seen[1] = true;
        while (!q.empty())
        {
            int c = q.front();
            q.pop();
            if (c == 100)
            {
                break;
            }
            for (int d = 1; d <= 6; ++d)
            {
                int nx = c + d;
                if (nx <= 100 && !seen[jump[nx]])
                {
                    seen[jump[nx]] = 1;
                    prev[jump[nx]] = c;
                    q.push(jump[nx]);
                }
            }
        }
        if (seen[100])
        {
            vector<int> path;
            for (int v = 100; v != -1; v = prev[v])
            {
                path.push_back(v);
            }
            reverse(path.begin(), path.end());
            cout << "[BFS] Min Moves: " << path.size() - 1 << " Path: ";
            for (size_t i = 0; i < path.size(); ++i)
            {
                cout << path[i] << (i + 1 == path.size() ? "\n" : "->");
            }
        }
        else
        {
            cout << "[BFS] Unreachable\n";
        }
    }

    void showDijkstraPath()
    {
        vector<int> jump(NUM_CELLS + 1);
        for (int i = 1; i <= NUM_CELLS; ++i)
        {
            jump[i] = i;
        }
        for (auto &p : ladderPos)
        {
            jump[p.first] = p.second;
        }
        for (auto &s : snakePos)
        {
            jump[s.first] = s.second;
        }
        vector<int> dist(NUM_CELLS + 1, 1e9), prev(NUM_CELLS + 1, -1);
        priority_queue<pair<int, int>, vector<pair<int, int>>, greater<>> pq;
        dist[1] = 0;
        pq.push({ 0, 1 });
        while (!pq.empty())
        {
            auto [d, c] = pq.top();
            pq.pop();
            if (c == 100)
            {
                break;
            }
            for (int dice = 1; dice <= 6; ++dice)
            {
                int nx = c + dice;
                if (nx <= 100)
                {
                    int to = jump[nx];
                    if (dist[to] > dist[c] + 1)
                    {
                        dist[to] = dist[c] + 1;
                        prev[to] = c;
                        pq.push({ dist[to], to });
                    }
                }
            }
        }
        if (dist[100] < 1e9)
        {
            vector<int> path;
            for (int v = 100; v != -1; v = prev[v])
            {
                path.push_back(v);
            }
            reverse(path.begin(), path.end());
            cout << "[Dijkstra] Min Moves: " << dist[100] << " Path: ";
            for (size_t i = 0; i < path.size(); ++i)
            {
                cout << path[i] << (i + 1 == path.size() ? "\n" : "->");
            }
        }
        else
        {
            cout << "[Dijkstra] Unreachable\n";
        }
    }
};

int main()
{
    RenderWindow window(VideoMode(WIN_SIZE, WIN_SIZE), "Snake and Ladder");
    Font font;
    if (!font.loadFromFile("./assets/DejaVuSans.ttf"))
    {
        std::cerr << "Failed to load font! Make sure DejaVuSans.ttf is in assets folder.\n";
        return 1;
    }
    int numPlayers = choosePlayers(window, font);
    if (numPlayers < 2 || numPlayers > 4)
    {
        return 0;
    }
    SnakeLadderGame game(numPlayers, window, font);
    game.start();
    return 0;
}

