// De Castro, Jediaelle Denise & Young, Cedric
// compile : clang++ -std=c++11 week2.cpp -o week2
#include <iostream>
#include <string>
#include <cstdlib>

using namespace std;

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
    // clearing the terminal for Windows and macOS
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif

}

int main() {
    string command;
    displayHeader();

    while (true) {

        cout << "Enter a command: ";
        getline(cin, command);

        if (command == "initialize") {
            cout << "initialize command recognized. Doing something." << endl;
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