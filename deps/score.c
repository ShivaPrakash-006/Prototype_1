#include "score.h"
#include "includes.h"
#include "objects.h"

char *extractScores(char *fileName) {
  FILE *scoreJson = fopen(fileName, "r");
  if (scoreJson == NULL) {
    printf("Unable to open '%s'.", fileName);
    return NULL;
  }

  fseek(scoreJson, 0, SEEK_END);
  int length = ftell(scoreJson);
  rewind(scoreJson);

  char *jsonData = (char *)malloc(length + 1);
  fread(jsonData, 1, length, scoreJson);
  jsonData[length] = '\0';

  return jsonData;
}

char *updateScores(char *jsonData, char *username, int score, int time) {
  cJSON *root =
      jsonData[0] != '\0' ? cJSON_Parse(jsonData) : cJSON_CreateObject();

  cJSON *userScores = cJSON_GetObjectItem(root, "Scores");
  if (userScores == NULL) {
    userScores = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "Scores", userScores);
  }

  cJSON *scoreObj = cJSON_CreateObject();
  cJSON_AddStringToObject(scoreObj, "Username", username);
  cJSON_AddNumberToObject(scoreObj, "Score", score);
  cJSON_AddNumberToObject(scoreObj, "Time", time);
  cJSON_AddItemToArray(userScores, scoreObj);

  // SORT DATA HERE

  jsonData = cJSON_PrintUnformatted(root);

  cJSON_Delete(root);
  return jsonData;
}

bool saveScores(char *jsonData, char *fileName) {
  bool success = true;

  FILE *scoresJson = fopen(fileName, "w");
  if (scoresJson == NULL) {
    printf("Unable to open '%s'.", fileName);
    success = false;
  } else {
    fputs(jsonData, scoresJson);
    fclose(scoresJson);
  }

  return success;
}

int compareScores(const void *a, const void *b) {
  const cJSON *scoreA = *(const cJSON **)a;
  const cJSON *scoreB = *(const cJSON **)b;

  int score1 = cJSON_GetObjectItem(scoreA, "Score")->valueint;
  int score2 = cJSON_GetObjectItem(scoreB, "Score")->valueint;

  return score2 - score1;
}

int compareTime(const void *a, const void *b) {
  const cJSON *scoreA = *(const cJSON **)a;
  const cJSON *scoreB = *(const cJSON **)b;

  int time1 = cJSON_GetObjectItem(scoreA, "Time")->valueint;
  int time2 = cJSON_GetObjectItem(scoreB, "Time")->valueint;

  return time2 - time1;
}

int compareName(const void *a, const void *b) {
  const cJSON *scoreA = *(const cJSON **)a;
  const cJSON *scoreB = *(const cJSON **)b;

  const char *name1 = cJSON_GetObjectItem(scoreA, "Username")->valuestring;
  const char *name2 = cJSON_GetObjectItem(scoreB, "Username")->valuestring;

  return strncmp(name1, name2, 50);
}

void sortScores(cJSON *jsonData, enum Sort type) {
  cJSON *scores = cJSON_GetObjectItem(jsonData, "Scores");
  int count = cJSON_GetArraySize(scores);

  if (count < 2)
    return;

  cJSON **scoreList = malloc(count * sizeof(cJSON *));
  for (int i = 0; i < count; i++)
    scoreList[i] = cJSON_GetArrayItem(scores, i);

  if (type == SCORE)
    qsort(scoreList, count, sizeof(cJSON *), compareScores);
  else if (type == TIME)
    qsort(scoreList, count, sizeof(cJSON *), compareTime);
  else
    qsort(scoreList, count, sizeof(cJSON *), compareName);

  cJSON *sortedScores = cJSON_CreateArray();
  for (int i = 0; i < count; i++)
    cJSON_AddItemToArray(sortedScores, cJSON_Duplicate(scoreList[i], 1));

  cJSON_ReplaceItemInObject(jsonData, "Scores", sortedScores);
  free(scoreList);
}
