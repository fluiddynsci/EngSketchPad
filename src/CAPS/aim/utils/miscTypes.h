// Structures for miscellaneous utilities - Written by Dr. Ryan Durscher AFRL/RQVC

#ifndef MISCTYPES_H
#define MISCTYPES_H

// General container for to map attribute names to an assigned index
typedef struct {

    char *mapName;

    int numAttribute;
    char **attributeName;
    int  *attributeIndex;

} mapAttrToIndexStruct;

#endif
