#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <curl/curl.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/system_properties.h>

// --- Q-CLOUD BRANDING COLORS ---
const std::string RESET  = "\033[0m";
const std::string RED    = "\033[31m";
const std::string GREEN  = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string BLUE   = "\033[34m";
const std::string MAGENTA = "\033[35m";
const std::string CYAN   = "\033[36m";
const std::string BOLD   = "\033[1m";

// Configuration
const std::string GIT_URL = "https://raw.githubusercontent.com/aqbaloch6205/Q-Cloud-Ecosystem/refs/heads/main/Ecosystem.txt";
const std::string ADMIN_CONTACT = "@Qcloudfx";
const std::string DEVELOPER = "Abdul Qadeer";
const std::string BRAND = "Q-Cloud Ecosystem";

// --- LINKER FIX SHIM ---
extern "C" {
    ssize_t __sendto_chk(int fd, const void* buf, size_t count, size_t buf_size, 
                         int flags, const struct sockaddr* dest_addr, socklen_t addrlen) {
        return sendto(fd, buf, count, flags, dest_addr, addrlen);
    }
}

// --- LOGGING ---
void log_qcloud(const std::string& msg) {
    std::cout << MAGENTA << BOLD << "[Q-CLOUD] " << RESET << msg << std::endl;
}
void log_success(const std::string& msg) {
    std::cout << GREEN << BOLD << "[SUCCESS] " << RESET << msg << std::endl;
}
void log_error(const std::string& msg) {
    std::cerr << RED << BOLD << "[DENIED] " << RESET << msg << std::endl;
}

// --- IDENTITY ENGINE ---
std::string get_system_info() {
    char rom[PROP_VALUE_MAX], kernel[PROP_VALUE_MAX];
    __system_property_get("ro.product.mod_device", rom);
    __system_property_get("ro.build.display.id", kernel);
    
    std::string identity = "LG V60 / ";
    if (strlen(rom) > 0) identity += rom;
    else identity += "Q-Cloud Ecosystem";
    
    return identity;
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void notify_user(const std::string& title, const std::string& tag, const std::string& message) {
    // Verified SU 2000 method
    std::string cmd = "su 2000 -c \"cmd notification post -t '" + title + "' '" + tag + "' '" + message + "'\" >/dev/null 2>&1";
    system(cmd.c_str());
}

std::string get_license_token() {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen("service call iphonesubinfo 1 s16 com.android.shell", "r");
    if (!pipe) return "";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) { result += buffer; }
    pclose(pipe);
    
    std::string clean = "";
    for (char c : result) { if (isdigit(c)) clean += c; }
    if (clean.length() < 7) return "";
    // Professional 7-digit License Token
    return clean.substr(clean.length() - 7);
}

int main(int argc, char* argv[]) {
    std::string system_identity = get_system_info();
    bool debug = (argc > 1 && std::string(argv[1]) == "-d");

    // Professional Header
    std::cout << MAGENTA << BOLD << "========================================" << RESET << std::endl;
    std::cout << CYAN << BOLD << "       " << BRAND << " CORE" << RESET << std::endl;
    std::cout << WHITE << "    Developed by: " << YELLOW << DEVELOPER << RESET << std::endl;
    std::cout << WHITE << "    Identity: " << system_identity << RESET << std::endl;
    std::cout << MAGENTA << BOLD << "========================================" << RESET << std::endl;

    if (!debug) sleep(10); 

    while (true) {
        std::string l_token = get_license_token();
        if (l_token.empty()) {
            log_error("Hardware ID failure. Retrying...");
            sleep(5); continue;
        }

        log_qcloud("License Token: " + YELLOW + l_token + RESET);

        CURL* curl = curl_easy_init();
        std::string readBuffer;

        if (curl) {
            log_qcloud("Validating with Cloud Server...");
            curl_easy_setopt(curl, CURLOPT_URL, GIT_URL.c_str());
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);

            if (res != CURLE_OK) {
                log_qcloud(CYAN + "Waiting for Connection..." + RESET);
                sleep(10); continue;
            }
        }

        // --- VALIDATION ---
        if (readBuffer.find(l_token) != std::string::npos) {
            log_success("SYSTEM AUTHORIZED");
            notify_user(BRAND, "License", "Welcome to " + system_identity + ". Verified.");
            return 0; 
        } else {
            log_error("UNAUTHORIZED ACCESS DETECTED");
            std::string fail_msg = "Token [" + l_token + "] is not registered for " + system_identity + ". Contact " + ADMIN_CONTACT + " | Rebooting in 25s";
            
            notify_user("License Denied", "Q-Cloud", fail_msg);

            if (debug) {
                log_qcloud(YELLOW + "Debug: Reboot Suppressed" + RESET);
                return 1;
            }

            log_qcloud(RED + "Rebooting in 25s for security..." + RESET);
            sleep(25);
            sync();
            reboot(RB_AUTOBOOT);
            return 1;
        }
    }
    return 0;
}