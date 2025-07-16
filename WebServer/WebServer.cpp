#include <iostream>
#include <string>
#include <vector>
#include <sstream> // For string streams
#include <ctime>   // For current time in response

// Required for Winsock
#include <winsock2.h>
#include <ws2tcpip.h> // Required for inet_pton, etc.
#pragma comment(lib, "ws2_32.lib") // Link with ws2_32.lib

// Function to handle a client connection
void handleClient(SOCKET clientSocket) {
    char buffer[4096]; // Buffer to receive HTTP request
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesReceived <= 0) {
        std::cerr << "Failed to receive data or client disconnected. Error: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        return;
    }
    buffer[bytesReceived] = '\0'; // Null-terminate the received data

    std::cout << "\n--- Received Request ---\n" << buffer << "------------------------\n";

    // Very basic HTTP request parsing: just get the first line (Method Path HTTP/Version)
    std::string request(buffer);
    std::istringstream iss(request);
    std::string httpMethod, requestPath, httpVersion;
    iss >> httpMethod >> requestPath >> httpVersion;

    std::string responseBody;
    std::string contentType = "Content-Type: text/html; charset=utf-8";
    std::string statusCode = "200 OK";

    // Simple routing based on requestPath
    if (requestPath == "/") {
        responseBody = R"(<!DOCTYPE html>
<html>
<head>
    <title>C++ Web Server on Windows</title>
    <style>
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; text-align: center; margin-top: 50px; background-color: #f0f0f0; color: #333; }
        .container { background-color: #fff; padding: 30px; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); display: inline-block; }
        h1 { color: #0056b3; }
        p { line-height: 1.6; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Hello from a C++ Web Server on Windows!!!</h1>
        <p>This is a basic example using Winsock.</p>
        <p>Server time (Lewisville, Texas, USA): )";

        // Get current time
        time_t now = time(0);
        char dt[26];
        ctime_s(dt, sizeof(dt), &now); // ctime_s is safer for Windows
        responseBody += dt;
        responseBody += R"(</p>
        <p>Try navigating to <a href="/about">/about</a></p>
    </div>
</body>
</html>)";
    }
    else if (requestPath == "/about") {
        responseBody = R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">

    <title>About This Server</title>
    <style>
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; text-align: center; margin-top: 50px; background-color: #f0f0f0; color: #333; }
        .container { background-color: #fff; padding: 30px; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); display: inline-block; }
        h1 { color: #0056b3; }
        p { line-height: 1.6; }
    </style>
</head>
<body>
    <div class="container">
        <h1>About This Simple Server! الله</h1>
        <p>This server demonstrates fundamental Winsock API usage in C++.</p>
        <p>It's single-threaded and suitable for learning purposes.</p>
        <p><a href="/">Go back to Home</a></p>
    </div>
</body>
</html>)" + contentType;
    }
    else {
        statusCode = "404 Not Found";
        responseBody = R"(<!DOCTYPE html>
<html>
<head>
    <title>404 Not Found</title>
    <style>
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; text-align: center; margin-top: 50px; background-color: #f0f0f0; color: #333; }
        .container { background-color: #fff; padding: 30px; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); display: inline-block; }
        h1 { color: #e74c3c; }
        p { line-height: 1.6; }
    </style>
</head>
<body>
    <div class="container">
        <h1>404 Not Found</h1>
        <p>The requested resource ')" + requestPath + R"(' was not found on this server.</p>
        <p><a href="/">Go back to Home</a></p>
    </div>
</body>
</html>)";
    }

    // Construct the HTTP response
    std::ostringstream oss;
    oss << "HTTP/1.1 " << statusCode << "\r\n";
    oss << "Content-Type: " << contentType << "\r\n";
    oss << "Content-Length: " << responseBody.length() << "\r\n";
    oss << "Connection: close\r\n"; // Simple: close connection after each response
    oss << "\r\n"; // End of headers
    oss << responseBody;

    std::string httpResponse = oss.str();
    int bytesSent = send(clientSocket, httpResponse.c_str(), httpResponse.length(), 0);
    if (bytesSent == SOCKET_ERROR) {
        std::cerr << "Failed to send data. Error: " << WSAGetLastError() << std::endl;
    }
    else {
        std::cout << "Sent " << bytesSent << " bytes response." << std::endl;
    }

    // Close the client socket
    closesocket(clientSocket);
    std::cout << "Client disconnected." << std::endl;
}

int main() {
    // 1. Initialize Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData); // Request Winsock 2.2
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }

    SOCKET listenSocket = INVALID_SOCKET;
    sockaddr_in serverAddr;
    int port = 8080;

    // 2. Create a socket
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "Error at socket(): " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // Optional: Set SO_REUSEADDR to allow immediate reuse of the port after server shutdown
    int yes = 1;
    if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes)) == SOCKET_ERROR) {
        std::cerr << "setsockopt SO_REUSEADDR failed with error: " << WSAGetLastError() << std::endl;
    }

    // 3. Bind the socket
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port); // Convert port to network byte order
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Listen on all network interfaces

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed with error: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // 4. Listen for incoming connections
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) { // SOMAXCONN is a reasonable backlog
        std::cerr << "Listen failed with error: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server listening on port " << port << std::endl;
    std::cout << "Open your browser and navigate to http://localhost:" << port << "/" << std::endl;
    std::cout << "Or try http://localhost:" << port << "/about" << std::endl;
    std::cout << "Press Ctrl+C to stop the server." << std::endl;

    // 5. Accept and handle connections in a loop
    while (true) {
        SOCKET clientSocket = INVALID_SOCKET;
        // Accept a client connection (this is blocking)
        clientSocket = accept(listenSocket, NULL, NULL); // NULL for client address info in this example

        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed with error: " << WSAGetLastError() << std::endl;
            // Depending on the error, you might want to break or continue
            continue;
        }

        std::cout << "\nClient connected (Socket: " << clientSocket << ")." << std::endl;
        handleClient(clientSocket); // Handle the client request
        // In a real server, this would typically be spawned into a new thread or handled asynchronously
    }

    // 6. Cleanup (This part is typically only reached on graceful shutdown, e.g., Ctrl+C)
    closesocket(listenSocket);
    WSACleanup();

    return 0;
}