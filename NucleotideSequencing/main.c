#include "functions.h"

// struct LinkedList* findDestination(struct LinkedList *destination, int olignonucleotideAmount);
// void dijkstra(struct LinkedList *source, struct OlignonucleotideMap *map, struct Queue *queue);
struct LinkedList* findNewWay(char *dnaChainGenerated, int lastDnaIndex, struct LinkedList *source, struct LinkedList *firstNucleotide, struct LinkedList *head, int olignonucleotideAmount, struct OlignonucleotideMap *map, struct Queue *queue, int graphCoverageMIN, int graphCoverageMAX, double graphCoverage);
int trackPath(char *dnaChainGenerated, struct LinkedList *bestVertex, struct LinkedList *head, struct OlignonucleotideMap *map, int i, int oligonucleotideLength, int oligonucleotidesMaxAmount, int *nucleotidesUsed);
void reconstruction(char *dnaChainGenerated, struct LinkedList *firstNucleotide, struct LinkedList *head, bool areRepetitions, int oligonucleotideLength, int oligonucleotidesMaxAmount, int olignonucleotideAmount, int graphCoverageMIN, int graphCoverageMAX);

int main(int argc, char** argv){
    srand(time(NULL));
    //variables declaration
    int dnaLength, oligonucleotideLength, olignonucleotideAmount, oligonucleotidesMaxAmount, targetPositiveErrorAmount, targetNegativeErrorAmount, positiveErrorAmount, negativeErrorAmount, allowDuplicates, readDna, graphCoverageMIN, graphCoverageMAX;
    char *dnaSequence, **positiveErrors, *dnaChainGenerated;
    int *negativeErrors;
    bool areRepetitions;
    struct LinkedList *head, *firstNucleotide;

    if(argc != 9){
        fprintf(stderr, "Error: Wrong number of arguments");
        exit(0);
    }

    //getting dna and oligonucleotide length from scanf and setting random dna sequence
    dnaLength = atoi(argv[1]); oligonucleotideLength = atoi(argv[2]); oligonucleotidesMaxAmount = dnaLength - oligonucleotideLength + 1; allowDuplicates = atoi(argv[5]); readDna = atoi(argv[6]); graphCoverageMIN = atoi(argv[7]); graphCoverageMAX = atoi(argv[8]) > graphCoverageMIN ? atoi(argv[8]) : graphCoverageMIN;
    printf("DNA chain Length: %d\n", dnaLength); printf("oligonucleotide Length: %d\n", oligonucleotideLength);printf("readDna: %d\n", readDna);

    if(dnaLength < oligonucleotideLength){
        fprintf(stderr, "Error: DNA length is shorther than oligonucleotide's length");
        exit(0);
    }

    //generating random DNA sequence
    dnaSequence = (char*)calloc(dnaLength + 1, sizeof(char));
    
    if(readDna){
        FILE *dnaFile = fopen(DNAREADFILE, "r");
         
        fgets(dnaSequence, dnaLength+1, dnaFile);
        fclose(dnaFile);

        firstNucleotide = (struct LinkedList*)malloc(sizeof(struct LinkedList) + oligonucleotideLength + 1);
        memcpy(firstNucleotide->oligonucleotide, dnaSequence, oligonucleotideLength); firstNucleotide->oligonucleotide[oligonucleotideLength] = '\0'; firstNucleotide->next = NULL; firstNucleotide->timesUsed = 0;
        head = firstNucleotide;
        olignonucleotideAmount = 1;

        fillList(&head, &olignonucleotideAmount, dnaSequence, oligonucleotideLength, oligonucleotidesMaxAmount);
        areRepetitions = (olignonucleotideAmount != oligonucleotidesMaxAmount) ? true : false;
    }else{
        do{
            // printf("nowa proba instancji, %d, %d\n", oligonucleotidesMaxAmount, olignonucleotideAmount);
            for(int i = 0; i < dnaLength; i++){
                dnaSequence[i] = randNucleotide();
            }

            //generating first oligonucleotide present in the DNA sequence
            firstNucleotide = (struct LinkedList*)malloc(sizeof(struct LinkedList) + oligonucleotideLength + 1);
            memcpy(firstNucleotide->oligonucleotide, dnaSequence, oligonucleotideLength); firstNucleotide->oligonucleotide[oligonucleotideLength] = '\0'; firstNucleotide->next = NULL; firstNucleotide->timesUsed = 0;
            head = firstNucleotide;
            olignonucleotideAmount = 1;

            //generating a sorted list of oligonucleotides present in the DNA sequence
            fillList(&head, &olignonucleotideAmount, dnaSequence, oligonucleotideLength, oligonucleotidesMaxAmount);
            areRepetitions = (olignonucleotideAmount != oligonucleotidesMaxAmount) ? true : false;

        }while(areRepetitions && allowDuplicates == 0);
    }

    targetNegativeErrorAmount = atoi(argv[4]); targetNegativeErrorAmount = targetNegativeErrorAmount > olignonucleotideAmount ? olignonucleotideAmount : targetNegativeErrorAmount;
    targetPositiveErrorAmount = atoi(argv[3]); targetPositiveErrorAmount = targetPositiveErrorAmount > ((int)pow(4, oligonucleotideLength) - olignonucleotideAmount) ? ((int)pow(4, oligonucleotideLength) - olignonucleotideAmount) : targetPositiveErrorAmount;
    // printList(head);
    // printf("%s, %d\n", (char*)(firstNucleotide->oligonucleotide), olignonucleotideAmount);
    
    //generating positive errors
    positiveErrors = (char**)calloc(targetPositiveErrorAmount, sizeof(char*));
    positiveErrorAmount = generatePositiveErrors(positiveErrors, head, oligonucleotideLength, olignonucleotideAmount, targetPositiveErrorAmount);
    selectionSortStrings(positiveErrors, positiveErrorAmount, oligonucleotideLength);
    
    // printf("\n%d\n", positiveErrorAmount);
    // for(int i = 0; i < positiveErrorAmount; i++){
    //     printf("%s\t|\t", positiveErrors[i]);
    // }

    //generating negative errors
    negativeErrorAmount = (targetNegativeErrorAmount <= olignonucleotideAmount-1) ? targetNegativeErrorAmount : olignonucleotideAmount-1;
    negativeErrors = (int*)calloc(negativeErrorAmount, sizeof(int));
    generateNegativeErrors(negativeErrors, negativeErrorAmount, olignonucleotideAmount);
    selectionSort(negativeErrors, negativeErrorAmount);

    // printf("\n%d\n", negativeErrorAmount);
    // for(int i = 0; i < negativeErrorAmount; i++){
    //     printf("%d\t|\t", negativeErrors[i]);
    // }printf("\nfirst nucleotide: %s\n", firstNucleotide->oligonucleotide);

    //applying negative errors
    deletingNegativeErrors(&head, firstNucleotide, negativeErrors, negativeErrorAmount);
    olignonucleotideAmount -= negativeErrorAmount;
    
    //applying positive errors
    addingPositiveErrors(&head, positiveErrors, positiveErrorAmount, oligonucleotideLength);
    olignonucleotideAmount += positiveErrorAmount;

    // printList(head);
    // printf("%s, %d\n", (char*)(firstNucleotide->oligonucleotide), olignonucleotideAmount);

    assingKey(head);

    //generating Incidence list for 3 levels of oligonucleotide matching
    struct LinkedList *source = head;
    int notEqual, maxConnectionLevel = ((oligonucleotideLength - 1) >= TARGETMAXCONNECTIONLEVEL) ? TARGETMAXCONNECTIONLEVEL : (oligonucleotideLength - 1);
    int incidenceIterators[3];
    int incidenceIteratorsLimit[] = {4, 20, 84};

    for(int i = 0; i < olignonucleotideAmount; i++){
        // for(int k = 0; k < maxConnectionLevel; k++){
        //     incidenceIterators[k] = (k*k*k) + 3*(k*k);
        //     //source->incidenceList = (struct LinkedList **)malloc(MAXEDGES * sizeof(struct LinkedList *));
        // }

        incidenceIterators[0] = 0; incidenceIterators[1] = 4; incidenceIterators[2] = 20; // MAX Level 1 connections is 4, so first 4 slots is reserved for them, next 16 is for level 2 and last 64 is for level 3
        
        struct LinkedList *destination = head;

        for(int j = 0; j < olignonucleotideAmount; j++){
            if(!areRepetitions && i == j){
                destination = destination->next;
                continue;
            }
            
            for(int connectionLevel = 1; connectionLevel <= maxConnectionLevel; connectionLevel++){
                
                notEqual = strncmp(source->oligonucleotide + connectionLevel, destination->oligonucleotide, oligonucleotideLength - connectionLevel);
                if(!notEqual){
                    //add to corresponding Incidence list
                    source->incidenceList[incidenceIterators[connectionLevel-1]] = destination;
                    incidenceIterators[connectionLevel-1]++;
                    // printf("%-15s|%-15s|%-15s|\t%d\t|\t%d\t|\t%d\n", source->oligonucleotide, source->incidenceList[incidenceIterators[connectionLevel-1] -1]->oligonucleotide, source->oligonucleotide + connectionLevel, connectionLevel, incidenceIterators[connectionLevel-1]-1, i);
                }
            }
            
            destination = destination->next;
        }

        for(int connectionLevel = 1; connectionLevel <= maxConnectionLevel; connectionLevel++){
            if(incidenceIterators[connectionLevel-1] < incidenceIteratorsLimit[connectionLevel-1])
                source->incidenceList[incidenceIterators[connectionLevel-1]] = NULL;
        }

        source = source->next;
    }
    // printf("domain generated3\n");

    //trying to reconstruct DNA
    dnaChainGenerated = (char *)malloc((dnaLength+1) * sizeof(char));
    reconstruction(dnaChainGenerated, firstNucleotide, head, areRepetitions, oligonucleotideLength, oligonucleotidesMaxAmount, olignonucleotideAmount, graphCoverageMIN, graphCoverageMAX);

    printf("\nDNA:%s\n", dnaSequence);

    // printf("\nTest1");
    int levenshtein = levenshteinDistance(dnaChainGenerated, dnaSequence);
    printf("the Levinstein distance is %d\n", levenshtein);
    
    // Levinstein(dnaChainGenerated, dnaSequence);
    // printf("\nTest3");

    //save to file
    FILE *resultFile;

    resultFile = fopen(RESULTFILE, "w+");
    fprintf(resultFile,"DNA LENGTH, Oligonucleotide Length, Negative Errors Amount, Positive Errors Amount\n");
    fprintf(resultFile,"%d, %d, %d, %d\n", dnaLength, oligonucleotideLength, negativeErrorAmount, positiveErrorAmount);
    fprintf(resultFile, "\n\nLevinstein distance, MIN graph coverage, MAX graph coverage\n");
    fprintf(resultFile, "%d, %d, %d", levenshtein, graphCoverageMIN, graphCoverageMAX);

    fclose(resultFile);

    //save default DNA chain
    if(!readDna){
        FILE *dnaFile;

        dnaFile = fopen(DNAREADFILE, "w+");
        fprintf(dnaFile,"%s", dnaSequence);

        fclose(dnaFile);
    }

    //freeing allocated memory
    free(negativeErrors);

    for(int i = 0; i < positiveErrorAmount; i++){
        free(positiveErrors[i]);
    }free(positiveErrors);

    while(head){
        struct LinkedList *temp = head->next;
        free(head);
        head = temp;
    }

    free(dnaSequence); free(dnaChainGenerated);
    return EXIT_SUCCESS;
}

struct LinkedList* findNewWay(char *dnaChainGenerated, int lastDnaIndex, struct LinkedList *source, struct LinkedList *firstNucleotide, struct LinkedList *head, int olignonucleotideAmount, struct OlignonucleotideMap *map, struct Queue *queue, int graphCoverageMIN, int graphCoverageMAX, double graphCoverage){
    struct LinkedList *destination;
    bool graphtooSmall = graphCoverage < graphCoverageMIN, graphtooBig = graphCoverage > graphCoverageMAX;

    for(int i = 0; i < olignonucleotideAmount; i++){
        INIT_MAP(map[i]);
    }

    //find path
    // printf("\n\n%s\t\t|", (*source)->oligonucleotide);
    dijkstra(source, head, map, queue, olignonucleotideAmount, graphtooSmall, graphtooBig);
    // printf("\t\t%s\n\n", (*source)->oligonucleotide);

    //select vertex
    struct LinkedList *bestVertex = NULL;
    int bestIndex, bestNeighbor, neighbor;
    while(head){
        if(head == source || map[head->index].pathLength == INT_MAX){
            head = head->next;
            continue;
        }
        
        if(!bestVertex){
            bestVertex = head;
            bestIndex = bestVertex->index;
            findConnection(bestVertex, MAXUSEDNODE, &bestNeighbor, graphCoverageMIN, graphCoverageMAX, graphCoverage);
        }

        if((graphtooBig && head->timesUsed > bestVertex->timesUsed) || (!graphtooBig && head->timesUsed < bestVertex->timesUsed)){
            findConnection(head, MAXUSEDNODE, &neighbor, graphCoverageMIN, graphCoverageMAX, graphCoverage);

            bestVertex = head;
            bestIndex = bestVertex->index;
            bestNeighbor = neighbor;
            
        }else if(head->timesUsed == bestVertex->timesUsed){
            findConnection(head, MAXUSEDNODE, &neighbor, graphCoverageMIN, graphCoverageMAX, graphCoverage);
            if((map[head->index].pathLength < map[bestIndex].pathLength) && (neighbor <= bestNeighbor && neighbor > 0)){
                bestVertex = head;
                bestIndex = bestVertex->index;
                bestNeighbor = neighbor;
            }
        }
        // printf("%s\t\t|%d\t\t|%s\t\t|%d\n", head->oligonucleotide, map[head->index].pathLength, map[head->index].visited ? "visited" : "not visited", head->index);
        head = head->next;
    }

    return bestVertex;
}

int trackPath(char *dnaChainGenerated, struct LinkedList *bestVertex, struct LinkedList *head, struct OlignonucleotideMap *map, int i, int oligonucleotideLength, int oligonucleotidesMaxAmount, int *nucleotidesUsed){
    struct LinkedList *prev = map[bestVertex->index].prev;
    int pathLengthLong = map[bestVertex->index].pathLength, pathLengthShort = map[prev->index].pathLength, totalPath = 0;
    
    while(i + pathLengthShort >= oligonucleotidesMaxAmount-1){
        bestVertex = prev; prev = map[bestVertex->index].prev;
        pathLengthLong = pathLengthShort; pathLengthShort = map[prev->index].pathLength;
    }

    int connectionLevel = (pathLengthLong - pathLengthShort);

    if(i + pathLengthLong >= oligonucleotidesMaxAmount-1){
        totalPath = (oligonucleotidesMaxAmount-1) - (i + pathLengthShort);
        memcpy(dnaChainGenerated + oligonucleotideLength + i + pathLengthShort, bestVertex->oligonucleotide + oligonucleotideLength - connectionLevel, totalPath);

        bestVertex->timesUsed++;
        bestVertex = prev; 
        pathLengthLong = pathLengthShort;
        
    }

    while(bestVertex->index != head->index){
        prev = map[bestVertex->index].prev; pathLengthShort = map[prev->index].pathLength;

        connectionLevel = (pathLengthLong - pathLengthShort);
        totalPath += connectionLevel;

        memcpy(dnaChainGenerated + oligonucleotideLength + i + pathLengthShort, bestVertex->oligonucleotide + oligonucleotideLength - connectionLevel, connectionLevel);

        if(bestVertex->timesUsed == 0){
            (*nucleotidesUsed)++;
        }

        bestVertex->timesUsed += 1;
        bestVertex = prev; 
        pathLengthLong = pathLengthShort;
    }

    return totalPath;
}

void reconstruction(char *dnaChainGenerated, struct LinkedList *firstNucleotide, struct LinkedList *head, bool areRepetitions, int oligonucleotideLength, int oligonucleotidesMaxAmount, int olignonucleotideAmount, int graphCoverageMIN, int graphCoverageMAX){
    //celowac w jak najwieksze zapelnienie grafu, oraz w jak najwiecej polaczen 1 stopnia
    //w przypadku braku dobrego polaczenia skorzystac z algorytmu przeszukiwania sciezki do wierzcholka ktory nie byl jeszcze uzyty

    int foundConnection = 0, nucleotidesUsed = 1, i = 0;
    double graphCoverage = 0;
    struct LinkedList *current = firstNucleotide;
    strncpy(dnaChainGenerated, firstNucleotide->oligonucleotide, oligonucleotideLength);
    firstNucleotide->timesUsed += 1;

    struct OlignonucleotideMap *map = (struct OlignonucleotideMap *)malloc(olignonucleotideAmount * sizeof(struct OlignonucleotideMap));

    struct Queue *queue = NULL;

    // printf("\n%s\n", dnaChainGenerated);

    while(i < oligonucleotidesMaxAmount-1){
        graphCoverage = (nucleotidesUsed / (double)olignonucleotideAmount) * 100;
        // printf("\nszukanie: %d\n", i);
        current = findConnection(current, MAXUSEDNODE, &foundConnection, graphCoverageMIN, graphCoverageMAX, graphCoverage); //possibly add parameters to (limit / lead) the search -> how much used node is acceptable / at which point stop accepting used nodes (ratio found DNA chain to covered density of the graph)
        // printf("current: %s\n", current->oligonucleotide);

        if(foundConnection){
            // printf("Normal Case: %d\n", i);
            if(i+foundConnection >= oligonucleotidesMaxAmount-1){
                memcpy(dnaChainGenerated + oligonucleotideLength + i, current->oligonucleotide + oligonucleotideLength - foundConnection, (oligonucleotidesMaxAmount-1) - i);
                break;
            }

            memcpy(dnaChainGenerated + oligonucleotideLength + i, current->oligonucleotide + oligonucleotideLength - foundConnection, foundConnection);
            i+=foundConnection;

            if(current->timesUsed == 0){
                nucleotidesUsed++;
                // printf("nucleotidesUsed: %d\n", nucleotidesUsed);
            }

            current->timesUsed += 1;
            
        }else{
            //use algorithm to find path
            struct LinkedList *bestVertex = findNewWay(dnaChainGenerated, i, current, firstNucleotide, head, olignonucleotideAmount, map, queue, graphCoverageMIN, graphCoverageMAX, graphCoverage);
            
            i += trackPath(dnaChainGenerated, bestVertex, current, map, i, oligonucleotideLength, oligonucleotidesMaxAmount, &nucleotidesUsed);
            current = bestVertex;
        }
    }
    
    printf("Coverage: %lf\n", graphCoverage);
    printf("\n%s\n", dnaChainGenerated);

    free(map);
    
    if(queue)
        free(queue);
}