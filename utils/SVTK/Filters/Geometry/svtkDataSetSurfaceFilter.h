/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataSetSurfaceFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDataSetSurfaceFilter
 * @brief   Extracts outer (polygonal) surface.
 *
 * svtkDataSetSurfaceFilter is a faster version of svtkGeometry filter, but it
 * does not have an option to select bounds.  It may use more memory than
 * svtkGeometryFilter.  It only has one option: whether to use triangle strips
 * when the input type is structured.
 *
 * @sa
 * svtkGeometryFilter svtkStructuredGridGeometryFilter.
 */

#ifndef svtkDataSetSurfaceFilter_h
#define svtkDataSetSurfaceFilter_h

#include "svtkFiltersGeometryModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkPointData;
class svtkPoints;
class svtkIdTypeArray;
class svtkStructuredGrid;

// Helper structure for hashing faces.
struct svtkFastGeomQuadStruct
{
  struct svtkFastGeomQuadStruct* Next;
  svtkIdType SourceId;
  int numPts;
  svtkIdType* ptArray;
};
typedef struct svtkFastGeomQuadStruct svtkFastGeomQuad;

class SVTKFILTERSGEOMETRY_EXPORT svtkDataSetSurfaceFilter : public svtkPolyDataAlgorithm
{
public:
  static svtkDataSetSurfaceFilter* New();
  svtkTypeMacro(svtkDataSetSurfaceFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * When input is structured data, this flag will generate faces with
   * triangle strips.  This should render faster and use less memory, but no
   * cell data is copied.  By default, UseStrips is Off.
   */
  svtkSetMacro(UseStrips, svtkTypeBool);
  svtkGetMacro(UseStrips, svtkTypeBool);
  svtkBooleanMacro(UseStrips, svtkTypeBool);
  //@}

  //@{
  /**
   * If PieceInvariant is true, svtkDataSetSurfaceFilter requests
   * 1 ghost level from input in order to remove internal surface
   * that are between processes. False by default.
   */
  svtkSetMacro(PieceInvariant, int);
  svtkGetMacro(PieceInvariant, int);
  //@}

  //@{
  /**
   * If on, the output polygonal dataset will have a celldata array that
   * holds the cell index of the original 3D cell that produced each output
   * cell. This is useful for cell picking. The default is off to conserve
   * memory. Note that PassThroughCellIds will be ignored if UseStrips is on,
   * since in that case each tringle strip can represent more than on of the
   * input cells.
   */
  svtkSetMacro(PassThroughCellIds, svtkTypeBool);
  svtkGetMacro(PassThroughCellIds, svtkTypeBool);
  svtkBooleanMacro(PassThroughCellIds, svtkTypeBool);
  svtkSetMacro(PassThroughPointIds, svtkTypeBool);
  svtkGetMacro(PassThroughPointIds, svtkTypeBool);
  svtkBooleanMacro(PassThroughPointIds, svtkTypeBool);
  //@}

  //@{
  /**
   * If PassThroughCellIds or PassThroughPointIds is on, then these ivars
   * control the name given to the field in which the ids are written into.  If
   * set to nullptr, then svtkOriginalCellIds or svtkOriginalPointIds (the default)
   * is used, respectively.
   */
  svtkSetStringMacro(OriginalCellIdsName);
  virtual const char* GetOriginalCellIdsName()
  {
    return (this->OriginalCellIdsName ? this->OriginalCellIdsName : "svtkOriginalCellIds");
  }
  svtkSetStringMacro(OriginalPointIdsName);
  virtual const char* GetOriginalPointIdsName()
  {
    return (this->OriginalPointIdsName ? this->OriginalPointIdsName : "svtkOriginalPointIds");
  }
  //@}

  //@{
  /**
   * If the input is an unstructured grid with nonlinear faces, this parameter
   * determines how many times the face is subdivided into linear faces.  If 0,
   * the output is the equivalent of its linear counterpart (and the midpoints
   * determining the nonlinear interpolation are discarded).  If 1 (the
   * default), the nonlinear face is triangulated based on the midpoints.  If
   * greater than 1, the triangulated pieces are recursively subdivided to reach
   * the desired subdivision.  Setting the value to greater than 1 may cause
   * some point data to not be passed even if no nonlinear faces exist.  This
   * option has no effect if the input is not an unstructured grid.
   */
  svtkSetMacro(NonlinearSubdivisionLevel, int);
  svtkGetMacro(NonlinearSubdivisionLevel, int);
  //@}

  //@{
  /**
   * Direct access methods that can be used to use the this class as an
   * algorithm without using it as a filter.
   */
  virtual int StructuredExecute(
    svtkDataSet* input, svtkPolyData* output, svtkIdType* ext, svtkIdType* wholeExt);
#ifdef SVTK_USE_64BIT_IDS
  virtual int StructuredExecute(svtkDataSet* input, svtkPolyData* output, int* ext32, int* wholeExt32)
  {
    svtkIdType ext[6];
    svtkIdType wholeExt[6];
    for (int cc = 0; cc < 6; cc++)
    {
      ext[cc] = ext32[cc];
      wholeExt[cc] = wholeExt32[cc];
    }
    return this->StructuredExecute(input, output, ext, wholeExt);
  }
#endif
  virtual int UnstructuredGridExecute(svtkDataSet* input, svtkPolyData* output);
  virtual int DataSetExecute(svtkDataSet* input, svtkPolyData* output);
  virtual int StructuredWithBlankingExecute(svtkStructuredGrid* input, svtkPolyData* output);
  virtual int UniformGridExecute(svtkDataSet* input, svtkPolyData* output, svtkIdType* ext,
    svtkIdType* wholeExt, bool extractface[6]);
#ifdef SVTK_USE_64BIT_IDS
  virtual int UniformGridExecute(
    svtkDataSet* input, svtkPolyData* output, int* ext32, int* wholeExt32, bool extractface[6])
  {
    svtkIdType ext[6];
    svtkIdType wholeExt[6];
    for (int cc = 0; cc < 6; cc++)
    {
      ext[cc] = ext32[cc];
      wholeExt[cc] = wholeExt32[cc];
    }
    return this->UniformGridExecute(input, output, ext, wholeExt, extractface);
  }
#endif
  //@}

protected:
  svtkDataSetSurfaceFilter();
  ~svtkDataSetSurfaceFilter() override;

  svtkTypeBool UseStrips;

  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  // Helper methods.

  /**
   * Estimates the total number of points & cells on the surface to render
   * ext -- the extent of the structured data in question (in)
   * wholeExt -- the global extent of the structured data (in)
   * numPoints -- the estimated number of points (out)
   * numCells -- the estimated number of cells (out)
   */
  void EstimateStructuredDataArraySizes(
    svtkIdType* ext, svtkIdType* wholeExt, svtkIdType& numPoints, svtkIdType& numCells);

  void ExecuteFaceStrips(svtkDataSet* input, svtkPolyData* output, int maxFlag, svtkIdType* ext,
    int aAxis, int bAxis, int cAxis, svtkIdType* wholeExt);

  void ExecuteFaceQuads(svtkDataSet* input, svtkPolyData* output, int maxFlag, svtkIdType* ext,
    int aAxis, int bAxis, int cAxis, svtkIdType* wholeExt, bool checkVisibility);

  void ExecuteFaceQuads(svtkDataSet* input, svtkPolyData* output, int maxFlag, svtkIdType* ext,
    int aAxis, int bAxis, int cAxis, svtkIdType* wholeExt);

  void InitializeQuadHash(svtkIdType numPoints);
  void DeleteQuadHash();
  virtual void InsertQuadInHash(
    svtkIdType a, svtkIdType b, svtkIdType c, svtkIdType d, svtkIdType sourceId);
  virtual void InsertTriInHash(
    svtkIdType a, svtkIdType b, svtkIdType c, svtkIdType sourceId, svtkIdType faceId = -1);
  virtual void InsertPolygonInHash(const svtkIdType* ids, int numpts, svtkIdType sourceId);
  void InitQuadHashTraversal();
  svtkFastGeomQuad* GetNextVisibleQuadFromHash();

  svtkFastGeomQuad** QuadHash;
  svtkIdType QuadHashLength;
  svtkFastGeomQuad* QuadHashTraversal;
  svtkIdType QuadHashTraversalIndex;

  svtkIdType* PointMap;
  svtkIdType GetOutputPointId(
    svtkIdType inPtId, svtkDataSet* input, svtkPoints* outPts, svtkPointData* outPD);

  class svtkEdgeInterpolationMap;

  svtkEdgeInterpolationMap* EdgeMap;
  svtkIdType GetInterpolatedPointId(svtkIdType edgePtA, svtkIdType edgePtB, svtkDataSet* input,
    svtkCell* cell, double pcoords[3], svtkPoints* outPts, svtkPointData* outPD);

  svtkIdType NumberOfNewCells;

  // Better memory allocation for faces (hash)
  void InitFastGeomQuadAllocation(svtkIdType numberOfCells);
  svtkFastGeomQuad* NewFastGeomQuad(int numPts);
  void DeleteAllFastGeomQuads();
  // -----
  svtkIdType FastGeomQuadArrayLength;
  svtkIdType NumberOfFastGeomQuadArrays;
  unsigned char** FastGeomQuadArrays; // store this data as an array of bytes
  // These indexes allow us to find the next available face.
  svtkIdType NextArrayIndex;
  svtkIdType NextQuadIndex;

  int PieceInvariant;

  svtkTypeBool PassThroughCellIds;
  void RecordOrigCellId(svtkIdType newIndex, svtkIdType origId);
  virtual void RecordOrigCellId(svtkIdType newIndex, svtkFastGeomQuad* quad);
  svtkIdTypeArray* OriginalCellIds;
  char* OriginalCellIdsName;

  svtkTypeBool PassThroughPointIds;
  void RecordOrigPointId(svtkIdType newIndex, svtkIdType origId);
  svtkIdTypeArray* OriginalPointIds;
  char* OriginalPointIdsName;

  int NonlinearSubdivisionLevel;

private:
  svtkDataSetSurfaceFilter(const svtkDataSetSurfaceFilter&) = delete;
  void operator=(const svtkDataSetSurfaceFilter&) = delete;
};

#endif
