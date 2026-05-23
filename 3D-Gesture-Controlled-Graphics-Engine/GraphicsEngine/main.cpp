#define NOMINMAX
#include <SDL3/SDL.h>
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

// Using 4D vectors [x, y, z, w] to allow unified matrix transformations for 3D points.
typedef float HPoint3D[4];
typedef float Transformation3D[4][4];

// Applies a 4x4 transformation matrix 'a' to an array of homogeneous points 'p'.
void apply3D(Transformation3D a, int n, HPoint3D p[], HPoint3D ap[]) {
    int i, j;
    for (i = 0; i < n; i++) {
        for (j = 0; j < 4; j++) {
            ap[i][j] = a[j][0] * p[i][0] + a[j][1] * p[i][1] + a[j][2] * p[i][2] + a[j][3] * p[i][3];
        }
    }
}

// Scaling Matrix: Modifies the diagonal elements for 3D scaling (Sx, Sy, Sz).
void scale3D_matrix(float sx, float sy, float sz, Transformation3D a) {
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) a[i][j] = 0;
    a[0][0] = sx; a[1][1] = sy; a[2][2] = sz; a[3][3] = 1.0f;
}

// Y-Axis Rotation Matrix: Populated with cosine and sine for Y-axis rotation.
void rotateY3D_matrix(float angle, Transformation3D a) {
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) a[i][j] = 0;
    float rad = angle * (3.14159f / 180.0f);
    a[0][0] = cos(rad); a[0][2] = sin(rad);
    a[1][1] = 1.0f;
    a[2][0] = -sin(rad); a[2][2] = cos(rad);
    a[3][3] = 1.0f;
}

// X-Axis Rotation Matrix: Populated with cosine and sine for X-axis rotation.
void rotateX3D_matrix(float angle, Transformation3D a) {
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) a[i][j] = 0;
    float rad = angle * (3.14159f / 180.0f);
    a[0][0] = 1.0f;
    a[1][1] = cos(rad); a[1][2] = -sin(rad);
    a[2][1] = sin(rad); a[2][2] = cos(rad);
    a[3][3] = 1.0f;
}

// Manual pixel calculation using integer arithmetic, avoiding built-in SDL_RenderLine.
void linie(SDL_Renderer* renderer, int xa, int ya, int xb, int yb) {
    int dx = xb - xa;
    int dy = yb - ya;
    int dx2, dy2, s, px = 1, py = 1, x = xa, y = ya;

    if (dx < 0) { px = -1; dx = -dx; }
    if (dy < 0) { py = -1; dy = -dy; }

    dx2 = dx + dx; dy2 = dy + dy;

    if (dx > dy) {
        s = dy2 - dx;
        while (x != xb) {
            SDL_RenderPoint(renderer, (float)x, (float)y);
            x += px;
            if (s >= 0) { s -= dx2; y += py; }
            s += dy2;
        }
        SDL_RenderPoint(renderer, (float)x, (float)y);
    }
    else {
        s = dx2 - dy;
        while (y != yb) {
            SDL_RenderPoint(renderer, (float)x, (float)y);
            y += py;
            if (s >= 0) { s -= dy2; x += px; }
            s += dx2;
        }
        SDL_RenderPoint(renderer, (float)x, (float)y);
    }
}

int main(int argc, char* argv[]) {

    // UDP Socket Initialization for local Python communication
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;
    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5005);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    bind(udpSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    u_long mode = 1;
    ioctlsocket(udpSocket, FIONBIO, &mode); // Non-blocking socket

    if (!SDL_Init(SDL_INIT_VIDEO)) return 1;
    int width = 800, height = 600;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_CreateWindowAndRenderer("3D Homogeneous Engine", width, height, 0, &window, &renderer);

    // Defined using Homogeneous Coordinates (w=1).
    HPoint3D cubeVerts[8] = {
        {-1, -1, -1, 1}, { 1, -1, -1, 1}, { 1,  1, -1, 1}, {-1,  1, -1, 1},
        {-1, -1,  1, 1}, { 1, -1,  1, 1}, { 1,  1,  1, 1}, {-1,  1,  1, 1}
    };
    int cubeEdges[12][2] = { {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7} };

    HPoint3D pyramidVerts[5] = {
        { 0, -1,  0, 1},
        {-1,  1, -1, 1}, { 1,  1, -1, 1}, { 1,  1,  1, 1}, {-1,  1,  1, 1}
    };
    int pyramidEdges[8][2] = { {0,1},{0,2},{0,3},{0,4},{1,2},{2,3},{3,4},{4,1} };

    bool running = true;
    SDL_Event event;
    std::string currentColor = "None";
    float targetRotX = 0.0f, targetRotY = 0.0f, currentScale = 1.0f, autoRotY = 0.0f;
    char buffer[256];
    sockaddr_in senderAddr;
    int addrLen = sizeof(senderAddr);

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
        }

        // Parse UDP network data to dynamically update translation and scaling parameters
        int bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0, (SOCKADDR*)&senderAddr, &addrLen);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            std::string msgStr(buffer);
            std::stringstream ss(msgStr);
            std::string cStr, xStr, yStr, aStr;

            if (std::getline(ss, cStr, ',') && std::getline(ss, xStr, ',') && std::getline(ss, yStr, ',') && std::getline(ss, aStr, ',')) {
                currentColor = cStr;
                currentScale = std::max(0.2f, std::min(3.0f, std::stoi(aStr) / 35000.0f));
                targetRotY = (std::stoi(xStr) - 320) * 0.4f;
                targetRotX = (std::stoi(yStr) - 240) * 0.4f;
            }
        }

        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);

        if (currentColor != "None") {
            HPoint3D* activeVerts = cubeVerts;
            int (*activeEdges)[2] = cubeEdges;
            int numVerts = 8, numEdges = 12;

            if (currentColor == "Red") { SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255); }
            else if (currentColor == "Blue") {
                SDL_SetRenderDrawColor(renderer, 50, 150, 255, 255);
                activeVerts = pyramidVerts; activeEdges = pyramidEdges;
                numVerts = 5; numEdges = 8;
            }
            else if (currentColor == "Green") {
                SDL_SetRenderDrawColor(renderer, 50, 255, 50, 255);
                autoRotY += 3.0f; if (autoRotY > 360.0f) autoRotY -= 360.0f;
            }

            float finalRotX = (currentColor == "Green") ? 20.0f : targetRotX;
            float finalRotY = (currentColor == "Green") ? autoRotY : targetRotY;

            // Constructing the matrices based on dynamic data
            Transformation3D scaleMat, rotXMat, rotYMat;
            scale3D_matrix(currentScale, currentScale, currentScale, scaleMat);
            rotateX3D_matrix(finalRotX, rotXMat);
            rotateY3D_matrix(finalRotY, rotYMat);

            HPoint3D transformedVerts[8];

            // Applying transformations sequentially using matrix multiplication
            apply3D(scaleMat, numVerts, activeVerts, transformedVerts);
            apply3D(rotXMat, numVerts, transformedVerts, transformedVerts);
            apply3D(rotYMat, numVerts, transformedVerts, transformedVerts);

            for (int i = 0; i < numEdges; i++) {
                HPoint3D p1, p2;
                for (int j = 0; j < 4; j++) {
                    p1[j] = transformedVerts[activeEdges[i][0]][j];
                    p2[j] = transformedVerts[activeEdges[i][1]][j];
                }

                float fov = 400.0f;
                // Translating along the Z-axis to prevent near-clipping division by zero
                float z1 = p1[2] + 4.0f; if (z1 <= 0.1f) z1 = 0.1f;
                float z2 = p2[2] + 4.0f; if (z2 <= 0.1f) z2 = 0.1f;

                // Applying the projection formula: x_projected = (x * fov) / z
                int x1 = (int)((p1[0] * fov) / z1 + (width / 2.0f));
                int y1 = (int)((p1[1] * fov) / z1 + (height / 2.0f));
                int x2 = (int)((p2[0] * fov) / z2 + (width / 2.0f));
                int y2 = (int)((p2[1] * fov) / z2 + (height / 2.0f));

                // Sending calculated 2D coordinates to Bresenham algorithm
                linie(renderer, x1, y1, x2, y2);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    closesocket(udpSocket);
    WSACleanup();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}