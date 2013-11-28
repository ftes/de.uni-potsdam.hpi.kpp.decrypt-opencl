#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <math.h>
#include <unistd.h>
#include "Hotspot.h"
#include "Coordinate.h"

using namespace std;

int width;
int height;
int numberOfRounds;
int currentRound = 0;
bool writeCoords = false;
vector<Coordinate> coords;
vector<Hotspot> hotspots;

vector<vector<double >> *oldHeatmap;
vector<vector<double >> *currentHeatmap;

string outFile = "output.txt";

void setHotspots() {
    for (Hotspot h : hotspots) {
        if (currentRound >= h.startRound && currentRound < h.endRound) {
            (*currentHeatmap)[h.y][h.x] = 1;
        }
    }
}

vector<vector<int >> parseCsv(string fileName) {
    ifstream file(fileName.c_str());
    vector < vector<int >> result;
    string line;
    //ignore first line
    getline(file, line);
    while (getline(file, line)) {
        vector<int> items;
        stringstream ss(line);
        int i;
        while (ss >> i) {
            items.push_back(i);
            if (ss.peek() == ',') ss.ignore();
        }
        result.push_back(items);
    }
    file.close();
    return result;
}

double getAverageHeat(int centerX, int centerY) {
    int fromX = centerX == 0 ? 0 : centerX - 1;
    int toX = centerX == width - 1 ? centerX : centerX + 1;
    int fromY = centerY == 0 ? 0 : centerY - 1;
    int toY = centerY == height - 1 ? centerY : centerY + 1;

    double sum = 0;

	//somehow slows this down
	//#pragma omp parallel for collapse(2) reduction(+:sum)
    for (int y = fromY; y < toY + 1; y++) {
        for (int x = fromX; x < toX + 1; x++) {
            sum += (*oldHeatmap)[y][x];
        }
    }

    return sum / 9;
}

void performRound() {
    auto tmp = oldHeatmap;
    oldHeatmap = currentHeatmap;
    currentHeatmap = tmp;

	#pragma omp parallel for collapse(2)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
        	(*currentHeatmap)[y][x] = getAverageHeat(x, y);
        }
    }

    currentRound++;
    setHotspots();
}

// have to use *& to pass pointer as reference, otherwise we get a copy
// of the pointer, and if we assign a new memory location to it this is local

void initializeHeatmap(vector<vector<double >> *&heatmap) {
    heatmap = new vector < vector<double >> (height, vector<double>(width, 0));
}

string getOutputValue(double cell) {
    string value;
    if (cell > 0.9) {
        value = "X";
    } else {
        value = to_string((int) floor((cell + 0.09) * 10));
    }
    return value;
}

void writeOutput() {
    remove(outFile.c_str());
    ofstream output(outFile.c_str());
    if (!writeCoords) {
        for (vector<double> row : *currentHeatmap) {
            for (double cell : row) {
                output << getOutputValue(cell).c_str();
            }
            output << "\n";
        }
    } else {
        for (Coordinate coord : coords) {
            double cell = (*currentHeatmap)[coord.y][coord.x];
            output << cell <<"\n";
        }
    }
    output.close();
}

void printHeatmap(vector<vector<double >> *heatmap) {
    for (vector<double> row : *heatmap) {
        for (double cell : row) {
            printf("%s ", getOutputValue(cell).c_str());
        }
        printf("\n");
    }
}

int main(int argc, char* argv[]) {
    //read input
    width = atoi(argv[1]);
    height = atoi(argv[2]);
    numberOfRounds = atoi(argv[3]);

    string hotspotsFile = string(argv[4]);

    if (argc == 6) {
        string coordFile = string(argv[5]);
        for (vector<int> line : parseCsv(coordFile)) {
            Coordinate coord(line.at(0), line.at(1));
            coords.push_back(coord);
        }
        writeCoords = true;
    }

    //read hotspots
    for (vector<int> line : parseCsv(hotspotsFile)) {
        Hotspot hotspot(line.at(0), line.at(1), line.at(2), line.at(3));
        hotspots.push_back(hotspot);
    }

    initializeHeatmap(currentHeatmap);
    initializeHeatmap(oldHeatmap);
    setHotspots();

    while (currentRound < numberOfRounds) {
        performRound();
    }

    writeOutput();

    exit(0);
}
