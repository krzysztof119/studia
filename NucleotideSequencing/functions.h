#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <limits.h>

#define NUCLEOTIDESAMOUNT 4
#define TARGETMAXCONNECTIONLEVEL 3
#define MAXEDGES 84
#define MAXUSEDNODE 3
#define MAXUSEDDIJKSTRA 2
#define DNAREADFILE "dnachain.txt"
#define RESULTFILE "results.csv"

struct LinkedList{
    int timesUsed, index;
    struct LinkedList *next;
    struct LinkedList *incidenceList[MAXEDGES];
    char oligonucleotide[];
};

struct OlignonucleotideMap{
    int pathLength;
    bool visited;
    struct LinkedList *prev;
};

struct Queue{
    int priority;
    struct LinkedList *current;
    struct Queue *next;
};

#define INIT_MAP(X) X.pathLength = INT_MAX; X.visited = false; X.prev = NULL
// #define MIN(x,y) ((x) < (y) ? (x) : (y))

void swap(int *xp, int *yp){ 
    int temp = *xp; 
    *xp = *yp; 
    *yp = temp; 
} 

void selectionSort(int *arr, int n){ 
    int i, j, min_idx; 
  
    // One by one move boundary of unsorted subarray 
    for (i = 0; i < n-1; i++) 
    { 
        // Find the minimum element in unsorted array 
        min_idx = i; 
        for (j = i+1; j < n; j++) 
          if (arr[j] < arr[min_idx]) 
            min_idx = j; 
  
        // Swap the found minimum element with the first element 
           if(min_idx != i) 
            swap(&arr[min_idx], &arr[i]); 
    } 
} 

void selectionSortStrings(char **arr, int positiveErrorAmount, int oligonucleotideLength){
    int i, j, min_idx;
  
    // One by one move boundary of unsorted subarray
    char *minStr = (char*)malloc(oligonucleotideLength * sizeof(char));
    for (i = 0; i < positiveErrorAmount-1; i++)
    {
        // Find the minimum element in unsorted array
        int min_idx = i;
        strcpy(minStr, arr[i]);
        for (j = i+1; j < positiveErrorAmount; j++)
        {
            // If min is greater than arr[j]
            if (strcmp(minStr, arr[j]) > 0)
            {
                // Make arr[j] as minStr and update min_idx
                strcpy(minStr, arr[j]);
                min_idx = j;
            }
        }
  
        // Swap the found minimum element with the first element
        if (min_idx != i)
        {
            char *temp = (char*)malloc(oligonucleotideLength * sizeof(char));
            strcpy(temp, arr[i]); //swap item[pos] and item[i]
            strcpy(arr[i], arr[min_idx]);
            strcpy(arr[min_idx], temp);
        }
    }
}

char randNucleotide(){
    int r = rand() % NUCLEOTIDESAMOUNT;

    switch(r){
        case 0:
            return 'A';
        
        case 1:
            return 'C';
        
        case 2:
            return 'G';

        case 3:
            return 'T';
        
        default:
            perror("Unexpected behaviour");
    }
}

int randRange(int n){
    int limit = RAND_MAX - (RAND_MAX % n);
    int randomNumber = 0;

    do{
        randomNumber = rand();
    }while(randomNumber >= limit);

    return randomNumber % n;
}

void fillList(struct LinkedList **head, int *olignonucleotideAmount, char *dnaSequence, int oligonucleotideLength, int oligonucleotidesMaxAmount){
    for(int i = 1; i < oligonucleotidesMaxAmount; i++){
        
        struct LinkedList *temp = *head;
        struct LinkedList *newOlignonucleotide = (struct LinkedList*)malloc(sizeof(struct LinkedList) + oligonucleotideLength + 1);
        memcpy(newOlignonucleotide->oligonucleotide, dnaSequence + i, oligonucleotideLength); newOlignonucleotide->oligonucleotide[oligonucleotideLength] = '\0'; newOlignonucleotide->timesUsed = 0;
        
        int result = strncmp(dnaSequence + i, temp->oligonucleotide, oligonucleotideLength);

        if(result < 0){
            newOlignonucleotide->next = temp;
            *head = newOlignonucleotide;
            (*olignonucleotideAmount)++;
            continue;
        }

        if(result == 0){
            free(newOlignonucleotide);
            continue;
        }

        while(temp->next){
            result = strncmp(dnaSequence + i, temp->next->oligonucleotide, oligonucleotideLength);

            if(result <= 0){
                break;
            }

            temp = temp->next;
        }
        
        if(result == 0){
            free(newOlignonucleotide);
            continue;
        }
        
        (*olignonucleotideAmount)++;

        if(result > 0){
            newOlignonucleotide->next = NULL;
            temp->next = newOlignonucleotide;
            continue;
        }
        
        newOlignonucleotide->next = (temp->next) ? temp->next : NULL;
        temp->next = newOlignonucleotide;
    }
}

int generatePositiveErrors(char **positiveErrors, struct LinkedList *head, int oligonucleotideLength, int olignonucleotideAmount, int targetPositiveErrorAmount){
    int unusedOlignonucleotideAmount = pow(4, oligonucleotideLength) - olignonucleotideAmount;
    int positiveErrorAmount = (targetPositiveErrorAmount <= unusedOlignonucleotideAmount) ? targetPositiveErrorAmount : unusedOlignonucleotideAmount;

    for(int i = 0; i < positiveErrorAmount; i++){
        positiveErrors[i] = (char*)calloc(oligonucleotideLength + 1, sizeof(char));
        int notRepeats = 0;

        while(!notRepeats){
            struct LinkedList *temp = head;

            for(int j = 0; j < oligonucleotideLength; j++){
                positiveErrors[i][j] = randNucleotide();
            }positiveErrors[i][oligonucleotideLength] = '\0';
            
            notRepeats = strncmp(positiveErrors[i], temp->oligonucleotide, oligonucleotideLength);
            
            while(temp->next){
                if(notRepeats <= 0){
                    break;
                }

                temp = temp->next;
                notRepeats = strncmp(positiveErrors[i], temp->oligonucleotide, oligonucleotideLength);
            }

            if(notRepeats){
                for(int j = 0; j < i; j++){
                    notRepeats = strncmp(positiveErrors[i], positiveErrors[j], oligonucleotideLength);
                    
                    if(!notRepeats){
                        break;
                    }
                }
            }
        }
    }
    return positiveErrorAmount;
}

void generateNegativeErrors(int *negativeErrors, int negativeErrorAmount, int olignonucleotideAmount){
    for(int i = 0; i < negativeErrorAmount; i++){
        bool notRepeats;

        do{
            notRepeats = true;
            negativeErrors[i] = randRange(olignonucleotideAmount-1);
            
            //checking for repeats
            for(int j = 0; j < i; j++){
                if(negativeErrors[i] == negativeErrors[j]){
                    notRepeats = false;
                    break;
                }
            }
        }while(!notRepeats);
    }
}

void deletingNegativeErrors(struct LinkedList **head, struct LinkedList *firstNucleotide, int *negativeErrors, int negativeErrorAmount){
    struct LinkedList *temp = *head;
    struct LinkedList *toDelete;
    int i = 0, j = 0;

    while(i < negativeErrorAmount){
        if(j < negativeErrors[i]){
            if(temp == firstNucleotide){
                temp = temp->next;
            }

            toDelete = temp;
            temp = temp->next;

            j++;
            continue;
        }

        if(temp == firstNucleotide){
            if(temp == *head){
                *head = temp;
            }
            
            toDelete = temp->next;
            temp->next = toDelete->next;
            free(toDelete);

            i++; j++;
            continue;
        }

        if(temp == *head){
            toDelete = temp;
            temp = temp->next;
            *head = temp;
            free(toDelete);

            i++; j++;
            continue;
        }

        temp = toDelete;
        toDelete = temp->next;
        temp->next = toDelete->next;
        free(toDelete);

        i++;
    }
}

void addingPositiveErrors(struct LinkedList **head, char **positiveErrors, int positiveErrorAmount, int oligonucleotideLength){
    struct LinkedList *temp = *head;
    for(int i = 0; i < positiveErrorAmount; i++){
        struct LinkedList *newOlignonucleotide = (struct LinkedList*)malloc(sizeof(struct LinkedList) + oligonucleotideLength + 1);
        memcpy(newOlignonucleotide->oligonucleotide, positiveErrors[i], oligonucleotideLength + 1); newOlignonucleotide->next = NULL; newOlignonucleotide->timesUsed = 0;
        int result = strncmp(positiveErrors[i], temp->oligonucleotide, oligonucleotideLength);

        if(result < 0){
            newOlignonucleotide->next = temp;

            if(i == 0){
                *head = newOlignonucleotide;
            }

            temp = newOlignonucleotide;
            continue;
        }

        while(temp->next){
            result = strncmp(positiveErrors[i], temp->next->oligonucleotide, oligonucleotideLength);

            if(result < 0){
                break;
            }

            temp = temp->next;
        }

        if(result > 0){
            newOlignonucleotide->next = NULL;
            temp->next = newOlignonucleotide;
            temp = newOlignonucleotide;
            continue;
        }
        
        newOlignonucleotide->next = (temp->next) ? temp->next : NULL;
        temp->next = newOlignonucleotide;
        temp = newOlignonucleotide;
    }
}

void printList(struct LinkedList *head){
   struct LinkedList *p = head;
   printf("\n[");

   //start from the beginning
   while(p != NULL) {
      printf("%s\t|\t", p->oligonucleotide);
      p = p->next;
   }
   printf("]");
}

void assingKey(struct LinkedList *head){
    int i = 0;
    
    while(head){
        head->index = i;
        i++; head = head->next;
    }
}

struct Queue* addQueue(struct Queue *queueStart, struct LinkedList *current, int priority){
    struct Queue *newNode = (struct Queue *)malloc(sizeof(struct Queue));
    newNode->current = current; newNode->next = NULL; newNode->priority = priority;

    if (!queueStart){
        // printf("New queue: %s\n", newNode->current->oligonucleotide);
        return newNode;
    }

    if (priority <= queueStart->priority){
        newNode->next = queueStart;
        // printf("queue newNode: %s\n", newNode->current->oligonucleotide);
        return newNode;
    }
    
    struct Queue *queue = queueStart;
    while (queue->next){
        if (newNode->priority <= queue->next->priority)
            break;    
        queue = queue->next;
    }

    newNode->next = queue->next;
    queue->next = newNode;

    // printf("queue: %s\n", newNode->current->oligonucleotide);
    return queueStart;
}

void printQueue(struct Queue *queue){
    printf("\n\n");
    while(queue){
        printf("%s -> ", queue->current->oligonucleotide);
        queue = queue->next;
    }printf("end Queue\n\n\n");
}

void dijkstra(struct LinkedList *current, struct LinkedList *head, struct OlignonucleotideMap *map, struct Queue *queue, int olignonucleotideAmount, bool graphtooSmall, bool graphtooBig){
    struct Queue *temp;

    map[current->index].pathLength = 0;
    queue = addQueue(queue, current, map[current->index].pathLength);

    int i = 0, currentPathLength, neighborPathLength, index, desertedFirstNode = 1, searchLevel = TARGETMAXCONNECTIONLEVEL + 1;
    while(queue){
        current = queue->current;
        currentPathLength = queue->priority;
        map[current->index].visited = true;

        temp = queue;
        queue = queue->next;
        free(temp);
        
        i = 0;
        while(i < MAXEDGES){
            if(!(current->incidenceList[i])){
                //printf("\nNot Found: %s, %d", current->oligonucleotide, i);
                if(i >= 20){
                    break;
                }else if(i < 4){
                    i = 4;
                }else{
                    i = 20;
                }
                
                continue;
            }

            index = current->incidenceList[i]->index;
            desertedFirstNode = 0;

            switch(i){
                case 0 ... 3:
                    neighborPathLength = currentPathLength + 1;
                    break;
                
                case 4 ... 19:
                    neighborPathLength = currentPathLength + 2;
                    break;

                case 20 ... 83:
                    neighborPathLength = currentPathLength + 3;
                    break;
            }
            
            // printf("caseNormal: %s, %s, %s, %d, %d\n",current->oligonucleotide, current->incidenceList[i]->oligonucleotide, map[index].visited ? "true" : "false", neighborPathLength, map[index].pathLength);
            if((!map[index].visited) && neighborPathLength < map[index].pathLength){
                map[index].pathLength = neighborPathLength;
                map[index].prev = current;
                
                queue = addQueue(queue, current->incidenceList[i], map[index].pathLength);
            }
            i++;
        }

        if(desertedFirstNode){
            // printf("\nDeserted Node\n");
            int oligonucleotideLength = strlen(current->oligonucleotide);

            while(desertedFirstNode){
                struct LinkedList *target = head;
                neighborPathLength = currentPathLength + searchLevel;

                while(target){
                    if(current->index == target->index){
                        target = target->next;
                        continue;
                    }

                    int notEqual = strncmp(current->oligonucleotide + searchLevel, target->oligonucleotide, oligonucleotideLength - searchLevel);
                    if(!notEqual && ((graphtooSmall && target->timesUsed == 0) || (graphtooBig && target->timesUsed != 0) || (!graphtooSmall && !graphtooBig && target->timesUsed < MAXUSEDDIJKSTRA))){
                        //add to corresponding Incidence list
                        desertedFirstNode = 0;
                        index = target->index;
                        // printf("caseDeserted: %s, %s, %s, %d, %d\n",current->oligonucleotide, target->oligonucleotide, map[index].visited ? "true" : "false", neighborPathLength, map[index].pathLength);

                        map[index].pathLength = neighborPathLength;
                        map[index].prev = current;
                        
                        queue = addQueue(queue, target, map[index].pathLength);
                        // printf("%-15s|%-15s|%-15s|\t%d\t|\t%d\t|\t%d\n", source->oligonucleotide, source->incidenceList[incidenceIterators[connectionLevel-1] -1]->oligonucleotide, source->oligonucleotide + connectionLevel, connectionLevel, incidenceIterators[connectionLevel-1]-1, i);
                    }
                    target = target->next;
                }searchLevel++;
            }
        }
            // printQueue(queue);
    }
    // printf("queue: %s", queue ? "exist" : "don't exist");
}

struct LinkedList* findConnection(struct LinkedList *head, int usedNodeLimit, int* foundConnection, int graphCoverageMIN, int graphCoverageMAX, double graphCoverage){
    struct LinkedList *bestConnection = head;
    int bestConnectionLevel = 0, coverageValue = INT_MAX;
    bool graphtooSmall = graphCoverage < graphCoverageMIN, graphtooBig = graphCoverage > graphCoverageMAX, founVertex = false;
    // printf("tooSmall: %s, toBig: %s\n", graphtooSmall ? "yes" : "no", graphtooBig ? "yes" : "no");
    
    
    int i = 0;
    while(i < MAXEDGES){
        if(!(head->incidenceList[i])){
            // printf("%d\n", i);
            // if meet requirements -> break, unless not enough covered graph -> than search for not used node, or if i >= 20 -> assume there are no more edges to check

            if(i < 4){
                i = 4;
            }else if(i >= 20){
                break;
            }else{
                i = 20;
            }
            continue;
        }

        if(i == 4 || i == 20){
            if(bestConnectionLevel != 0){
                if((graphtooBig && bestConnection->timesUsed > 0) || (graphtooSmall && bestConnection->timesUsed == 0) || (!graphtooSmall && !graphtooBig && bestConnection->timesUsed <= usedNodeLimit)){
                    founVertex = true;
                    break;
                }
            }
        }

        if((graphtooBig && head->incidenceList[i]->timesUsed > bestConnection->timesUsed) || (graphtooSmall && head->incidenceList[i]->timesUsed == 0) || (!graphtooSmall && !graphtooBig && head->incidenceList[i]->timesUsed <= bestConnection->timesUsed && bestConnection->timesUsed <= usedNodeLimit)){
            bestConnection = head->incidenceList[i];
            
            switch(i){
                case 0 ... 3:
                    bestConnectionLevel = 1;
                    break;
                
                case 4 ... 19:
                    bestConnectionLevel = 2;
                    break;

                case 20 ... 83:
                    bestConnectionLevel = 3;
                    break;
            }
        }
        i++;
    }
    
    if(founVertex){
        *foundConnection = bestConnectionLevel;
        return bestConnection;
    }

    *foundConnection = 0;
    return head;
}

// Funkcja minimalna, która zwraca najmniejszą z trzech liczb
int minim(int a, int b, int c) {
    if (a < b && a < c)
        return a;
    else if (b < a && b < c)
        return b;
    else
        return c;
}

// Funkcja obliczająca odległość Levenshteina
int levenshteinDistance(const char *s1, const char *s2) {
    int len1 = strlen(s1);
    int len2 = strlen(s2);

    // Tablica dynamiczna do przechowywania wyników częściowych
    int **dist = (int **)malloc((len1 + 1) * sizeof(int *));
    for (int i = 0; i <= len1; i++) {
        dist[i] = (int *)malloc((len2 + 1) * sizeof(int));
    }

    // Inicjalizacja pierwszego wiersza i kolumny
    for (int i = 0; i <= len1; i++) {
        dist[i][0] = i;
    }
    for (int j = 0; j <= len2; j++) {
        dist[0][j] = j;
    }

    // Wypełnianie tablicy wyników częściowych
    for (int i = 1; i <= len1; i++) {
        for (int j = 1; j <= len2; j++) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            dist[i][j] = minim(dist[i - 1][j] + 1,         // Usunięcie
                             dist[i][j - 1] + 1,         // Wstawienie
                             dist[i - 1][j - 1] + cost); // Zamiana
        }
    }

    int distance = dist[len1][len2];

    // Zwalnianie pamięci
    for (int i = 0; i <= len1; i++) {
        free(dist[i]);
    }
    free(dist);

    return distance;
}