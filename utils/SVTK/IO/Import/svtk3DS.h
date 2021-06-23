/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtk3DS.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtk3DS_h
#define svtk3DS_h

#include <ctype.h>

class svtkLight;
class svtkCamera;
class svtkProperty;

typedef float svtk3DSVector[3];

/* A generic list type */
#define SVTK_LIST_INSERT(root, node)                                                                \
  list_insert((svtk3DSList**)&root, reinterpret_cast<svtk3DSList*>(node))
#define SVTK_LIST_FIND(root, name) list_find((svtk3DSList**)&root, name)
#define SVTK_LIST_DELETE(root, node) list_delete((svtk3DSList**)&root, (svtk3DSList*)node)
#define SVTK_LIST_KILL(root) list_kill((svtk3DSList**)&root)

#define SVTK_LIST_FIELDS                                                                            \
  char name[80];                                                                                   \
  void* next;

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int dword;

typedef struct
{
  SVTK_LIST_FIELDS
} svtk3DSList;

typedef struct
{
  int a, b, c;
} svtk3DSFace;

typedef struct
{
  float red, green, blue;
} svtk3DSColour;

/* Omni light command */
typedef struct
{
  SVTK_LIST_FIELDS

  svtk3DSVector pos; /* Light position */
  svtk3DSColour col; /* Light colour */
  svtkLight* aLight;
} svtk3DSOmniLight;

/* Spotlight command */
typedef struct
{
  SVTK_LIST_FIELDS

  svtk3DSVector pos;    /* Spotlight position */
  svtk3DSVector target; /* Spotlight target location */
  svtk3DSColour col;    /* Spotlight colour */
  float hotspot;       /* Hotspot angle (degrees) */
  float falloff;       /* Falloff angle (degrees) */
  int shadow_flag;     /* Shadow flag (not used) */
  svtkLight* aLight;
} svtk3DSSpotLight;

/* Camera command */
typedef struct
{
  SVTK_LIST_FIELDS

  svtk3DSVector pos;    /* Camera location */
  svtk3DSVector target; /* Camera target */
  float bank;          /* Banking angle (degrees) */
  float lens;          /* Camera lens size (mm) */
  svtkCamera* aCamera;
} svtk3DSCamera;

/* Material list */
typedef struct
{
  SVTK_LIST_FIELDS

  int external; /* Externally defined material? */
} svtk3DSMaterial;

/* Object summary */
typedef struct
{
  SVTK_LIST_FIELDS

  svtk3DSVector center;  /* Min value of object extents */
  svtk3DSVector lengths; /* Max value of object extents */
} svtk3DSSummary;

/* Material property */
typedef struct
{
  SVTK_LIST_FIELDS

  svtk3DSColour ambient;
  svtk3DSColour diffuse;
  svtk3DSColour specular;
  float shininess;
  float transparency;
  float reflection;
  int self_illum;
  char tex_map[40];
  float tex_strength;
  char bump_map[40];
  float bump_strength;
  svtkProperty* aProperty;
} svtk3DSMatProp;

class svtkActor;
class svtkPolyDataMapper;
class svtkPolyDataNormals;
class svtkStripper;
class svtkPoints;
class svtkCellArray;
class svtkPolyData;

/* A mesh object */
typedef struct
{
  SVTK_LIST_FIELDS

  int vertices;         /* Number of vertices */
  svtk3DSVector* vertex; /* List of object vertices */

  int faces;            /* Number of faces */
  svtk3DSFace* face;     /* List of object faces */
  svtk3DSMaterial** mtl; /* Materials for each face */

  int hidden; /* Hidden flag */
  int shadow; /* Shadow flag */
  svtkActor* anActor;
  svtkPolyDataMapper* aMapper;
  svtkPolyDataNormals* aNormals;
  svtkStripper* aStripper;
  svtkPoints* aPoints;
  svtkCellArray* aCellArray;
  svtkPolyData* aPolyData;

} svtk3DSMesh;

typedef struct
{
  dword start;
  dword end;
  dword length;
  word tag;
} svtk3DSChunk;

typedef struct
{
  byte red;
  byte green;
  byte blue;
} svtk3DSColour_24;

#endif
// SVTK-HeaderTest-Exclude: svtk3DS.h
