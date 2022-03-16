/*
 * (c) Cranium, aka Череп. 2014
 * GNU GPL
 *
 */

#include "CGame.h"
#include <vector>
#include <iostream>
#include <cstring>
#include <conio.h>
#include <cstdio>
#include <math.h>


// форматная строка для форматирования результата игры
const char *recordFormatStr = "%-15s  %9.4f  %4u  %7.2f  %s";

SRecord::SRecord() {
    name[0] = '\0';
    rating = 0.0;
    length = 0;
    game_time = 0;
    date = static_cast<time_t>(0);
}

void SRecord::as_string(char *buffer) {
    sprintf(buffer, recordFormatStr, name, rating, length, game_time, ctime(&date));
}

ostream& operator << (ostream& os, const SRecord& rec) {
    os
        << rec.rating << ' '
        << rec.length << ' '
        << rec.game_time << ' '
        << rec.date << ' '
        << rec.name << endl;
    return os;
}

istream& operator >> (istream& is, SRecord& rec) {
    is
        >> rec.rating
        >> rec.length
        >> rec.game_time
        >> rec.date;
    is.ignore(1);
    is.getline(&rec.name[0], 16);
    return is;
}

// Функция сравнения результатов по рейтингу.
// Необходима для работы qsort() для сортировки по убыванию.
int rec_compare(const void *_op1, const void *_op2) {
    const SRecord *op1 = reinterpret_cast<const SRecord *>(_op1);
    const SRecord *op2 = reinterpret_cast<const SRecord *>(_op2);
    return static_cast<int>(op2->rating - op1->rating);
}

/*----- end of struct SRecord -------------------------------------*/

// очистка буфера клавиатуры
void clearkeys() {
    while (_kbhit())
        _getch();
}

// Конструктор
// _scr     - объект, осуществляющий вывод на консоль
// _width   - ширина игрового поля (x)
// _height  - высота игрового поля (y)
// _latency - задержка между изменением позиции в миллисекундах

CGame::CGame(CScreen& _scr, int _width, int _height, int _latency) :
    width(_width), height(_height), latency(_latency), scr(_scr) {

    srand(static_cast<unsigned int>(time(NULL)));

    duration_game = 0;
    rating = 0.0;

    // инициализация таблицы команд управления игрой
    cmd_table[0] = CmdPair(27, CMD_EXIT);   // escape key
    cmd_table[1] = CmdPair('K', CMD_LEFT);  // стрелка влево
    cmd_table[2] = CmdPair('M', CMD_RIGHT); // стрелка вправо
    cmd_table[3] = CmdPair('H', CMD_UP);    // стрелка вверх
    cmd_table[4] = CmdPair('P', CMD_DOWN);  // стрелка вниз
}

CGame::Command CGame::get_command() {
    int ch;

    ch = _getch();
    if (ch == 0 || ch == 0xe0) {
        ch = _getch();
    }

    for (int i = 0; i < 5; i++) {
        if (cmd_table[i].first == ch) {
            return cmd_table[i].second;
        }
    }
    return CMD_NOCOMMAND;
}

// Координата еды вычисляется случайным образом.
// Ограничение: координата не должна попадать в тело змеи.
SCoord CGame::make_food() {
    SCoord food;
    do {
        food.x = rand() % (width - 2) + 1;
        food.y = rand() % (height - 2) + 1;
    } while (snake.into(food));

    return food;
}

const char BORDER = '#';    // символ для рисования рамки игрового поля

void CGame::draw_field() {

    scr.cls();

    for (int y = 0; y < height; y++) {
        if (y == 0 || y == height - 1) {
            for (int x = 0; x < width; x++)
                scr.pos(x, y, BORDER);
        }
        else {
            scr.pos(0, y, BORDER);
            scr.pos(width - 1, y, BORDER);
        }
    }
    scr.pos(0, height);
    _cprintf("Length: ****  Rating: ****.**** (****.****)  Time: ****.**");
}

void CGame::print_stat() {
    scr.pos(8, height);
    _cprintf("%04u", snake.size());
    scr.pos(22, height);
    _cprintf("%09.4f", rating);
    scr.pos(33, height);
    _cprintf("%09.4f", rating_i);
    scr.pos(51, height);
    _cprintf("%07.2f", duration_game);
}

void CGame::top10_table() {
    scr.cls();
    char buf[80];

    scr.pos_str(width / 2 - 12, 2, "***** T O P    1 0 *****");
    scr.pos_str(5, 4, "Name              Rating    Length  Time   Date");

    for (int i = 0; i < 10; i++) {
        ttop10[i].as_string(buf);
        scr.pos_str(5, 5 + i, buf);
    }
}

void CGame::top10(bool after_game) {

    char buf[80];
    char buf_encoded[NAMELENGTH];

    top10_table();      // показать таблицу 10 лучших результатов

    time_t date = time(NULL);
    if (after_game) {
        // если игра была сыграна, то показать текущий результат
        scr.pos(5, 16);
        _cprintf(recordFormatStr, "Your result", rating, snake.size(), duration_game, ctime(&date));
    }

    if (rating > ttop10[9].rating) {    // если рейтинг игры больше, чем меньший из 10 лучших...
        // запросить имя игрока
        scr.pos_str(5, 20, "Your name: _");
        scr.pos(16, 20);
        cin.getline(&buf[0], NAMELENGTH);
        clearkeys();
        OemToCharBuff(buf, buf_encoded, static_cast<DWORD>(NAMELENGTH));
        // заменить последнюю запись в таблице 10 лучших результатов
        strcpy(ttop10[9].name, buf_encoded);
        ttop10[9].date = date;
        ttop10[9].game_time = duration_game;
        ttop10[9].length = snake.size();
        ttop10[9].rating = rating;
        // отсортировать результаты по убыванию
        qsort(ttop10, 10, sizeof(SRecord), rec_compare);
        // обновить таблицу на экране
        top10_table();

        // обновить файл с 10 лучшими результатами
        write_top10();
    }
}

void CGame::pak(int y) {
    scr.pos_str(width / 2 - 15, y, "Press any key for continue...");
    _getch();
    clearkeys();
}

bool CGame::once_more() {
    scr.pos_str(width / 2 - 12, height - 3, "O n c e    m o r e ?");

    int ch = _getch();
    clearkeys();
    if (ch == 'N' || ch == 'n' || ch == 27)
        return false;
    return true;
}

const char *top10_filename = "snake.dat";   // имя файла для хранения 10 лучших результатов

void CGame::read_top10() {
    ifstream fin(top10_filename);
    if (fin) {
        for (int i = 0; i < 10; i++)
            fin >> ttop10[i];
    }
    fin.close();
}

void CGame::write_top10() {
    ofstream fout(top10_filename);
    if (fout) {
        for (int i = 0; i < 10; i++)
            fout << ttop10[i];
    }
    fout.close();
}

const char *ver_number = "v 1.1";
const char *copyright = "(c) Cranium, 2014.";

void CGame::logo() {
    scr.pos_str(width / 2 - 9, 10, "O l d s c h o o l");
    scr.pos_str(width / 2 - 7, 12, "S  N  A  K  E");
    scr.pos_str(width / 2 - 3, 16, ver_number);
    scr.pos_str(width / 2 - 9, height, copyright);
    pak(22);
}

void CGame::goodbye() {
    scr.cls();
    _cprintf("Oldschool Snake %s\n%s\n", ver_number, copyright);
}

const char FOOD = '$';      // символ для вывода еды


void CGame::print_correct_way(vector<int> correct_way) {
    string up = std::to_string(correct_way[0]);
    string right = std::to_string(correct_way[1]);
    string down = std::to_string(correct_way[2]);
    string left = std::to_string(correct_way[3]);

    scr.pos_str(4, height + 5, up.c_str());
    scr.pos_str(5, height + 5, right.c_str());
    scr.pos_str(6, height + 5, down.c_str());
    scr.pos_str(7, height + 5, left.c_str());

}

void CGame::print_input(vector<float> input) {
    string apple_above_snake = std::to_string(input[0]);
    string apple_right_snake = std::to_string(input[1]);
    string apple_below_snake = std::to_string(input[2]);
    string apple_left_snake = std::to_string(input[3]);
    string obstacle_above_snake = std::to_string(input[4]);
    string obstacle_right_snake = std::to_string(input[5]);
    string obstacle_below_snake = std::to_string(input[6]);
    string obstacle_left_snake = std::to_string(input[7]);
    string snake_dir_up =  std::to_string(input[8]);
    string snake_dir_right =  std::to_string(input[9]);
    string snake_dir_down =  std::to_string(input[10]);
    string snake_dir_left =  std::to_string(input[11]);
    string food_x = std::to_string(input[12]);
    string food_y = std::to_string(input[13]);
    string hd_x = std::to_string(input[14]);
    string hd_y = std::to_string(input[15]);


    scr.pos_str(1, height + 1, apple_above_snake.c_str());
    scr.pos_str(2, height + 1, apple_right_snake.c_str());
    scr.pos_str(3, height + 1, apple_below_snake.c_str());
    scr.pos_str(4, height + 1, apple_left_snake.c_str());
    scr.pos_str(5, height + 1, obstacle_above_snake.c_str());
    scr.pos_str(6, height + 1, obstacle_right_snake.c_str());
    scr.pos_str(7, height + 1, obstacle_below_snake.c_str());
    scr.pos_str(8, height + 1, obstacle_left_snake.c_str());
    scr.pos_str(9, height + 1, snake_dir_up.c_str());
    scr.pos_str(10, height + 1, snake_dir_right.c_str());
    scr.pos_str(11, height + 1, snake_dir_down.c_str());
    scr.pos_str(12, height + 1, snake_dir_left.c_str());
    scr.pos_str(13, height + 1, food_x.c_str());
    scr.pos_str(13, height + 2, food_y.c_str());
    scr.pos_str(13, height + 3, hd_x.c_str());
    scr.pos_str(13, height + 4, hd_y.c_str());
}

float distance(int x1,int y1,int x2,int y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float distance = sqrt(dx*dx + dy*dy);
    return distance;
}

vector<int> CGame::correct_way(vector<float> info, SCoord food, SCoord head_act){
    int up = 0;
    int down = 0;
    int right = 0;
    int left = 0;
    SCoord head = head_act;
    SCoord head_up = head + SCoord(0,1);
    SCoord head_right = head + SCoord(1,0);
    SCoord head_down = head + SCoord(0,-1);
    SCoord head_left = head + SCoord(-1,0);
    float now_distance = distance(head.x, head.y, food.x, food.y);
    float head_up_distance = distance(head_up.x, head_up.y, food.x, food.y);
    float head_right_distance = distance(head_right.x, head_right.y, food.x, food.y);
    float head_down_distance = distance(head_down.x, head_down.y, food.x, food.y);
    float head_left_distance = distance(head_left.x, head_left.y, food.x, food.y);
    float delta_up_distance = now_distance - head_up_distance;
    float delta_right_distance = now_distance - head_right_distance;
    float delta_down_distance = now_distance - head_down_distance;
    float delta_left_distance = now_distance - head_left_distance;

    vector<float> delta_distances = {delta_up_distance, delta_right_distance, delta_down_distance, delta_left_distance};

    float min_delta_distance = 100.00;

    for (int i = 1; i <= 4; i++) {
        if (delta_distances[i] < min_delta_distance && (int)info[3+i] == 0){
            if (i == 1) {up = 1, right=0, down=0, left=0;}
            if (i == 2) {up = 0, right=1, down=0, left=0;}
            if (i == 3) {up = 0, right=0, down=1, left=0;}
            if (i == 4) {up = 0, right=0, down=0, left=1;}
        }
    }
    vector<int> out = {up, right, down, left};
    return out;

}

vector<float> CGame::info(SCoord food) {
    float apple_above_snake = 0;
    float apple_right_snake = 0;
    float apple_below_snake = 0;
    float apple_left_snake = 0;
    float obstacle_above_snake = 0;
    float obstacle_right_snake = 0;
    float obstacle_below_snake = 0;
    float obstacle_left_snake = 0;
    float snake_dir_up = 0;
    float snake_dir_right = 0;
    float snake_dir_down = 0;
    float snake_dir_left = 0;

    SCoord hd = snake.head();
    SCoord hd_pl_x = hd + SCoord(1,0);
    SCoord hd_mn_x = hd + SCoord(-1,0);
    SCoord hd_pl_y = hd + SCoord(0,1);
    SCoord hd_mn_y = hd + SCoord(0,-1);

    if (hd.y == 1 || snake.into(hd_mn_y)) obstacle_above_snake = 1;
    if (hd.x == 1 || snake.into(hd_mn_x)) obstacle_left_snake = 1;
    if (hd.y == height-2 || snake.into(hd_pl_y)) obstacle_below_snake = 1;
    if (hd.x == width-2 || snake.into(hd_pl_x)) obstacle_right_snake = 1;

    if (hd.x - food.x < 0) apple_right_snake = 1;
    if (hd.y - food.y > 0) apple_above_snake = 1;
    if (hd.x - food.x > 0) apple_left_snake = 1;
    if (hd.y - food.y < 0) apple_below_snake = 1;

    if (snake.head() - snake.worm[snake.worm.size()-2] == SCoord(1,0)) snake_dir_right = 1;
    if (snake.head() - snake.worm[snake.worm.size()-2] == SCoord(-1,0)) snake_dir_left = 1;
    if (snake.head() - snake.worm[snake.worm.size()-2] == SCoord(0,1)) snake_dir_down = 1;
    if (snake.head() - snake.worm[snake.worm.size()-2] == SCoord(0,-1)) snake_dir_up = 1;

    vector<float> out = {apple_above_snake, apple_right_snake, apple_below_snake, apple_left_snake,
                       obstacle_above_snake, obstacle_right_snake, obstacle_below_snake, obstacle_left_snake,
                       snake_dir_up, snake_dir_right, snake_dir_down, snake_dir_left,
                         (float)food.x/(float)(width-2), (float)food.y/(float)(height-2), (float)hd.x/(float)(width-2), (float)hd.y/(float)(height-2)};
    return out;
}

void CGame::game_loop() {

    duration_game = 0;
    rating = rating_i = 0.0;

    draw_field();           // нарисовать игровое поле

    snake.reset(SCoord(width / 2, height / 2));     // установить змею: длина 2,
                                                    // положение - в середине игрового поля,
                                                    // направление - влево
    Command cmd = CMD_NOCOMMAND;
    State stt = STATE_OK;
    // delta  содержит приращение координат (dx, dy) для каждого перемещения змеи по полю
    SCoord delta(-1, 0);                // начальное движение - влево
    SCoord food = make_food();          // вычислить координаты еды
    scr.pos(food.x, food.y, FOOD);      // вывести еду на экран

    snake.draw(scr);                    // первичное рисование змеи

    print_stat();                       // вывести начальную статистику игры

    clock_t time1, time2, duration;
    time1 = clock();

    do {

        if (_kbhit())                   // если в буфере клавиатуры есть информация,
            cmd = get_command();        // то принять команду

        // обработка команд


        switch (cmd) {
        case CMD_LEFT:
            delta = SCoord(-1, 0);
            break;
        case CMD_RIGHT:
            delta = SCoord(1, 0);
            break;
        case CMD_UP:
            delta = SCoord(0, -1);
            break;
        case CMD_DOWN:
            delta = SCoord(0, 1);
            break;
        case CMD_EXIT:
            stt = STATE_EXIT;
        default:
            break;
        };

        vector<float> input = info(food);
        print_input(input);

        vector<int> correct_w = correct_way(input, food, snake.head());
        print_correct_way(correct_w);



        SCoord hd = snake.head();       // координата головы змеи
        hd += delta;                    // координата головы змеи после приращения (следующая позиция)
        // если голова змеи столкнулась с границей поля или с телом змеи, то змея умирает
        if (hd.x == 0 || hd.x == width-1 || hd.y == 0 || hd.y == height-1 || snake.into(hd))
            stt = STATE_DIED;

        if (stt == STATE_OK) {          // если змея ещё жива, то
            snake.move(delta, scr);     // сдвинуть змею на delta

            if (snake.head() == food) { // если координата головы змеи совпадает с координатой еды, то
                snake.grow(food, 3);    // увеличить длину змеи
                food = make_food();     // вычислить координаты новой еды
                scr.pos(food.x, food.y, FOOD); // вывести еду на экран

                // Вычисление времени игры, частичного и общего рейтинга.
                // Частичный рейтинг вычисляется как длина змеи, делённая на время в секундах,
                // затраченное на подход к еде (время от съедения предыдущей еды до съедения следующей).
                // Таким образом, чем чаще змея ест и чем она длиннее, тем выше частичный рейтинг.
                time2 = clock();
                duration = time2 - time1;
                duration_game += static_cast<double>(duration) / CLOCKS_PER_SEC;
                rating_i = static_cast<double>(snake.size()) / duration * CLOCKS_PER_SEC;
                rating += rating_i;     // общий рейтинг - сумма частичных рейтингов за игру
                time1 = time2;

                print_stat();           // вывод текущей статистики игры
            }

            Sleep(latency);             // задержка перед следующим изменением позиции
        }

    } while (stt == STATE_OK);          // играем, пока змея жива

    //scr.pos_str(width / 2 - 8, 10, " G a m e    o v e r ");
    clearkeys();
    _getch();
    clearkeys();
}
