#include "FtpClient.h"
#include <iostream>

void LogToConsole(const std::string& msg) {
    std::cout << msg << std::endl;
}

int main() {
    FtpClient client;
    client.setLogCallback(LogToConsole);
    
    std::cout << "Testing FTP connection..." << std::endl;
    // test.rebex.net:21 demo:password protocol: FTP
    if (client.connect("test.rebex.net", 21, "demo", "password", "FTP")) {
        std::vector<RemoteFileInfo> files;
        if (client.listDirectory("/", files)) {
            std::cout << "Found " << files.size() << " files." << std::endl;
            for (const auto& f : files) {
                std::cout << " - " << f.name << std::endl;
            }
        } else {
            std::cout << "Failed to list directory." << std::endl;
        }
    } else {
        std::cout << "Failed to connect." << std::endl;
    }
    return 0;
}