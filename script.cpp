#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <cctype>
#include <limits>
#include <cstdlib>
#include <ctime>

using namespace std;

// Convert string to lowercase and remove spaces/punctuation
string normalize(const string& str) {
    string result;
    for (char c : str) {
        if (isalnum(static_cast<unsigned char>(c))) {
            result += tolower(static_cast<unsigned char>(c));
        }
    }
    return result;
}

// Compute similarity percentage between two strings (normalized)
double similarityPercentage(const string& s1, const string& s2) {
    string str1 = normalize(s1);
    string str2 = normalize(s2);

    const size_t len1 = str1.size();
    const size_t len2 = str2.size();

    if (len1 == 0 && len2 == 0) return 100.0;
    if (len1 == 0 || len2 == 0) return 0.0;

    vector<vector<int>> dp(len1 + 1, vector<int>(len2 + 1));
    for (size_t i = 0; i <= len1; ++i) dp[i][0] = i;
    for (size_t j = 0; j <= len2; ++j) dp[0][j] = j;

    for (size_t i = 1; i <= len1; ++i) {
        for (size_t j = 1; j <= len2; ++j) {
            dp[i][j] = min({
                dp[i - 1][j] + 1,
                dp[i][j - 1] + 1,
                dp[i - 1][j - 1] + (str1[i - 1] == str2[j - 1] ? 0 : 1)
            });
        }
    }

    int maxLen = max(len1, len2);
    return 100.0 - (static_cast<double>(dp[len1][len2]) / maxLen * 100.0);
}

// Function to clear screen
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Function to display help menu in a single line
void displayHelp(bool showHint) {
    if (!showHint) return;
    cout << "\033[37mHelp Menu: Enter/n = next, p = previous, s = start, d = display answer, r = random (skips memorized), m = toggle memorized, j = search, c = clear, h = help, q = quit, f = fuzzy.\033[0m\n\n";
}

int main(int argc, char* argv[]) {

    srand(static_cast<unsigned int>(time(nullptr)));

    clearScreen();

    string answersFileName;
    if (argc > 1) answersFileName = argv[1];
    else {
        cout << "Script File: ";
        getline(cin, answersFileName);
    }

    ifstream answersFile(answersFileName);
    if (!answersFile) {
        cerr << "Error opening answers file." << endl;
        return 1;
    }

    vector<string> answers;
    string line;
    while (getline(answersFile, line)) answers.push_back(line);
    answersFile.close();

    // 🔥 Memorized flags (only for questions = even indices)
    vector<bool> memorized(answers.size(), false);

    string userAnswer;
    int questionNumber = 0;
    bool showHint = true;
    double fuzzySuccessPercentage = 50.0;

    while (questionNumber < (int)answers.size() - 1 && questionNumber >= 0) {

        cout << "\033[33m" << answers[questionNumber];

        if (memorized[questionNumber]) {
            cout << " [MEMORIZED]";
        }

        cout << "\033[0m\n\n> ";

        getline(cin, userAnswer);

        clearScreen();

        string lowerInput = normalize(userAnswer);

        if (lowerInput == "h") {
            showHint = !showHint;
            cout << (showHint ? "Help is now on.\n\n" : "Help is now off.\n\n");
        }
        else if (lowerInput == "d") {
            cout << "\033[36mCurrent answer: \n\n"
                 << answers[questionNumber + 1] << "\033[0m\n\n";
        }
        else if (lowerInput == "f") {
            double newPercentage;
            cout << "Enter new fuzzy success percentage (0-100, current "
                 << fuzzySuccessPercentage << "%): ";
            cin >> newPercentage;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            if (newPercentage >= 0 && newPercentage <= 100) {
                fuzzySuccessPercentage = newPercentage;
                cout << "Updated to " << fuzzySuccessPercentage << "%\n\n";
            } else {
                cout << "Invalid value.\n\n";
            }
        }
        else if (lowerInput == "c") {
            clearScreen();
            cout << "Screen refreshed.\n\n";
        }
        else if (lowerInput == "m") {
            memorized[questionNumber] = !memorized[questionNumber];
            cout << (memorized[questionNumber] ? "Marked as memorized.\n\n"
                                              : "Unmarked memorized.\n\n");
        }
        else if (userAnswer.empty() || lowerInput == "n") {
            questionNumber += 2;
        }
        else if (lowerInput == "p") {
            questionNumber -= 2;
            if (questionNumber < 0) questionNumber = 0;
        }
        else if (lowerInput == "s") {
            questionNumber = 0;
        }
        else if (lowerInput == "q") {
            cout << "Exiting script.\n";
            break;
        }
        else if (lowerInput == "r") {
            vector<int> available;

            for (int i = 0; i < (int)answers.size(); i += 2) {
                if (!memorized[i]) {
                    available.push_back(i);
                }
            }

            if (available.empty()) {
                cout << "All questions are memorized!\n\n";
            } else {
                int pick = rand() % available.size();
                questionNumber = available[pick];
                cout << "Jumped to a random non-memorized question!\n\n";
            }
        }
        else if (lowerInput == "j") {
            cout << "Question to search for: ";
            string searchQuery;
            getline(cin, searchQuery);

            string searchNorm = normalize(searchQuery);
            bool found = false;

            for (size_t i = 0; i < answers.size(); i += 2) {
                if (normalize(answers[i]).find(searchNorm) != string::npos) {
                    questionNumber = (int)i;
                    found = true;
                    cout << "Jumped to matching question!\n\n";
                    break;
                }
            }

            if (!found) {
                cout << "No match found.\n\n";
            }
        }
        else {
            double similarity = similarityPercentage(userAnswer, answers[questionNumber + 1]);

            if (similarity >= fuzzySuccessPercentage) {
                cout << answers[questionNumber + 1]
                     << "\nCorrect! (" << similarity << "%)\n\n";
                questionNumber += 2;
            } else {
                cout << "\033[31mThe answer was (" << similarity << "%): \n"
                     << answers[questionNumber + 1] << "\n\n\033[0m";
            }
        }

        displayHelp(showHint);
    }

    if (questionNumber >= (int)answers.size()) {
        cout << "You remembered everything!\n";
    }

    return 0;
}
