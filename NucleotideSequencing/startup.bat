@echo off

set /A			dnaLength			= 700
set /A		oligonucleotideLength	= 10
set /A	TargetPositiveErrorAmount	= 0
set /A	TargetNegativeErrorAmount	= 0
set /A 			allowDuplicates		= 0
set /A 			readDna				= 0
set /A 			graphCoverageMIN	= 0
set /A 			graphCoverageMAX	= 100

cd %~dp0

gcc -o main.exe main.c && main.exe %dnaLength% %oligonucleotideLength% %TargetPositiveErrorAmount% %TargetNegativeErrorAmount% %allowDuplicates% %readDna% %graphCoverageMIN% %graphCoverageMAX%
pause