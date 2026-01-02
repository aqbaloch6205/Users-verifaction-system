#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <curl/curl.h>
#include <unistd.h>
#include <sys/reboot.h>

// ANSI Color Codes for Terminal
const std::string RESET  = "\033[0m";
const std::string RED    = "\033[31m";
const std::string GREEN  = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string BLUE   = "\033[34m";
const std::string CYAN   = "\033[36m";
const std::string BOLD   = "\033[1m";

const std::string GIT_URL = "https://raw.githubusercontent.com/AbdulMods/ThunderOsUsers/refs/heads/main/Thunderos1.3users.txt";
const std::string ADMIN_CONTACT = "@Qcloudfx";

void log_info(const std::string& msg) {
    std::cout << BLUE << "[INFO] " << RESET << msg << std::endl;
}

void log_success(const std::string& msg) {
    std::cout << GREEN << BOLD << "[SUCCESS] " << RESET << msg << std::endl;
}

void log_error(const std::string& msg) {
    std::cerr << RED << BOLD << "[ERROR] " << RESET << msg << std::endl;
}

// Callback for CURL to handle response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Execute shell command safely
void exec_silent(const std::string& cmd) {
    system(cmd.c_str());
}

// Get raw command output
std::string get_cmd_output(const std::string& cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

void notify_user(const std::string& title, const std::string& message) {
    // Method 1: cmd notification (Android 10+)
    std::string cmd1 = "cmd notification post -t \"" + title + "\" -S bigtext --id 1001 thunder_auth \"" + message + "\" >/dev/null 2>&1";
    exec_silent(cmd1);
    
    // Method 2: Broadcast (Legacy/Custom ROMs)
    std::string cmd2 = "am broadcast -a android.intent.action.MAIN --es title \"" + title + "\" --es msg \"" + message + "\" >/dev/null 2>&1";
    exec_silent(cmd2);
}

std::string get_imei_suffix() {
    // Using service call as it's the most reliable method on custom ROMs
    std::string raw = get_cmd_output("service call iphonesubinfo 1 s16 com.android.shell");
    std::string clean = "";
    for (char c : raw) {
        if (isdigit(c)) clean += c;
    }
    
    if (clean.length() < 7) return "";
    return clean.substr(clean.length() - 7);
}

int main(int argc, char* argv[]) {
    bool debug_mode = false;
    if (argc > 1 && std::string(argv[1]) == "-d") {
        debug_mode = true;
    }

    // Wait for system to initialize if running at boot
    if (!debug_mode) sleep(10);

    std::cout << CYAN << BOLD << "========================================" << RESET << std::endl;
    std::cout << CYAN << BOLD << "   ThunderOS Verification System 2.1   " << RESET << std::endl;
    std::cout << CYAN << BOLD << "========================================" << RESET << std::endl;

    while (true) {
        std::string suffix = get_imei_suffix();
        
        if (suffix.empty()) {
            log_error("Could not retrieve Device ID. Retrying...");
            sleep(5);
            continue;
        }

        // Corrected string concatenation
        log_info(std::string("Device ID (Suffix): ") + YELLOW + suffix + RESET);

        CURL* curl = curl_easy_init();
        std::string readBuffer;

        if (curl) {
            log_info("Connecting to verification server...");
            
            curl_easy_setopt(curl, CURLOPT_URL, GIT_URL.c_str());
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "ThunderOS-Auth/2.0");

            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);

            if (res != CURLE_OK) {
                log_error(std::string("Network Error: ") + curl_easy_strerror(res));
                sleep(10);
                continue;
            }
        } else {
            log_error("Failed to initialize network subsystem.");
            sleep(10);
            continue;
        }

        // Check if our suffix exists in the downloaded data
        bool verified = false;
        if (readBuffer.find(suffix) != std::string::npos) {
            verified = true;
        }

        if (verified) {
            log_success("VERIFICATION SUCCESSFUL");
            notify_user("ThunderOS Verified", "Welcome! Device authorized successfully.");
            return 0; // Exit program, we are good
        } else {
            log_error("VERIFICATION FAILED");
            std::string msg = "Verification Failed! Contact " + ADMIN_CONTACT + ". Rebooting in 30s.";
            notify_user("ThunderOS Security", msg);

            if (debug_mode) {
                log_info("Debug Mode: Reboot suppressed.");
                return 1;
            }

            sleep(30);
            sync();
            reboot(RB_AUTOBOOT);
        }
    }
    return 0;
}
