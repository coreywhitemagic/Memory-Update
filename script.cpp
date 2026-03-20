#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <cctype>
#include <limits>
#include <cstdlib>
#include <ctime>
#include <random>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

using namespace std;

// Normalize string
string normalize(const string& str) {
    string result;
    for (char c : str) {
        if (isalnum(static_cast<unsigned char>(c))) {
            result += tolower(static_cast<unsigned char>(c));
        }
    }
    return result;
}

// Similarity (Levenshtein)
double similarityPercentage(const string& s1, const string& s2) {
    string str1 = normalize(s1);
    string str2 = normalize(s2);
    size_t len1 = str1.size(), len2 = str2.size();
    if (len1 == 0 && len2 == 0) return 100.0;
    if (len1 == 0 || len2 == 0) return 0.0;
    vector<vector<int>> dp(len1 + 1, vector<int>(len2 + 1));
    for (size_t i = 0; i <= len1; i++) dp[i][0] = i;
    for (size_t j = 0; j <= len2; j++) dp[0][j] = j;
    for (size_t i = 1; i <= len1; i++) {
        for (size_t j = 1; j <= len2; j++) {
            dp[i][j] = min({
                dp[i - 1][j] + 1,
                dp[i][j - 1] + 1,
                dp[i - 1][j - 1] + (str1[i - 1] == str2[j - 1] ? 0 : 1)
            });
        }
    }
    int maxLen = max(len1, len2);
    return 100.0 - (double)dp[len1][len2] / maxLen * 100.0;
}

// Clear screen
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Wait for any key
void waitForKey() {
#ifdef _WIN32
    _getch();
#else
    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    int ch = getchar(); // store result to avoid warning
    (void)ch;
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
}

// Compact help
void showHelpScreen() {
    clearScreen();
    cout << "\033[37m=== HELP ===\n\n";
    cout << "Navigation:\n\n";
    cout << " n/Enter - next\n";
    cout << " p       - previous\n";
    cout << " s       - start\n";
    cout << " r       - random\n\n";
    cout << "Actions:\n\n";
    cout << " d - show answer\n";
    cout << " m - mark as memorized\n";
    cout << " v - skip memorized questions\n";
    cout << " j - jump to question\n";
    cout << " f - fuzzy %\n";
    cout << " t - statistics\n";
    cout << " c - clear\n\n";
    cout << "Other:\n\n";
    cout << " h - help\n";
    cout << " q - quit\n";
    cout << "\nPress any key...\033[0m";
    waitForKey();
    clearScreen();
}

// Stats
void displayStatistics(const vector<bool>& memorized,
                       const vector<int>& randomDeck,
                       int deckIndex,
                       double fuzzySuccessPercentage,
                       const vector<bool>& correctAnswers,
                       const vector<bool>& wrongAnswers) {
    int total = memorized.size() / 2;
    int mem = 0, correct = 0, wrong = 0;
    for (size_t i = 0; i < memorized.size(); i += 2) {
        if (memorized[i]) mem++;
        if (correctAnswers[i]) correct++;
        if (wrongAnswers[i]) wrong++;
    }
    int answered = correct + wrong;
    int remainingDeck = 0;
    for (size_t i = deckIndex; i < randomDeck.size(); i++) {
        if (!memorized[randomDeck[i]]) remainingDeck++;
    }
    double correctPercent = answered ? (correct * 100.0 / answered) : 0.0;
    double wrongPercent   = answered ? (wrong * 100.0 / answered) : 0.0;
    cout << "\033[36m--- Stats ---\n";
    cout << "Total: " << total << "\n";
    cout << "Mem: " << mem << " (" << (total ? mem * 100.0 / total : 0) << "%)\n";
    cout << "Correct: " << correct << " (" << correctPercent << "%)\n";
    cout << "Wrong: " << wrong << " (" << wrongPercent << "%)\n";
    cout << "Deck left: " << remainingDeck << "\n";
    cout << "Fuzzy: " << fuzzySuccessPercentage << "%\n";
    cout << "-------------\033[0m\n\n";
}

int main(int argc, char* argv[]) {
    srand(time(nullptr));
    clearScreen();

    string fileName;
    if (argc > 1) fileName = argv[1];
    else {
        cout << "Script File: ";
        getline(cin, fileName);
    }

    ifstream file(fileName);
    if (!file) {
        cerr << "Error opening file.\n";
        return 1;
    }

    vector<string> answers;
    string line;
    while (getline(file, line)) answers.push_back(line);
    file.close();

    vector<bool> memorized(answers.size(), false);
    vector<bool> correct(answers.size(), false);
    vector<bool> wrong(answers.size(), false);

    int q = 0;
    bool skipMode = false;
    double fuzzy = 50.0;
    vector<int> deck;
    int deckIndex = 0;
    string input;

    cout << "\033[0m(h for help)\n\n";

    while (q >= 0 && q < (int)answers.size() - 1) {
        if (skipMode && memorized[q]) {
            int start = q;
            do {
                q += 2;
                if (q >= (int)answers.size()) q = 0;
            } while (memorized[q] && q != start);
        }

        cout << "\033[33m" << answers[q];
        if (memorized[q]) cout << " [MEM]";
        if (skipMode) cout << " [SKIP]";
        cout << "\033[0m\n\n> ";

        getline(cin, input);
        clearScreen();
        string cmd = normalize(input);

        if (cmd == "h") {
            showHelpScreen();
            continue;
        }
        else if (cmd == "q") break;
        else if (cmd == "n" || input.empty()) q += 2;
        else if (cmd == "p") q -= 2;
        else if (cmd == "s") q = 0;
        else if (cmd == "d") {
            cout << answers[q + 1] << "\n\n";
        }
        else if (cmd == "m") {
            memorized[q] = !memorized[q];
        }
        else if (cmd == "v") {
            skipMode = !skipMode;
        }
        else if (cmd == "c") {
            clearScreen();
        }
        else if (cmd == "t") {
            displayStatistics(memorized, deck, deckIndex, fuzzy, correct, wrong);
        }
        else if (cmd == "f") {
            double val;
            cout << "New %: ";
            cin >> val;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            if (val >= 0 && val <= 100) fuzzy = val;
        }
        else if (cmd == "r") {
            if (deckIndex >= (int)deck.size()) {
                deck.clear();
                for (int i = 0; i < (int)answers.size(); i += 2) {
                    if (!skipMode || !memorized[i])
                        deck.push_back(i);
                }
                shuffle(deck.begin(), deck.end(), default_random_engine(rand()));
                deckIndex = 0;
            }
            if (!deck.empty()) q = deck[deckIndex++];
        }
        else if (cmd == "j") {
            cout << "Search: ";
            string s;
            getline(cin, s);
            string sn = normalize(s);
            for (size_t i = 0; i < answers.size(); i += 2) {
                if (normalize(answers[i]).find(sn) != string::npos) {
                    q = i;
                    break;
                }
            }
        }
        else {
            // Check answer similarity but DO NOT auto-advance
            double sim = similarityPercentage(input, answers[q + 1]);
            if (sim >= fuzzy) {
                cout << "Correct (" << sim << "%)\n\n";
                correct[q] = true;
            } else {
                cout << "\033[31mWrong (" << sim << "%)\n"
                     << answers[q + 1] << "\033[0m\n\n";
                wrong[q] = true;
            }
        }

        if (q >= (int)answers.size()) q = 0;
        if (q < 0) q = (int)answers.size() - 2;
    }

    cout << "Done.\n";
    return 0;
}
