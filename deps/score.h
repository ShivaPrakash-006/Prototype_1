#ifndef SCORE_H_
#define SCORE_H_

#include "includes.h"

typedef struct ScoreObj {
  char username[50];
  int score;
  int time;
} ScoreObj;

char *extractScores(char *fileName); 
char *updateScores(char *jsonData, char *username, int score, int time); 
bool saveScores(char *jsonData, char *fileName); 
int compareScores(const void *a, const void *b); 
int compareTime(const void *a, const void *b); 
int compareName(const void *a, const void *b); 
void sortScores(cJSON *jsonData, enum Sort type); 

#endif //SCORE_H_
