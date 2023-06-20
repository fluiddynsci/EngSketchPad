/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             AIM Volume Mesh Functions
 *
 * *      Copyright 2014-2023, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <string.h>

#ifdef WIN32
#include <Windows.h>
#include <direct.h>
#define strcasecmp stricmp
#define access     _access
#define F_OK       0
#define SLASH     '\\'
#else
#include <dirent.h>
#include <dlfcn.h>
#include <unistd.h>
#define SLASH     '/'
#endif

#include "aimUtil.h"
#include "aimMesh.h"

/* used to preserve the indexing order when there is a mixture of
 * solid/sheet/wire bodies
 */
#define CAPS_BODY_INDX "--CAPS-BODY-INDX--"


static /*@null@*/ DLL writerDLopen(const char *name)
{
  int             i, len;
  DLL             dll;
  char            *full, *env;
#ifdef WIN32
  WIN32_FIND_DATA ffd;
  char            dir[MAX_PATH];
  size_t          length_of_arg;
  HANDLE hFind =  INVALID_HANDLE_VALUE;
#else
/*@-unrecog@*/
  char            dir[PATH_MAX];
/*@+unrecog@*/
  struct dirent   *de;
  DIR             *dr;
#endif

  env = getenv("ESP_ROOT");
  if (env == NULL) {
    printf(" Information: Could not find $ESP_ROOT\n");
    return NULL;
  }

  if (name == NULL) {
    printf(" Information: Dynamic Loader invoked with NULL name!\n");
    return NULL;
  }
  len  = strlen(name);
  full = (char *) malloc((len+5)*sizeof(char));
  if (full == NULL) {
    printf(" Information: Dynamic Loader MALLOC Error!\n");
    return NULL;
  }
  for (i = 0; i < len; i++) full[i] = name[i];
  full[len  ] = '.';
#ifdef WIN32
  full[len+1] = 'D';
  full[len+2] = 'L';
  full[len+3] = 'L';
  full[len+4] =  0;
  _snprintf(dir, MAX_PATH, "%s\\lib\\*", env);
  hFind = FindFirstFile(dir, &ffd);
  if (INVALID_HANDLE_VALUE == hFind) {
    printf(" Information: Dynamic Loader could not open %s\n", dir);
    free(full);
    return NULL;
  }
  i = 0;
  do {
    if (strcasecmp(full, ffd.cFileName) == 0) i++;
  } while (FindNextFile(hFind, &ffd) != 0);
  FindClose(hFind);
  if (i > 1) {
    printf(" Information: Dynamic Loader more than 1 file: %s!\n", full);
    free(full);
    return NULL;
  }

  if (i == 1) {
    hFind = FindFirstFile(dir, &ffd);
    do {
      if (strcasecmp(full, ffd.cFileName) == 0) break;
    } while (FindNextFile(hFind, &ffd) != 0);
    dll = LoadLibrary(ffd.cFileName);
    FindClose(hFind);
  } else {
    dll = LoadLibrary(full);
  }

#else

  full[len+1] = 's';
  full[len+2] = 'o';
  full[len+3] =  0;
  snprintf(dir, PATH_MAX, "%s/lib", env);
  dr = opendir(dir);
  if (dr == NULL) {
    printf(" Information: Dynamic Loader could not open %s\n", dir);
    free(full);
    return NULL;
  }
  i = 0;
  while ((de = readdir(dr)) != NULL)
/*@-unrecog@*/
    if (strcasecmp(full, de->d_name) == 0) i++;
/*@+unrecog@*/
  closedir(dr);
  if (i > 1) {
    printf(" Information: Dynamic Loader more than 1 file: %s!\n", full);
    free(full);
    return NULL;
  }

  if (i == 1) {
    dr = opendir(dir);
    while ((de = readdir(dr)) != NULL)
      if (strcasecmp(full, de->d_name) == 0) break;
/*@-nullderef@*/
    dll = dlopen(de->d_name, RTLD_NOW | RTLD_NODELETE);
/*@-nullderef@*/
    closedir(dr);
  } else {
    dll = dlopen(full, RTLD_NOW | RTLD_NODELETE);
  }
#endif

  if (!dll) {
    printf(" Information: Dynamic Loader Error for %s\n", full);
#ifndef WIN32
/*@-mustfreefresh@*/
    printf("              %s\n", dlerror());
/*@+mustfreefresh@*/
#endif
    free(full);
    return NULL;
  }
  free(full);

  return dll;
}


static void writerDLclose(/*@only@*/ DLL dll)
{
#ifdef WIN32
  FreeLibrary(dll);
#else
  dlclose(dll);
#endif
}


static wrDLLFunc writerDLget(DLL dll, const char *symname)
{
  wrDLLFunc data;

#ifdef WIN32
  data = (wrDLLFunc) GetProcAddress(dll, symname);
#else
/*@-castfcnptr@*/
  data = (wrDLLFunc) dlsym(dll, symname);
/*@+castfcnptr@*/
#endif

  return data;
}


static int writerDLoaded(writerContext cntxt, const char *name)
{
  int i;

  for (i = 0; i < cntxt.aimWriterNum; i++)
    if (strcasecmp(name, cntxt.aimWriterName[i]) == 0) return i;

  return -1;
}


static int writerDYNload(writerContext *cntxt, const char *name)
{
  int i, len, ret;
  DLL dll;

  if (cntxt->aimWriterNum >= MAXWRITER) {
    printf(" Information: Number of Writers > %d!\n", MAXWRITER);
    return EGADS_INDEXERR;
  }
  dll = writerDLopen(name);
  if (dll == NULL) return EGADS_NULLOBJ;

  ret                      = cntxt->aimWriterNum;
  cntxt->aimExtension[ret] = (AIMext)    writerDLget(dll, "meshExtension");
  cntxt->aimWriter[ret]    = (AIMwriter) writerDLget(dll, "meshWrite");
  if (cntxt->aimExtension[ret] == NULL) {
    writerDLclose(dll);
    printf(" Error: Missing symbol 'meshExtension' in %s\n", name);
    return EGADS_EMPTY;
  }
  if (cntxt->aimWriter[ret] == NULL) {
    writerDLclose(dll);
    printf(" Error: Missing symbol 'meshWrite' in %s\n", name);
    return EGADS_EMPTY;
  }

  len = strlen(name) + 1;
  cntxt->aimWriterName[ret] = (char *) EG_alloc(len*sizeof(char));
  if (cntxt->aimWriterName[ret] == NULL) {
    writerDLclose(dll);
    return EGADS_MALLOC;
  }
  for (i = 0; i < len; i++) cntxt->aimWriterName[ret][i] = name[i];
  cntxt->aimWriterDLL[ret] = dll;
  cntxt->aimWriterNum++;

  return ret;
}


/*@null@*/ /*@dependent@*/ static const char *
aim_writerExtension(void *aimStruc, const char *writerName)
{
  int     i;
  aimInfo *aInfo;

  aInfo = (aimInfo *) aimStruc;
  i     = writerDLoaded(aInfo->wCntxt, writerName);
  if (i == -1) {
    i  = writerDYNload(&aInfo->wCntxt, writerName);
    if (i < 0) return NULL;
  }

  return aInfo->wCntxt.aimExtension[i]();
}


static int
aim_writeMesh(void *aimStruc, const char *writerName, aimMesh *mesh)
{
  int     i;
  aimInfo *aInfo;

  aInfo = (aimInfo *) aimStruc;
  i     = writerDLoaded(aInfo->wCntxt, writerName);
  if (i == -1) {
    i  = writerDYNload(&aInfo->wCntxt, writerName);
    if (i < 0) return i;
  }

  return aInfo->wCntxt.aimWriter[i](aimStruc, mesh);
}


/* ************************* Exposed Functions **************************** */

int
aim_deleteMeshes(void *aimStruc, const aimMeshRef *meshRef)
{
  int           i;
#ifdef WIN32
  char          file[MAX_PATH];
#else
  char          file[PATH_MAX];
#endif
  const char    *ext;
  aimInfo       *aInfo;
  writerContext cntxt;

  aInfo = (aimInfo *) aimStruc;
  if (aInfo == NULL)                    return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC)  return CAPS_BADOBJECT;
  cntxt = aInfo->wCntxt;

  for (i = 0; i < cntxt.aimWriterNum; i++) {
    ext = aim_writerExtension(aimStruc, cntxt.aimWriterName[i]);
    if (ext == NULL) continue;
#ifdef WIN32
    _snprintf(file, MAX_PATH, "%s%s", meshRef->fileName, ext);
    _unlink(file);
#else
    snprintf(file, PATH_MAX, "%s%s", meshRef->fileName, ext);
    unlink(file);
#endif
  }

  return CAPS_SUCCESS;
}


int
aim_queryMeshes(void *aimStruc, int index, aimMeshRef *meshRef)
{
  int          i, j, k, ret, nWrite = 0;
  char         *writerName[MAXWRITER];
#ifdef WIN32
  char         file[MAX_PATH];
#else
  char         file[PATH_MAX];
#endif
  const char   *ext;
  aimInfo      *aInfo;
  capsObject   *vobject, *source, *last;
  capsProblem  *problem;
  capsAnalysis *analysis, *another;
  capsValue    *value;

  aInfo    = (aimInfo *) aimStruc;
  if (aInfo    == NULL)                 return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC)  return CAPS_BADOBJECT;
  problem  = aInfo->problem;
  analysis = (capsAnalysis *) aInfo->analysis;

  if ((index < 1) || (index > analysis->nAnalysisOut)) return CAPS_BADINDEX;
  vobject = analysis->analysisOut[index-1];
  if (vobject == NULL)            return CAPS_NULLOBJ;
  value = (capsValue *) vobject->blind;
  if (value == NULL)              return CAPS_NULLBLIND;
  if (value->type != PointerMesh) return CAPS_BADTYPE;

  /* find all of our linked writers */
  for (i = 0; i < problem->nAnalysis; i++) {
    if (problem->analysis[i]        == NULL) continue;
    if (problem->analysis[i]->blind == NULL) continue;
    another = problem->analysis[i]->blind;
    if (analysis == another)                 continue;
    for (j = 0; j < another->nAnalysisIn; j++) {
      source = another->analysisIn[j];
      last   = NULL;
      do {
        if (source->magicnumber != CAPSMAGIC)      break;
        if (source->type        != VALUE)          break;
        if (source->blind       == NULL)           break;
        value  = (capsValue *) source->blind;
        if (value->link == another->analysisIn[j]) break;
        last   = source;
        source = value->link;
      } while (value->link != NULL);
      if (last != vobject) continue;
      /* we hit our object from a AnalysisIn link */
      value = (capsValue *) another->analysisIn[j]->blind;
      if (value->meshWriter == NULL) {
        printf(" CAPS Warning: Link found but NULL writer!\n");
        continue;
      }
      for (k = 0; k < nWrite; k++)
        if (strcmp(writerName[k], value->meshWriter) == 0) break;
      if (k != nWrite) continue;
      writerName[nWrite] = value->meshWriter;
      nWrite++;
    }
  }
  if (nWrite == 0) return CAPS_NOTFOUND;

  /* look at the extensions for our files -- do they exist? */
  for (ret = i = 0; i < nWrite; i++) {
    ext = aim_writerExtension(aimStruc, writerName[i]);
    if (ext == NULL) continue;
#ifdef WIN32
    _snprintf(file, MAX_PATH, "%s%s", meshRef->fileName, ext);
    if (_access(file, F_OK) != 0) ret++;
#else
    snprintf(file,  PATH_MAX, "%s%s", meshRef->fileName, ext);
    if (access(file,  F_OK) != 0) ret++;
#endif
  }

  return ret;
}


int
aim_writeMeshes(void *aimStruc, int index, aimMesh *mesh)
{
  int          i, j, k, stat, nWrite = 0;
  char         *writerName[MAXWRITER];
#ifdef WIN32
  char         file[MAX_PATH];
#else
  char         file[PATH_MAX];
#endif
  const char   *ext;
  aimInfo      *aInfo;
  capsObject   *vobject, *source, *last;
  capsProblem  *problem;
  capsAnalysis *analysis, *another;
  capsValue    *value;

  aInfo    = (aimInfo *) aimStruc;
  if (aInfo == NULL)                    return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC)  return CAPS_BADOBJECT;
  problem  = aInfo->problem;
  analysis = (capsAnalysis *) aInfo->analysis;

  if ((index < 1) || (index > analysis->nAnalysisOut)) return CAPS_BADINDEX;
  vobject = analysis->analysisOut[index-1];
  if (vobject == NULL)            return CAPS_NULLOBJ;
  value = (capsValue *) vobject->blind;
  if (value == NULL)              return CAPS_NULLBLIND;
  if (value->type != PointerMesh) return CAPS_BADTYPE;

  /* find all of our linked writers */
  for (i = 0; i < problem->nAnalysis; i++) {
    if (problem->analysis[i]        == NULL) continue;
    if (problem->analysis[i]->blind == NULL) continue;
    another = problem->analysis[i]->blind;
    if (analysis == another)                 continue;
    for (j = 0; j < another->nAnalysisIn; j++) {
      source = another->analysisIn[j];
      last   = NULL;
      do {
        if (source->magicnumber != CAPSMAGIC)      break;
        if (source->type        != VALUE)          break;
        if (source->blind       == NULL)           break;
        value  = (capsValue *) source->blind;
        if (value->link == another->analysisIn[j]) break;
        last   = source;
        source = value->link;
      } while (value->link != NULL);
      if (last != vobject) continue;
      /* we hit our object from a AnalysisIn link */
      value = (capsValue *) another->analysisIn[j]->blind;
#ifdef DEBUG
      printf(" *** Found Writer = %s ***\n", value->meshWriter);
#endif
      if (value->meshWriter == NULL) {
        printf(" CAPS Error: Link found but NULL writer!\n");
        return CAPS_NOTFOUND;
      }
      for (k = 0; k < nWrite; k++)
        if (strcmp(writerName[k], value->meshWriter) == 0) break;
      if (k != nWrite) continue;
      writerName[nWrite] = value->meshWriter;
      nWrite++;
    }
  }
  if (nWrite == 0) return CAPS_NOTFOUND;

  /* write the files */
  for (i = 0; i < nWrite; i++) {
    /* does the file exist? */
    ext = aim_writerExtension(aimStruc, writerName[i]);
    if (ext == NULL) continue;
#ifdef WIN32
    _snprintf(file, MAX_PATH, "%s%s", mesh->meshRef->fileName, ext);
    if (_access(file, F_OK) == 0) continue;
#else
    snprintf(file,  PATH_MAX, "%s%s", mesh->meshRef->fileName, ext);
    if (access(file,  F_OK) == 0) continue;
#endif
    /* no -- write it */
    stat = aim_writeMesh(aimStruc, writerName[i], mesh);
    if (stat != CAPS_SUCCESS) {
      printf(" CAPS Error: aim_writeMesh = %d for Writer = %s!\n",
             stat, writerName[i]);
      return stat;
    }
  }

  return CAPS_SUCCESS;
}


int
aim_initMeshBnd(aimMeshBnd *meshBnd)
{
  if (meshBnd == NULL) return CAPS_NULLOBJ;

  meshBnd->groupName = NULL;
  meshBnd->ID        = 0;

  return CAPS_SUCCESS;
}


int
aim_initMeshRef(aimMeshRef *meshRef)
{
  if (meshRef == NULL) return CAPS_NULLOBJ;

  meshRef->nmap     = 0;
  meshRef->maps     = NULL;
  meshRef->nbnd     = 0;
  meshRef->bnds     = NULL;
  meshRef->fileName = NULL;
  meshRef->_delTess = (int)false;

  return CAPS_SUCCESS;
}


int
aim_freeMeshRef(/*@null@*/ aimMeshRef *meshRef)
{
  int status = CAPS_SUCCESS;
  int i, state, nvert;
  ego body;

  if (meshRef == NULL) return CAPS_NULLOBJ;

  if (meshRef->maps != NULL)
    for (i = 0; i < meshRef->nmap; i++) {
      AIM_FREE(meshRef->maps[i].map);

      if (meshRef->_delTess == (int)true) {
        status = EG_statusTessBody(meshRef->maps[i].tess, &body, &state, &nvert);
        if (status != CAPS_SUCCESS) return status;

        EG_deleteObject(meshRef->maps[i].tess);
        EG_deleteObject(body);
      }
    }

  if (meshRef->bnds != NULL)
     for (i = 0; i < meshRef->nbnd; i++)
       AIM_FREE(meshRef->bnds[i].groupName);

  AIM_FREE(meshRef->maps);
  AIM_FREE(meshRef->bnds);
  AIM_FREE(meshRef->fileName);

  aim_initMeshRef(meshRef);

  return CAPS_SUCCESS;
}


int
aim_initMeshData(aimMeshData *meshData)
{
  if (meshData == NULL) return CAPS_NULLOBJ;

  meshData->dim = 0;

  meshData->nVertex = 0;
  meshData->verts   = NULL;

  meshData->nElemGroup = 0;
  meshData->elemGroups = NULL;

  meshData->nTotalElems = 0;
  meshData->elemMap     = NULL;

  return CAPS_SUCCESS;
}


int
aim_freeMeshData(/*@null@*/ aimMeshData *meshData)
{
  int i;
  if (meshData == NULL) return CAPS_SUCCESS;

  AIM_FREE(meshData->verts);

  for (i = 0; i < meshData->nElemGroup; i++) {
    AIM_FREE(meshData->elemGroups[i].groupName);
    AIM_FREE(meshData->elemGroups[i].elements);
  }
  AIM_FREE(meshData->elemGroups);

  AIM_FREE(meshData->elemMap);

  aim_initMeshData(meshData);

  return CAPS_SUCCESS;
}


int
aim_addMeshElemGroup(void *aimStruc,
                     /*@null@*/ const char* groupName,
                     int ID,
                     enum aimMeshElem elementTopo,
                     int order,
                     int nPoint,
                     aimMeshData *meshData)
{
  int status = CAPS_SUCCESS;

  /* resize the element group */
  AIM_REALL(meshData->elemGroups, meshData->nElemGroup+1, aimMeshElemGroup, aimStruc, status);

  meshData->elemGroups[meshData->nElemGroup].groupName = NULL;
  meshData->elemGroups[meshData->nElemGroup].ID = ID;
  meshData->elemGroups[meshData->nElemGroup].elementTopo = elementTopo;
  meshData->elemGroups[meshData->nElemGroup].order = order;
  meshData->elemGroups[meshData->nElemGroup].nPoint = nPoint;
  meshData->elemGroups[meshData->nElemGroup].nElems = 0;
  meshData->elemGroups[meshData->nElemGroup].elements = NULL;

  if (groupName != NULL)
    AIM_STRDUP(meshData->elemGroups[meshData->nElemGroup].groupName, groupName, aimStruc, status);

  meshData->nElemGroup++;

cleanup:
  return status;
}


int
aim_addMeshElem(void *aimStruc,
                int nElems,
                aimMeshElemGroup *elemGroup)
{
  int status = CAPS_SUCCESS;

  /* resize the element group */
  AIM_REALL(elemGroup->elements, elemGroup->nPoint*(elemGroup->nElems + nElems), int, aimStruc, status);

  elemGroup->nElems += nElems;

cleanup:
  return status;
}


int
aim_readBinaryUgridHeader(void *aimStruc, aimMeshRef *meshRef,
                          int *nVertex, int *nTri, int *nQuad,
                          int *nTet, int *nPyramid, int *nPrism, int *nHex)
{
  int    status = CAPS_SUCCESS;

  char filename[PATH_MAX];
  FILE *fp = NULL;

  if (meshRef  == NULL) return CAPS_NULLOBJ;
  if (meshRef->fileName  == NULL) return CAPS_NULLOBJ;

  snprintf(filename, PATH_MAX, "%s%s", meshRef->fileName, ".lb8.ugrid");

  fp = fopen(filename, "rb");
  if (fp == NULL) {
    AIM_ERROR(aimStruc, "Cannot open file: %s\n", filename);
    status = CAPS_IOERR;
    goto cleanup;
  }

  /* read a binary UGRID file */
  status = fread(nVertex, sizeof(int), 1, fp);
  if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }
  status = fread(nTri,     sizeof(int), 1, fp);
  if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }
  status = fread(nQuad,    sizeof(int), 1, fp);
  if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }
  status = fread(nTet,     sizeof(int), 1, fp);
  if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }
  status = fread(nPyramid, sizeof(int), 1, fp);
  if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }
  status = fread(nPrism,   sizeof(int), 1, fp);
  if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }
  status = fread(nHex,     sizeof(int), 1, fp);
  if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

  status = CAPS_SUCCESS;

cleanup:
/*@-dependenttrans@*/
  if (fp != NULL) fclose(fp);
/*@+dependenttrans@*/
  fp = NULL;
  return status;
}


static int
aim_readBinaryUgridElements(void *aimStruc, FILE *fp, /*@null@*/ FILE *fpMV, /*@null@*/ char **volName,
                            int nPoint, enum aimMeshElem elementTopo, int nElems,
                            int *elementIndex, aimMeshData *meshData)
{
  int status = CAPS_SUCCESS;
  int i, j, ID, igroup;
  int nMapGroupID = 0;
  int *mapGroupID = NULL;

  if (nElems > 0) {

    if (fpMV != NULL) {
      AIM_NOTNULL(volName, aimStruc, status);

      for (i = 0; i < nElems; i++) {

        /* read the volume ID */
        status = fread(&ID, sizeof(int), 1, fpMV);
        if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }
        if (ID <= 0) {
          AIM_ERROR(aimStruc, "Volume ID must be a positive number: %d!", ID);
          status = CAPS_IOERR;
          goto cleanup;
        }

        /* add the volume group if necessary */
        if (ID > nMapGroupID) {
          AIM_REALL(mapGroupID, ID, int, aimStruc, status);
          for (j = nMapGroupID; j < ID; j++) mapGroupID[j] = -1;
          nMapGroupID = ID;
        }
        if (mapGroupID[ID-1] == -1) {
          status = aim_addMeshElemGroup(aimStruc, volName[ID-1], ID, elementTopo, 1, nPoint, meshData);
          AIM_STATUS(aimStruc, status);
          mapGroupID[ID-1] = meshData->nElemGroup-1;
        }

        igroup = mapGroupID[ID-1];

        /* add the element to the group */
        status = aim_addMeshElem(aimStruc, 1, &meshData->elemGroups[igroup]);
        AIM_STATUS(aimStruc, status);

        /* read the element connectivity */
        status = fread(&meshData->elemGroups[igroup].elements[nPoint*(meshData->elemGroups[igroup].nElems-1)],
                       sizeof(int), nPoint, fp);
        if (status != nPoint) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

        meshData->elemMap[*elementIndex][0] = igroup;
        meshData->elemMap[*elementIndex][1] = meshData->elemGroups[igroup].nElems-1;

        *elementIndex += 1;
      }

    } else {

      status = aim_addMeshElemGroup(aimStruc, NULL, 1, elementTopo, 1, nPoint, meshData);
      AIM_STATUS(aimStruc, status);

      igroup = meshData->nElemGroup-1;

      /* add the elements to the group */
      status = aim_addMeshElem(aimStruc, nElems, &meshData->elemGroups[igroup]);
      AIM_STATUS(aimStruc, status);

      /* read the element connectivity */
      status = fread(meshData->elemGroups[igroup].elements,
                     sizeof(int), nPoint*nElems, fp);
      if (status != nPoint*nElems) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

      for (i = 0; i < nElems; i++) {
        meshData->elemMap[*elementIndex][0] = igroup;
        meshData->elemMap[*elementIndex][1] = i;

        *elementIndex += 1;
      }
    }
  }

  status = CAPS_SUCCESS;
cleanup:

  AIM_FREE(mapGroupID);

  return status;
}


int
aim_readBinaryUgrid(void *aimStruc, aimMesh *mesh)
{
  int    status = CAPS_SUCCESS;

  int    nLine, nTri, nQuad;
  int    nTet, nPyramid, nPrism, nHex;
  int    i, j, elementIndex, nPoint, nElems, ID, igroup;
  int    line[2], *mapGroupID=NULL, nMapGroupID=0;
  int    nRegion = 0, nVolName=0, nmap=0, nID;
  enum aimMeshElem elementTopo;
  char filename[PATH_MAX], groupName[PATH_MAX], **volName=NULL;
  size_t len;
  FILE *fp = NULL, *fpID = NULL, *fpMV=NULL;
  aimMeshData *meshData = NULL;

  if (mesh           == NULL) return CAPS_NULLOBJ;
  if (mesh->meshRef  == NULL) return CAPS_NULLOBJ;
  if (mesh->meshRef->fileName  == NULL) return CAPS_NULLOBJ;

  status = aim_freeMeshData(mesh->meshData);
  AIM_STATUS(aimStruc, status);
  AIM_FREE(mesh->meshData);

  AIM_ALLOC(meshData, 1, aimMeshData, aimStruc, status);
  status = aim_initMeshData(meshData);
  AIM_STATUS(aimStruc, status);

  snprintf(filename, PATH_MAX, "%s%s", mesh->meshRef->fileName, ".lb8.ugrid");

  fp = fopen(filename, "rb");
  if (fp == NULL) {
    AIM_ERROR(aimStruc, "Cannot open file: %s\n", filename);
    status = CAPS_IOERR;
    goto cleanup;
  }

  // Open a 2nd copy to read in the BC ID's
  fpID = fopen(filename, "rb");
  if (fpID == NULL) {
    AIM_ERROR(aimStruc, "Cannot open file: %s\n", filename);
    status = CAPS_IOERR;
    goto cleanup;
  }

  /* read a binary UGRID file */
  status = fread(&meshData->nVertex, sizeof(int), 1, fp);
  if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }
  status = fread(&nTri,     sizeof(int), 1, fp);
  if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }
  status = fread(&nQuad,    sizeof(int), 1, fp);
  if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }
  status = fread(&nTet,     sizeof(int), 1, fp);
  if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }
  status = fread(&nPyramid, sizeof(int), 1, fp);
  if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }
  status = fread(&nPrism,   sizeof(int), 1, fp);
  if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }
  status = fread(&nHex,     sizeof(int), 1, fp);
  if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

  /* skip the header */
  status = fseek(fpID, 7*sizeof(int), SEEK_CUR);
  if (status != 0) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

  /*
  printf("\n Header from UGRID file: %d  %d %d  %d %d %d %d\n", numNode,
         numTriangle, numQuadrilateral, numTetrahedral, numPyramid, numPrism,
         numHexahedral);
   */

  snprintf(filename, PATH_MAX, "%s%s", mesh->meshRef->fileName, ".mapvol");

  // File for volume IDs
  fpMV = fopen(filename, "rb");
  if (fpMV != NULL) {
    status = fread(&nRegion, sizeof(int), 1, fpMV);
    if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

    /* read the maximum ID value */
    status = fread(&nVolName, sizeof(int), 1, fpMV);
    if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

    if (nRegion+nVolName == 0) {
      AIM_ERROR(aimStruc, "Ivnalid mapvol file with zero nRegion and nVolName!");
      status = CAPS_IOERR;
      goto cleanup;
    }

    AIM_ALLOC(volName, nVolName, char*, aimStruc, status);
    for (i = 0; i < nVolName; i++) volName[i] = NULL;

    for (i = 0; i < nRegion; i++) {
      status = fread(&ID, sizeof(int), 1, fpMV);
      if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

      status = fread(&len, sizeof(size_t), 1, fpMV);
      if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

      AIM_ALLOC(volName[ID-1], len, char, aimStruc, status);

      status = fread(volName[ID-1], sizeof(char), len, fpMV);
      if (status != len) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }
    }

    status = fread(&nElems, sizeof(int), 1, fpMV);
    if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

    if (nElems != nTet+nPyramid+nPrism+nHex) {
      AIM_ERROR(aimStruc, "Element count missmatch in mapvol file!");
      status = CAPS_IOERR;
      goto cleanup;
    }
  }


  AIM_ALLOC(meshData->verts, meshData->nVertex, aimMeshCoords, aimStruc, status);

  /* read all of the vertices */
  status = fread(meshData->verts, sizeof(aimMeshCoords), meshData->nVertex, fp);
  if (status != meshData->nVertex) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

  /* skip the verts */
  status = fseek(fpID, meshData->nVertex*sizeof(aimMeshCoords), SEEK_CUR);
  if (status != 0) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }


  // Numbers
  meshData->nTotalElems =  nTri     +
                           nQuad    +
                           nTet     +
                           nPyramid +
                           nPrism   +
                           nHex;

  // allocate the element map that maps back to the original element numbering
  AIM_ALLOC(meshData->elemMap, meshData->nTotalElems, aimMeshIndices, aimStruc, status);

  /*
  printf("Volume mesh:\n");
  printf("\tNumber of nodes          = %d\n", meshData->nVertex);
  printf("\tNumber of elements       = %d\n", meshData->nTotalElems);
  printf("\tNumber of triangles      = %d\n", numTriangle);
  printf("\tNumber of quadrilatarals = %d\n", numQuadrilateral);
  printf("\tNumber of tetrahedrals   = %d\n", numTetrahedral);
  printf("\tNumber of pyramids       = %d\n", numPyramid);
  printf("\tNumber of prisms         = %d\n", numPrism);
  printf("\tNumber of hexahedrals    = %d\n", numHexahedral);
*/

  /* skip the Tri+Quad elements for the ID */
  status = fseek(fpID, (3*nTri+4*nQuad)*sizeof(int), SEEK_CUR);
  if (status != 0) { status = CAPS_IOERR; goto cleanup; }

  /* Start of element index */
  elementIndex = 0;

  /* Elements triangles */
  nPoint = 3;
  elementTopo = aimTri;
  for (i = 0; i < nTri; i++) {

    /* read the BC ID */
    status = fread(&ID, sizeof(int), 1, fpID);
    if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }
    if (ID <= 0) {
      AIM_ERROR(aimStruc, "BC ID must be a positive number: %d!", ID);
      status = CAPS_IOERR;
      goto cleanup;
    }

    /* add the BC group if necessary */
    if (ID > nMapGroupID) {
      AIM_REALL(mapGroupID, ID, int, aimStruc, status);
      for (j = nMapGroupID; j < ID; j++) mapGroupID[j] = -1;
      nMapGroupID = ID;
    }
    if (mapGroupID[ID-1] == -1) {
      status = aim_addMeshElemGroup(aimStruc, NULL, ID, elementTopo, 1, nPoint, meshData);
      AIM_STATUS(aimStruc, status);
      mapGroupID[ID-1] = meshData->nElemGroup-1;
    }

    igroup = mapGroupID[ID-1];

    /* add the element to the group */
    status = aim_addMeshElem(aimStruc, 1, &meshData->elemGroups[igroup]);
    AIM_STATUS(aimStruc, status);

    /* read the element connectivity */
    status = fread(&meshData->elemGroups[igroup].elements[nPoint*(meshData->elemGroups[igroup].nElems-1)],
                   sizeof(int), nPoint, fp);
    if (status != nPoint) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

    meshData->elemMap[elementIndex][0] = igroup;
    meshData->elemMap[elementIndex][1] = meshData->elemGroups[igroup].nElems-1;

    elementIndex += 1;
  }

  /* Elements quadrilateral */
  nPoint = 4;
  elementTopo = aimQuad;
  for (i = 0; i < nQuad; i++) {

    /* read the BC ID */
    status = fread(&ID, sizeof(int), 1, fpID);
    if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }
    if (ID <= 0) {
      AIM_ERROR(aimStruc, "BC ID must be a positive number: %d!", ID);
      status = CAPS_IOERR;
      goto cleanup;
    }

    /* add the BC group if necessary */
    if (ID > nMapGroupID) {
      AIM_REALL(mapGroupID, ID, int, aimStruc, status);
      for (j = nMapGroupID; j < ID; j++) mapGroupID[j] = -1;
      nMapGroupID = ID;
    }
    if (mapGroupID[ID-1] == -1) {
      status = aim_addMeshElemGroup(aimStruc, NULL, ID, elementTopo, 1, nPoint, meshData);
      AIM_STATUS(aimStruc, status);
      mapGroupID[ID-1] = meshData->nElemGroup-1;
    }

    igroup = mapGroupID[ID-1];

    /* add the element to the group */
    status = aim_addMeshElem(aimStruc, 1, &meshData->elemGroups[igroup]);
    AIM_STATUS(aimStruc, status);

    /* read the element connectivity */
    status = fread(&meshData->elemGroups[igroup].elements[nPoint*(meshData->elemGroups[igroup].nElems-1)],
                   sizeof(int), nPoint, fp);
    if (status != nPoint) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

    meshData->elemMap[elementIndex][0] = igroup;
    meshData->elemMap[elementIndex][1] = meshData->elemGroups[igroup].nElems-1;

    elementIndex += 1;
  }

  /*@-dependenttrans@*/
  fclose(fpID); fpID = NULL;
  /*@+dependenttrans@*/

  // skip face ID section of the file
  status = fseek(fp, (nTri + nQuad)*sizeof(int), SEEK_CUR);
  if (status != 0) { status = CAPS_IOERR; goto cleanup; }

  // Elements Tetrahedral
  nPoint = 4;
  elementTopo = aimTet;
  nElems = nTet;

  status = aim_readBinaryUgridElements(aimStruc, fp, fpMV, volName,
                                       nPoint, elementTopo, nElems,
                                       &elementIndex, meshData);
  AIM_STATUS(aimStruc, status);

  // Elements Pyramid
  nPoint = 5;
  elementTopo = aimPyramid;
  nElems = nPyramid;

  status = aim_readBinaryUgridElements(aimStruc, fp, fpMV, volName,
                                       nPoint, elementTopo, nElems,
                                       &elementIndex, meshData);
  AIM_STATUS(aimStruc, status);

  // Elements Prism
  nPoint = 6;
  elementTopo = aimPrism;
  nElems = nPrism;

  status = aim_readBinaryUgridElements(aimStruc, fp, fpMV, volName,
                                       nPoint, elementTopo, nElems,
                                       &elementIndex, meshData);
  AIM_STATUS(aimStruc, status);

  // Elements Hex
  nPoint = 8;
  elementTopo = aimHex;
  nElems = nHex;

  status = aim_readBinaryUgridElements(aimStruc, fp, fpMV, volName,
                                       nPoint, elementTopo, nElems,
                                       &elementIndex, meshData);
  AIM_STATUS(aimStruc, status);

  // 2D grid
  if (nTet+nPyramid+nPrism+nHex == 0) {
    meshData->dim = 2;
    AIM_FREE(mapGroupID);
    nMapGroupID = 0;

    status = fread(&nLine, sizeof(int), 1, fp);
    if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

    meshData->nTotalElems += nLine;
    AIM_REALL(meshData->elemMap, meshData->nTotalElems, aimMeshIndices, aimStruc, status);

    // Elements Line
    nPoint = 2;
    elementTopo = aimLine;
    nElems = nLine;

    for (i = 0; i < nElems; i++) {
      /* read the element connectivity */
      status = fread(line, sizeof(int), nPoint, fp);
      if (status != nPoint) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }
      status = fread(&ID, sizeof(int), 1, fp);
      if (status != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }
      if (ID <= 0) {
        AIM_ERROR(aimStruc, "BC ID must be a positive number: %d!", ID);
        status = CAPS_IOERR;
        goto cleanup;
      }

      /* add the BC group if necessary */
      if (ID > nMapGroupID) {
        AIM_REALL(mapGroupID, ID, int, aimStruc, status);
        for (j = nMapGroupID; j < ID; j++) mapGroupID[j] = -1;
        nMapGroupID = ID;
      }
      if (mapGroupID[ID-1] == -1) {
        status = aim_addMeshElemGroup(aimStruc, NULL, ID, elementTopo, 1, nPoint, meshData);
        AIM_STATUS(aimStruc, status);
        mapGroupID[ID-1] = meshData->nElemGroup-1;
      }

      igroup = mapGroupID[ID-1];

      /* add the elements to the group */
      status = aim_addMeshElem(aimStruc, 1, &meshData->elemGroups[igroup]);
      AIM_STATUS(aimStruc, status);

      meshData->elemGroups[igroup].elements[nPoint*meshData->elemGroups[igroup].nElems-2] = line[0];
      meshData->elemGroups[igroup].elements[nPoint*meshData->elemGroups[igroup].nElems-1] = line[1];

      meshData->elemMap[elementIndex][0] = igroup;
      meshData->elemMap[elementIndex][1] = i;

      elementIndex += 1;
    }

  } else {
    meshData->dim = 3;
  }

  /* read in the groupName from mapbc file */
  snprintf(filename, PATH_MAX, "%s%s", mesh->meshRef->fileName, ".mapbc");

  if (access(filename, F_OK) == 0) {
    // Correct the groupID's to be consistent with groupMap
    fpID = fopen(filename, "r");
    if (fpID == NULL) {
      AIM_ERROR(aimStruc, "Failed to open %s", filename);
      status = CAPS_IOERR;
      goto cleanup;
    }
    status = fscanf(fpID, "%d", &nmap);
    if (status != 1) {
      AIM_ERROR(aimStruc, "Failed to read %s", filename);
      status = CAPS_IOERR;
      goto cleanup;
    }

    nID = 0;
    for (i = 0; i < nMapGroupID; i++)
      if (mapGroupID[i] != -1) nID++;

    if (nmap != nID) {
      AIM_ERROR(aimStruc, "Number of maps in %s (%d) should be %d",
                filename, nmap, nID);
      status = CAPS_IOERR;
      goto cleanup;
    }

    for (i = 0; i < nmap; i++) {
      status = fscanf(fpID, "%d %d %s", &ID, &j, groupName);
      if (status != 3) {
        AIM_ERROR(aimStruc, "Failed to read %s", filename);
        status = CAPS_IOERR;
        goto cleanup;
      }

      if (ID <= 0 || nMapGroupID < ID) {
        AIM_ERROR(aimStruc, "ID (%d) in %s out of bounds [1,%d]", ID, filename, nMapGroupID);
        status = CAPS_IOERR;
        goto cleanup;
      }

      if (mapGroupID[ID-1] == -1) {
        AIM_ERROR(aimStruc, "Unknown BC ID (%d) in %s", ID, filename);
        status = CAPS_IOERR;
        goto cleanup;
      }

      AIM_STRDUP(meshData->elemGroups[mapGroupID[ID-1]].groupName, groupName, aimStruc, status);
    }
    
    /*@-dependenttrans@*/
    fclose(fpID); fpID = NULL;
    /*@+dependenttrans@*/
  }

  mesh->meshData = meshData;
  meshData = NULL;

  status = CAPS_SUCCESS;

cleanup:
  if (status != CAPS_SUCCESS) {
    aim_freeMeshData(meshData);
    AIM_FREE(meshData);
  }
  AIM_FREE(mapGroupID);

  if (volName != NULL) {
    for (i = 0; i < nVolName; i++)
      AIM_FREE(volName[i]);
    AIM_FREE(volName);
  }

/*@-dependenttrans@*/
  if (fp   != NULL) fclose(fp);
  if (fpID != NULL) fclose(fpID);
  if (fpMV != NULL) fclose(fpMV);
/*@+dependenttrans@*/

  return status;
}


int
aim_storeMeshRef(void *aimStruc, const aimMeshRef *meshRef,
                 const char *meshextension)
{
  int    status = CAPS_SUCCESS;

  int         imap, ibnd, state, nvert;
  int         i;
  char        filename_src[PATH_MAX];
  char        filename_dst[PATH_MAX];
  char        aimFile[PATH_MAX];
  const char  *meshRefFile = "meshRef.dat";
  const char  *meshRefegads = "meshRef.egads";
  FILE        *fp = NULL;
  aimInfo     *aInfo;
  size_t      len, strLen;
  ego         model, body, *bodies = NULL;

  aInfo = (aimInfo *) aimStruc;
  if (aInfo == NULL)                    return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC)  return CAPS_BADOBJECT;

  if (meshRef == NULL)           return CAPS_NULLOBJ;
  if (meshRef->fileName == NULL) return CAPS_NULLOBJ;

  if (meshextension == NULL)     return CAPS_NULLOBJ;

  // create the full filename to the mesh in the meshing AIM directory
  snprintf(filename_src, PATH_MAX, "%s%s", meshRef->fileName, meshextension);

  // get the mesh filename without the directory
  i = strlen(filename_src);
  while(i > 0 && filename_src[i] != SLASH) { i--; }
  if (i < 0) { status = CAPS_IOERR; goto cleanup; }
  strcpy(filename_dst, filename_src+i+1);

  // copy the mesh from the meshing AIM directory to the current AIM directory
  status = aim_cpFile(aimStruc, filename_src, filename_dst);
  AIM_STATUS(aimStruc, status);

  // open the file to store the meshRef structure
  status = aim_file(aimStruc, meshRefFile, aimFile);
  AIM_STATUS(aimStruc, status);

  fp = fopen(aimFile, "wb");
  if (fp == NULL) {
    AIM_ERROR(aimStruc, "Cannot open file: %s\n", aimFile);
    status = CAPS_IOERR;
    goto cleanup;
  }

  len = fwrite(&meshRef->nmap, sizeof(int), 1, fp);
  if (len != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

  AIM_ALLOC(bodies, 2*meshRef->nmap, ego, aimStruc, status);

  for (imap = 0; imap < meshRef->nmap; imap++) {
    status = EG_statusTessBody(meshRef->maps[imap].tess, &body, &state, &nvert);
    AIM_STATUS(aimStruc, status);

    // construct an egads file to write out the tessellation and the body
    status = EG_copyObject(body, NULL, &bodies[imap]);
    AIM_STATUS(aimStruc, status);
    status = EG_copyObject(meshRef->maps[imap].tess, bodies[imap], &bodies[imap+meshRef->nmap]);
    AIM_STATUS(aimStruc, status);

    // store the body index
    status = EG_attributeAdd(bodies[imap], CAPS_BODY_INDX, ATTRINT, 1, &imap, NULL, NULL);
    AIM_STATUS(aimStruc, status);
  }

  status = EG_makeTopology(aInfo->problem->context, NULL, MODEL, 2*meshRef->nmap,
                           NULL, meshRef->nmap, bodies, NULL, &model);
  AIM_STATUS(aimStruc, status);

  status = aim_file(aimStruc, meshRefegads, aimFile);
  AIM_STATUS(aimStruc, status);

  remove(aimFile);
  status = EG_saveModel(model, aimFile);
  AIM_STATUS(aimStruc, status);

  EG_deleteObject(model);

  for (imap = 0; imap < meshRef->nmap; imap++) {
    status = EG_statusTessBody(meshRef->maps[imap].tess, &body, &state, &nvert);
    AIM_STATUS(aimStruc, status);

    // write the surface to volume map
    len = fwrite(meshRef->maps[imap].map, sizeof(int), nvert, fp);
    if (len != nvert) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }
  }

  len = fwrite(&meshRef->nbnd, sizeof(int), 1, fp);
  if (len != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

  for (ibnd = 0; ibnd < meshRef->nbnd; ibnd++) {

    strLen = strlen(meshRef->bnds[ibnd].groupName)+1;

    len = fwrite(&strLen, sizeof(size_t), 1, fp);
    if (len != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

    len = fwrite(meshRef->bnds[ibnd].groupName, sizeof(char), strLen, fp);
    if (len != strLen) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

    len = fwrite(&meshRef->bnds[ibnd].ID, sizeof(int), 1, fp);
    if (len != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }
  }

  // write meshRef->filename (i.e. filename_dst without the extension)
  strLen = strlen(filename_dst);
  filename_dst[strLen - strlen(meshextension)] = '\0';
  status = aim_file(aimStruc, filename_dst, aimFile);
  AIM_STATUS(aimStruc, status);

  strLen = strlen(aimFile)+1;

  len = fwrite(&strLen, sizeof(size_t), 1, fp);
  if (len != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

  len = fwrite(aimFile, sizeof(char), strLen, fp);
  if (len != strLen) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

cleanup:
  /*@-dependenttrans@*/
  if (fp != NULL) fclose(fp);
  /*@+dependenttrans@*/
  AIM_FREE(bodies);

  return status;
}


int
aim_loadMeshRef(void *aimStruc, aimMeshRef *meshRef)
{
  int    status = CAPS_SUCCESS;

  int         j, imap, ibnd, state, nvert, nbody;
  int         oclass, mtype, *senses, alen, atype;
  char        aimFile[PATH_MAX];
  double      limits[4];
  const int   *aints=NULL;
  const double *areals=NULL;
  const char  *astring=NULL;
  const char  *meshRefFile = "meshRef.dat";
  const char  *meshRefegads = "meshRef.egads";
  FILE        *fp = NULL;
  aimInfo     *aInfo;
  size_t      len, strLen;
  ego         geom, model, body, tessbody, *bodies;

  aInfo = (aimInfo *) aimStruc;
  if (aInfo == NULL)                    return CAPS_NULLOBJ;
  if (aInfo->magicnumber != CAPSMAGIC)  return CAPS_BADOBJECT;

  if (meshRef == NULL)           return CAPS_NULLOBJ;
  if (meshRef->nmap != 0 ||
      meshRef->maps != NULL ||
      meshRef->nbnd != 0 ||
      meshRef->bnds != NULL ||
      meshRef->fileName != NULL ) {
    AIM_ERROR(aimStruc, "meshRef members not initialized to NULL values!");
    status = CAPS_NULLOBJ;
    goto cleanup;
  }

  // open the file to read the meshRef structure
  status = aim_file(aimStruc, meshRefFile, aimFile);
  AIM_STATUS(aimStruc, status);

  fp = fopen(aimFile, "rb");
  if (fp == NULL) {
    AIM_ERROR(aimStruc, "Cannot open file: %s\n", aimFile);
    status = CAPS_IOERR;
    goto cleanup;
  }

  len = fread(&meshRef->nmap, sizeof(int), 1, fp);
  if (len != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

  AIM_ALLOC(meshRef->maps, meshRef->nmap, aimMeshTessMap, aimStruc, status);
  for (imap = 0; imap < meshRef->nmap; imap++) {
    meshRef->maps[imap].tess = NULL;
    meshRef->maps[imap].map = NULL;
  }

  status = aim_file(aimStruc, meshRefegads, aimFile);
  AIM_STATUS(aimStruc, status);

  status = EG_loadModel(aInfo->problem->context, 0, aimFile, &model);
  AIM_STATUS(aimStruc, status);

  status = EG_getTopology(model, &geom, &oclass, &mtype, limits,
                          &nbody, &bodies, &senses);
  AIM_STATUS(aimStruc, status);
  
  if (nbody != meshRef->nmap) {
    AIM_ERROR(aimStruc, "Missmatch between %s and %s!", meshRefFile, meshRefegads);
    status = CAPS_IOERR;
    goto cleanup;
  }

  // copy the body and tessellatoin out of the model
  for (imap = 0; imap < meshRef->nmap; imap++) {
    status = EG_copyObject(bodies[imap], NULL, &body);
    AIM_STATUS(aimStruc, status);

    // get the original body index
    status = EG_attributeRet(body, CAPS_BODY_INDX, &atype, &alen, &aints, &areals, &astring);
    AIM_STATUS(aimStruc, status);
    AIM_NOTNULL(aints, aimStruc, status);

    status = EG_attributeDel(body, CAPS_BODY_INDX);
    AIM_STATUS(aimStruc, status);

    // find the tessellation for this body
    for (j = 0; j < meshRef->nmap; j++) {
      status = EG_statusTessBody(bodies[j+meshRef->nmap], &tessbody, &state, &nvert);
      AIM_STATUS(aimStruc, status);

      if (tessbody == bodies[imap]) {
        // copy the tessellation with the copied body
        status = EG_copyObject(bodies[j+meshRef->nmap], body, &meshRef->maps[aints[0]].tess);
        AIM_STATUS(aimStruc, status);
        break;
      }
    }
  }

  // delete the model
  EG_deleteObject(model);

  for (imap = 0; imap < meshRef->nmap; imap++) {
    // read the surface to volume map
    status = EG_statusTessBody(meshRef->maps[imap].tess, &body, &state, &nvert);
    AIM_STATUS(aimStruc, status);

    AIM_ALLOC(meshRef->maps[imap].map, nvert, int, aimStruc, status);

    len = fread(meshRef->maps[imap].map, sizeof(int), nvert, fp);
    if (len != nvert) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }
  }

  // read the bounds
  len = fread(&meshRef->nbnd, sizeof(int), 1, fp);
  if (len != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

  AIM_ALLOC(meshRef->bnds, meshRef->nbnd, aimMeshBnd, aimStruc, status);
  for (ibnd = 0; ibnd < meshRef->nbnd; ibnd++) {
    meshRef->bnds[ibnd].groupName = NULL;
    meshRef->bnds[ibnd].ID = 0;
  }

  for (ibnd = 0; ibnd < meshRef->nbnd; ibnd++) {

    len = fread(&strLen, sizeof(size_t), 1, fp);
    if (len != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

    AIM_ALLOC(meshRef->bnds[ibnd].groupName, strLen, char, aimStruc, status);

    len = fread(meshRef->bnds[ibnd].groupName, sizeof(char), strLen, fp);
    if (len != strLen) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

    len = fread(&meshRef->bnds[ibnd].ID, sizeof(int), 1, fp);
    if (len != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }
  }

  // read meshRef->filename
  len = fread(&strLen, sizeof(size_t), 1, fp);
  if (len != 1) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

  AIM_ALLOC(meshRef->fileName, strLen, char, aimStruc, status);

  len = fread(meshRef->fileName, sizeof(char), strLen, fp);
  if (len != strLen) { status = CAPS_IOERR; AIM_STATUS(aimStruc, status); }

  // indicate that the tessellation and body should be deleted
  meshRef->_delTess = (int)true;

cleanup:
  /*@-dependenttrans@*/
  if (fp != NULL) fclose(fp);
  /*@+dependenttrans@*/

  return status;
}


// Map the old surface tessellation on to the new bodies
int aim_morphMeshUpdate(void *aimInfo,  aimMeshRef *meshRef, int numBody, ego *bodies)
{
    int status = CAPS_SUCCESS;
    int i=0;
    int state, numVert;
    ego bodyMapping;
    ego body, tessbody;
    ego tess;

    // Have the number of bodies changed?
    if (meshRef->nmap != numBody) {
        AIM_ERROR(aimInfo, "The number of original surface meshes does NOT equal the number of current bodies!\n");
        status = CAPS_MISMATCH;
        goto cleanup;
    }

    // Are the bodies topological equivalent?
    for (i = 0; i < meshRef->nmap; i++) {

        status = EG_statusTessBody(meshRef->maps[i].tess, &body, &state, &numVert);
        AIM_STATUS(aimInfo, status);

        status = EG_mapBody(body, bodies[i], "_faceID",  &bodyMapping); // "_faceID" - same as in OpenCSM
        if (status != EGADS_SUCCESS || bodyMapping != NULL) {

            AIM_ERROR(aimInfo, "New and old body %d (of %d) do not appear to be topologically equivalent!", i+1, meshRef->nmap);

            if (bodyMapping != NULL) {
                AIM_ADDLINE(aimInfo, "Body mapping isn't NULL!");
            }

            status = CAPS_MISMATCH;
            goto cleanup;
        }
    }

    // Now lets "tweak" the surface tessellation - map the old tessellation to the new bodies
    for (i = 0; i < meshRef->nmap; i++) {

        status = EG_statusTessBody(meshRef->maps[i].tess, &tessbody, &state, &numVert);
        AIM_STATUS(aimInfo, status);

        // nothing to do if bodies are the same
        if (tessbody == bodies[i]) continue;

        printf("Projecting tessellation %d (of %d) on to new body\n", i+1, meshRef->nmap);

        status = EG_mapTessBody(meshRef->maps[i].tess,
                                bodies[i],
                                &tess);
        AIM_STATUS(aimInfo, status);

        if (meshRef->_delTess == (int)true) {
            EG_deleteObject(meshRef->maps[i].tess);
            EG_deleteObject(tessbody);
        }
        meshRef->maps[i].tess = tess;
        status = aim_newTess(aimInfo, tess);
        AIM_STATUS(aimInfo, status);
    }

    meshRef->_delTess = (int)false;

    cleanup:
        return status;
}
