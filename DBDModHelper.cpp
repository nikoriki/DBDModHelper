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

void clearScreen() {
    system("cls");
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
        if (choice == 1) {
            return;
        } else {
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
        std::string fullPath = std::string(appDataPath) + "\\DBDModHelper\\";
        std::filesystem::create_directories(fullPath);
        fullPath += key + ".txt";
        std::ofstream file(fullPath);
        if (file.is_open()) {
            file << path;
            file.close();
        }
        free(appDataPath);
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
            if (std::filesystem::exists(path)) {
                return path;
            }
        }
    }
    return "";
}

std::string checkFolderValidity(const std::string& path, const std::string& expectedEnding) {
    if (path.empty() || path.find(expectedEnding) == std::string::npos) {
        return "error";
    }
    return path;
}

void renameMods(const std::string& folderPath, const std::string& platform) {
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
    std::cout << "Successfully renamed.\n";
    system("pause");
}

void installMods(const std::string& modsFolder, const std::string& dbdFolder) {
    for (const auto& file : std::filesystem::directory_iterator(modsFolder)) {
        std::filesystem::copy(file.path(), dbdFolder, std::filesystem::copy_options::overwrite_existing);
    }
    std::cout << "Successfully installed the mods.\n";
    system("pause");
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
            std::cout << "Successfully removed mods.\n";
        } catch (const std::exception& e) {
            std::cout << "An error occurred while removing mods: " << e.what() << "\n";
        }
    }
    system("pause");
}

void menu(const std::string& modsFolder, const std::string& dbdFolder) {
    while (true) {
        clearScreen();
        displayCat();
        std::cout << "1 - Rename mods\n2 - Install mods into the DBD folder\n3 - Remove mods from the DBD folder\n4 - Quit\n5 - My Github\n6 - Troubleshooting/Errors\n";
        int choice;
        std::cin >> choice;
        std::cin.ignore();
        switch (choice) {
            case 1:
                clearScreen();
                displayCat();
                std::cout << "Select the platform:\n1 - Epic Games\n2 - Microsoft Store\n3 - Steam\n";
                int platformChoice;
                std::cin >> platformChoice;
                std::cin.ignore();
                if (platformChoice == 1) {
                    renameMods(modsFolder, "EGS");
                } else if (platformChoice == 2) {
                    renameMods(modsFolder, "WinGDK");
                } else if (platformChoice == 3) {
                    renameMods(modsFolder, "Windows");
                } else {
                    std::cout << "Invalid choice.\n";
                    system("pause");
                }
                break;
            case 2:
                installMods(modsFolder, dbdFolder);
                break;
            case 3:
                clearScreen();
                displayCat();
                removeMods(dbdFolder);
                break;
            case 4:
                clearScreen();
                displayCat();
                std::cout << "Goodbye, Stranger.\n";
                std::this_thread::sleep_for(std::chrono::seconds(1));
                exit(0);
            case 5:
                system("start https://github.com/nikoriki");
                break;
            case 6:
                troubleshootingScreen();
                break;
            default:
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
    typeText("This program is completely free and open source.\nMake sure to download it from my GitHub and not a sketchy Discord server.\n\n", 50);
    std::cout << "+-----------------------------+\n|      github.com/nikoriki    |\n+-----------------------------+\nPlease type \"github.com/nikoriki\" to continue: ";
    while (true) {
        std::string input;
        std::getline(std::cin, input);
        if (input == "github.com/nikoriki") {
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
            break;
        } else {
            clearScreen();
            displayCat();
            std::cout << "This program is completely free and open source.\nMake sure to download it from my GitHub and not a sketchy Discord server.\n\n";
            std::cout << "+-----------------------------+\n|      github.com/nikoriki    |\n+-----------------------------+\nPlease type \"github.com/nikoriki\" to continue: ";
        }
    }
}
