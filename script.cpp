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

// Display help menu in a single line
void displayHelp(bool showHint) {
    if (!showHint) return;
    cout << "\033[37mHelp Menu: Enter/n = next, p = previous, s = start, d = display answer, "
            "r = random (deck shuffle), m = toggle memorized, v = skip memorized mode, "
            "j = search, c = clear, t = statistics, h = help, q = quit, f = fuzzy.\033[0m\n\n";
}

// Display statistics including correct/wrong percentages
void displayStatistics(const vector<bool>& memorized,
                       const vector<int>& randomDeck,
                       int deckIndex,
                       double fuzzySuccessPercentage,
                       const vector<bool>& correctAnswers,
                       const vector<bool>& wrongAnswers) {
    int totalQuestions = (int)memorized.size() / 2;
    int memorizedCount = 0, correctCount = 0, wrongCount = 0;

    for (size_t i = 0; i < memorized.size(); i += 2) {
        if (memorized[i]) memorizedCount++;
        if (correctAnswers[i]) correctCount++;
        if (wrongAnswers[i]) wrongCount++;
    }

    int answered = correctCount + wrongCount; // only answered questions
    int remaining = totalQuestions - memorizedCount;

    int remainingDeck = 0;
    for (size_t i = deckIndex; i < randomDeck.size(); ++i) {
        if (!memorized[randomDeck[i]]) remainingDeck++;
    }

    double correctPercent = answered ? (correctCount * 100.0 / answered) : 0.0;
    double wrongPercent   = answered ? (wrongCount * 100.0 / answered) : 0.0;

    cout << "\033[36m--- Statistics ---\n";
    cout << "Total questions: " << totalQuestions << "\n";
    cout << "Memorized: " << memorizedCount << " (" 
         << (totalQuestions ? memorizedCount * 100.0 / totalQuestions : 0) << "%)\n";
    cout << "Correct answers: " << correctCount << " (" << correctPercent << "%)\n";
    cout << "Wrong answers: " << wrongCount << " (" << wrongPercent << "%)\n";
    cout << "Remaining random: " << remainingDeck << "\n";
    cout << "Fuzzy success threshold: " << fuzzySuccessPercentage << "%\n";
    cout << "------------------\033[0m\n\n";
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

    vector<bool> memorized(answers.size(), false);
    vector<bool> correctAnswers(answers.size(), false);
    vector<bool> wrongAnswers(answers.size(), false);

    string userAnswer;
    int questionNumber = 0;
    bool showHint = true;
    bool skipMode = false; // skip memorized mode
    double fuzzySuccessPercentage = 50.0;

    // Deck for random questions
    vector<int> randomDeck;
    int deckIndex = 0;

    while (questionNumber < (int)answers.size() - 1 && questionNumber >= 0) {

        // Skip memorized at display time if skip mode is ON
        if (skipMode && memorized[questionNumber]) {
            int start = questionNumber;
            do {
                questionNumber += 2;
                if (questionNumber >= (int)answers.size())
                    questionNumber = 0;
                if (questionNumber == start) break;
            } while (memorized[questionNumber]);
        }

        cout << "\033[33m" << answers[questionNumber];

        if (memorized[questionNumber]) cout << " [MEMORIZED]";
        if (skipMode) cout << " [SKIP MODE]";
        cout << "\033[0m\n\n> ";

        getline(cin, userAnswer);
        clearScreen();
        string lowerInput = normalize(userAnswer);

        if (lowerInput == "h") {
            showHint = !showHint;
            cout << (showHint ? "Help is now on.\n\n" : "Help is now off.\n\n");
        }
        else if (lowerInput == "v") {
            skipMode = !skipMode;
            cout << (skipMode ? "Skip memorized mode ON.\n\n"
                              : "Skip memorized mode OFF.\n\n");
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
            continue; // stay on current question
        }
        else if (lowerInput == "t") {
            displayStatistics(memorized, randomDeck, deckIndex, fuzzySuccessPercentage, correctAnswers, wrongAnswers);
        }
        else if (userAnswer.empty() || lowerInput == "n") {
            int start = questionNumber;
            do {
                questionNumber += 2;
                if (questionNumber >= (int)answers.size()) questionNumber = 0;
                if (!skipMode || !memorized[questionNumber]) break;
            } while (questionNumber != start);
        }
        else if (lowerInput == "p") {
            int start = questionNumber;
            do {
                questionNumber -= 2;
                if (questionNumber < 0) questionNumber = (int)answers.size() - 2;
                if (!skipMode || !memorized[questionNumber]) break;
            } while (questionNumber != start);
        }
        else if (lowerInput == "s") questionNumber = 0;
        else if (lowerInput == "q") {
            cout << "Exiting script.\n";
            break;
        }
        else if (lowerInput == "r") {
            // Deck-style random
            if (deckIndex >= (int)randomDeck.size()) {
                randomDeck.clear();
                for (int i = 0; i < (int)answers.size(); i += 2) {
                    if (!skipMode || !memorized[i])
                        randomDeck.push_back(i);
                }
                if (randomDeck.empty()) {
                    cout << "No available questions for random mode!\n\n";
                    continue;
                }
                shuffle(randomDeck.begin(), randomDeck.end(), default_random_engine(rand()));
                deckIndex = 0;
            }
            questionNumber = randomDeck[deckIndex++];
            cout << (skipMode ? "Jumped to a random non-memorized question!\n\n"
                              : "Jumped to a random question!\n\n");
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
            if (!found) cout << "No match found.\n\n";
        }
        else {
            double similarity = similarityPercentage(userAnswer, answers[questionNumber + 1]);
            if (similarity >= fuzzySuccessPercentage) {
                cout << answers[questionNumber + 1]
                     << "\nCorrect! (" << similarity << "%)\n\n";
                correctAnswers[questionNumber] = true;
                questionNumber += 2;
            } else {
                cout << "\033[31mThe answer was (" << similarity << "%): \n"
                     << answers[questionNumber + 1] << "\n\n\033[0m";
                wrongAnswers[questionNumber] = true;
            }
        }

        displayHelp(showHint);
    }

    if (questionNumber >= (int)answers.size()) {
        cout << "You remembered everything!\n";
    }

    return 0;
}
