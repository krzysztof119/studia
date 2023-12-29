#define _CRT_SECURE_NO_DEPRECATE

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <math.h>

#if defined(_WIN32)
    #include <windows.h>
    #include <processthreadsapi.h>

#elif defined(__linux__)
    #include <unistd.h>

#elif defined(__APPLE__)
    #include <sys/param.h>
    #include <sys/sysctl.h>

#endif

enum searchMethod { functionSieveMethod = 0, domainSieveMethod = 1, functionModuloMethod = 2, domainModuloMethod = 3 };
enum threadMode { sequential = 0, parallelMaxLogicalProcessors = 1, parallelMaxPhysicalProcessors = 2, parallelHalfPhysicalProcessors = 3 };

void getSearchScope(unsigned int* numberMAX, unsigned int* numberMIN, unsigned int* numberSqrt, int* numberLoops);

void getProcessorInfo(int* logicalProcessorsNumber, int* physicalProcessorsNumber);

void printTimes(double timeProcessorSeconds, double timeWallclockSeconds, const char* methodName0, const char* methodName1, const char* modeName, double timeWallclockSeqSeconds);

void saveToFile(char* fileName, bool* content, unsigned int numberMAX, unsigned int numberMIN);

void compareSequenceToParallel(enum searchMethod method, unsigned int numberMAX, unsigned int numberMIN, unsigned int numberSqrt, int numberLoops, int logicalProcessorsNumber, int physicalProcessorsNumber);

bool* findPrimeNumbers(enum searchMethod method, unsigned int numberMAX, unsigned int numberMIN, unsigned int numberSqrt, const int numberOfThreads, bool keepResultArray);

void functionSieve(unsigned int numberMAX, unsigned int numberMIN, unsigned int numberSqrt, int primePatternSize, bool* primeNumbers, int* primeNumbersPattern, const int numberOfThreads);
void domainSieve(unsigned int numberMAX, unsigned int numberMIN, unsigned int numberSqrt, int primePatternSize, bool* primeNumbers, int* primeNumbersPattern, const int numberOfThreads);
void functionModulo(unsigned int numberMAX, unsigned int numberMIN, unsigned int numberSqrt, int primePatternSize, bool* primeNumbers, int* primeNumbersPattern, const int numberOfThreads);
void domainModulo(unsigned int numberMAX, unsigned int numberMIN, unsigned int numberSqrt, int primePatternSize, bool* primeNumbers, int* primeNumbersPattern, const int numberOfThreads);

int main(int argc, char** argv) {
    // declaration of variables
    unsigned int sieveNumberMAX = 0, moduloNumberMAX = 0, sieveNumberMIN = 0, moduloNumberMIN = 0, sieveNumberSqrt = 0, moduloNumberSqrt = 0; int sieveNumberLoops = 1, moduloNumberLoops = 1;
    int physicalProcessorsNumber, logicalProcessorsNumber;

    printf("\n---------------------------------------------------------------------------------\n");
    printf("Set scope values for Sieve method (up to 4.000.000.000, recommended to set at arround 1.000.000.000 at most):\n");
    getSearchScope(&sieveNumberMAX, &sieveNumberMIN, &sieveNumberSqrt, &sieveNumberLoops);
    printf("\n---------------------------------------------------------------------------------\n");

    printf("Set scope values for Modulo method (since it works slower, it is recommended to set scope smaller like 10.000.000 at most):\n");
    getSearchScope(&moduloNumberMAX, &moduloNumberMIN, &moduloNumberSqrt, &moduloNumberLoops);
    printf("\n---------------------------------------------------------------------------------\n");

    getProcessorInfo(&logicalProcessorsNumber, &physicalProcessorsNumber);

    omp_set_dynamic(0); //enforce omp to use specified number of threads

    // measure time of given searchMethod and save results
    compareSequenceToParallel(functionSieveMethod, sieveNumberMAX, sieveNumberMIN, sieveNumberSqrt, sieveNumberLoops, logicalProcessorsNumber, physicalProcessorsNumber);

    compareSequenceToParallel(domainSieveMethod, sieveNumberMAX, sieveNumberMIN, sieveNumberSqrt, sieveNumberLoops, logicalProcessorsNumber, physicalProcessorsNumber);

    compareSequenceToParallel(functionModuloMethod, moduloNumberMAX, moduloNumberMIN, moduloNumberSqrt, moduloNumberLoops, logicalProcessorsNumber, physicalProcessorsNumber);

    compareSequenceToParallel(domainModuloMethod, moduloNumberMAX, moduloNumberMIN, moduloNumberSqrt, moduloNumberLoops, logicalProcessorsNumber, physicalProcessorsNumber);

    #ifdef _WIN32
        system("pause");
    #endif
    return EXIT_SUCCESS;
}

void getSearchScope(unsigned int* numberMAX, unsigned int* numberMIN, unsigned int* numberSqrt, int* numberLoops) {
    // getting input value and detecting problematic behaviour
    printf("Set lower limit of scope: ");
    (void) !scanf("%u", numberMIN);

    if (*numberMIN < 1) {
        printf("NumberMin too small");
        exit(0);
    }
    if (*numberMIN < 2) {
        *numberMIN = 2;
    }

    // getting input value, calculating square root and detecting problematic behaviour
    printf("Set upper limit of scope: ");
    (void) !scanf("%u", numberMAX);
    *numberSqrt = (int)sqrt(*numberMAX);

    if (*numberMAX > 4000000000) {
        printf("numberMAX too big");
        exit(0);
    }

    if (*numberMAX <= 1 || *numberMIN > *numberMAX) {
        printf("NumberMax too small");
        exit(0);
    }

    printf("Set number of times to repeat the process (might increase measurement accuracy, requires at least 1, the higher value, the longer the time): ");
    (void) !scanf("%d", numberLoops);

    if (*numberLoops < 1) {
        printf("numberLoops too small");
        exit(0);
    }

    return;
}

void getProcessorInfo(int* logicalProcessorsNumber, int* physicalProcessorsNumber) {
    *logicalProcessorsNumber = omp_get_max_threads();

#ifdef _WIN32
    FILE* fp; char myString[5] = "";

    fp = _popen("powershell -command \"(Get-WmiObject Win32_Processor).NumberOfCores\"", "r");
    if (!fp) {
        perror("WARNING: Cannot obtain information about amount of physical processors (cores)!");
        exit(0);
    }

    (void) !fscanf(fp, "%s", myString);
    *physicalProcessorsNumber = atoi(myString);

#elif defined(__linux__) || defined(__APPLE__)
    //perror("WARNING: Opperating System not supported - the number of cores cannot be acquired");
    *physicalProcessorsNumber = sysconf(_SC_NPROCESSORS_ONLN);

#else
    perror("WARNING: Opperating System not supported - the number of cores cannot be acquired");
    *physicalProcessorsNumber = omp_get_max_threads();

#endif
}

void printTimes(double timeProcessorSeconds, double timeWallclockSeconds, const char* methodName0, const char* methodName1, const char* modeName, double timeWallclockSeqSeconds) {
    if (timeProcessorSeconds)
        printf("\033[0;31m%s %s\n\033[0m\t\033[0;33m%s\033[0m processor time: \033[1;37m%lf\033[0;0m seconds\n", methodName0, methodName1, modeName, timeProcessorSeconds);
    else
        printf("Range is to small to measure \033[0;33m%s\033[0m processor time of \033[0;31m%s %s\n\033[0m\n", modeName, methodName0, methodName1);

    if (timeWallclockSeconds)
        printf("\033[0;31m%s %s\n\033[0m\t\033[0;33m%s\033[0m calculation time: \033[1;37m%lf\033[0;0m seconds\n", methodName0, methodName1, modeName, timeWallclockSeconds);
    else
        printf("Range is to small to measure \033[0;33m%s\033[0m calculation time of \033[0;31m%s %s\n\033[0m\n", modeName, methodName0, methodName1);
    printf("\n");

    if (timeWallclockSeqSeconds < 0)
        return;

    if (timeWallclockSeqSeconds && timeWallclockSeconds || (timeWallclockSeqSeconds / timeWallclockSeconds) )
        printf("\033[0;32mSpeedUp:\033[0m \033[1;37m%lf\033[0;0m\n", (timeWallclockSeqSeconds / timeWallclockSeconds));
    else
        printf("Range is to small to measure \033[0;32mSpeedUp:\033[0m\n");

    printf("\n");
}

void saveToFile(char* fileName, bool* content, unsigned int numberMAX, unsigned int numberMIN) {
    FILE* resultFile = fopen(fileName, "w");
    unsigned int countSaved = 0;

    for (unsigned int i = numberMIN; i <= numberMAX; i++) {
        if (content[i - 2]) {
            fprintf(resultFile, "%d\n", i);
            countSaved++;
        }
    }

    fclose(resultFile);

    printf("\033[1;37m%u\033[0;0m prime numbers saved to file: %s\n", countSaved, fileName);
    return;
}

void compareSequenceToParallel(enum searchMethod method, unsigned int numberMAX, unsigned int numberMIN, unsigned int numberSqrt, int numberLoops, int logicalProcessorsNumber, int physicalProcessorsNumber) {
    #ifdef _WIN32
        FILETIME lpCreationTime, lpExitTime, lpKernelTime, lpUserTime;
        ULARGE_INTEGER uStart, uStop;
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
    #else
        clock_t pstart, pstop;
    #endif
    double StartWtime, EndWtime, timeProcessorSeqSeconds = -1, timeProcessorSeconds, timeWallclockSeconds;
    const char* methodName[][2] = { {"Function", "Sieve"}, {"Domain", "Sieve"}, {"Function", "Modulo"}, {"Domain", "Modulo"} };
    const char* modeName[] = { "Sequential", "ParallelMaxLogicProc", "ParallelMaxPhysicProc", "ParallelHalfPhysicProc" };
    const char* fileEnd[] = { "Seq.txt", "Pml.txt", "Pmp.txt", "Php.txt" };
    const int numberOfThreads[] = { 1, logicalProcessorsNumber, physicalProcessorsNumber, physicalProcessorsNumber / 2 };
    bool* primeNumbers = NULL;

    char* fileName = calloc(strlen(methodName[method][0]) + strlen(methodName[method][1]) + 8, sizeof(char));//Seq, Pml, Pmp, Php
    if (!fileName) {
        perror("ERROR: Allocation failed for fileName!");
        exit(0);
    }
    else {
        memcpy(fileName, methodName[method][0], strlen(methodName[method][0]));
        memcpy(&fileName[strlen(methodName[method][0])], methodName[method][1], strlen(methodName[method][1]));
    }

    printf("\033[0;31m\t\t\t\t%s %s\033[0m", methodName[method][0], methodName[method][1]);

    for (int i = sequential; i <= parallelHalfPhysicalProcessors; i++) {
        timeProcessorSeconds = 0; timeWallclockSeconds = 0;

        printf("\n---------------------------------------------------------------------------------\n");

        bool skip = false;
        for (int k = 0; k < i; k++) {
            if (numberOfThreads[k] == numberOfThreads[i]) {
                printf("There number of threads for: \033[0;33m%s\033[0m is the same as: \033[0;33m%s\033[0m\n\n", modeName[k], modeName[i]);
                skip = true;
            }
        }
        if (skip)
            continue;

        for (int j = 0; j < numberLoops; j++) {
            StartWtime = omp_get_wtime();
            
            #ifdef _WIN32

                GetProcessTimes(GetCurrentProcess(), &lpCreationTime, &lpExitTime, &lpKernelTime, &lpUserTime);
                uStart.LowPart = lpUserTime.dwLowDateTime; uStart.HighPart = lpUserTime.dwHighDateTime;

                primeNumbers = findPrimeNumbers(method, numberMAX, numberMIN, numberSqrt, numberOfThreads[i], j == (numberLoops - 1));

                GetProcessTimes(GetCurrentProcess(), &lpCreationTime, &lpExitTime, &lpKernelTime, &lpUserTime);
                uStop.LowPart = lpUserTime.dwLowDateTime; uStop.HighPart = lpUserTime.dwHighDateTime;
                
            
            #else

                pstart = clock();

                primeNumbers = findPrimeNumbers(method, numberMAX, numberMIN, numberSqrt, numberOfThreads[i], i != sequential, j == (numberLoops - 1));

                pstop = clock();
            
            #endif   

            EndWtime = omp_get_wtime();

            #ifdef _WIN32
                timeProcessorSeconds += (uStop.QuadPart - uStart.QuadPart);
            #else       
                timeProcessorSeconds += (double)(pstop - pstart) / CLOCKS_PER_SEC;
            #endif

            timeWallclockSeconds += (EndWtime - StartWtime);
        }
        #ifdef _WIN32
            timeProcessorSeconds /= numberLoops * (double)freq.QuadPart;
        #else
            timeProcessorSeconds /= numberLoops;
        #endif

        timeWallclockSeconds /= numberLoops;
        printTimes(timeProcessorSeconds, timeWallclockSeconds, methodName[method][0], methodName[method][1], modeName[i], timeProcessorSeqSeconds);
        if (i == sequential)
            timeProcessorSeqSeconds = timeWallclockSeconds;

        if (!fileName || !primeNumbers) {
            perror("ERROR: Cannot extract results\n");
        }
        else {
            memcpy(&fileName[strlen(methodName[method][0]) + strlen(methodName[method][1])], fileEnd[i], 8);
            saveToFile(fileName, primeNumbers, numberMAX, numberMIN);

            printf("\n");
        }if(primeNumbers) free(primeNumbers);
    }free(fileName);

    printf("---------------------------------------------------------------------------------\n");
    return;
}

bool* findPrimeNumbers(enum searchMethod method, unsigned int numberMAX, unsigned int numberMIN, unsigned int numberSqrt, const int numberOfThreads, bool keepResultArray) {
    // declaration of variables
    int primePatternSize = 0;
    bool* primeNumbersSqrt = NULL, *primeNumbers = NULL;
    int* primeNumbersPattern = NULL;

    // detection problematic case
    if (numberSqrt < 2) {
        printf("Number too small");
        exit(0);
    }

    // allocating memory and detecting error
    primeNumbersSqrt = calloc((numberSqrt - 1), sizeof(bool));
    if (!primeNumbersSqrt) {
        perror("Error: Allocation for primeNumbersSqrt failed!");
        exit(0);
    }

    // finding prime numbers in sqrtNymbers scope
    for (int i = 2; i <= (int)numberSqrt; i++) {
        primeNumbersSqrt[i - 2] = true;
        primePatternSize++;

        for (int j = 2; j <= sqrt(i); j++) {
            if (i % j == 0) {
                primeNumbersSqrt[i - 2] = false;
                primePatternSize--;
                break;
            }
        }
    }

    // allocating memory and detecting error
    primeNumbersPattern = malloc(primePatternSize * sizeof(int));
    if (!primeNumbersPattern) {
        perror("ERROR: Allocation for primeNumbersPattern failed!");
        exit(0);
    }

    // storing prime numbers to numberSqrt inside allocated memory
    int counter = 0;
    for (int i = 2; i <= (int)numberSqrt; i++) {
        if (primeNumbersSqrt[i - 2]) {
            if (counter >= primePatternSize) {
                perror("Error while creating pattern");
                exit(0);
            }

            primeNumbersPattern[counter] = i;
            counter++;
        }
    }

    // increasing allocated memory to contain all numbers to inputted numberMAX
    //printf("block size: %d, %s, new size: %d\n", _msize(primeNumbersSqrt), primeNumbersSqrt[2] ? "true" : "false", ((numberMAX - 1) * sizeof(bool)));
    primeNumbers = realloc(primeNumbersSqrt, (numberMAX - 1) * sizeof(bool));
    //printf("block size: %d\n", _msize(primeNumbers));
    if (!primeNumbers) {
        perror("ERROR: Allocation for primeNumbers failed!");
        exit(0);
    }
    memset(primeNumbers + (numberSqrt - 1), true, ((numberMAX - 1) - (numberSqrt - 1)) * sizeof(bool));

    switch (method) {
    case functionSieveMethod:
        functionSieve(numberMAX, numberMIN, numberSqrt, primePatternSize, primeNumbers, primeNumbersPattern, numberOfThreads);
        break;

    case domainSieveMethod:
        domainSieve(numberMAX, numberMIN, numberSqrt, primePatternSize, primeNumbers, primeNumbersPattern, numberOfThreads);
        break;

    case functionModuloMethod:
        functionModulo(numberMAX, numberMIN, numberSqrt, primePatternSize, primeNumbers, primeNumbersPattern, numberOfThreads);
        break;

    case domainModuloMethod:
        domainModulo(numberMAX, numberMIN, numberSqrt, primePatternSize, primeNumbers, primeNumbersPattern, numberOfThreads);
        break;

    default:
        perror("ERROR: Searching Method not found!");
        exit(0);
        break;
    }

    // free allocated memory
    free(primeNumbersPattern);

    if (!keepResultArray) {
        free(primeNumbers);
        return NULL;
    }
    

    return primeNumbers;
}

void functionSieve(unsigned int numberMAX, unsigned int numberMIN, unsigned int numberSqrt, int primePatternSize, bool* primeNumbers, int* primeNumbersPattern, const int numberOfThreads) {
    if (numberMIN <= numberSqrt)
        numberMIN = numberSqrt + 1;

    omp_set_num_threads(numberOfThreads);

    int i;
    #pragma omp parallel for schedule(dynamic, 1) if(numberOfThreads > 1)
    for (i = 0; i < primePatternSize; i++) { // <-- podzial na liczby pierwsze do sqrt(numberMAX)
        int prime = primeNumbersPattern[i];
        int start = numberMIN / prime;

        if (start < 2)
            start = 2;

        for (unsigned int j = start; prime * j <= numberMAX; j++) { // <- iteracja po komorkach sieve'a
            if (primeNumbers[prime * j - 2] ? false : true)
                continue;

            primeNumbers[prime * j - 2] = false;
        }
    }

}

void domainSieve(unsigned int numberMAX, unsigned int numberMIN, unsigned int numberSqrt, int primePatternSize, bool* primeNumbers, int* primeNumbersPattern, const int numberOfThreads) {
    if (numberMIN <= numberSqrt)
        numberMIN = numberSqrt + 1;
    int blockSize = 2 << 18, loopSize = (int)ceil((float)(numberMAX - numberMIN) / blockSize) + 1;

    omp_set_num_threads(numberOfThreads);

    int i;
    #pragma omp parallel for schedule(dynamic, 1) if(numberOfThreads > 1)
    for (i = 1; i < loopSize; i++) { // <-- podzial na bloki
        unsigned int start = numberMIN + blockSize * (i - 1);
        unsigned int end = numberMIN + blockSize * (i);

        if (end > numberMAX)
            end = numberMAX;

        for (int k = 0; k < primePatternSize; k++) {  // <-- iteracja po sieve
            int prime = primeNumbersPattern[k];
            unsigned int index_start = start / prime;

            if (index_start * prime < start)
                index_start++;

            for (unsigned int j = index_start; j * prime <= end; j++) { // <-- iteracja po komorkach bloku
                if (primeNumbers[j * prime - 2] ? false : true)
                    continue;

                primeNumbers[j * prime - 2] = false;
            }
        }
    }

    return;
}

void functionModulo(unsigned int numberMAX, unsigned int numberMIN, unsigned int numberSqrt, int primePatternSize, bool* primeNumbers, int* primeNumbersPattern, const int numberOfThreads) {
    if (numberMIN <= numberSqrt)
        numberMIN = numberSqrt + 1;

    omp_set_num_threads(numberOfThreads);

    int i;
    #pragma omp parallel for schedule(dynamic, 1) if(numberOfThreads > 1)
    for (i = 0; i < primePatternSize; i++) {
        int prime = primeNumbersPattern[i];

        for (unsigned int j = numberMIN; j <= numberMAX; j++) {
            if (j % prime == 0) {
                if (primeNumbers[j - 2]) {
                    primeNumbers[j - 2] = false;
                }
            }
        }
    }

    return;
}

void domainModulo(unsigned int numberMAX, unsigned int numberMIN, unsigned int numberSqrt, int primePatternSize, bool* primeNumbers, int* primeNumbersPattern, const int numberOfThreads) {
    if (numberMIN <= numberSqrt)
        numberMIN = numberSqrt + 1;
    int blockSize = 2 << 11, loopSize = (int)ceil((float)(numberMAX - numberMIN) / blockSize) + 1;

    omp_set_num_threads(numberOfThreads);

    int i;
    #pragma omp parallel for schedule(dynamic, 1) if(numberOfThreads > 1)
    for (i = 1; i < loopSize; i++) { // <-- podzial na bloki
        unsigned int start = numberMIN + blockSize * (i - 1);
        unsigned int end = numberMIN + blockSize * (i);

        if (end > numberMAX)
            end = numberMAX;

        for (unsigned int j = start; j <= end; j++) {
            for (int k = 0; k < primePatternSize; k++) {
                int prime = primeNumbersPattern[k];

                if (j % prime == 0) {
                    if (primeNumbers[j - 2]) {
                        primeNumbers[j - 2] = false;
                    }
                }
            }
        }
    }

    return;
}