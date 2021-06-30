// This software has been cleared for public release on 05 Nov 2020, case number 88ABW-2020-3462.

#include "egads.h"
#include "aimUtil.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

#ifdef WIN32
#define strcasecmp  stricmp
#endif

#include "deprecateUtils.h"
#include "miscUtils.h"

// Change from the use of capsGroup for mesh sizing to capsMesh
int deprecate_SizingAttr(int numTuple,
                         capsTuple meshBCTuple[],
                         mapAttrToIndexStruct *meshMap,
                         mapAttrToIndexStruct *groupMap){
    int status;
    int i, attrIndex;

    for (i = 0; i < numTuple; i++) {

        status = get_mapAttrToIndexIndex(meshMap, (const char *) meshBCTuple[i].name, &attrIndex);
        if (status == CAPS_NOTFOUND) {
            status = get_mapAttrToIndexIndex(groupMap, (const char *) meshBCTuple[i].name, &attrIndex);
            if (status == CAPS_SUCCESS) {
                printf("DEPRACATED: The capsGroup attribute (capsGroup=%s) is no longer used to specify mesh sizing parameters. Please "
                        "use capsMesh instead.\n", meshBCTuple[i].name);
            }

            return CAPS_BADVALUE;
        }
    }

    return CAPS_SUCCESS;
}
