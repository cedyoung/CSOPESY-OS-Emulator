// Group 4: Jediaelle Denise De Castro, Cedric Young, Abigail Vicencio
#include <iostream>
#include <string>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <atomic>

using namespace std;

atomic<int> marqueeSpeed(50);
atomic<bool> marqueeRunning(false);
thread marqueeThread;

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


void moveCursor(int x, int y) {
    cout << "\033[" << y << ";" << x << "H";
}

void marqueeAnimation() {
    int x = 1;
    int y = 1;
    int dx = 1;
    int dy = 1;

    while (marqueeRunning) {
        cout << "\033[2J\033[H";
        
        // Move cursor to y, x
        cout << "\033[" << y << ";" << x << "H";

        cout << marqueeText(true, "") << flush;
        x += dx;
        y += dy;

        // Bounce left/right
        if (x <= 1 || x >= 70) {
            dx *= -1;
        }
        // Bounce top/bottom
        if (y <= 1 || y >= 20) {
            dy *= -1;
        }

        this_thread::sleep_for(
            chrono::milliseconds(marqueeSpeed.load)
        );
    }
}

void startMarquee() {
    if (!marqueeRunning) {
        marqueeRunning = true;
        marqueeThread = thread(marqueeAnimation);
        cout << "Marquee started." << endl;
    }

    else {
        cout << "Marquee is already running." << endl;
    }
}

void stopMarquee() {
    if (marqueeRunning) {
        marqueeRunning = false;

        if (marqueeThread.joinable()) {
            marqueeThread.join();
        }

        cout << "Marquee stopped." << endl;
    }

    else {
        cout << "Marquee is not running." << endl;
    }
}

void setSpeed() {
    string command;

    cout << "Enter speed (ms): ";
    getline(cin, command);

    try {
        int newSpeed = stoi(command);

        if (newSpeed <= 0) {
            cout << "Speed must be a positive integer." << endl;
            return;

        }

        marqueeSpeed = newSpeed;
        cout << "Marquee speed set to " << newSpeed << " ms." << endl;

    }
    catch (const exception&) {
        cout << "Please enter a valid integer." << endl;
    }
}

int main() {
    string command;
    string text;
    displayHeader();

    while (true) {

        cout << "Enter a command: ";
        getline(cin, command);

        if (command == "initialize") {
            cout << "initialize command recognized. Doing something." << endl;
        }

        else if (command == "help") {
            cout << "\"help\" - displays the commands and its description.\n"
                <<  "\"start_marquee\" - starts the marquee \"animation\" \n"
                <<  "\"stop_marquee\" - stops the marquee \"animation\" \n"
                <<  "\"set_text\" - accepts a text input and displays it as a marquee\n" 
                <<  "\"set_speed\" - sets the marquee animation refresh in milliseconds\n"
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

        else if(command == "set_text"){
            cout << "Set Marquee Text: ";
            getline(cin, text);
            marqueeText(false, text);
        }

        else if (command == "clear") {
            clearScreen();
            displayHeader();
        }

        else if (command == "exit") {
            break;
        }

        else {
            cout << "Command not recognized." << endl;
        }

        cout << endl;
    }

    return 0;
}
