/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCameraPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkCompositeMapperHelper2_h
#define svtkCompositeMapperHelper2_h

#ifndef __SVTK_WRAP__

#include "svtkColor.h"
#include "svtkOpenGL.h"
#include "svtkOpenGLPolyDataMapper.h"
#include "svtkRenderingOpenGL2Module.h"

class svtkPolyData;
class svtkCompositePolyDataMapper2;

// this class encapsulates values tied to a
// polydata
class svtkCompositeMapperHelperData
{
public:
  svtkPolyData* Data;
  unsigned int FlatIndex;
  double Opacity;
  bool IsOpaque;
  bool Visibility;
  bool Pickability;
  bool OverridesColor;
  svtkColor3d AmbientColor;
  svtkColor3d DiffuseColor;

  bool Marked;

  unsigned int StartVertex;
  unsigned int NextVertex;

  // point line poly strip edge stripedge
  unsigned int StartIndex[svtkOpenGLPolyDataMapper::PrimitiveEnd];
  unsigned int NextIndex[svtkOpenGLPolyDataMapper::PrimitiveEnd];

  // stores the mapping from svtk cells to gl_PrimitiveId
  svtkNew<svtkOpenGLCellToSVTKCellMap> CellCellMap;
};

//===================================================================
// We define a helper class that is a subclass of svtkOpenGLPolyDataMapper
class SVTKRENDERINGOPENGL2_EXPORT svtkCompositeMapperHelper2 : public svtkOpenGLPolyDataMapper
{
public:
  static svtkCompositeMapperHelper2* New();
  svtkTypeMacro(svtkCompositeMapperHelper2, svtkOpenGLPolyDataMapper);

  void SetParent(svtkCompositePolyDataMapper2* p) { this->Parent = p; }

  svtkCompositeMapperHelperData* AddData(svtkPolyData* pd, unsigned int flatIndex);

  // Description:
  // Implemented by sub classes. Actual rendering is done here.
  void RenderPiece(svtkRenderer* ren, svtkActor* act) override;

  // keep track of what data is being used as the multiblock
  // can change
  void ClearMark();
  void RemoveUnused();
  bool GetMarked() { return this->Marked; }
  void SetMarked(bool v) { this->Marked = v; }

  /**
   * Accessor to the ordered list of PolyData that we last drew.
   */
  std::vector<svtkPolyData*> GetRenderedList() { return this->RenderedList; }

  /**
   * allows a mapper to update a selections color buffers
   * Called from a prop which in turn is called from the selector
   */
  void ProcessSelectorPixelBuffers(
    svtkHardwareSelector* sel, std::vector<unsigned int>& pixeloffsets, svtkProp* prop) override;

  virtual void ProcessCompositePixelBuffers(svtkHardwareSelector* sel, svtkProp* prop,
    svtkCompositeMapperHelperData* hdata, std::vector<unsigned int>& mypixels);

protected:
  svtkCompositePolyDataMapper2* Parent;
  std::map<svtkPolyData*, svtkCompositeMapperHelperData*> Data;

  bool Marked;

  svtkCompositeMapperHelper2() { this->Parent = nullptr; };
  ~svtkCompositeMapperHelper2() override;

  void DrawIBO(svtkRenderer* ren, svtkActor* actor, int primType, svtkOpenGLHelper& CellBO,
    GLenum mode, int pointSize);

  virtual void SetShaderValues(
    svtkShaderProgram* prog, svtkCompositeMapperHelperData* hdata, size_t primOffset);

  /**
   * Make sure appropriate shaders are defined, compiled and bound.  This method
   * orchistrates the process, much of the work is done in other methods
   */
  void UpdateShaders(svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* act) override;

  // Description:
  // Perform string replacements on the shader templates, called from
  // ReplaceShaderValues
  void ReplaceShaderColor(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act) override;

  // Description:
  // Build the VBO/IBO, called by UpdateBufferObjects
  void BuildBufferObjects(svtkRenderer* ren, svtkActor* act) override;
  virtual void AppendOneBufferObject(svtkRenderer* ren, svtkActor* act,
    svtkCompositeMapperHelperData* hdata, svtkIdType& flat_index, std::vector<unsigned char>& colors,
    std::vector<float>& norms);

  // Description:
  // Returns if we can use texture maps for scalar coloring. Note this doesn't
  // say we "will" use scalar coloring. It says, if we do use scalar coloring,
  // we will use a texture. Always off for this mapper.
  int CanUseTextureMapForColoring(svtkDataObject*) override;

  std::vector<unsigned int> VertexOffsets;

  // vert line poly strip edge stripedge
  std::vector<unsigned int> IndexArray[PrimitiveEnd];

  void RenderPieceDraw(svtkRenderer* ren, svtkActor* act) override;

  bool PrimIDUsed;
  bool OverideColorUsed;

  svtkHardwareSelector* CurrentSelector;

  // bookkeeping required by svtkValuePass
  std::vector<svtkPolyData*> RenderedList;

  // used by the hardware selector
  std::vector<std::vector<unsigned int> > PickPixels;

  std::map<svtkAbstractArray*, svtkDataArray*> ColorArrayMap;

private:
  svtkCompositeMapperHelper2(const svtkCompositeMapperHelper2&) = delete;
  void operator=(const svtkCompositeMapperHelper2&) = delete;
};

#endif

#endif
// SVTK-HeaderTest-Exclude: svtkCompositePolyDataMapper2Internal.h
