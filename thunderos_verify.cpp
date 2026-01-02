#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/reboot.h>
#include <curl/curl.h>
#include <regex>

const std::string GIT_URL = "https://raw.githubusercontent.com/AbdulMods/ThunderOsUsers/refs/heads/main/Thunderos1.3users.txt";
const std::string ADMIN_CONTACT = "@Qcloudfx";

std::string exec_cmd(const std::string& cmd) {
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

void send_notification(const std::string& title, const std::string& message) {
    std::string cmd = "cmd notification post -t \"" + title + "\" -S bigtext --id 1001 thunder_auth \"" + message + "\"";
    exec_cmd(cmd);
}

std::string get_imei_suffix() {
    std::string raw = exec_cmd("service call iphonesubinfo 1 s16 com.android.shell");
    std::string clean = "";
    for (char c : raw) { if (isdigit(c)) clean += c; }
    if (clean.length() < 7) return "";
    return clean.substr(clean.length() - 7);
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string fetch_list() {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, GIT_URL.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); 
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if (res == CURLE_OK) return readBuffer;
    }
    return "";
}

int main() {
    sleep(10); // Wait for SystemUI

    while (true) {
        std::string my_id = get_imei_suffix();
        if (my_id.empty()) { sleep(5); continue; }

        std::string list_data = fetch_list();
        if (list_data.empty()) { sleep(10); continue; }

        std::regex imei_regex("imei[^0-9]*([0-9]{7,})", std::regex_constants::icase);
        auto words_begin = std::sregex_iterator(list_data.begin(), list_data.end(), imei_regex);
        auto words_end = std::sregex_iterator();

        bool verified = false;
        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::string found = (*i)[1].str();
            if (found.substr(found.length() - 7) == my_id) {
                verified = true;
                break;
            }
        }

        if (verified) {
            send_notification("ThunderOS Success", "Verified successfully! Enjoy the experience.");
            return 0;
        } else {
            std::string fail_msg = "Rebooting in 30s. Please contact admin " + ADMIN_CONTACT + " for verification.";
            send_notification("ThunderOS Alert", fail_msg);
            sleep(30);
            sync();
            reboot(RB_AUTOBOOT);
            return 1;
        }
    }
}
