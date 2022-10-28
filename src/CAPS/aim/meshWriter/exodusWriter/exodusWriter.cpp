/*
 *      CAPS: Computational Aircraft Prototype Syntheses
 *
 *             Exodus 3D Mesh Writer Code
 *
 *      Copyright 2014-2022, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <string.h>
#include "aimUtil.h"
#include "aimMesh.h"

#include "exodusWriter.h"

#include <exodusII.h>

#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <array>

const char *meshExtension()
{
/*@-observertrans@*/
  return MESHEXTENSION;
/*@+observertrans@*/
}

static int getFaceIndex(const int *tElemConn, std::array<int,3> &aFaceConn)
{
  int tExodusFaceMap[4][3] = {{0,1,3},{1,2,3},{2,0,3},{0,2,1}};
  for(int i = 0; i < 4; i++)
  {
    std::array<int,3> tCurFaceConn;
    for(int j = 0; j < 3; j++)
      tCurFaceConn[j] = tElemConn[tExodusFaceMap[i][j]];
    std::sort(tCurFaceConn.begin(), tCurFaceConn.end());
    if (tCurFaceConn == aFaceConn)
      return i+1;
  }
  return -1;
}


int meshWrite(void *aimInfo, aimMesh *mesh)
{
  int  status; // Function return status

  int CPU_word_size = sizeof(double);
  int IO_word_size = sizeof(double);
  int exoid = 0;

  int i, j, d, igroup, nPoint, nElems, nBnds=0, nCellElem=0, nCellGroup=0, elemID = 0;
  int blk, bnd, cellID = 1;

  ex_init_params par;
  double *x=NULL, *y=NULL, *z=NULL;
  char **coord_names = NULL, **block_names = NULL, **side_set_names=NULL;
  const char *name=NULL;

  char title[MAX_STR_LENGTH+1];
  int64_t num_attr = 0;

  char filename[PATH_MAX];

  ego body;
  int state, np;

  int atype, len;
  const int *ints = NULL;
  const double *reals = NULL;
  const char *str = NULL;

  aimMeshData *meshData = NULL;
  aimMeshRef *meshRef = NULL;

  std::array<int,3> tFaceNodes;
  int tSortedTetFaceMap[4][3] = {{0,1,2},{1,2,3},{0,1,3},{0,2,3}};
  std::map<std::array<int,3>, std::pair<int,const int*> > mFaceToTetMap;
  std::map<std::string, std::vector<int> > sidesetGroups;
  typedef std::map<std::string, std::vector<int> >::const_iterator sidesetGroups_iterator;

  printf("\nWriting exodus file ....\n");

  if (mesh == NULL) return CAPS_NULLVALUE;
  if (mesh->meshRef  == NULL) return CAPS_NULLVALUE;
  if (mesh->meshData == NULL) return CAPS_NULLVALUE;

//  if (mesh->meshData->dim != 2 && mesh->meshData->dim != 3) {
//    AIM_ERROR(aimInfo, "meshData dim = %d must be 2 or 3!!!", mesh->meshData->dim);
  if ( mesh->meshData->dim != 3) {
    AIM_ERROR(aimInfo, "meshData dim = %d must be 3!!!", mesh->meshData->dim);
    return CAPS_BADVALUE;
  }

  snprintf(filename, PATH_MAX, "%s%s", mesh->meshRef->fileName, MESHEXTENSION);

  exoid = ex_create(filename, EX_CLOBBER | EX_NETCDF4 | EX_NOCLASSIC,
                    &CPU_word_size, &IO_word_size);
  if (exoid == 0) {
    AIM_ERROR(aimInfo, "Cannot open file: %s\n", filename);
    return CAPS_IOERR;
  }

  snprintf(title,MAX_STR_LENGTH,"CAPS Generated");
  ex_copy_string(par.title,title,MAX_STR_LENGTH+1);

  meshRef = mesh->meshRef;
  meshData = mesh->meshData;

  for (igroup = 0; igroup < meshData->nElemGroup; igroup++) {
    if (meshData->elemGroups[igroup].order != 1) {
      AIM_ERROR(aimInfo, "CAPS Exodus writer only supports linear mesh elements at the moment! group %d order = %d",
                igroup, meshData->elemGroups[igroup].order);
      status = CAPS_IOERR;
      goto cleanup;
    }

    if (meshData->elemGroups[igroup].elementTopo != aimTri &&
        meshData->elemGroups[igroup].elementTopo != aimTet) {
      AIM_ERROR(aimInfo, "CAPS Exodus writer only supports tetrahedeal meshes at the moment",
                igroup, meshData->elemGroups[igroup].order);
      status = CAPS_IOERR;
      goto cleanup;
    }

    // count the number of trace element groups
    if (meshData->dim == 2) {

      if (meshData->elemGroups[igroup].elementTopo == aimTri ||
          meshData->elemGroups[igroup].elementTopo == aimQuad) {
        nCellElem += meshData->elemGroups[igroup].nElems;
        nCellGroup++;
      }

    } else {

      if (meshData->elemGroups[igroup].elementTopo == aimTet ||
          meshData->elemGroups[igroup].elementTopo == aimPyramid ||
          meshData->elemGroups[igroup].elementTopo == aimPrism ||
          meshData->elemGroups[igroup].elementTopo == aimHex) {
        nCellElem += meshData->elemGroups[igroup].nElems;
        nCellGroup++;
      }

    }
  }

  /* agglomerate side set groups based on semi-colon separated groupName */
  for (igroup = 0, bnd = 0; igroup < meshData->nElemGroup; igroup++) {

    if (meshData->dim == 2) {
      if (meshData->elemGroups[igroup].elementTopo != aimLine)
        continue;
    } else {
      if (meshData->elemGroups[igroup].elementTopo != aimTri &&
          meshData->elemGroups[igroup].elementTopo != aimQuad)
        continue;
    }

    char copy[1025];
    if (meshData->elemGroups[igroup].groupName != NULL) {
      char *rest, *token;

      strncpy(copy, meshData->elemGroups[igroup].groupName, 1024);
      rest = copy;
      while((token = strtok_r(rest, ";", &rest))) {
        sidesetGroups[std::string(token)].push_back(igroup);
      }
    } else {
      snprintf(copy, 104, "BndGroup%d", bnd);
      sidesetGroups[std::string(copy)].push_back(igroup);
    }
  }

  nBnds = sidesetGroups.size();


  par.num_dim       = meshData->dim;
  par.num_nodes     = meshData->nVertex;
  par.num_edge      = 0;
  par.num_edge_blk  = 0;
  par.num_face      = 0;
  par.num_face_blk  = 0;
  par.num_elem      = nCellElem;
  par.num_elem_blk  = nCellGroup;
  par.num_node_sets = 0;
  par.num_edge_sets = 0;
  par.num_face_sets = 0;
  par.num_side_sets = nBnds;
  par.num_elem_sets = 0;
  par.num_node_maps = 0;
  par.num_edge_maps = 0;
  par.num_face_maps = 0;
  par.num_elem_maps = 0;
  par.num_assembly  = 0;
  par.num_blob      = 0;

  status = ex_put_init_ext(exoid, &par);
  AIM_STATUS(aimInfo, status);

  AIM_ALLOC(x, meshData->nVertex, double, aimInfo, status);
  AIM_ALLOC(y, meshData->nVertex, double, aimInfo, status);
  if (meshData->dim == 3)
    AIM_ALLOC(z, meshData->nVertex, double, aimInfo, status);

  for (i = 0; i < meshData->nVertex; i++)
  {
    x[i] = meshData->verts[i][0];
    y[i] = meshData->verts[i][1];
    if (meshData->dim == 3)
      z[i] = meshData->verts[i][2];
  }

  /* write all of the vertices */
  status = ex_put_coord(exoid, x, y, z);
  AIM_STATUS(aimInfo, status);

  AIM_FREE(x);
  AIM_FREE(y);
  AIM_FREE(z);

  /* write coordinate names */
  AIM_ALLOC(coord_names, meshData->dim, char*, aimInfo, status);
  for (d = 0; d < meshData->dim; d++)
    coord_names[d] = NULL;

  for (d = 0; d < meshData->dim; d++)
    AIM_ALLOC(coord_names[d], MAX_STR_LENGTH+1, char, aimInfo, status);

  snprintf(coord_names[0], MAX_STR_LENGTH,"x");
  snprintf(coord_names[1], MAX_STR_LENGTH,"y");
  if (meshData->dim == 3) snprintf(coord_names[2], MAX_STR_LENGTH,"z");

  status = ex_put_coord_names(exoid, coord_names);
  AIM_STATUS(aimInfo, status);

  for (d = 0; d < meshData->dim; d++)
    AIM_FREE(coord_names[d]);
  AIM_FREE(coord_names);

  /* write volume elements blocks */
  cellID = 1;
  for (igroup = 0; igroup < meshData->nElemGroup; igroup++) {

    if (meshData->dim == 2) {
      if (meshData->elemGroups[igroup].elementTopo == aimLine)
        continue;
    } else {
      if (meshData->elemGroups[igroup].elementTopo == aimTri ||
          meshData->elemGroups[igroup].elementTopo == aimQuad)
        continue;
    }

    nPoint = meshData->elemGroups[igroup].nPoint;
    nElems = meshData->elemGroups[igroup].nElems;
    elemID = meshData->elemGroups[igroup].ID;

         if (meshData->elemGroups[igroup].elementTopo == aimTri    ) name = "tri";
    else if (meshData->elemGroups[igroup].elementTopo == aimQuad   ) name = "quad";
    else if (meshData->elemGroups[igroup].elementTopo == aimTet    ) name = "tet";
    else if (meshData->elemGroups[igroup].elementTopo == aimPyramid) name = "pyramid";
    else if (meshData->elemGroups[igroup].elementTopo == aimPrism  ) name = "prism";
    else if (meshData->elemGroups[igroup].elementTopo == aimHex    ) name = "hex";

    /* write the block info */
    status = ex_put_block(exoid, EX_ELEM_BLOCK, elemID, name, nElems, nPoint, 0, 0, num_attr);
    AIM_STATUS(aimInfo, status);

    /* write the element connectivity */
    status = ex_put_conn(exoid, EX_ELEM_BLOCK, elemID, meshData->elemGroups[igroup].elements, NULL, NULL);
    AIM_STATUS(aimInfo, status);

    /* construct the trace to cellID map */
    for(i = 0; i < nElems; i++, cellID++)
    {
      const int *tElemConn = meshData->elemGroups[igroup].elements + nPoint*i;
      for(j = 0; j < 4; j++) // assuming 4 faces on a tet
      {
        tFaceNodes[0] = tElemConn[tSortedTetFaceMap[j][0]];
        tFaceNodes[1] = tElemConn[tSortedTetFaceMap[j][1]];
        tFaceNodes[2] = tElemConn[tSortedTetFaceMap[j][2]];
        std::sort(tFaceNodes.begin(), tFaceNodes.end());
        mFaceToTetMap[tFaceNodes] = std::pair<int,const int*>(cellID, tElemConn);
      }  
    }
  }

  AIM_ALLOC(block_names, nCellGroup, char*, aimInfo, status);
  for (blk = 0; blk < nCellGroup; blk++)
    block_names[blk] = NULL;

  // if there is only one tessellation then look for the "_name" to indicate the block_name
  if (meshRef->nmap == 1) {
    status = EG_statusTessBody(meshRef->maps[0].tess, &body, &state, &np);
    AIM_STATUS(aimInfo, status);

    EG_attributeRet(body, "_name", &atype, &len, &ints, &reals, &str);
  }

  for (blk = 0, igroup = 0; igroup < meshData->nElemGroup; igroup++)
  {
    if (meshData->dim == 2) {
      if (meshData->elemGroups[igroup].elementTopo == aimLine)
        continue;
    } else {
      if (meshData->elemGroups[igroup].elementTopo == aimTri ||
          meshData->elemGroups[igroup].elementTopo == aimQuad)
        continue;
    }

    AIM_ALLOC(block_names[blk], MAX_STR_LENGTH+1, char, aimInfo, status);

    if (meshData->elemGroups[igroup].groupName != NULL)
      snprintf(block_names[blk], MAX_STR_LENGTH,"%s", meshData->elemGroups[igroup].groupName);
    else if (str != NULL)
      snprintf(block_names[blk], MAX_STR_LENGTH,"%s",str);
    else
      snprintf(block_names[blk], MAX_STR_LENGTH,"Block%d",blk);

    blk++;
  }

  /* write the block names */
  status = ex_put_names(exoid, EX_ELEM_BLOCK, block_names);
  AIM_STATUS(aimInfo, status);

  /* write side sets */
  elemID = 0;
  for (sidesetGroups_iterator sideset = sidesetGroups.begin(); sideset != sidesetGroups.end(); ++sideset) {

    nElems = 0;
    for (i = 0; i < (int)sideset->second.size(); i++) {
      igroup = sideset->second[i];
      nElems += meshData->elemGroups[igroup].nElems;
      elemID += meshData->elemGroups[igroup].ID + nBnds*i;
    }

    std::vector<int> elem_list(nElems,-1);
    std::vector<int> side_list(nElems,-1);

    /* write the side set info */
    status = ex_put_set_param(exoid, EX_SIDE_SET, elemID, nElems, 0);
    AIM_STATUS(aimInfo, status);

    int ielem = 0;
    for (i = 0; i < (int)sideset->second.size(); i++) {
      igroup = sideset->second[i];
      nElems = meshData->elemGroups[igroup].nElems;
      nPoint = meshData->elemGroups[igroup].nPoint;

      for(j = 0; j < nElems; j++, ielem++) {
        const int *tElemConn = meshData->elemGroups[igroup].elements + nPoint*j;

        tFaceNodes[0] = tElemConn[0];
        tFaceNodes[1] = tElemConn[1];
        tFaceNodes[2] = tElemConn[2];

        std::sort(tFaceNodes.begin(), tFaceNodes.end());
        std::pair<int,const int*> tAttachedElem = mFaceToTetMap.at(tFaceNodes);

        elem_list[ielem] = tAttachedElem.first;
        side_list[ielem] = getFaceIndex(tAttachedElem.second, tFaceNodes);
      }
    }

    /* write the side set elem and side lists */
    status = ex_put_set(exoid, EX_SIDE_SET, elemID, elem_list.data(), side_list.data());
    AIM_STATUS(aimInfo, status);
    
    elemID++;
  }

  AIM_ALLOC(side_set_names, nBnds, char*, aimInfo, status);
  for (bnd = 0; bnd < nBnds; bnd++)
    side_set_names[bnd] = NULL;

  bnd = 0;
  for (sidesetGroups_iterator sideset = sidesetGroups.begin(); sideset != sidesetGroups.end(); ++sideset) {
    AIM_ALLOC(side_set_names[bnd], MAX_STR_LENGTH+1, char, aimInfo, status);
    snprintf(side_set_names[bnd], MAX_STR_LENGTH, "%s", sideset->first.c_str());
    bnd++;
  }

  status = ex_put_names(exoid, EX_SIDE_SET, side_set_names);
  AIM_STATUS(aimInfo, status);

  printf("Finished writing Exodus file\n\n");

  status = CAPS_SUCCESS;

cleanup:

  ex_close(exoid);

  AIM_FREE(x);
  AIM_FREE(y);
  AIM_FREE(z);
  if (coord_names != NULL) {
    for (d = 0; d < meshData->dim; d++)
      AIM_FREE(coord_names[d]);
    AIM_FREE(coord_names);
  }
  if (block_names != NULL) {
    for (blk = 0; blk < nCellGroup; blk++)
      AIM_FREE(block_names[blk]);
    AIM_FREE(block_names);
  }
  if (side_set_names != NULL) {
    for (bnd = 0; bnd < nBnds; bnd++)
      AIM_FREE(side_set_names[bnd]);
    AIM_FREE(side_set_names);
  }

  return status;
}
