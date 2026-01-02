#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/reboot.h>
#include <curl/curl.h>
#include <regex>
#include <iomanip>

// Configuration
const std::string GIT_URL = "https://raw.githubusercontent.com/AbdulMods/ThunderOsUsers/refs/heads/main/Thunderos1.3users.txt";
const std::string ADMIN_CONTACT = "@Qcloudfx";

// ANSI Color Codes for Terminal Debugging
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

void log_info(const std::string& msg) { std::cout << CYAN << "[INFO] " << RESET << msg << std::endl; }
void log_success(const std::string& msg) { std::cout << GREEN << BOLD << "[SUCCESS] " << RESET << msg << std::endl; }
void log_warn(const std::string& msg) { std::cout << YELLOW << "[WARN] " << RESET << msg << std::endl; }
void log_error(const std::string& msg) { std::cout << RED << BOLD << "[ERROR] " << RESET << msg << std::endl; }

std::string get_cmd_output(const std::string& cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    while (fgets(buffer, sizeof buffer, pipe) != NULL) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

void notify_user(const std::string& title, const std::string& message) {
    // Shell notification
    std::string cmd = "cmd notification post -t \"" + title + "\" -S bigtext --id 1001 thunder_auth \"" + message + "\" >/dev/null 2>&1";
    system(cmd.c_str());
    // Also log to logcat for backend visibility
    std::string log_cmd = "logcat -t 1 -p i -s ThunderOS:\"" + message + "\"";
    system(log_cmd.c_str());
}

std::string get_imei_suffix() {
    log_info("Executing service call to retrieve IMEI...");
    std::string raw = get_cmd_output("service call iphonesubinfo 1 s16 com.android.shell");
    
    std::string clean = "";
    for (char c : raw) { if (isdigit(c)) clean += c; }
    
    if (clean.length() < 7) {
        log_error("Failed to parse IMEI. Raw output was too short.");
        return "";
    }
    
    std::string suffix = clean.substr(clean.length() - 7);
    log_info("Device IMEI parsed. Last 7 digits: " + YELLOW + suffix + RESET);
    return suffix;
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string fetch_list() {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    log_info("Connecting to GitHub: " + GIT_URL);
    
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, GIT_URL.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); 
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "ThunderOS-Auth/1.3");
        
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            log_error("CURL Fetch Failed: " + std::string(curl_easy_strerror(res)));
        }
        curl_easy_cleanup(curl);
        return (res == CURLE_OK) ? readBuffer : "";
    }
    return "";
}

int main(int argc, char** argv) {
    bool debug_mode = false;
    if (argc > 1 && std::string(argv[1]) == "-d") {
        debug_mode = true;
        log_warn("DEBUG MODE ENABLED: Will not reboot on failure.");
    }

    std::cout << BOLD << "==========================================" << RESET << std::endl;
    std::cout << BOLD << "       ThunderOS Native Verification      " << RESET << std::endl;
    std::cout << BOLD << "==========================================" << RESET << std::endl;

    while (true) {
        std::string my_id = get_imei_suffix();
        if (my_id.empty()) {
            log_warn("Retrying IMEI retrieval in 5s...");
            sleep(5);
            continue;
        }

        std::string list_data = fetch_list();
        if (list_data.empty()) {
            log_warn("Server unreachable. Retrying in 10s...");
            sleep(10);
            continue;
        }

        log_info("Server list fetched. Parsing valid IMEIs...");
        
        // Match 7+ digits following "imei" label
        std::regex imei_regex("imei[\\s:=]*([0-9]{7,})", std::regex_constants::icase);
        auto words_begin = std::sregex_iterator(list_data.begin(), list_data.end(), imei_regex);
        auto words_end = std::sregex_iterator();

        bool verified = false;
        int count = 0;
        
        std::cout << CYAN << "--- Valid Suffixes in List ---" << RESET << std::endl;
        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::string found = (*i)[1].str();
            std::string found_suffix = found.substr(found.length() - 7);
            count++;
            
            if (found_suffix == my_id) {
                std::cout << GREEN << " -> " << found_suffix << " [MATCH]" << RESET << std::endl;
                verified = true;
            } else {
                std::cout << " -> " << found_suffix << std::endl;
            }
        }
        std::cout << CYAN << "------------------------------" << RESET << std::endl;
        log_info("Found " + std::to_string(count) + " entries in remote database.");

        if (verified) {
            log_success("DEVICE VERIFIED SUCESSFULLY!");
            notify_user("ThunderOS Success", "User Verified Successfully! Enjoy full experience.");
            return 0;
        } else {
            log_error("VERIFICATION FAILED!");
            std::string fail_msg = "Rebooting in 30s. Contact admin " + ADMIN_CONTACT + " for verification.";
            log_warn("Message: " + fail_msg);
            
            notify_user("ThunderOS Security", fail_msg);
            
            if (debug_mode) {
                log_warn("Debug mode: Reboot suppressed. Exiting loop.");
                return 1;
            }

            sleep(30);
            log_error("REBOOTING NOW...");
            sync();
            reboot(RB_AUTOBOOT);
            return 1;
        }
    }
}
