#include <vector>
#include <stdio.h>
#include "Oxide/Oxide/vendor/glm/vec3.hpp"
#include <cmath>

typedef unsigned int uint;

void GenerateConnections(uint numpoints, std::vector<std::pair<uint, uint>>& connections) {
    //2D Sides
    //XZ
    for (int xs = 0; xs < numpoints; xs++) {
        for (int ys = 0; ys < numpoints; ys++) {
            for (int zstep = -1; zstep < 2; zstep+=2) {
                for (int xstep = -1; xstep < 2; xstep+=2) {
                    int ystep = 0;
                    int x = xs;
                    int y = ys;
                    int z = (ystep<0) ? (numpoints-1) : 0;
                    while (true) {
                            if (x + xstep < 0 || y + ystep < 0 || x + zstep < 0) break;
                            if (x + xstep == numpoints || y + ystep == numpoints || z + zstep == numpoints) break;
                            connections.push_back({x * numpoints * numpoints + y * numpoints + z, (x+xstep) * numpoints * numpoints + (y+ystep) * numpoints + (z+zstep)});
                            x += xstep;
                            y += ystep;
                            z += zstep;
                    }
                }
            }
        }
    }
    //XY
    for (int xs = 0; xs < numpoints; xs++) {
        for (int zs = 0; zs < numpoints; zs++) {
            for (int ystep = -1; ystep < 2; ystep+=2) {
                for (int xstep = -1; xstep < 2; xstep+=2) {
                    int zstep = 0;
                    int x = xs;
                    int y = (ystep<0) ? (numpoints-1) : 0;
                    int z = zs;
                    while (true) {
                            if (x + xstep < 0 || y + ystep < 0 || x + zstep < 0) break;
                            if (x + xstep == numpoints || y + ystep == numpoints || z + zstep == numpoints) break;
                            connections.push_back({x * numpoints * numpoints + y * numpoints + z, (x+xstep) * numpoints * numpoints + (y+ystep) * numpoints + (z+zstep)});
                            x += xstep;
                            y += ystep;
                            z += zstep;
                    }
                }
            }
        }
    }
    //ZY
    for (int xs = 0; xs < numpoints; xs++) {
        for (int zs = 0; zs < numpoints; zs++) {
            for (int ystep = -1; ystep < 2; ystep+=2) {
                for (int zstep = -1; zstep < 2; zstep+=2) {
                    int xstep = 0;
                    int x = xs;
                    int y = (ystep<0) ? (numpoints-1) : 0;
                    int z = zs;
                    while (true) {
                            if (x + xstep < 0 || y + ystep < 0 || x + zstep < 0) break;
                            if (x + xstep == numpoints || y + ystep == numpoints || z + zstep == numpoints) break;
                            connections.push_back({x * numpoints * numpoints + y * numpoints + z, (x+xstep) * numpoints * numpoints + (y+ystep) * numpoints + (z+zstep)});
                            x += xstep;
                            y += ystep;
                            z += zstep;
                    }
                }
            }
        }
    }
    //Diagonals
    for (int xs = 0; xs < numpoints; xs++) {
        for (int zs = 0; zs < numpoints; zs++) {
            for (int ystep = -1; ystep < 2; ystep+=2) {
                for (int xstep = -1; xstep < 2; xstep+=2) {
                    for (int zstep = -1; zstep < 2; zstep+=2) {
                        int x = xs;
                        int z = zs;
                        int y = (ystep < 0) ? (numpoints - 1) : 0;
                        while (true) {
                            if (x + xstep < 0 || y + ystep < 0 || x + zstep < 0) break;
                            if (x + xstep == numpoints || y + ystep == numpoints || z + zstep == numpoints) break;
                            connections.push_back({x * numpoints * numpoints + y * numpoints + z, (x+xstep) * numpoints * numpoints + (y+ystep) * numpoints + (z+zstep)});
                            x += xstep;
                            y += ystep;
                            z += zstep;
                        }
                    }
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    float scale = 1.0f;
    uint numpoints = 100;
    std::vector<glm::vec3> points(pow(numpoints,3));
    std::vector<std::pair<uint, uint>> connections;
    for (int x = 0; x < numpoints; x++) {
        for (int y = 0; y < numpoints; y++) {
            for (int z = 0; z < numpoints; z++) {
                points[x * numpoints * numpoints + y * numpoints + z] = glm::vec3(scale/numpoints * x, scale/numpoints * y, scale/numpoints * z);
                if (z != numpoints - 1)
                    connections.push_back({x * numpoints * numpoints + y * numpoints + z, (x + 1) * numpoints * numpoints + y * numpoints + z});
                if (y != numpoints - 1)
                    connections.push_back({x * numpoints * numpoints + y * numpoints + z, x * numpoints * numpoints + (y+1) * numpoints + z});
                if (x != numpoints - 1)
                    connections.push_back({x * numpoints * numpoints + y * numpoints + z, x * numpoints * numpoints + y * numpoints + z+1});
            }
        }
    }
    GenerateConnections(numpoints, connections);
    
}