#ifndef DATAHEADER_H
#define DATAHEADER_H

int configLines;

int   savedDirs;
int   savedActs;
Dir** allDirs;
Act** allActs;

int   dirStackTop;
Dir** dirStack;

Dir* startDir;

int isDirDecl(char* string);
int isDirRef(char* string);
int isActRef(char* string);

Dir* newDirFromRef(char* string, Dir* parent);
Dir* newDirFromDecl(char* string);
Act* newActFromRef(char* string, Dir* parent);

Dir* addDir(Dir* dir);
Act* addAct(Act* act);

void dirStackInit();
void dirStackPush(Dir* dir);
void dirStackPop();
Dir* dirStackPeek();
int  dirStackIsEmpty();

char** allocForDirToStrings();
char** dirToStrings(char** ret, int* count);
int selectElement(char choice);

#endif