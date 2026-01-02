#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <curl/curl.h>
#include <unistd.h>

// ANSI Color Codes for Terminal
const std::string RESET  = "\033[0m";
const std::string RED    = "\033[31m";
const std::string GREEN  = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string BLUE   = "\033[34m";
const std::string CYAN   = "\033[36m";
const std::string BOLD   = "\033[1m";

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

std::string get_imei() {
    // Attempt to read IMEI from system property via getprop
    // Note: On Android 10+, this requires root or system signature
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen("getprop persist.sys.vold_decrypt_imei", "r");
    if (!pipe) return "";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        result += buffer;
    }
    pclose(pipe);
    
    // Cleanup whitespace
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
    result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
    
    return result;
}

std::string extract_suffix(const std::string& imei) {
    if (imei.length() < 7) return "0000000";
    return imei.substr(imei.length() - 7);
}

int main(int argc, char* argv[]) {
    bool debug_mode = false;
    if (argc > 1 && std::string(argv[1]) == "-d") {
        debug_mode = true;
    }

    std::cout << CYAN << BOLD << "========================================" << RESET << std::endl;
    std::cout << CYAN << BOLD << "   ThunderOS Verification System 2.0   " << RESET << std::endl;
    std::cout << CYAN << BOLD << "========================================" << RESET << std::endl;

    log_info("Initializing hardware identification...");
    
    std::string imei = get_imei();
    if (imei.empty()) {
        log_error("Could not retrieve Device ID. Ensure root access.");
        return 1;
    }

    std::string suffix = extract_suffix(imei);
    // Fixed the concatenation error here by ensuring strings are used
    log_info(std::string("Device ID parsed. Last 7 digits: ") + YELLOW + suffix + RESET);

    if (debug_mode) {
        log_success("Debug Mode: Verification bypassed.");
        std::cout << YELLOW << "IMEI: " << imei << RESET << std::endl;
        return 0;
    }

    log_info("Contacting authentication server...");

    CURL* curl = curl_easy_init();
    if (curl) {
        std::string readBuffer;
        // Replace with your actual API endpoint
        std::string url = "https://your-api.com/verify?id=" + suffix;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            log_error(std::string("Network failure: ") + curl_easy_strerror(res));
            curl_easy_cleanup(curl);
            return 1;
        }

        if (readBuffer.find("AUTHORIZED") != std::string::npos) {
            log_success("Hardware Signature Verified. Welcome.");
        } else {
            log_error("Device Not Authorized. System will halt.");
            // system("reboot"); // Uncomment for production
        }

        curl_easy_cleanup(curl);
    }

    return 0;
}
