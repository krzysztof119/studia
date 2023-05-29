#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <algorithm>
#include <cstdlib>

using namespace std;

#define INPUTFILENAME1 "AdjacencyList.txt"
#define INPUTFILENAME2 "Colored.txt"
#define OUTPUTFILENAME "TabuSearch.txt"

#define TABUITERATIONS 1000 // number of iterations of tabuSearch
#define TABULISTSIZE 72 // size of tabu list
#define PATIENCE 10 // how many failed iterations to start deleting from tabu list 

//vector<array<int, 2>> tabu(0); // just an old concept, probably will use something else 


void saveToFile(ofstream* outputFile, vector<vector<int>> *graph){  // print output to file (for now, mostly for tests)
    for(int i = 0; i < (*graph).size(); i++){
            *outputFile << i+1 << ". " << (*graph)[i][0];
        if(i + 1 < (*graph).size())
            *outputFile << "\n";
    }
    return;
}


void setColors(vector<vector<int>>* graph, int maxColor, int deletedColor){
    if(maxColor == deletedColor)
        return;

    for(int i = 0; i < (*graph).size(); i++)
        if((*graph)[i][0] == maxColor)
            (*graph)[i][0] = deletedColor;
    
    return;
}

bool notTabu(vector<int>* colors, vector<int>* tabuList){
    for(int i = 0; i< colors->size();i++){
        for(int j = 0; j < tabuList->size(); j++)
            if((*colors)[i] == (*tabuList)[j])
                return false;
    }
    return true;
}

int leastColors(vector<vector<int>> *graph, int maxColor, vector<vector<int>>* colors, vector<int>* tabuList){ // finding which color has least vertices 
    // ignore values for tabu list
    
    colors->clear();  // resets vector 
    colors->resize(maxColor); // setting up empty vectors representing possible colors

    for(int i = 0; i < graph->size(); i++)    // matching vertices with vector representing it's color
        (*colors)[(*graph)[i][0] - 1].push_back(i+1); 

    int result = -1, help = -1;   // finding which vector representing color is shortest
    for(int j = 0; j < colors->size(); j++)
        if(((*colors)[j].size() < help || help == -1) && notTabu(&(*colors)[j], tabuList)){
            help = (*colors)[j].size();
            result =  j;
        }
    
    return result;
}


int checkConflicts(vector<vector<int>>* graph, int vertex, vector<vector<int>>* conflicts, int newColor){   // checks how much conflicts are for 'vertex'
    int result = 0;
    vector<int> temp(0);
    temp.push_back(newColor);

    for(int i = 1; i < (*graph)[vertex-1].size(); i++) //checking if there is conflict with vertex and it's edges
        if((*graph)[vertex-1][0] == (*graph)[((*graph)[vertex-1][i] - 1)][0]){  // if there is then increment result and note it in temp
            result++;
            temp.push_back((*graph)[vertex-1][i]);
        }
    
    if(conflicts == nullptr){
        temp.clear();
        return result;
    }

    for(int i = 0; i < conflicts->size(); i++){
        if((*conflicts)[i].size() != 0)
            continue;
            
        (*conflicts)[i].insert((*conflicts)[i].begin(), temp.begin(), temp.end());
        temp.clear();
    }

    return result;
}

bool resolveConflicts(vector<vector<int>>* graph, vector<vector<int>>* conflicts, int maxColor, int conflictIteration, int conflictVertexIteration, int bypass){
    
    for(int k = 1; k < maxColor; k++){
        if(k == bypass)
            continue;

        (*graph)[(*conflicts)[conflictIteration][conflictVertexIteration] -1][0] = k;
        if(checkConflicts(graph, (*conflicts)[conflictIteration][conflictVertexIteration], nullptr, 1) == 0)
            return true;
    }

    return false;
}



void tabuSearch(vector<vector<int>>* graph, int maxColor){
    vector<vector<int>> colors(0);  // representation of belongingness vertices for colors (colors[color][-] = vertex, example. colors[0][3] = 5 -> vertex 5 have color 1 <- (0+1=1))
    vector<vector<int>> conflicts(0); // color causing least conflicts and conflicting vertices
    vector<int> tabuList(0);
    int* nonConflictingColors = new int [graph->size()]; // consist vertex and color to have backup if one of the colors of vertices will not be possible to change
    int index = 0, iterations = 0, fails = 0;
    bool colorAlright, global;

    while(iterations < TABUITERATIONS){
        for(int i = 0; i < graph->size(); i++){
            nonConflictingColors[i] = (*graph)[i][0];
        }

        index = leastColors(graph, maxColor, &colors, &tabuList);   // finding which color has least vertices (result can be used as index to vertex 'colors' to access list of vertices for this color), -1 -> not possible to find color to change
        while(index == -1){
            tabuList.pop_back();
            index = leastColors(graph, maxColor, &colors, &tabuList);
        }  
        global = true;

        for(int i = 0; i < colors[index].size(); i++){  // checking how many conflict are (or even if there is any)
            conflicts.clear();
            conflicts.resize(maxColor - 2);
            //cout<<"\nvertex: "<< colors[index][i] << "\t" << conflicts.size() << endl;

            for(int j = 1; j <= maxColor; j++){
                //cout<<"\ncolor: "<< j <<endl;

                if(j != index+1){
                    (*graph)[colors[index][i] - 1][0] = j;
                    checkConflicts(graph, colors[index][i], &conflicts, j);
                    //cout << "\t" << checkConflicts(graph, colors[index][i], &conflicts, j) << "\t" << conflicts.size();
                }
                /*else
                    cout << "NULL";*/
            }

            sort(conflicts.begin(), conflicts.end(), [](vector<int> a, vector<int> b) -> bool {return a.size() > b.size(); });

            if(tabuList.size() < maxColor)
                tabuList.insert(tabuList.begin(), colors[index][i]);
            
            if(tabuList.size() > TABULISTSIZE || (fails > PATIENCE && tabuList.size() > 0))
                tabuList.pop_back();
            
            for(int z = (conflicts.size() -1); z >= 0; z--){

                (*graph)[(colors[index][i] - 1)][0] = conflicts[z][0];
                
                colorAlright = false;

                if(conflicts[z].size() == 1){
                    colorAlright = true;
                }
                else{

                    for(int x = 1; x < conflicts[z].size(); x++){
                        
                        if(resolveConflicts(graph, &conflicts, maxColor, z, x, index+1)){
                            colorAlright = true;
                        }

                        else{
                            if(z == 0)
                                global = false;
                            colorAlright = false;
                            for(int i = 1; i <= x; i++)
                                (*graph)[conflicts[z][i] -1][0] = nonConflictingColors[conflicts[z][i] -1];
                            break;
                        }
                    }
                }
                if(colorAlright || !global)
                    break;
            }
            if(!colorAlright || !global)
                break;
            
        }

        if(!colorAlright || index == -1 || !global){
            fails++;
            for(int i = 0; i < graph->size(); i++)
                (*graph)[i][0] = nonConflictingColors[i];
        }
        else{
            setColors(graph, maxColor, index+1);
            maxColor--;
            fails = 0;
        }
        iterations++;
    }

    cout << "Tabu Search result:\t" << maxColor << endl;
    conflicts.clear();colors.clear();tabuList.clear();
    delete [] nonConflictingColors;
    cout<<(*graph).size();

    return;
}

int main (int argc,char *argv[]){
    fstream inputFile1(INPUTFILENAME1);
    ofstream outputFile(OUTPUTFILENAME); 
    
    int vertices = 0, vertex1 = 0, vertex2 = 0, maxColor = 0, temp = 0;

    srand(time(NULL));  // seting random seed so it's not always the same output for exact input

    inputFile1 >> vertices;
    if (vertices <= 0) 
        return -1;
    
    int gate = false;
    vector<vector<int>> graph(vertices);
    while (inputFile1 >> vertex1 && inputFile1 >> vertex2) {    // creating structure of graph
    
        gate = false;
        for (int i = 0; i < graph[vertex1-1].size(); i++) {
            if(graph[vertex1-1].size() != 0)
            if (graph[vertex1-1][i] == vertex2) {
                gate=true;
                break;
            }
        }
        if(!gate)
            graph[vertex1-1].push_back(vertex2);

        gate = false;
        for (int i = 0; i < graph[vertex2-1].size(); i++) {
            if (graph[vertex2-1][i] == vertex1) {
                gate=true;
                break;
            } 
        }
        if(!gate)
            graph[vertex2-1].push_back(vertex1);
    }
    inputFile1.close();
    fstream inputFile2(INPUTFILENAME2);

    inputFile2 >> temp;
    if (temp != vertices)
        return -1;

    while (inputFile2 >> vertex1 && inputFile2 >> vertex2){     // adding colors of vertices to the structure of graph
        graph[vertex1-1].insert(graph[vertex1-1].begin(), vertex2);
    }
    inputFile2.close();

    maxColor = vertex1; // geting highest color value from greedyAlg 
    cout << "Greedy Algorithm result:\t" <<maxColor << endl;

    tabuSearch(&graph, maxColor);   // tabuSearch

    saveToFile(&outputFile, &graph);    // output
    outputFile.close();
    return 0;
}
