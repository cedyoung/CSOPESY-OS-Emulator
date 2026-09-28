// Group 4: Jediaelle Denise De Castro, Cedric Young, Abigail Vicencio
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <conio.h>

using namespace std;

atomic<int> marqueeSpeed(50);
atomic<bool> marqueeRunning(false);
atomic<bool> marqueeScreenActive(false);

thread marqueeThread;
mutex consoleMutex;

const int MARQUEE_WIDTH = 70;
const int MARQUEE_HEIGHT = 15;
const int STATUS_ROW = 17;
const int COMMAND_ROW = 19;


const size_t INPUT_MAX_WIDTH = 79;

void displayHeader() {
    cout << R"(
  _____  _____  ____  _____  ______  _______     __
 / ____|/ ____|/ __ \|  __ \|  ____|/ ____\ \   / /
| |    | (___ | |  | | |__) | |__  | (___  \ \_/ /
| |     \___ \| |  | |  ___/|  __|  \___ \  \   /
| |____ ____) | |__| | |    | |____ ____) |  | |
 \_____|_____/ \____/|_|    |______|_____/   |_|

)";

    cout << "Hello, Welcome to CSOPESY commandLine!" << endl;
    cout << "Type 'exit' to quit, 'clear' to clear the screen" << endl;
    cout << "** IMPORTANT: Type 'initialize' to load config and start system **" << endl;
}

void clearScreen() {
    cout << "\033[2J\033[1;1H";
}

void moveCursor(int x, int y) {
    cout << "\033[" << y << ";" << x << "H";
}

void clearLine(int row) {
    moveCursor(1, row);
    cout << "\033[2K";
}

void showStatus(string message) {
    lock_guard<mutex> lock(consoleMutex);

    clearLine(STATUS_ROW);
    moveCursor(1, STATUS_ROW);
    cout << message << flush;
}


int readKey() {
    int c = _getch();


    if (c == 0 || c == 224) {
        _getch();
        return 0;
    }

    return c;
}

bool readLine(const string& prompt, string& result) {
    string line;


    auto redraw = [&]() {
        if (marqueeScreenActive) {
            moveCursor(1, COMMAND_ROW);
        }
        cout << "\r\033[2K" << prompt << line << flush;
    };

    {
        lock_guard<mutex> lock(consoleMutex);
        redraw();
    }

    while (true) {
        int key = readKey();

        if (key < 0) {
            return false;
        }

        lock_guard<mutex> lock(consoleMutex);

        if (key == '\r' || key == '\n') {
            if (marqueeScreenActive) {
                moveCursor(1, COMMAND_ROW + 1);
            }
            else {
                cout << '\n';
            }
            cout << flush;
            break;
        }
        else if (key == 8 || key == 127) {
            if (!line.empty()) {
                line.pop_back();
                redraw();
            }
        }
        else if (key >= 32 && key < 127 &&
                 prompt.length() + line.length() < INPUT_MAX_WIDTH) {
            line += static_cast<char>(key);
            redraw();
        }
    }

    result = line;
    return true;
}

//getSet = true to get text, getSet = false to set text
string marqueeText(bool getSet, string text){
    static string marqueeText;

    if(getSet){
        return marqueeText;
    }else{
        marqueeText = text;
        return "";
    }

}



void marqueeAnimation() {
    int x = 1;
    int y = 1;
    int dx = 1;
    int dy = 1;

    int oldX = x;
    int oldY = y;

    string oldText = "";

    while (marqueeRunning) {
        string text = marqueeText(true, "");

        {
            lock_guard<mutex> lock(consoleMutex);


            cout << "\0337";


            if (!oldText.empty()) {
                moveCursor(oldX, oldY);
                cout << string(oldText.length(), ' ');
            }


            moveCursor(x, y);
            cout << text;


            cout << "\0338" << flush;
        }

        oldX = x;
        oldY = y;
        oldText = text;

        x += dx;
        y += dy;

        if (x <= 1) {
            x = 1;
            dx = 1;
        }
        else if (x + text.length() >= MARQUEE_WIDTH) {
            x = MARQUEE_WIDTH - text.length();
            dx = -1;
        }

        if (y <= 1) {
            y = 1;
            dy = 1;
        }
        else if (y >= MARQUEE_HEIGHT) {
            y = MARQUEE_HEIGHT;
            dy = -1;
        }

        this_thread::sleep_for(
            chrono::milliseconds(marqueeSpeed.load())
        );
    }
}

void startMarquee() {
    if (!marqueeRunning) {
        if (!marqueeScreenActive) {
            clearScreen();
            marqueeScreenActive = true;
        }

        marqueeRunning = true;
        marqueeThread = thread(marqueeAnimation);
        showStatus("Marquee started.");
    }

    else {
        showStatus("Marquee is already running.");
    }
}

void stopMarquee() {
    if (marqueeRunning) {
        marqueeRunning = false;

        if (marqueeThread.joinable()) {
            marqueeThread.join();
        }

        showStatus("Marquee stopped.");
    }

    else {
        showStatus("Marquee is not running.");
    }
}

void printMessage(string message) {
    if (marqueeScreenActive) {
        showStatus(message);
    }
    else {
        cout << message << endl;
    }
}

void setSpeed() {
    string input;


    if (!readLine("Enter speed (ms): ", input)) {
        return;
    }

    try {
        int newSpeed = stoi(input);

        if (newSpeed <= 0) {
            printMessage("Speed must be a positive integer.");
            return;
        }

        marqueeSpeed = newSpeed;
        printMessage("Marquee speed set to " + to_string(newSpeed) + " ms.");

    }
    catch (const exception&) {
        printMessage("Please enter a valid integer.");
    }
}

int main() {
    string command;
    string text;
    displayHeader();

    while (true) {

        if (!readLine("Enter a command: ", command)) {
            break;
        }

        // commands
        if (command == "initialize") {
            printMessage("initialize command recognized. Doing something.");
        }

        else if (command == "help") {
            cout << "\"help\" - displays the commands and its description.\n"
                <<  "\"start_marquee\" - starts the marquee \"animation\" \n"
                <<  "\"stop_marquee\" - stops the marquee \"animation\" \n"
                <<  "\"set_text\" - accepts a text input and displays it as a marquee\n" 
                <<  "\"set_speed\" - sets the marquee animation refresh in milliseconds\n"
                << "\"clear\" - clears the screen\n"
                <<  "\"exit\" - terminates the console"
                << endl;
        }

        else if (command == "screen") {
            cout << "screen command recognized. Doing something." << endl;
        }

        else if (command == "scheduler-start") {
            cout << "scheduler-start command recognized. Doing something." << endl;
        }

        else if (command == "scheduler-stop") {
            cout << "scheduler-stop command recognized. Doing something." << endl;
        }

        else if (command == "report-util") {
            cout << "report-util command recognized. Doing something." << endl;
        }

        else if (command == "start_marquee") {
            startMarquee();
        }

        else if (command == "stop_marquee") {
            stopMarquee();
        }

        else if (command == "set_speed") {
            setSpeed();
        }

        else if (command == "set_text") {
            if (readLine("Set Marquee Text: ", text)) {
                marqueeText(false, text);
                printMessage("Marquee text changed.");
            }
        }

        else if (command == "clear") {
            clearScreen();
            displayHeader();
        }

        else if (command == "exit") {
            break;
        }

        else {
            printMessage("Command not recognized.");
        }
    }

    return 0;
}
