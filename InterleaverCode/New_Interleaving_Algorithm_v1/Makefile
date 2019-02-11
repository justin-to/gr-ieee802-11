CC=gcc
CFLAGS=-Wall

default : DSCJammingAnalysis DSCJammingAnalysisB6


DSCJammingAnalysis : DSCJammingAnalysis.o
	$(CC) -o $@ $(CFLAGS) $<

DSCJammingAnalysis.o : DSCJammingAnalysis.c
	$(CC) -c -o $@ $(CFLAGS) $<


DSCJammingAnalysisB6 : DSCJammingAnalysisB6.o
	$(CC) -o $@ $(CFLAGS) $<

DSCJammingAnalysisB6.o : DSCJammingAnalysisB6.c
	$(CC) -c -o $@ $(CFLAGS) $<


clean : 
	rm *.o
	rm DSCJammingAnalysis*
	rm *.tmp