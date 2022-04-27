#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <stdlib.h>
#include <sys/select.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#define ECHOFLAGS (ECHO | ECHOE | ECHOK | ECHONL)
#define DETA_X 2
#define DETA_Y 2
#define EDGE_THICKNESS 2
#define DIFFICULTY_FACTOR 50

bool gameOver;
const int width = 20;
const int height = 20;
int x, y, fruitX, fruitY, score;
enum eDirection
{
    STOP = 0,
    LEFT,
    RIGHT,
    UP,
    DOWN
};
eDirection dir;
int tailX[100], tailY[100];
int nTail = 1;
bool isFullWidth = false;
bool isPause = false;
bool useSaved = false;
bool isSaved = false;
bool loadSaved = false;

// savedData
int tailX_saved[100], tailY_saved[100];
int nTail_saved;
bool isFullWidth_saved;
int x_saved, y_saved;
int fruitX_saved, fruitY_saved;
int score_saved;
eDirection dir_saved;

static void sleep_ms(unsigned int secs)
{
    struct timeval tval;
    tval.tv_sec = secs / 1000;
    tval.tv_usec = (secs * 1000) % 1000000;
    select(0, NULL, NULL, NULL, &tval);
}

int kbhit(void)

{
    struct termios oldt, newt;
    int ch;
    int oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if (ch != EOF)
    {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}

int set_disp_mode(int fd, int option)
{
    int err;
    struct termios term;
    if (tcgetattr(fd, &term) == -1)
    {
        perror("Cannot get the attribution of the terminal");
        return 1;
    }
    if (option)
        term.c_lflag |= ECHOFLAGS;
    else
        term.c_lflag &= ~ECHOFLAGS;
    err = tcsetattr(fd, TCSAFLUSH, &term);
    if (err == -1 && err == EINTR)
    {
        perror("Cannot set the attribution of the terminal");
        return 1;
    }
    return 0;
}

void setPos(int x, int y)
{
    if (isFullWidth)
        x = 2 * x + DETA_X;
    else
        x += DETA_X;
    y += DETA_Y;
    printf("\033[%d;%dH", y, x);
}

unsigned long GetTickCount()
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

void readSavedData()
{
    FILE *fp = NULL;
    char buff[255];

    fp = fopen("saved.txt", "r");

    if (fp == 0 || loadSaved == true)
        return;

    //校验存档
    fgets(buff, 255, (FILE *)fp);
    if (atoi(buff) != 111)
        return;

    //读取数据
    fgets(buff, 255, (FILE *)fp);
    isFullWidth_saved = atoi(buff);
    fgets(buff, 255, (FILE *)fp);
    nTail_saved = atoi(buff);
    fgets(buff, 255, (FILE *)fp);
    score_saved = atoi(buff);
    fgets(buff, 255, (FILE *)fp);
    x_saved = atoi(buff);
    fgets(buff, 255, (FILE *)fp);
    y_saved = atoi(buff);
    fgets(buff, 255, (FILE *)fp);
    fruitX_saved = atoi(buff);
    fgets(buff, 255, (FILE *)fp);
    fruitY_saved = atoi(buff);
    fgets(buff, 255, (FILE *)fp);
    switch (atoi(buff))
    {
    case 0:
        dir_saved = STOP;
        break;
    case 1:
        dir_saved = LEFT;
        break;
    case 2:
        dir_saved = RIGHT;
        break;
    case 3:
        dir_saved = UP;
        break;
    case 4:
        dir_saved = DOWN;
        break;
    default:
        break;
    }
    for (int i = 0; i < 100; i++)
    {
        fgets(buff, 255, (FILE *)fp);
        tailX_saved[i] = atoi(buff);
    }
    for (int i = 0; i < 100; i++)
    {
        fgets(buff, 255, (FILE *)fp);
        tailY_saved[i] = atoi(buff);
    }

    loadSaved = true;
    fclose(fp);
}

void saveData()
{
    if (gameOver == true || isPause == false)
        return;

    FILE *fp = NULL;

    fp = fopen("saved.txt", "w");
    fprintf(fp, "111\n");
    fprintf(fp, "%d\n", isFullWidth);
    fprintf(fp, "%d\n", nTail);
    fprintf(fp, "%d\n", score);
    fprintf(fp, "%d\n", x);
    fprintf(fp, "%d\n", y);
    fprintf(fp, "%d\n", fruitX);
    fprintf(fp, "%d\n", fruitY);
    fprintf(fp, "%d\n", dir);
    for (int i = 0; i < 100; i++)
    {
        fprintf(fp, "%d\n", tailX[i]);
    }
    for (int i = 0; i < 100; i++)
    {
        fprintf(fp, "%d\n", tailY[i]);
    }
    isSaved = true;
    fclose(fp);
}

void Inital(void);
void DrawMap();
void Prompt_info(int, int);

void loadData()
{
    if (gameOver == true || isPause == false || loadSaved == false || useSaved == true)
    {
        return;
    }
    Inital();
    nTail = nTail_saved;
    isFullWidth = isFullWidth_saved;
    x = x_saved;
    y = y_saved;
    fruitX = fruitX_saved;
    fruitY = fruitY_saved;
    score = score_saved;
    dir = dir_saved;
    for (int i = 0; i < 100; i++)
    {
        tailX[i] = tailX_saved[i];
        tailY[i] = tailY_saved[i];
    }
    useSaved = true;
    DrawMap();
    if (!isFullWidth)
        Prompt_info(3, 0);
    else
        Prompt_info(5, 0);
    readSavedData();
}

void Inital(void)
{
    useSaved = false;
    printf("\033[0m");
    system("clear");
    set_disp_mode(STDIN_FILENO, 0);
    printf("\033[?25l");
    gameOver = false;
    isFullWidth = false;
    dir = STOP;
    x = width / 2;
    y = height / 2;
    fruitX = rand() % width;
    fruitY = rand() % height;
    score = 0;
    nTail = 1;
    memset(tailX, 0, sizeof(tailX));
    memset(tailY, 0, sizeof(tailY));
    readSavedData();
}

void Prompt_info(int _x, int _y)
{
    int initialX = 20, initialY = 0;

    printf("\033[0m");
    setPos(_x + initialX, _y + initialY);
    printf("■游戏说明：");
    initialY++;
    setPos(_x + initialX, _y + initialY);
    printf("    A.蛇身自撞，游戏结束");
    initialY++;
    setPos(_x + initialX, _y + initialY);
    printf("    B.蛇可穿墙");
    initialY++;
    initialY++;
    setPos(_x + initialX, _y + initialY);
    printf("■操作说明：");
    initialY++;
    setPos(_x + initialX, _y + initialY);
    printf("    □向左移动：A");
    initialY++;
    setPos(_x + initialX, _y + initialY);
    printf("    □向右移动：D");
    initialY++;
    setPos(_x + initialX, _y + initialY);
    printf("    □向下移动：S");
    initialY++;
    setPos(_x + initialX, _y + initialY);
    printf("    □向上移动：W");
    initialY++;
    setPos(_x + initialX, _y + initialY);
    printf("    □开始游戏：任意方向键");
    initialY++;
    setPos(_x + initialX, _y + initialY);
    printf("    □存档：X键（只能在暂停状态下存档）");
    initialY++;
    setPos(_x + initialX, _y + initialY);
    printf("    □读档：L键（只能在暂停状态下读档）");
    initialY++;
    setPos(_x + initialX, _y + initialY);
    printf("    □暂停：P键");
    initialY += 2;
    setPos(_x + initialX, _y + initialY);
    printf("■存档情况：");
    initialY++;
}

void gameOver_info()
{
    setPos(4, 8);
    printf("\033[31;43m");
    printf("游戏结束!!");
    setPos(2, 9);
    printf("Y 重新开始/N 退出");
}

void showScore(int _x, int _y)
{
    setPos(_x + 22, _y + 14);
    printf("\033[0m");
    if (loadSaved == true)
    {
        printf("已检测到上次存档");
    }
    else
    {
        printf("未检测到上次存档");
    }

    setPos(_x + 22, _y + 15);
    printf("\033[0m");
    if (useSaved)
        printf("已加载存档");

    setPos(_x + 22, _y + 16);
    printf("\033[3m");
    if (isSaved == true)
    {
        printf("已存档");
    }

    setPos(_x + 20, _y + 17);
    printf("\033[0m");
    printf("●当前积分：");
    printf("\033[33m");
    printf("%d\n", score);
    setPos(_x + 20, _y + 19);
    printf("\033[0m");
    printf("●当前难度：");
    printf("\033[33m");
    printf("%d\n", (score / DIFFICULTY_FACTOR + 1));
}

void DrawMap()
{
    system("clear");
    printf("\033[33m");
    setPos(-1, -1);
    for (int i = 0; i < width + 2; i++)
        if (isFullWidth)
            printf("□ ");
        else
            printf("#");

    for (int i = 0; i < height; i++)
    {
        setPos(-1, i);
        for (int j = 0; j < width + 2; j++)
        {
            if (j == 0)
                if (isFullWidth)
                    printf("□");
                else
                    printf("#");
            else if (j == width + 1)
                if (isFullWidth)
                    printf(" □");
                else
                    printf("#");
            else if (isFullWidth)
                printf("  ");
            else
                printf(" ");
        }
        printf("\n");
    }
    setPos(-1, height);
    for (int i = 0; i < width + 2; i++)
        if (isFullWidth)
            printf("□ ");
        else
            printf("#");
    printf("\n");
}

void eraseSnake()
{
    for (int i = 0; i < nTail; i++)
    {
        setPos(tailX[i], tailY[i]);
        printf(" ");
    }
}

void DrawLocally()
{
    setPos(fruitX, fruitY);
    printf("\033[31;5m");
    printf("🍎");

    printf("\033[0m");

    for (int i = 0; i < nTail; i++)
    {
        setPos(tailX[i], tailY[i]);
        if (i == 0)
        {
            printf("\033[34;1m");
            printf("●");
            printf("\033[0m");
        }
        else
        {
            printf("\033[32m");
            printf("●");
            printf("\033[0m");
        }
    }

    setPos(0, height + 1);
}

void Draw(void)
{
    bool flag;
    system("clear");
    for (int i = 0; i < width; i++)
        printf("#");
    printf("\n");

    for (int i = 0; i < height; i++)
    {
        flag = false;
        for (int j = 0; j < width; j++)
        {
            flag = false;
            if (j == 0)
                printf("#");

            if (i == y && j == x)
            {
                printf("\033[34;1m");
                printf("●");
                printf("\033[0m");
                flag = true;
            }
            else if (i == fruitY && j == fruitX)
            {
                printf("\033[31m");
                printf("🍎");
                printf("\033[0m");
                flag = true;
            }
            else
            {
                for (int k = 1; k < nTail; k++)
                {
                    if (tailX[k] == j && tailY[k] == i)
                    {
                        printf("\033[32m");
                        printf("●");
                        printf("\033[0m");
                        flag = true;
                    }
                }
            }
            if (!flag)
                printf(" ");
            if (j == width - 1)
                printf("#");
        }
        printf("\n");
    }
    for (int i = 0; i < width; i++)
    {
        printf("#");
    }
    printf("\n");
    printf("游戏得分:%d\n", score);
}

void Input(void)
{
    if (kbhit())
    {
        char key = getchar();
        switch (key)
        {
        case 'w':
        case 'W':
            if (dir != DOWN)
            {
                dir = UP;
            }
            break;
        case 's':
        case 'S':
            if (dir != UP)
            {
                dir = DOWN;
            }
            break;
        case 'a':
        case 'A':
            if (dir != RIGHT)
            {
                dir = LEFT;
            }
            break;
        case 'D':
        case 'd':
            if (dir != LEFT)
            {
                dir = RIGHT;
            }
            break;
        case 'x':
        case 'X':
            saveData();
            break;
        case 'l':
        case 'L':

            loadData();

            break;
        case ' ':
            isFullWidth = !isFullWidth;
            DrawMap();
            Prompt_info(5, 0);
            break;
        case 'p':
        case 'P':
            isPause = !isPause;
        default:
            break;
        }
    }
}

void Logic()
{
    switch (dir)
    {
    case LEFT:
        // sleep_ms(100);
        x--;
        break;
    case RIGHT:
        // sleep_ms(100);
        x++;
        break;
    case UP:
        // sleep_ms(100);
        y--;
        break;
    case DOWN:
        // sleep_ms(100);
        y++;
        break;
    default:
        break;
    }
    // if (x > width || x < 0 || y > height || y < 0)
    //     gameOver = true;

    if (x == fruitX && y == fruitY)
    {
        score += 10;
        fruitX = rand() % width;
        fruitY = rand() % height;
        nTail++;
    }

    if (x >= width)
        x = 0;
    else if (x < 0)
        x = width - 1;
    if (y >= height)
        y = 0;
    else if (y < 0)
        y = height - 1;

    int prevX = tailX[0];
    int prevY = tailY[0];
    int prev2X, prev2Y;
    tailX[0] = x;
    tailY[0] = y;

    for (int i = 1; i < nTail; i++)
    {
        prev2X = tailX[i];
        prev2Y = tailY[i];
        tailX[i] = prevX;
        tailY[i] = prevY;
        prevX = prev2X;
        prevY = prev2Y;
    }
    for (int i = 1; i < nTail; i++)
    {
        if (tailX[i] == x && tailY[i] == y)
            gameOver = true;
    }
}

int main(void)
{
    const int FRAMES_PER_SECOND = 5;
    const int SKIP_TICKS = 1000 / FRAMES_PER_SECOND;
    const int MAX_FRAMESKIP = 5;
    unsigned long next_Game_Tick = GetTickCount();
    int sleep_Time = 0;
    bool gameQuit = false;
    do
    {
        Inital();
        DrawMap();
        Prompt_info(3, 0);

        while (!gameOver)
        {
            int loops = 0;
            next_Game_Tick += SKIP_TICKS;
            // Draw();

            while (next_Game_Tick - GetTickCount() > 0 && loops < MAX_FRAMESKIP)
            {
                Input();
                eraseSnake();
                if (!isPause)
                    Logic();
                loops += 200 / (score / DIFFICULTY_FACTOR + 1);
            }
            DrawLocally();

            showScore(3, 1);

            sleep_Time = next_Game_Tick - GetTickCount();
            if (sleep_Time >= 0)
                sleep_ms(sleep_Time);
            // sleep_ms(100);
        }
        gameOver_info();
        setPos(0, 23);
        while (gameOver)
        {
            if (kbhit())
            {
                char key = getchar();
                switch (key)
                {
                case 'y':
                case 'Y':
                    gameOver = false;
                    printf("\033[0m");
                    printf("\033[2J");
                    // system("clear");
                    break;
                case 'n':
                case 'N':
                    gameOver = false;
                    gameQuit = true;
                    break;
                default:
                    break;
                }
            }
        }
    } while (!gameQuit);
    printf("\n");
    printf("\033[0m");
    return 0;
}