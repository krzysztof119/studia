#include <iostream>
#include <fstream>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace std;

#define VERTICES 500 // liczba wierzcholkow
#define EXPECTED_DENSITY 0.55 // oczekiwana gestosci grafu (0 -> 0%, 1 -> 100%)
#define FILENAME "AdjacencyList.txt" // nazwa pliku wyjsciowego

vector<array<int, 2>> graph(0);
int edges = 0; // licznik krawedzi

// sprawdzanie istnieje krawedz miedzy wierzcholkami
bool exists(int vertex1, int vertex2) { 
    // przeszukiwanie wewnatrz wektora utworzonych krawedzi
    for (int i = 0; i < graph.size(); i++) {
        if ((graph[i][0] == vertex1 && graph[i][1] == vertex2) || (graph[i][0] == vertex2 && graph[i][1] == vertex1)) 
            return true; // jesli znaleziono krawedz, zwroc true
    }
    return false; // jesli nie znaleziono krawedzi, zwroc false
}

//poczatek wykonywania programu
int main() {
    ofstream myFile; // utworzenie zmiennej dla pliku wyjsciowego
    myFile.open(FILENAME); // utworzenie pliku
    myFile << VERTICES << endl; // wpisanie ilosci wiercholkow do pliku
    int density = (((VERTICES * VERTICES) - VERTICES) / 2), vertex1 = 1, vertex2 = 1; // maksymalna gestosc grafu, numer wierzcholka 1, numer wierzcholka 2

    srand(time(NULL)); // ustawienie losowego seeda

    // tworzenie krawedzi dopoki gestosc grafu jest mniejsza od oczekiwanej
    while ((float)edges / density < EXPECTED_DENSITY) {
        vertex1 = rand() % VERTICES + 1; //losowa liczba od 1 do liczby wierzcholkow
        vertex2 = rand() % VERTICES + 1; //losowa liczba od 1 do liczby wierzcholkow

        // sprawdzanie czy dana krawedz istnieje, bardz wierzcholki wskazuja ten sam wierzcholek
        if (exists(vertex1, vertex2) || (vertex1 == vertex2)) 
            continue;  // pominiecie iteracji

        graph.push_back({ vertex1, vertex2 }); // utworzenie krawedzi
        edges++; // zwiekszenie licznika krawedzi
    }

    sort(graph.begin(), graph.end()); // posortowanie wektora krawedzi rosnaca

    // wypisanie krawedzi do pliku
    for (int i = 0; i < graph.size(); i++) {
        if (i + 1 == edges) myFile << graph[i][0] << " " << graph[i][1];
        else myFile << graph[i][0] << " " << graph[i][1] << endl;
    }

    graph.push_back({ density, edges }); // bycie fancy, albo niepotrzebna linijka kodu
    cout << endl << graph[edges][0] << " " << graph[edges][1] << " " << (float)graph[edges][1] / graph[edges][0]; // wypisanie maksymalnej liczby krawedzi, liczby utworzonych krawedzi, oraz gestosci utworzonego grafu

    return 0;
}
