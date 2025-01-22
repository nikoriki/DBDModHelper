#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <windows.h>
#include <shlobj.h>
#include <regex>
#include <random>
#include <winhttp.h>


#pragma comment(lib, "winhttp.lib")

const std::string LOCAL_VERSION = "1.1.0";
const std::wstring GITHUB_HOST = L"raw.githubusercontent.com";
const std::wstring VERSION_PATH = L"/nikoriki/DBDModHelper/main/version.txt?nocache=" + std::to_wstring(std::time(nullptr));

bool introEnabled = true;

void clearScreen() {
    system("cls");
}

void displayLoadingMessage(const std::string& message) {
    std::cout << message;
    std::cout.flush();
}

void displayCat() {
    std::cout << R"(
       _                        
      \`*-.                    
       )  _`-.                  
      .  : `. .                
      : _   '  \                
      ; *` _.   `*-._          
      `-.-'          `-.       
        ;       `       `.     
        :.       .        \    
        . \  .   :   .-'   .   
        '  `+.;  ;  '      :   
        :  '  |    ;       ;-. 
        ; '   : :`-:     _.`* ;
     .*' /  .*' ; .*`- +'  `*' 
       `*-*   `*-*  `*-*'       
    )" << "\n\n";
}

std::string generateRandomProgramName(int length = 24) {
    const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static std::default_random_engine engine{std::random_device{}()};
    static std::uniform_int_distribution<int> dist(0, charset.size() - 1);
    std::string randomName;
    for (int i = 0; i < length; ++i) {
        randomName += charset[dist(engine)];
    }
    return randomName;
}

void updateProgramTitle() {
    while (true) {
        std::string randomTitle = generateRandomProgramName();
        std::wstring wideTitle(randomTitle.begin(), randomTitle.end());
        SetConsoleTitleW(wideTitle.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void displayOutdatedMessage() {
    clearScreen();
    std::cerr << "\033[1;31m"
              << "This version is outdated. Install the new one on my GitHub:\n"
              << "https://github.com/nikoriki\n"
              << "\033[0m";
    std::this_thread::sleep_for(std::chrono::seconds(3));
    system("start https://github.com/nikoriki");
    exit(0);
}

std::string fetchRemoteVersion() {
    std::string response;

    for (int attempts = 0; attempts < 3; ++attempts) {
        HINTERNET hSession = WinHttpOpen(L"DBDModHelper/1.0",
                                         WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                         WINHTTP_NO_PROXY_NAME,
                                         WINHTTP_NO_PROXY_BYPASS, 0);

        if (!hSession) {
            std::cerr << "Error: Unable to open HTTP session.\n";
            return "";
        }

        HINTERNET hConnect = WinHttpConnect(hSession, GITHUB_HOST.c_str(),
                                            INTERNET_DEFAULT_HTTPS_PORT, 0);

        if (!hConnect) {
            std::cerr << "Error: Unable to connect to GitHub.\n";
            WinHttpCloseHandle(hSession);
            return "";
        }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", VERSION_PATH.c_str(),
                                                NULL, WINHTTP_NO_REFERER,
                                                WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                WINHTTP_FLAG_SECURE);

        if (!hRequest) {
            std::cerr << "Error: Unable to open HTTP request.\n";
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "";
        }

        BOOL bResult = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                          WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

        if (bResult && WinHttpReceiveResponse(hRequest, NULL)) {
            DWORD dwSize = 0;
            WinHttpQueryDataAvailable(hRequest, &dwSize);

            if (dwSize > 0) {
                char* buffer = new char[dwSize + 1];
                ZeroMemory(buffer, dwSize + 1);
                DWORD dwRead = 0;
                WinHttpReadData(hRequest, buffer, dwSize, &dwRead);
                response = buffer;
                delete[] buffer;
            }

            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);

            if (!response.empty()) break;
        } else {
            std::cerr << "Error: HTTP request failed. Retrying...\n";
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (response.empty()) {
        std::cerr << "Error: Unable to fetch remote version after multiple attempts.\n";
    }

    return response;
}


void checkForUpdates() {
    std::cout << "Checking for updates..." << std::endl;
    std::string remoteVersion = fetchRemoteVersion();

    if (remoteVersion.empty()) {
        std::cerr << "Error: Unable to fetch the remote version.\n";
        return;
    }

    std::cout << "Fetched remote version (raw): '" << remoteVersion << "'\n";

    remoteVersion.erase(remoteVersion.find_last_not_of("\n\r ") + 1);
    std::cout << "Fetched remote version (trimmed): '" << remoteVersion << "'\n";

    if (remoteVersion != LOCAL_VERSION) {
        std::cerr << "Local version: '" << LOCAL_VERSION << "'\n";
        displayOutdatedMessage();
    } else {
        std::cout << "You are using the latest version.\n";
    }
}

std::string checkFolderValidity(const std::string& path, const std::string& expectedEnding) {
    if (path.empty() || path.find(expectedEnding) == std::string::npos) {
        return "error";
    }
    return path;
}

void loadIntroSetting() {
    char* appDataPath;
    size_t len;
    _dupenv_s(&appDataPath, &len, "APPDATA");
    if (appDataPath) {
        std::string settingsPath = std::string(appDataPath) + "\\DBDModHelper\\settings.txt";
        free(appDataPath);
        std::ifstream file(settingsPath);
        if (file.is_open()) {
            std::string line;
            std::getline(file, line);
            introEnabled = (line == "1");
            file.close();
        }
    }
}

void saveIntroSetting() {
    char* appDataPath;
    size_t len;
    _dupenv_s(&appDataPath, &len, "APPDATA");
    if (appDataPath) {
        std::string settingsPath = std::string(appDataPath) + "\\DBDModHelper\\settings.txt";
        std::filesystem::create_directories(std::string(appDataPath) + "\\DBDModHelper\\");
        free(appDataPath);
        std::ofstream file(settingsPath);
        if (file.is_open()) {
            file << (introEnabled ? "1" : "0");
            file.close();
        }
    }
}

void typeText(const std::string& text, int delayMs = 50) {
    for (char c : text) {
        std::cout << c << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    }
}

void troubleshootingScreen() {
    while (true) {
        clearScreen();
        displayCat();
        std::cout << "Contact me through Discord @desgubernamentalizar / 342779250912264194\n";
        std::cout << "1 - Go back\n";
        int choice;
        std::cin >> choice;
        std::cin.ignore();
        if (choice == 1) return;
        else {
            std::cout << "Invalid choice.\n";
            system("pause");
        }
    }
}

std::string selectFolder(const std::string& promptMessage) {
    clearScreen();
    displayCat();
    std::cout << promptMessage << "\n";
    wchar_t path[MAX_PATH];
    BROWSEINFOW bi = {0};
    bi.lpszTitle = L"Select a folder";
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl != 0) {
        SHGetPathFromIDListW(pidl, path);
        std::wstring wpath(path);
        return std::string(wpath.begin(), wpath.end());
    }
    return "";
}

void saveFolderPath(const std::string& key, const std::string& path) {
    char* appDataPath;
    size_t len;
    _dupenv_s(&appDataPath, &len, "APPDATA");
    if (appDataPath) {
        std::string fullPath = std::string(appDataPath) + "\\DBDModHelper\\" + key + ".txt";
        std::filesystem::create_directories(std::string(appDataPath) + "\\DBDModHelper\\");
        free(appDataPath);
        std::ofstream file(fullPath);
        if (file.is_open()) {
            file << path;
            file.close();
        }
    }
}

std::string loadFolderPath(const std::string& key) {
    char* appDataPath;
    size_t len;
    _dupenv_s(&appDataPath, &len, "APPDATA");
    if (appDataPath) {
        std::string fullPath = std::string(appDataPath) + "\\DBDModHelper\\" + key + ".txt";
        free(appDataPath);
        std::ifstream file(fullPath);
        if (file.is_open()) {
            std::string path;
            std::getline(file, path);
            file.close();
            if (std::filesystem::exists(path)) return path;
        }
    }
    return "";
}

void changeDirectories(std::string& modsFolder, std::string& dbdFolder) {
    while (true) {
        clearScreen();
        displayCat();
        std::cout << "1 - Change DeadbyDaylight's pak folder directory\n";
        std::cout << "2 - Change mods folder directory\n";
        std::cout << "3 - Go back\n";
        int choice;
        std::cin >> choice;
        std::cin.ignore();
        switch (choice) {
            case 1: {
                std::string newDbdFolder;
                while (true) {
                    newDbdFolder = selectFolder("Select the new DeadbyDaylight's pak folder:");
                    if (checkFolderValidity(newDbdFolder, "DeadByDaylight\\Content\\Paks") != "error") {
                        saveFolderPath("dbdFolder", newDbdFolder);
                        dbdFolder = newDbdFolder;
                        clearScreen();
                        displayCat();
                        std::cout << "Directory updated successfully to: " << newDbdFolder << "\n";
                        break;
                    }
                    clearScreen();
                    displayCat();
                    std::cout << "Invalid folder. Ensure its path ends with: \"...DeadByDaylight\\Content\\Paks\"\n";
                    system("pause");
                }
                system("pause");
                break;
            }
            case 2: {
                std::string newModsFolder = selectFolder("Select the new mods folder:");
                saveFolderPath("modsFolder", newModsFolder);
                modsFolder = newModsFolder;
                clearScreen();
                displayCat();
                std::cout << "Directory updated successfully to: " << newModsFolder << "\n";
                system("pause");
                break;
            }
            case 3:
                return;
            default:
                clearScreen();
                displayCat();
                std::cout << "Invalid choice.\n";
                system("pause");
        }
    }
}

void settingsMenu(std::string& modsFolder, std::string& dbdFolder) {
    while (true) {
        clearScreen();
        displayCat();
        std::cout << "1 - Troubleshooting\n";
        std::cout << "2 - Change directories\n";
        std::cout << "3 - Menu intro (currently " << (introEnabled ? "ON" : "OFF") << ")\n";
        std::cout << "4 - Go back\n";
        int choice;
        std::cin >> choice;
        std::cin.ignore();
        switch (choice) {
            case 1:
                troubleshootingScreen();
                break;
            case 2:
                changeDirectories(modsFolder, dbdFolder);
                break;
            case 3:
                introEnabled = !introEnabled;
                saveIntroSetting();
                clearScreen();
                displayCat();
                std::cout << "Menu intro turned " << (introEnabled ? "ON" : "OFF") << ".\n";
                system("pause");
                break;
            case 4:
                return;
            default:
                clearScreen();
                displayCat();
                std::cout << "Invalid choice.\n";
                system("pause");
        }
    }
}

void renameMods(const std::string& folderPath, const std::string& platform) {
    try {
        for (const auto& file : std::filesystem::directory_iterator(folderPath)) {
            std::string filePath = file.path().string();
            std::string newFilePath = filePath;

            if (filePath.find("EGS") != std::string::npos) {
                newFilePath.replace(filePath.find("EGS"), 3, platform);
            } else if (filePath.find("Windows") != std::string::npos) {
                newFilePath.replace(filePath.find("Windows"), 7, platform);
            } else if (filePath.find("WinGDK") != std::string::npos) {
                newFilePath.replace(filePath.find("WinGDK"), 6, platform);
            }

            if (filePath != newFilePath) {
                std::filesystem::rename(filePath, newFilePath);
            }
        }
    } catch (const std::exception& e) {
        clearScreen();
        displayCat();
        std::cerr << "Error during renaming: " << e.what() << "\n";
        system("pause");
        return;
    }

    clearScreen();
    displayCat();
    std::cout << "Renaming completed successfully.\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void installMods(const std::string& modsFolder, const std::string& dbdFolder) {
    try {
        for (const auto& file : std::filesystem::directory_iterator(modsFolder)) {
            std::filesystem::copy(file.path(), dbdFolder, std::filesystem::copy_options::overwrite_existing);
        }
    } catch (const std::exception& e) {
        clearScreen();
        displayCat();
        std::cerr << "Error during installation: " << e.what() << "\n";
        system("pause");
        return;
    }

    clearScreen();
    displayCat();
    std::cout << "Mods installed successfully.\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void removeMods(const std::string& dbdFolder) {
    std::cout << "Are you sure? In case you want to remove ALL the mods from DeadbyDaylight's pak folder, please type Y or N\n";
    std::string choice;
    std::cin >> choice;
    if (choice == "Y" || choice == "y") {
        try {
            std::regex modFilePattern(R"(pakchunk(\d+)-(EGS|WinGDK|Windows)\.(ucas|utoc|sig|pak))");
            for (const auto& file : std::filesystem::directory_iterator(dbdFolder)) {
                std::string fileName = file.path().filename().string();
                std::smatch match;
                if (std::regex_match(fileName, match, modFilePattern)) {
                    int chunkNumber = std::stoi(match[1].str());
                    if (chunkNumber > 5999) {
                        std::filesystem::remove(file.path());
                    }
                }
            }
        } catch (const std::exception& e) {
            clearScreen();
            displayCat();
            std::cerr << "Error during removal: " << e.what() << "\n";
            system("pause");
            return;
        }

        clearScreen();
        displayCat();
        std::cout << "Mods removed successfully.\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void menu(std::string& modsFolder, std::string& dbdFolder) {
    while (true) {
        clearScreen();
        displayCat();
        std::cout << "1 - Rename mods\n";
        std::cout << "2 - Install mods into the DBD folder\n";
        std::cout << "3 - Remove mods from the DBD folder\n";
        std::cout << "4 - Quit\n";
        std::cout << "5 - My Github\n";

        std::cout << "\n0 - Settings\n";

        int choice;
        std::cin >> choice;
        std::cin.ignore();
        switch (choice) {
            case 0:
                settingsMenu(modsFolder, dbdFolder);
                break;
            case 1: {
                clearScreen();
                displayCat();
                std::cout << "Select the platform:\n";
                std::cout << "1 - Epic Games\n";
                std::cout << "2 - Microsoft Store\n";
                std::cout << "3 - Steam\n";

                int platformChoice;
                std::cin >> platformChoice;
                std::cin.ignore();

                std::string platform;
                if (platformChoice == 1) {
                    platform = "EGS";
                } else if (platformChoice == 2) {
                    platform = "WinGDK";
                } else if (platformChoice == 3) {
                    platform = "Windows";
                } else {
                    clearScreen();
                    displayCat();
                    std::cout << "Invalid choice.\n";
                    system("pause");
                    break;
                }

                renameMods(modsFolder, platform);
                break;
            }
            case 2:
                installMods(modsFolder, dbdFolder);
                break;
            case 3:
                removeMods(dbdFolder);
                break;
            case 4:
                clearScreen();
                displayCat();
                std::cout << "Goodbye, Stranger.\n";
                std::this_thread::sleep_for(std::chrono::seconds(1));
                exit(0);
            case 5:
                clearScreen();
                displayCat();
                std::cout << "Opening GitHub in your default browser...\n";
                system("start https://github.com/nikoriki");
                system("pause");
                break;
            default:
                clearScreen();
                displayCat();
                std::cout << "Invalid choice.\n";
                system("pause");
        }
    }
}

int main() {
    std::thread titleThread(updateProgramTitle);
    titleThread.detach();

    clearScreen();
    displayCat();

    checkForUpdates();

    std::cout << "\nWelcome to DBD Mod Helper. Press any key to continue...";
    std::cin.get();

    if (introEnabled) {
        clearScreen();
        displayCat();
        typeText("This program is completely free and open source.\nMake sure to download it from my GitHub and not a sketchy Discord server.\n\n", 50);
        std::cout << "+-----------------------------+\n|      github.com/nikoriki    |\n+-----------------------------+\nPlease type \"github.com/nikoriki\" to continue: ";
        while (true) {
            std::string input;
            std::getline(std::cin, input);
            if (input == "github.com/nikoriki") break;
            else {
                clearScreen();
                displayCat();
                std::cout << "This program is completely free and open source.\nMake sure to download it from my GitHub and not a sketchy Discord server.\n\n";
                std::cout << "+-----------------------------+\n|      github.com/nikoriki    |\n+-----------------------------+\nPlease type \"github.com/nikoriki\" to continue: ";
            }
        }
    }

    std::string modsFolder = loadFolderPath("modsFolder");
    if (modsFolder.empty()) {
        modsFolder = selectFolder("Select the mods folder.");
        saveFolderPath("modsFolder", modsFolder);
    }

    std::string dbdFolder = loadFolderPath("dbdFolder");
    if (dbdFolder.empty() || checkFolderValidity(dbdFolder, "DeadByDaylight\\Content\\Paks") == "error") {
        while (true) {
            dbdFolder = selectFolder("Select DeadbyDaylight's pak folder.");
            if (checkFolderValidity(dbdFolder, "DeadByDaylight\\Content\\Paks") != "error") {
                saveFolderPath("dbdFolder", dbdFolder);
                break;
            }
            std::cout << "That's the incorrect folder. Ensure its route is: \"...DeadByDaylight\\Content\\Paks\"\n";
            system("pause");
        }
    }

    menu(modsFolder, dbdFolder);
}
