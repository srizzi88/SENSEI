/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPolyDataMapper
 * @brief   map svtkPolyData to graphics primitives
 *
 * svtkPolyDataMapper is a class that maps polygonal data (i.e., svtkPolyData)
 * to graphics primitives. svtkPolyDataMapper serves as a superclass for
 * device-specific poly data mappers, that actually do the mapping to the
 * rendering/graphics hardware/software.
 */

#ifndef svtkPolyDataMapper_h
#define svtkPolyDataMapper_h

#include "svtkMapper.h"
#include "svtkRenderingCoreModule.h" // For export macro
//#include "svtkTexture.h" // used to include texture unit enum.

class svtkPolyData;
class svtkRenderer;
class svtkRenderWindow;

class SVTKRENDERINGCORE_EXPORT svtkPolyDataMapper : public svtkMapper
{
public:
  static svtkPolyDataMapper* New();
  svtkTypeMacro(svtkPolyDataMapper, svtkMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Implemented by sub classes. Actual rendering is done here.
   */
  virtual void RenderPiece(svtkRenderer*, svtkActor*){};

  /**
   * This calls RenderPiece (in a for loop if streaming is necessary).
   */
  void Render(svtkRenderer* ren, svtkActor* act) override;

  //@{
  /**
   * Specify the input data to map.
   */
  void SetInputData(svtkPolyData* in);
  svtkPolyData* GetInput();
  //@}

  //@{
  /**
   * Bring this algorithm's outputs up-to-date.
   */
  void Update(int port) override;
  void Update() override;
  svtkTypeBool Update(int port, svtkInformationVector* requests) override;
  svtkTypeBool Update(svtkInformation* requests) override;
  //@}

  //@{
  /**
   * If you want only a part of the data, specify by setting the piece.
   */
  svtkSetMacro(Piece, int);
  svtkGetMacro(Piece, int);
  svtkSetMacro(NumberOfPieces, int);
  svtkGetMacro(NumberOfPieces, int);
  svtkSetMacro(NumberOfSubPieces, int);
  svtkGetMacro(NumberOfSubPieces, int);
  //@}

  //@{
  /**
   * Set the number of ghost cells to return.
   */
  svtkSetMacro(GhostLevel, int);
  svtkGetMacro(GhostLevel, int);
  //@}

  //@{
  /**
   * Accessors / Mutators for handling seams on wrapping surfaces. Letters U and V stand for
   * texture coordinates (u,v).
   *
   * @note Implementation taken from the work of Marco Tarini:
   * Cylindrical and Toroidal Parameterizations Without Vertex Seams
   * Journal of Graphics Tools, 2012, number 3, volume 16, pages 144-150.
   */
  svtkSetMacro(SeamlessU, bool);
  svtkGetMacro(SeamlessU, bool);
  svtkBooleanMacro(SeamlessU, bool);
  svtkSetMacro(SeamlessV, bool);
  svtkGetMacro(SeamlessV, bool);
  svtkBooleanMacro(SeamlessV, bool);
  //@}

  /**
   * Return bounding box (array of six doubles) of data expressed as
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   */
  double* GetBounds() SVTK_SIZEHINT(6) override;
  void GetBounds(double bounds[6]) override { this->Superclass::GetBounds(bounds); }

  /**
   * Make a shallow copy of this mapper.
   */
  void ShallowCopy(svtkAbstractMapper* m) override;

  /**
   * Select a data array from the point/cell data
   * and map it to a generic vertex attribute.
   * vertexAttributeName is the name of the vertex attribute.
   * dataArrayName is the name of the data array.
   * fieldAssociation indicates when the data array is a point data array or
   * cell data array (svtkDataObject::FIELD_ASSOCIATION_POINTS or
   * (svtkDataObject::FIELD_ASSOCIATION_CELLS).
   * componentno indicates which component from the data array must be passed as
   * the attribute. If -1, then all components are passed.
   * Currently only point data is supported.
   */
  virtual void MapDataArrayToVertexAttribute(const char* vertexAttributeName,
    const char* dataArrayName, int fieldAssociation, int componentno = -1);

  // Specify a data array to use as the texture coordinate
  // for a named texture. See svtkProperty.h for how to
  // name textures.
  virtual void MapDataArrayToMultiTextureAttribute(
    const char* textureName, const char* dataArrayName, int fieldAssociation, int componentno = -1);

  /**
   * Remove a vertex attribute mapping.
   */
  virtual void RemoveVertexAttributeMapping(const char* vertexAttributeName);

  /**
   * Remove all vertex attributes.
   */
  virtual void RemoveAllVertexAttributeMappings();

  /**
   * see svtkAlgorithm for details
   */
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  svtkPolyDataMapper();
  ~svtkPolyDataMapper() override {}

  /**
   * Called in GetBounds(). When this method is called, the consider the input
   * to be updated depending on whether this->Static is set or not. This method
   * simply obtains the bounds from the data-object and returns it.
   */
  virtual void ComputeBounds();

  int Piece;
  int NumberOfPieces;
  int NumberOfSubPieces;
  int GhostLevel;
  bool SeamlessU, SeamlessV;

  int FillInputPortInformation(int, svtkInformation*) override;

private:
  svtkPolyDataMapper(const svtkPolyDataMapper&) = delete;
  void operator=(const svtkPolyDataMapper&) = delete;
};

#endif
