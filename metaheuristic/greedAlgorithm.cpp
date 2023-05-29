#include <iostream>
#include <fstream>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
using namespace std;
    

#define FILENAME "AdjacencyList.txt" // nazwa pliku do odczytu grafu
#define FILENAME2 "Colored.txt" // nazwa pliku do zapisu pokolorowania grafu


vector<array<int, 2>> graph(0); // Lista Sasiedztwa
 // utworzenie zmiennej dla pliku do zapisu


//algorytm zachlanny
int greedyAlg(int vertices, ofstream* myFile2) {
    int max = 0, vertex = 1, dye = 1; // najwieksza wartosc koloru, numer wierzcholka, kolor wierzcholka

    int* connectedColors = new int [vertices] {0}, counter = 0; // tablica kolorow z jakimi wierzcholek ma krawedz wspolna, ostatni indeks tablicy rozny od 0
    int* colors = new int [vertices] {1}; // tablicy kolorow wierzchokow

    // przechodzenie po wierzcholkach grafu
    for (int i = 0; vertex <= vertices; i++) {
        
        // sprawdzanie czy i + 1 miesci sie w zakresie wektora krawedzi, oraz czy dany indeks wektora jest rowny sprawdzanemu wierzcholkowi
        if (i + 1 < graph.size() && graph[i + 1][0] == vertex) 
            continue; // pominiecie iteracji petli

        // sprawdzanie czy i nie wyszlo poza zakres wektora krawedzi
        if (i >= graph.size()) 
            i = graph.size() - 1; // przypisanie do i indeksu koncowego wektora krawedzi

        fill(connectedColors, connectedColors + vertices, 0); // wypelnienie tablicy kolorow z jakimi wierzcholek ma krawedz wspolna wartosciami 0
        counter = 0; // ustawienie wartosci 0 na ostatni indeks tablicy rozny od 0

        // petla przypisania koloru do wierzcholka
        for (int j = 0; j <= i; j++) {

            // sprawdzenie czy krawedz nie zawiera szukanego wierzcholeka, kiedy pierwszy wierzcholek zawiera wartosc mniejsza (wieksze wierzcholki maja wartosc 0, wiec nie ma sensu ich sprawdzac)
            if (graph[j][0] < vertex && graph[j][1] != vertex) 
                continue; // pomin iteracje

            // sprawdzenie czy krawedz nie zawiera szukanego wierzcholka, kiedy drugi wierzcholek zawiera wartosc wieksza, badz rowna (wieksze wierzcholki, tak samo jak szukany maja wartosc 0, wiec nie ma sensu ich sprawdzac) (nie uwzgledniamy grafow z krawedziami do samych siebie, wiec warunek wieksze rowne moze byc zastapiony wieksze)
            if (graph[j][0] == vertex && graph[j][1] >= vertex) 
                break; // przerwij petle

            // wpisanie koloru grafu z ktorym istnieje krawedz do tablicy
            if (graph[j][0] < vertex) connectedColors[counter] = colors[graph[j][0] - 1];
            else connectedColors[counter] = colors[graph[j][1] - 1];
            counter++; // zwiekszenie indeksu
        }

        sort(connectedColors, connectedColors + vertex, [](int a, int b) -> bool {return a > b; }); // posortowanie tablicy kolorow polaczonych wierzcholkow malejaco
        dye = connectedColors[0] + 1; // przypisanie do koloru wartosci wiekszej o 1 niz pierwsza wartosc w tablicy

        // przeszukiwanie tablicy w celu znalezienia minimalnego koloru
        for (int i = 1; i <= counter; i++) {
            
            //sprawdzanie czy kolor jest wiekszy od wartosci komorki w tablicy i czy jest miejsce miedzy komorka i komorka ja poprzedzajaco
            if (dye > connectedColors[i] && (connectedColors[i - 1] - connectedColors[i] > 1)) 
                dye = connectedColors[i] + 1; // przypisanie wolnej wartosci przed komorka o indeksie i
        }

        colors[vertex - 1] = dye; // wpisanie koloru do tablicy kolorow

        // sprawdzanie czy wartosc koloru przekroczyla najwieksza wartosc koloru
        if (dye > max) 
            max = dye; // zaktualizowanie najwiekszej wartosci koloru

        *myFile2 << vertex << "\t" << dye << endl; // wpisanie wierzcholka i odpowiadajacego mu koloru do pliku
        vertex++; // przejscie na kolejny sprawdzany wierzcholek
    }
    
    // posprzatanie po sobie
    delete[] colors;
    colors = 0;
    delete[] connectedColors;
    connectedColors = 0;

    return max; // zwrocenie najwiekszej wartosci koloru
}

bool exist(int a, int b){
    for(int i = 0; i < graph.size(); i++){
        if((graph[i][0] == a && graph[i][1] == b) || (graph[i][0] == b && graph[i][1] == a))
            return true;
    }
    return false;
}

//poczatek wykonywania programu
int main() {
    fstream myFile(FILENAME); // deklaracja zmiennej pliku do odczytu
    ofstream myFile2(FILENAME2); // utworzenie pliku do zapisu
    int vertex1 = 0, vertex2 = 0, vertices = 0; //numer wierzcholka 1, numer wierzcholka 2, liczba wierzchokow

    myFile >> vertices; // wczytanie liczby wierzcholkow grafu z pliku

    // sprawdzenie czy graf posiada wierzcholki
    if (vertices <= 0) 
        return -1;

    // przepisywanie krawedzi grafu z pliku do wektora
    while (myFile >> vertex1 && myFile >> vertex2) {
        if(!exist(vertex1, vertex2))
            graph.push_back({ vertex1, vertex2 });
    }
    sort(graph.begin(), graph.end()); // posortowanie wektora z krawedziami grafu
    myFile2 << vertices << "\n"; 
    myFile2 << greedyAlg(vertices, &myFile2);
    //cout << greedyAlg(vertices); // wykonanie funkcji greedyAlg i wypisanie do konsoli ilosci kolorow potrzebnych do pokolorowania grafu

    return 0;
}
