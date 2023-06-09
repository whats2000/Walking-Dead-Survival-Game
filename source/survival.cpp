#include <graphics.h>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>
#include <algorithm>
#include <functional>
#include <fstream>

#define SCREEN_HEIGHT 500     // 設定遊戲視窗高度
#define SCREEN_WIDTH 500      // 設定遊戲視窗寬度
#define GRID_SIDE 40          // 設定遊戲方陣每邊格子數量
#define LEFT_MARGIN 30        // 設定左邊界
#define TOP_MARGIN 40         // 設定上邊界
#define RESOURCE_AMOUNT 1     // 設定每次產生資源數量
#define PER_RESOURCE_KILL 5   // 設定多少資源數量可以殺掉一個喪屍
#define INIT_SPEED 80         // 設定初始移動速度
#define MAX_QUEUE_SIZE 1600   // 設定柱列大小
#define DETECT_ZOMBIE_RANGE 8 // 玩家評估殭屍接近範圍
#define MAX_EVAL_PATH 10      // 玩家建立評估路徑數量
#define MAX_LEVEL 5           // 最高關卡數
#define PASS_SCORE 5          // 基礎通關分數
#define MAX_PASS_TIME 20      // 一關的通關時間 (second)
#define MAX_SCORES 10         // 排行榜紀錄數量

std::random_device rd;
std::mt19937 generator(rd());
std::uniform_int_distribution<int> dist(0, std::numeric_limits<int>::max());

// 宣告前進方向列舉函數
enum Direction {
    RIGHT, LEFT, UP, DOWN
};

// 宣告遊戲場出現物體列舉函數
enum Object {
    EMPTY,    // 空白
    WALL,     // 牆或障礙物
    RESOURCE  // 資原
};

// 宣告喪屍身體節點結構
struct Entity {
    int row;            // 節點位在第幾行
    int col;            // 節點位在第幾列
    Direction direct;   // 該節點的前進方向
    struct Entity *next;  // 指向下一個節點
};

// 定義指向節點結構的指標變數
typedef struct Entity *EntityPointer;

// 定義座標結構
struct Location {
    int row;
    int col;
};

typedef struct PathNode *PathPointer;

// 定義路徑節點結構，用來建立移動路徑
struct PathNode {
    int cost;   // 距離原點的步數
    int steps;  // 距離目標的步數
    Location loc;
    PathPointer parent;
    PathPointer next;
};

struct ResourceEvaluation {
    Location resource;  // 資源的座標
    int cost;      // 到達資源所需要的總成本
};

// 開啟游戲視窗
void openWindow();

// 處理遊戲結束邏輯
void closeGame();

// 連接迷宮房間
void connectVertex(int field[][GRID_SIDE], Location vertex1, Location vertex2);

// DFS 演算迷宮生成
void vertexDfsVisit(int field[][GRID_SIDE], Location startVertex);

// 生成迷宮
void generateMaze(int field[][GRID_SIDE]);

// 遊戲進行邏輯
char playGame(int field[][GRID_SIDE],
              EntityPointer zombie,
              EntityPointer player);

//(生存者死亡條件：撞牆和撞到喪屍)
bool IsGameOver(EntityPointer zombie,
                EntityPointer player,
                int field[][GRID_SIDE]);

// 遊戲結束訊息
char showGameOverMsg();

// 遊戲通關訊息
char showGamePassMsg();

// 顯示遊戲相關資訊
void showInfo();

// 繪製遊戲區域
void drawGameField(int field[][GRID_SIDE]);

// 繪製方塊
void drawSquare(int row, int col, int color);

// 讀取AI輸入，並設定到所有喪屍節點
void controlZombieDirection(
        int field[][GRID_SIDE],
        EntityPointer zombie,
        EntityPointer player);

// 讀取鍵盤方向輸入，或者AI輸入
void controlPlayerDirection(
        int field[][GRID_SIDE],
        EntityPointer player,
        EntityPointer zombie);

// 繪製喪屍群前進一步的改變
void moveZombie(int field[][GRID_SIDE],
                EntityPointer zombie);

// 繪製生存者前進一步的改變
void movePlayer(EntityPointer player);

// 產生資源
void createResource(int field[][GRID_SIDE], EntityPointer zombie);

// 判斷是否撞到牆
bool IsAtWall(int field[][GRID_SIDE], int row, int col);

// 判斷是否撞到喪屍的身體
bool IsAtZombie(EntityPointer zombie,
                int row,
                int col);

// 判斷是否撞到喪屍
bool IsCloseZombie(EntityPointer zombie, int row, int col);

// 處理生存者收集到資源邏輯
void playerCollectResource(int field[][GRID_SIDE],
                           EntityPointer player,
                           EntityPointer zombie);

// 增加喪屍數量
void addZombie(int field[][GRID_SIDE],
               EntityPointer zombie,
               EntityPointer player);

// 隨機殺掉一個喪屍
void killZombie(EntityPointer zombie);

// 計算下一步的座標
Location nextStepLoc(EntityPointer node, Direction direct);

// 尋找最接近第 K 的資源的座標
Location findNearestKthResource(int field[][GRID_SIDE], EntityPointer me, int k);

// 生存者如果無法找到有效路徑，暫時決定一個安全方向
Direction safeDirect(int field[][GRID_SIDE],
                     EntityPointer player,
                     EntityPointer zombie);

// 喪屍如果無法找到有效路徑，暫時決定一個安全方向
Direction safeDirect4Zombie(int field[][GRID_SIDE], EntityPointer zombie);

// 喪屍尋找兩點之間可到達的路徑，不需考慮會不會撞到其他喪屍或者生存者，只需考慮不能撞到牆
PathPointer zombieFindPath(int field[][GRID_SIDE],
                           Location startLoc,
                           Location goalLoc);

// 生存者尋找兩點之間可到達的路徑，必須考慮不能撞到喪屍或者牆
PathPointer playerFindPath(int field[][GRID_SIDE],
                           Location startLoc,
                           Location goalLoc,
                           EntityPointer zombie);

// 路徑柱列處理
void addPathQueue(PathNode pathNode);   // 將之後要拜訪的節點放入佇列裡
PathPointer popPathQueue();             // 傳回路徑佇列中的元素，並將它從佇列中刪除
bool isPathQueueEmpty();                // 判斷佇列是否為空
void resetPathQueue();                  // 重設佇列
void sortPathQueue();                   // 對佇列中的元素進行排序
bool IsInPathQueue(PathNode pathNode);  // 判斷該元素是否在佇列之中

// 回傳到目標位置的路徑串列
PathPointer buildPath(PathPointer goal);

// 計算兩點之間需要移動的步數
int calcSteps(Location start, Location goal);

// 判斷是否該節點已經拜訪過
bool visited(Location loc);

// 從路徑資料判斷下一步方向
Direction getDirectionByPath(EntityPointer head,
                             PathPointer path);

// 喪屍AI
Direction zombieAI(int field[][GRID_SIDE],
                   EntityPointer zombie,
                   Location target);

// 生存者AI
Direction playerAI(int field[][GRID_SIDE],
                   EntityPointer player,
                   EntityPointer zombie);

// 評估前往最佳地點
Location evalBestLocation(int field[][GRID_SIDE], EntityPointer player, EntityPointer zombie);

// 計算到達第 k 資源花費
ResourceEvaluation evalResourceCost(int field[][GRID_SIDE], EntityPointer player, EntityPointer zombie, int k);

// 計算路徑花費
int pathCost(PathPointer path);

// 計算距離
int calculateDistance(int row, int col, int row1, int col1);

// 展示排行榜
char displayLeaderboard(const std::vector<int> &scores);

// 更新排行榜
void updateLeaderboard(std::vector<int> &scores, int newScore);

// 加載排行榜
void loadLeaderboard(std::vector<int> &scores);

struct PathNode pathQueue[MAX_QUEUE_SIZE];  // 宣告將要拜訪的節點柱列
int front;  // queue 第一個元素前一個位置
int rear;   // queue 最後一個元素的位置

int speed = INIT_SPEED;            // 遊戲移動速度
int scoreSum = 0;                  // 紀錄分數
int killedCount = 0;               // 殺死喪屍數量
int totalTime = 0;                 // 紀錄遊戲時間
int stepCount = 0;                 // 步數計數器
int const scorePerResource = 1;    // 每一份資源可得分數
bool IFPlayAI = true;              // 是否開啟AI模式
bool showTarget = true;            // 是否顯示循路目標
Location prevTarget;               // 紀錄上個循路位置
int found[13][13] = {false}; // 迷宮房間紀錄訪問
int level = 1;                     // 關卡數
int level_sum_score = PASS_SCORE;  // 過關所需分數
bool levelMode = true;             // 是否開啟關卡模式
std::vector<int> leaderboard(MAX_SCORES, 0); // 用於存儲分數的矩陣

// 主程式
int main() {
    loadLeaderboard(leaderboard);

    openWindow();
    char key = ' ';

    // 設定遊戲場和障礙物
    int field[GRID_SIDE][GRID_SIDE] = {
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
            {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1},
            {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
            {1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0,
                    0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
                    0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
                    0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1,
                    0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
                    0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1},
            {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
                    0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
            {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
            {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
            {1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
                    0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
            {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
            {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
            {1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
                    0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1,
                    0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
                    0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
                    0, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1,
                    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1},
            {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1},
            {1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0,
                    0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

    while (key != 'q' && key != 'Q') {
        Entity headPlayer = {1, 2, RIGHT, nullptr};  // 設定勇者初始位置和方向
        Entity headZombie = {16, 16, RIGHT, nullptr};  // 設定喪屍屍頭初始位置和方向
        EntityPointer zombie = &headZombie;
        EntityPointer player = &headPlayer;

        key = playGame(field, zombie, player);  // 進行遊戲
        if (key == 'q' || key == 'Q')
            closeGame();  // 如果生存者輸入'q'離開遊戲
        else if (key == 's' || key == 'S') {
            generateMaze(field);
            // 釋放勇者的鏈結資源
            while (player != nullptr) {
                EntityPointer temp = player;
                player = player->next;
                delete temp;
            }
            // 釋放喪屍的鏈結資源
            while (zombie != nullptr) {
                EntityPointer temp = zombie;
                zombie = zombie->next;
                delete temp;
            }
            level = 1;
            scoreSum = 0;
            level_sum_score = PASS_SCORE;
            totalTime = 0;
            speed = INIT_SPEED;
            continue;  // 如果生存者輸入's' 繼續遊戲
        } else if (key == 'r' || key == 'R') {
            key = displayLeaderboard(leaderboard);// 顯示排行榜

            if (key == 'q' || key == 'Q')
                closeGame();  // 如果生存者輸入'q'離開遊戲
            else if (key == 's' || key == 'S') {

                generateMaze(field);
                // 釋放勇者的鏈結資源
                while (player != nullptr) {
                    EntityPointer temp = player;
                    player = player->next;
                    delete temp;
                }
                // 釋放喪屍的鏈結資源
                while (zombie != nullptr) {
                    EntityPointer temp = zombie;
                    zombie = zombie->next;
                    delete temp;
                }
                level = 1;
                scoreSum = 0;
                level_sum_score = PASS_SCORE;
                totalTime = 0;
                speed = INIT_SPEED;
                continue;  // 如果生存者輸入's' 繼續遊戲
            }
            continue;
        } else {
            totalTime = 0;
            speed = INIT_SPEED / (1 + 0.25 * (level - 1));
            continue;
        }
    }
}

// 連接迷宮房間
void connectVertex(int field[][GRID_SIDE], Location vertex1, Location vertex2) {
    switch (vertex1.row - vertex2.row) {
        // 打掉 vertex 1 上方
        case 1:
            field[1 + vertex1.row * 3 - 1][1 + vertex1.col * 3] = 0;
            field[1 + vertex1.row * 3 - 1][1 + vertex1.col * 3 + 1] = 0;
            break;
            // 打掉 vertex 1 下方
        case -1:
            field[1 + vertex1.row * 3 + 2][1 + vertex1.col * 3] = 0;
            field[1 + vertex1.row * 3 + 2][1 + vertex1.col * 3 + 1] = 0;
            break;
        default:
            break;
    }

    switch (vertex1.col - vertex2.col) {
        // 打掉 vertex 1 左方
        case 1:
            field[1 + vertex1.row * 3][1 + vertex1.col * 3 - 1] = 0;
            field[1 + vertex1.row * 3 + 1][1 + vertex1.col * 3 - 1] = 0;
            break;
            // 打掉 vertex 1 右方
        case -1:
            field[1 + vertex1.row * 3][1 + vertex1.col * 3 + 2] = 0;
            field[1 + vertex1.row * 3 + 1][1 + vertex1.col * 3 + 2] = 0;
            break;
        default:
            break;
    }
}

// DFS 演算迷宮生成
void vertexDfsVisit(int field[][GRID_SIDE], Location startVertex) {
    int searchDirection = dist(generator) % 4; // 對應 0: 上 1: 下 2: 左 3: 右
    found[startVertex.row][startVertex.col] = true;

    for (int i = 0; i < 4; i++) {
        switch ((searchDirection + i) % 4) {
            case 0:
                if (startVertex.row != 0) {
                    if (!found[startVertex.row - 1][startVertex.col]) {
                        connectVertex(field, startVertex, {startVertex.row - 1, startVertex.col});

                        vertexDfsVisit(field, {startVertex.row - 1, startVertex.col});
                    } else if (dist(generator) % 5 == 0) {
                        connectVertex(field, startVertex, {startVertex.row - 1, startVertex.col});
                    }
                }
                break;
            case 1:
                if (startVertex.row != 12) {
                    if (!found[startVertex.row + 1][startVertex.col]) {
                        connectVertex(field, startVertex, {startVertex.row + 1, startVertex.col});

                        vertexDfsVisit(field, {startVertex.row + 1, startVertex.col});
                    } else if (dist(generator) % 5 == 0) {
                        connectVertex(field, startVertex, {startVertex.row + 1, startVertex.col});
                    }
                }
                break;
            case 2:
                if (startVertex.col != 0) {
                    if (!found[startVertex.row][startVertex.col - 1]) {
                        connectVertex(field, startVertex, {startVertex.row, startVertex.col - 1});

                        vertexDfsVisit(field, {startVertex.row, startVertex.col - 1});
                    } else if (dist(generator) % 5 == 0) {
                        connectVertex(field, startVertex, {startVertex.row, startVertex.col - 1});
                    }
                }
                break;
            case 3:
                if (startVertex.col != 12) {
                    if (!found[startVertex.row][startVertex.col + 1]) {
                        connectVertex(field, startVertex, {startVertex.row, startVertex.col + 1});

                        vertexDfsVisit(field, {startVertex.row, startVertex.col + 1});
                    } else if (dist(generator) % 5 == 0) {
                        connectVertex(field, startVertex, {startVertex.row, startVertex.col + 1});
                    }
                }
                break;
            default:
                break;
        }
    }
}

// 生成迷宮
void generateMaze(int field[][GRID_SIDE]) {
    // 初始化迷宮地圖為全牆壁
    for (int i = 0; i < GRID_SIDE; ++i) {
        for (int j = 0; j < GRID_SIDE; ++j) {
            if (i % 3 == 0 || j % 3 == 0) {
                field[i][j] = 1; // 牆壁
            } else {
                field[i][j] = 0; // 通道
            }
        }
    }

    for (auto &i: found) {
        for (int &j: i) {
            j = 0;
        }
    }

    vertexDfsVisit(field, {dist(generator) % 13, dist(generator) % 13});

    for (int i = 3; i < 19; i += 3) {
        field[1][i] = 0;
        field[2][i] = 0;
    }

    // 删除上下左右都为空的牆壁
    for (int i = 3; i < GRID_SIDE - 3; i += 1) {
        for (int j = 3; j < GRID_SIDE - 3; j += 1) {
            if (field[i - 1][j] == 0 && field[i + 1][j] == 0 && field[i][j - 1] == 0 && field[i][j + 1] == 0) {
                field[i][j] = 0;
            }
        }
    }
}

// 開啟游戲視窗
void openWindow() {
    initwindow(SCREEN_WIDTH + LEFT_MARGIN * 3, SCREEN_HEIGHT + TOP_MARGIN * 3,
               "Hungry zombie Game");
}

// 處理遊戲結束邏輯
void closeGame() {
    exit(0);
}

// 遊戲進行邏輯
char playGame(int field[][GRID_SIDE], EntityPointer zombie, EntityPointer player) {
    speed = INIT_SPEED;
    stepCount = 0;
    killedCount = 0;
    drawGameField(field);           // 繪製遊戲區域
    createResource(field, zombie);  // 產生第一份資源

    while (true) {
        char key;
        controlPlayerDirection(
                field, player,
                zombie);  // 讀取生存者輸入方向鍵，並將新方向設定到各喪屍節點
        movePlayer(player);  // 依據節點的方向，繪製新的喪屍位置

        if (stepCount % 2 == 0) {
            controlZombieDirection(field, zombie, player);
            moveZombie(field, zombie);  // 依據節點的方向，繪製新的喪屍位置
        }

        // 新增喪屍數量
        if (stepCount % 30 == 0)
            addZombie(field, zombie, player);

        playerCollectResource(
                field, player,
                zombie);  // 判斷生存者是否有收集到資源，如果有增加分數

        showInfo();  // 顯示時間和分數資訊
        if (levelMode) {
            if (totalTime / 1000 > MAX_PASS_TIME && scoreSum < level_sum_score) {
                return char(showGameOverMsg());
            } else if (scoreSum >= level_sum_score) {
                return showGamePassMsg();
            }
        }

        if (IsGameOver(zombie, player, field))  // 判斷是否符合遊戲結束條件，
        {
            updateLeaderboard(leaderboard, scoreSum);// 更新排行榜
            return showGameOverMsg();  // 顯示遊戲結束訊息，並等待生存者輸入選項
        }
        // 除了收集到資源會產生新資源，系統也隨機產生新資源
        if (dist(generator) % 20 == 0)
            createResource(field, zombie);

        delay(speed);  // 決定生存者與喪屍移動速度，speed越小移動越快
        stepCount++;
        // 讀取非方向鍵的其他鍵盤輸入
        if (kbhit()) {
            key = char(getch());

            // 只有輸入大小寫的a, q 和
            // s，系統才有反應並將輸入結果拋到外層等待處理
            if (key == 'q' || key == 'Q' || key == 's' || key == 'S')
                return key;
            else if (key == 'a')  // 決定是否改變模式
                // ，主要有生存者模式和電腦操控的AI模式
                IFPlayAI = !IFPlayAI;
            else if (key == 'm')
                levelMode = !levelMode;
        }
    }
}

// 繪製遊戲區域，依據遊戲場矩陣設定繪製物件
void drawGameField(int field[][GRID_SIDE]) {
    int row, col;
    cleardevice();  // 清理螢幕畫面
    for (row = 0; row < GRID_SIDE; row++) {
        for (col = 0; col < GRID_SIDE; col++) {
            switch (field[row][col]) {
                case WALL:  // 牆在矩陣中的值是1
                    drawSquare(row, col, YELLOW);
                    break;
                case RESOURCE:  // 資源在矩陣中的值是2
                    drawSquare(row, col, GREEN);
                    break;
            }
        }
    }
}

// 繪製方塊
void drawSquare(int row, int col, int color) {
    const int squareHeight = SCREEN_HEIGHT / GRID_SIDE;
    const int SquareWidth = SCREEN_WIDTH / GRID_SIDE;
    int left, right, bottom, top;
    left = LEFT_MARGIN + col * SquareWidth + col;
    top = TOP_MARGIN + row * squareHeight + row;
    right = left + SquareWidth;
    bottom = top + squareHeight;

    setfillstyle(SOLID_FILL, color);  // 設定繪製物件的為實心和顏色
    bar(left, top, right, bottom);    // 設定繪製方塊的位置
}

// 繪製喪屍每前進一步的改變
void moveZombie(int field[][GRID_SIDE], EntityPointer zombie) {
    int currRow, currCol;

    while (zombie != nullptr) {
        currRow = zombie->row;
        currCol = zombie->col;

        if (field[currRow][currCol] == RESOURCE)
            drawSquare(currRow, currCol, GREEN);
        else
            drawSquare(currRow, currCol, BLACK);

        // 依據節點的方向屬性，設定移動下一步的位置
        switch (zombie->direct) {
            case RIGHT:
                zombie->col++;
                break;
            case LEFT:
                zombie->col--;
                break;
            case UP:
                zombie->row--;
                break;
            case DOWN:
                zombie->row++;
                break;
        }

        drawSquare(zombie->row, zombie->col, RED);

        zombie = zombie->next;
    }
}

// 繪製生存者每前進一步的改變
void movePlayer(EntityPointer player) {
    int currRow, currCol;
    if (player != nullptr) {
        currRow = player->row;
        currCol = player->col;

        switch (player->direct) {
            case RIGHT:
                player->col++;
                break;
            case LEFT:
                player->col--;
                break;
            case UP:
                player->row--;
                break;
            case DOWN:
                player->row++;
                break;
        }
        drawSquare(player->row, player->col, BLUE);
        drawSquare(currRow, currCol, BLACK);
    }
}

// 判斷生存者是否死亡(死亡條件：撞牆和撞到自己身體)
bool IsGameOver(EntityPointer zombie,
                EntityPointer player,
                int field[][GRID_SIDE]) {
    EntityPointer head = zombie;

    // 判斷是否撞到牆
    if (IsAtWall(field, head->row, head->col))
        return true;
    if (IsAtWall(field, player->row, player->col))
        return true;

    // 檢查是否AI撞到喪屍
    if (IsAtZombie(zombie, player->row, player->col))
        return true;

    return false;
}

// 判斷是否撞到牆
bool IsAtWall(int field[][GRID_SIDE], int row, int col) {
    if (field[row][col] == WALL)
        return true;
    return false;
}

// 判斷是否撞到喪屍
bool IsAtZombie(EntityPointer zombie, int row, int col) {
    while (zombie != nullptr) {
        if (row == zombie->row && col == zombie->col)
            return true;
        zombie = zombie->next;
    }
    return false;
}


// 遊戲結束訊息
char showGameOverMsg() {
    // cleardevice(); //清理所有螢幕資料，如果希望只顯示訊息時，取消註解
    char msg1[15] = "Game Over!!";
    if (totalTime / 1000 > MAX_PASS_TIME && levelMode) {
        strcpy(msg1, "Time  Out!!");
    }
    int i = 0;

    char msg2[40] = "press [q] to quit or [s] to restart";
    char msg3[40] = "or [r] for leaderboard!!";
    for (;; i++) {
        setcolor(i % 14);
        settextstyle(TRIPLEX_FONT, HORIZ_DIR, 7);
        outtextxy(50, SCREEN_HEIGHT / 2, msg1);

        setcolor(WHITE);
        settextstyle(TRIPLEX_FONT, HORIZ_DIR, 2);
        outtextxy(60, SCREEN_HEIGHT / 2 + 70, msg2);
        outtextxy(60, SCREEN_HEIGHT / 2 + 90, msg3);

        delay(200);

        setcolor(BLACK);
        settextstyle(TRIPLEX_FONT, HORIZ_DIR, 7);
        outtextxy(20, SCREEN_HEIGHT / 2, msg1);

        if (kbhit()) {
            char key;
            key = char(getch());
            if (key == 'q' || key == 'Q' || key == 's' || key == 'S' || key == 'r' || key == 'R') {
                return key;
            }
        }
    }
}

// 過關的訊息
char showGamePassMsg() {
    // cleardevice(); //清理所有螢幕資料，如果希望只顯示訊息時，取消註解
    level++;
    level_sum_score += PASS_SCORE * level;

    int i = 0;
    char msg1[21] = "";
    char msg2[40] = "";
    if (level <= MAX_LEVEL) {
        strcpy(msg2, "press [q] to quit or [c] to next level");
        sprintf(msg1, "You Pass Level %d!!", level - 1);
    } else {
        strcpy(msg2, "press [q] to quit or [s] to restart!!!");
        strcpy(msg1, "You Pass All Level!!");
    }

    char msg3[40] = "or [r] for leaderboard!!";

    for (;; i++) {
        setcolor(i % 14);
        settextstyle(TRIPLEX_FONT, HORIZ_DIR, 5);
        outtextxy(20, SCREEN_HEIGHT / 2, msg1);

        setcolor(WHITE);
        settextstyle(TRIPLEX_FONT, HORIZ_DIR, 1);
        outtextxy(60, SCREEN_HEIGHT / 2 + 70, msg2);

        if (level > MAX_LEVEL) {
            outtextxy(60, SCREEN_HEIGHT / 2 + 90, msg3);
        }

        delay(200);

        setcolor(BLACK);
        settextstyle(TRIPLEX_FONT, HORIZ_DIR, 5);
        outtextxy(20, SCREEN_HEIGHT / 2, msg1);

        if (kbhit()) {
            char key;
            key = char(getch());
            if (key == 'q' || key == 'Q' || key == 's' || key == 'S') {
                return key;
            }

            if (level <= MAX_LEVEL && (key == 'c' || key == 'C')) {
                return key;
            } else if (level > MAX_LEVEL && (key == 'r' || key == 'R')) {
                updateLeaderboard(leaderboard, scoreSum);// 更新排行榜
                return key;
            }
        }
    }
}

// 顯示遊戲相關資訊
void showInfo() {
    totalTime += speed;
    char levelMsg[45] = "";
    char timeMsg[45] = " Time:";
    char scoreMsg[45] = "Score:";
    char killedMsg[50] = "Killed Zombie:";
    char modeMsg[20] = "";
    char levelModeMsg[20] = "";
    char optMsg1[50] = "press [q] to quit, [s] to restart or";
    char optMsg2[60] = "press [a] to toggle AI mode, [m] to toggle level mode";

    char time[10];
    char score[10];
    char killed[10];

    sprintf(time, "%5d", totalTime / 1000);
    strcat(timeMsg, time);
    strcat(timeMsg, " sec.");

    setcolor(WHITE);                           // 設定文字顏色
    settextstyle(COMPLEX_FONT, HORIZ_DIR, 1);  // 設定字型，水平或垂直和字型大小
    outtextxy(0, 0, timeMsg);  // 依據坐標輸出文字到螢幕

    sprintf(score, "%5d", scoreSum);
    strcat(scoreMsg, score);
    strcat(scoreMsg, " point.");

    setcolor(WHITE);
    settextstyle(COMPLEX_FONT, HORIZ_DIR, 1);
    outtextxy(0, 19, scoreMsg);

    sprintf(levelMsg, "Level: %d Goal: %2d", level, level_sum_score);

    setcolor(WHITE);
    settextstyle(COMPLEX_FONT, HORIZ_DIR, 1);
    outtextxy(250, 0, levelMsg);

    sprintf(killed, "%3d", killedCount);
    strcat(killedMsg, killed);

    setcolor(WHITE);
    settextstyle(COMPLEX_FONT, HORIZ_DIR, 1);
    outtextxy(250, 19, killedMsg);

    if (IFPlayAI) {
        strcat(modeMsg, " AI Mode    ");
    } else {
        strcat(modeMsg, " Player Mode");
    }

    if (levelMode) {
        strcat(levelModeMsg, " Level ON ");
    } else {
        strcat(levelModeMsg, " Level OFF");
    }

    setcolor(LIGHTMAGENTA);
    settextstyle(COMPLEX_FONT, HORIZ_DIR, 1);
    outtextxy(SCREEN_HEIGHT - LEFT_MARGIN * 2, 0, modeMsg);

    setcolor(LIGHTMAGENTA);
    settextstyle(COMPLEX_FONT, HORIZ_DIR, 1);
    outtextxy(SCREEN_HEIGHT - LEFT_MARGIN * 2, 19, levelModeMsg);

    setcolor(LIGHTGREEN);
    settextstyle(COMPLEX_FONT, HORIZ_DIR, 1);
    outtextxy(0, TOP_MARGIN + (GRID_SIDE + 2) * SCREEN_HEIGHT / GRID_SIDE,
              optMsg1);
    outtextxy(0, TOP_MARGIN + (GRID_SIDE + 2) * SCREEN_HEIGHT / GRID_SIDE + 20,
              optMsg2);
}

// 讀取鍵盤方向輸入，並設定到生存者節點
void controlPlayerDirection(int field[][GRID_SIDE],
                            EntityPointer player,
                            EntityPointer zombie) {
    Direction playerDirect;

    // get key code by pressing keyboard
    int key = 0;
    if (kbhit())
        key = getch();

    // decide zombie's moving direction
    switch (key) {
        case KEY_RIGHT:
            playerDirect = RIGHT;
            break;
        case KEY_LEFT:
            playerDirect = LEFT;
            break;
        case KEY_UP:
            playerDirect = UP;
            break;
        case KEY_DOWN:
            playerDirect = DOWN;
            break;
        default:
            playerDirect = player->direct;
            break;
    }

    if (IFPlayAI)
        playerDirect = playerAI(field, player, zombie);

    player->direct = playerDirect;
}

// 讀取鍵盤方向輸入，並設定到所有喪屍節點
void controlZombieDirection(int field[][GRID_SIDE],
                            EntityPointer zombie,
                            EntityPointer player) {
    int count = 0;
    while (zombie != nullptr) {
        Location target = {player->row + count, player->col + count};
        Direction zombieDirect = zombieAI(field, zombie, target);
        zombie->direct = zombieDirect;
        zombie = zombie->next;
        count += 2;
    }
}

// 產生資源
void createResource(int field[][GRID_SIDE], EntityPointer zombie) {
    int row, col, i, amount = RESOURCE_AMOUNT;

    for (i = 0; i < amount; i++) {
        // 如果亂數產生的位置是在牆和喪屍身體重疊，則重新產生，直到符合條件為止
        do {
            row = dist(generator) % GRID_SIDE;
            col = dist(generator) % GRID_SIDE;
        } while (IsAtWall(field, row, col) || IsAtZombie(zombie, row, col));

        field[row][col] = RESOURCE;
        drawSquare(row, col, GREEN);
    }
}

// 系統處理生存者收集到資源邏輯
void playerCollectResource(int field[][GRID_SIDE],
                           EntityPointer player,
                           EntityPointer zombie) {
    // 如果生存者與資源位置重疊，就是收集到資源
    if (field[player->row][player->col] == RESOURCE) {
        field[player->row][player->col] = EMPTY;  // 將該資源清空
        printf("The player has eaten food at row: %d, col: %d\n", player->row,
               player->col);
        scoreSum += scorePerResource;   // 紀錄分數
        createResource(field, player);  // 產生新的資源

        // 收集一定數量的資源可以消滅一個喪屍
        if (scoreSum % PER_RESOURCE_KILL == 0)
            killZombie(zombie);
    }
}

// 增加喪屍數量
void addZombie(int field[][GRID_SIDE], EntityPointer zombie, EntityPointer player) {
    int row, col;
    EntityPointer tail = zombie;

    // 尋找最後一個喪屍節點
    while (tail->next != nullptr) {
        tail = tail->next;
    }

    // 建立新的喪屍節點
    auto newNode = new Entity;

    // 將最後一位喪屍的方向屬性給新節點
    newNode->direct = tail->direct;

    do {
        row = dist(generator) % GRID_SIDE;
        col = dist(generator) % GRID_SIDE;
    } while (IsAtWall(field, row, col) || IsAtZombie(zombie, row, col) ||
             (player->row == row && player->col == col));

    newNode->row = row;
    newNode->col = col;
    newNode->next = nullptr;

    tail->next = newNode;  // 將尾巴節點連接到新節點
}

// 殺掉一個喪屍
void killZombie(EntityPointer zombie) {
    EntityPointer temp, killed;
    temp = zombie;

    killed = zombie;

    // 不會殺光所有喪屍，至少會保留一個
    if (zombie->next == nullptr)
        return;
    while (killed->next != nullptr) {
        temp = killed;
        killed = killed->next;
    }
    temp->next = killed->next;
    drawSquare(killed->row, killed->col, BLACK);
    printf("\n(%d, %d) is killed\n", killed->row, killed->col);
    delete killed;
    killedCount++;
}

// 喪屍的AI控制
Direction zombieAI(int field[][GRID_SIDE],
                   EntityPointer zombie,
                   Location target) {
    Direction zombieDirect;
    Location start = {zombie->row, zombie->col};

    PathPointer path = zombieFindPath(field, start, target);
    if (path) {
        zombieDirect = getDirectionByPath(zombie, path);
    } else
        zombieDirect = safeDirect4Zombie(field, zombie);

    delete path;

    return zombieDirect;
}

// 從路徑資料判斷下一步方向
Direction getDirectionByPath(EntityPointer head, PathPointer path) {
    PathPointer nextPath = path->next;
    int horizontal = nextPath->loc.col - head->col;
    int vertical = nextPath->loc.row - head->row;
    if (horizontal == 1)
        return RIGHT;
    else if (horizontal == -1)
        return LEFT;

    if (vertical == 1)
        return DOWN;
    else if (vertical == -1)
        return UP;
    return head->direct;
}

// 喪屍如果無法找到有效路徑，暫時決定一個安全方向
Direction safeDirect4Zombie(int field[][GRID_SIDE], EntityPointer zombie) {
    Location loc = nextStepLoc(zombie, UP);
    if (!IsAtWall(field, loc.row, loc.col))
        return UP;
    loc = nextStepLoc(zombie, DOWN);
    if (!IsAtWall(field, loc.row, loc.col))
        return DOWN;
    loc = nextStepLoc(zombie, RIGHT);
    if (!IsAtWall(field, loc.row, loc.col))
        return RIGHT;
    loc = nextStepLoc(zombie, LEFT);
    if (!IsAtWall(field, loc.row, loc.col))
        return LEFT;
    return zombie->direct;
}

// 喪屍尋找兩點之間可到達的路徑，不需考慮會不會撞到其他喪屍或者生存者
PathPointer zombieFindPath(int field[][GRID_SIDE],
                           Location startLoc,
                           Location goalLoc) {
    resetPathQueue();
    int steps = calcSteps(startLoc, goalLoc);
    PathNode start = {0, steps, startLoc, nullptr, nullptr};
    addPathQueue(start);
    while (!isPathQueueEmpty()) {
        sortPathQueue();
        PathPointer current = popPathQueue();
        if (current == nullptr)
            return nullptr;
        if (current->loc.row == goalLoc.row && current->loc.col == goalLoc.col)
            return buildPath(current);
        int dirSize = 4;
        int iDir[] = {1, 0, -1, 0};
        int jDir[] = {0, 1, 0, -1};
        int i, j;
        for (i = 0, j = 0; i < dirSize; i++, j++) {
            Location neighborLoc = {current->loc.row + iDir[i],
                                    current->loc.col + jDir[j]};
            if (!visited(neighborLoc) &&
                !IsAtWall(field, neighborLoc.row, neighborLoc.col)) {
                steps = calcSteps(neighborLoc, goalLoc);
                int cost = current->cost + 1;
                PathNode neighbor = {cost, steps, neighborLoc, current, nullptr};
                if (!IsInPathQueue(neighbor)) {
                    addPathQueue(neighbor);
                }
            }
        }
    }
    return nullptr;
}

// 將之後要拜訪的節點放入佇列裡
void addPathQueue(PathNode pathNode) {
    if (rear == MAX_QUEUE_SIZE - 1) {
        printf("The queue is full rear: %d, front: %d\n", rear, front);
        resetPathQueue();
        return;
    }
    rear += 1;
    pathQueue[rear] = pathNode;
}

// 傳回佇列中的路徑座標節點，並將它從佇列中刪除
PathPointer popPathQueue() {
    if (front == rear) {
        printf("the queue is empty");
        return nullptr;
    }
    front++;
    auto node = new PathNode;
    node->cost = pathQueue[front].cost;
    node->steps = pathQueue[front].steps;
    node->loc = pathQueue[front].loc;
    node->parent = pathQueue[front].parent;
    node->next = pathQueue[front].next;
    return node;
}

// 判斷佇列是否為空
bool isPathQueueEmpty() {
    return front == rear;
}

// 重設佇列
void resetPathQueue() {
    front = -1;
    rear = -1;
}

// 對佇列中的元素進行排序
void sortPathQueue() {
    if (front == rear)
        return;
    int i, j;
    int nowTotal, nextTotal;
    for (i = front + 1; i < rear; i++) {
        for (j = i + 1; j <= rear; j++) {
            nowTotal = pathQueue[i].cost + pathQueue[i].steps;
            nextTotal = pathQueue[j].cost + pathQueue[j].steps;

            if (nowTotal > nextTotal) {
                PathNode temp = pathQueue[i];
                pathQueue[i] = pathQueue[j];
                pathQueue[j] = temp;
            }
        }
    }
}

// 判斷該元素是否在佇列之中
bool IsInPathQueue(PathNode pathNode) {
    int i;
    if (front == rear)
        return false;
    for (i = front; i <= rear; i++) {
        if (pathQueue[i].loc.row == pathNode.loc.row &&
            pathQueue[i].loc.col == pathNode.loc.col)
            return true;
    }
    return false;
}

// 回傳到目標位置的路徑串列
PathPointer buildPath(PathPointer goal) {
    //printf("buildPath ");
    //printf("(%d, %d)\n", goal->loc.row, goal->loc.col);
    if (goal->parent == nullptr) {
        delete goal;
        return nullptr;
    }
    PathPointer head = goal;
    head->next = nullptr;
    PathPointer temp = head;

    while (head->parent) {
        //printf("node (%d, %d)->", head->loc.row, head->loc.col);
        head = head->parent;
        head->next = temp;
        temp = head;
    }
    //printf("nullptr\n");
    delete goal;
    return head;
}

// 計算兩點之間需要移動的步數
int calcSteps(Location start, Location goal) {
    int steps = abs(start.row - goal.row) + abs(start.col - goal.col);
    return steps;
}

// 判斷是否該節點已經拜訪過
bool visited(Location loc) {
    int i;
    for (i = 0; i <= front; i++) {
        if (pathQueue[i].loc.row == loc.row && pathQueue[i].loc.col == loc.col)
            return true;
    }
    return false;
}

// 找尋最近的第 k 個資源
Location findNearestKthResource(int field[][GRID_SIDE], EntityPointer me, int k) {
    std::vector<Location> resources;
    int row, col;

    for (row = 0; row < GRID_SIDE; row++) {
        for (col = 0; col < GRID_SIDE; col++) {
            if (field[row][col] == RESOURCE) {
                Location resource = {row, col};
                resources.push_back(resource);
            }
        }
    }

    std::sort(resources.begin(), resources.end(), [me](const Location &a, const Location &b) {
        int rowDisA = abs(a.row - me->row);
        int colDisA = abs(a.col - me->col);
        int rowDisB = abs(b.row - me->row);
        int colDisB = abs(b.col - me->col);
        return (rowDisA + colDisA) < (rowDisB + colDisB);
    });

    if (k <= resources.size()) {
        return resources[k - 1];
    }

    // 如果 k 是不存在資源回傳不存在
    return {-1, -1};
}

// 生存者如果無法找到有效路徑，暫時決定一個安全方向
Direction safeDirect(int field[][GRID_SIDE],
                     EntityPointer player,
                     EntityPointer zombie) {
    Location loc = nextStepLoc(player, UP);
    if (!IsAtWall(field, loc.row, loc.col) &&
        !IsCloseZombie(zombie, loc.row, loc.col))
        return UP;
    loc = nextStepLoc(player, DOWN);
    if (!IsAtWall(field, loc.row, loc.col) &&
        !IsCloseZombie(zombie, loc.row, loc.col))
        return DOWN;
    loc = nextStepLoc(player, RIGHT);
    if (!IsAtWall(field, loc.row, loc.col) &&
        !IsCloseZombie(zombie, loc.row, loc.col))
        return RIGHT;
    loc = nextStepLoc(player, LEFT);
    if (!IsAtWall(field, loc.row, loc.col) &&
        !IsCloseZombie(zombie, loc.row, loc.col))
        return LEFT;
    return player->direct;
}

// 計算下一步的座標
Location nextStepLoc(EntityPointer node, Direction direct) {
    int currRow = node->row;
    int currCol = node->col;
    int nextRow, nextCol;
    Location loc{};
    switch (direct) {
        case RIGHT:
            nextRow = currRow;
            nextCol = currCol + 1;
            break;
        case LEFT:
            nextRow = currRow;
            nextCol = currCol - 1;
            break;
        case UP:
            nextRow = currRow - 1;
            nextCol = currCol;
            break;
        case DOWN:
            nextRow = currRow + 1;
            nextCol = currCol;
            break;
    }
    loc.row = nextRow;
    loc.col = nextCol;
    return loc;
}

// 生存者尋找兩點之間可到達的路徑，必須考慮會不會撞到牆或者喪屍
PathPointer playerFindPath(int field[][GRID_SIDE],
                           Location startLoc,
                           Location goalLoc,
                           EntityPointer zombie) {
    resetPathQueue();
    int steps = calcSteps(startLoc, goalLoc);
    PathNode start = {0, steps, startLoc, nullptr, nullptr};
    addPathQueue(start);
    while (!isPathQueueEmpty()) {
        sortPathQueue();
        PathPointer current = popPathQueue();
        if (current == nullptr)
            return nullptr;
        if (current->loc.row == goalLoc.row && current->loc.col == goalLoc.col)
            return buildPath(current);
        int dirSize = 4;
        int iDir[] = {1, 0, -1, 0};
        int jDir[] = {0, 1, 0, -1};
        int i, j;
        for (i = 0, j = 0; i < dirSize; i++, j++) {
            Location neighborLoc = {current->loc.row + iDir[i],
                                    current->loc.col + jDir[j]};
            if (!visited(neighborLoc) &&
                !IsAtWall(field, neighborLoc.row, neighborLoc.col) &&
                !IsCloseZombie(zombie, neighborLoc.row, neighborLoc.col)) {
                steps = calcSteps(neighborLoc, goalLoc);

                int cost = 1;

                // 檢查特定範圍內殭屍
                EntityPointer currZombie = zombie;
                while (currZombie != nullptr) {
                    int distanceToZombie = calculateDistance(neighborLoc.row, neighborLoc.col, currZombie->row,
                                                             currZombie->col);
                    if (distanceToZombie <= DETECT_ZOMBIE_RANGE) {
                        cost += (DETECT_ZOMBIE_RANGE - distanceToZombie) * 5;
                    }
                    currZombie = currZombie->next;
                }

                int wallCount = 0;

                for (int dx = -1; dx <= 1; dx++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        int newRow = neighborLoc.row + dx;
                        int newCol = neighborLoc.col + dy;
                        if (IsAtWall(field, newRow, newCol)) {
                            wallCount++;
                        }
                    }
                }

                cost += current->cost;

                if (wallCount > 3)
                    cost += wallCount * wallCount * 5;

                PathNode neighbor = {cost, steps, neighborLoc, current, nullptr};

                if (!IsInPathQueue(neighbor)) {
                    addPathQueue(neighbor);
                }
            }
        }
    }
    return nullptr;
}

// 判斷是否會撞到喪屍
bool IsCloseZombie(EntityPointer zombie, int row, int col) {
    if (zombie == nullptr)
        return false;
    EntityPointer head = zombie;
    while (zombie != nullptr) {
        if (row == zombie->row && col == zombie->col)
            return true;
        zombie = zombie->next;
    }
    int dirSize = 4;
    int iDir[] = {1, 0, -1, 0};
    int jDir[] = {0, 1, 0, -1};
    int i, j;
    while (head != nullptr) {
        for (i = 0, j = 0; i < dirSize; i++, j++) {
            if (row == (head->row + iDir[i]) && (col == head->col + jDir[j]))
                return true;
        }
        head = head->next;
    }

    return false;
}

// 實作生存者AI
Direction playerAI(int field[][GRID_SIDE],
                   EntityPointer player,
                   EntityPointer zombie) {
    Direction playerDirect;

    Location start = {player->row, player->col};

    Location target = evalBestLocation(field, player, zombie);

    PathPointer path = playerFindPath(field, start, target, zombie);

    if (showTarget) {
        switch (field[prevTarget.row][prevTarget.col]) {
            case WALL:  // 牆在矩陣中的值是1
                drawSquare(prevTarget.row, prevTarget.col, YELLOW);
                break;
            case RESOURCE:  // 資源在矩陣中的值是2
                drawSquare(prevTarget.row, prevTarget.col, GREEN);
                break;
        }
        drawSquare(target.row, target.col, LIGHTBLUE);
        prevTarget = target;
    }

    if (path) {
        playerDirect = getDirectionByPath(player, path);
    } else
        playerDirect = safeDirect(field, player, zombie);

    delete path;

    return playerDirect;
}

// 評估前往最佳地點
Location evalBestLocation(int field[][GRID_SIDE], EntityPointer player, EntityPointer zombie) {
    std::vector<ResourceEvaluation> evaluations;

    int k = MAX_EVAL_PATH;

    for (int i = 1; i <= k; i++) {
        ResourceEvaluation evaluation = evalResourceCost(field, player, zombie, i);
        evaluations.push_back(evaluation);
    }

    // 根據總成本排序
    std::sort(evaluations.begin(), evaluations.end(), [](const ResourceEvaluation &a, const ResourceEvaluation &b) {
        return a.cost < b.cost;
    });

    if (evaluations.empty() || evaluations[0].cost == -1) {
        // 沒有找到資源，回傳無效座標
        return {-1, -1};
    }

    printf("PathFind: [%d, %d]  Cost: %d\n",
           evaluations[0].resource.row,
           evaluations[0].resource.col,
           evaluations[0].cost);

    // 回傳最低成本座標
    return evaluations[0].resource;
}

// 計算到達第 k 資源花費
ResourceEvaluation evalResourceCost(int field[][GRID_SIDE], EntityPointer player, EntityPointer zombie, int k) {
    Location start = {player->row, player->col};
    Location resource = findNearestKthResource(field, player, k);
    PathPointer path = playerFindPath(field, start, resource, zombie);

    if (!path || resource.row == -1 || resource.col == -1) {
        delete path;

        // 當找不到資源或無效路徑回傳該資源為無效花費
        return {resource, 999};
    }

    int cost = pathCost(path);

    delete path;

    return {resource, cost};
}

// 計算路徑花費
int pathCost(PathPointer path) {
    while (path->next != nullptr) {
        path = path->next;
    }

    return path->cost;
}

// 計算距離
int calculateDistance(int row, int col, int row1, int col1) {
    return abs(row1 - row) + abs(col1 - col);
}

//顯示排行榜
char displayLeaderboard(const std::vector<int> &scores) {
    char msg1[15] = "Leaderboard";
    char msg2[140];
    char msg3[40] = "press [q] to quit or [s] to restart !!";
    int y = SCREEN_HEIGHT / 2 - (scores.size() * 30) / 2; // 設定 Y 座標位置正中間

    while (true) {
        // 清理畫面
        cleardevice();

        setcolor(WHITE);
        settextstyle(TRIPLEX_FONT, HORIZ_DIR, 2);

        // 顯示 msg1
        outtextxy((SCREEN_WIDTH - textwidth(msg1)) / 2, y - 30, msg1);

        // 逐行輸出名次和分數
        for (int i = 0; i < scores.size(); i++) {
            char temp[50];
            sprintf(temp, "%2d. %4d points", i + 1, scores[i]);
            outtextxy((SCREEN_WIDTH - textwidth(temp)) / 2, y + i * 30, temp); // 水平置中輸出文字

        }

        strcpy(msg2, msg3); // 將 msg3 複製到 msg2

        settextstyle(TRIPLEX_FONT, HORIZ_DIR, 2);
        outtextxy(60, y + 400, msg2);
        delay(200);

        if (kbhit()) {
            char key;
            key = char(getch());
            if (key == 'q' || key == 'Q' || key == 's' || key == 'S') {
                return key;
            }
        }
    }
}

//更新排行榜
void updateLeaderboard(std::vector<int> &scores, int newScore) {
    scores.push_back(newScore);
    std::sort(scores.begin(), scores.end(), std::greater<>());

    if (scores.size() > MAX_SCORES) {
        scores.pop_back(); // 如果超過最大排行榜數，移除最低分數一個
    }

    std::ofstream file("../resource/leaderboard.txt"); // 打開分數紀錄檔

    if (file.is_open()) {
        for (const int score : scores) {
            file << score << "\n"; // 將分數逐行儲存
        }

        file.close(); // 關閉文件
    } else {
        // 處理開啟失敗情況
        printf("Can't not open file\n");
    }
}

void loadLeaderboard(std::vector<int> &scores) {
    std::ifstream file("../resource/leaderboard.txt");

    if (file.is_open()) {
        int score;
        int index = 0;

        while (file >> score && index < MAX_SCORES) {
            scores[index] = score;
            ++index;
        }

        file.close();
    } else {
        // 處理開啟失敗情況
        printf("Can't not open file\n");
    }
}


