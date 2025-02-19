#ifndef ARGPASSHEADER_H
#define ARGPASSHEADER_H

typedef enum {
    XLeft,XMid,XRight
}xPos_enum_t;

typedef enum {
    YTop,YMid,YBot
}yPos_enum_t;

//structure for sharing informations about arguments
struct arguments {
    char *args[2];
    int silent, verbose, windowWidth, showMenuNames, showMenuNamesNested, fgColor, bgColor, topIndent, leftIndent, botIndent, winXOffset, winYOffset;
    xPos_enum_t winXPos;
    yPos_enum_t winYPos;
    char dirUpKey, actS, dirS;
    char *configFile, *contentFile, *font, *startDir;
};

extern struct arguments arguments;

void argumentsInit();
void argumentsApply(int argc,char** argv);

char* getHomePath();

#endif
