/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLPolyDataMapper
 * @brief   PolyDataMapper using OpenGL to render.
 *
 * PolyDataMapper that uses a OpenGL to do the actual rendering.
 */

#ifndef svtkOpenGLPolyDataMapper_h
#define svtkOpenGLPolyDataMapper_h

#include "svtkNew.h"          // For svtkNew
#include "svtkNew.h"          // for ivars
#include "svtkOpenGLHelper.h" // used for ivars
#include "svtkPolyDataMapper.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro
#include "svtkShader.h"                 // for methods
#include "svtkStateStorage.h"           // used for ivars

#include <map>    //for methods
#include <vector> //for ivars

class svtkCellArray;
class svtkGenericOpenGLResourceFreeCallback;
class svtkMatrix4x4;
class svtkMatrix3x3;
class svtkOpenGLCellToSVTKCellMap;
class svtkOpenGLRenderTimer;
class svtkOpenGLTexture;
class svtkOpenGLBufferObject;
class svtkOpenGLVertexBufferObject;
class svtkOpenGLVertexBufferObjectGroup;
class svtkPoints;
class svtkTexture;
class svtkTextureObject;
class svtkTransform;
class svtkOpenGLShaderProperty;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLPolyDataMapper : public svtkPolyDataMapper
{
public:
  static svtkOpenGLPolyDataMapper* New();
  svtkTypeMacro(svtkOpenGLPolyDataMapper, svtkPolyDataMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Implemented by sub classes. Actual rendering is done here.
   */
  void RenderPiece(svtkRenderer* ren, svtkActor* act) override;

  //@{
  /**
   * Implemented by sub classes. Actual rendering is done here.
   */
  virtual void RenderPieceStart(svtkRenderer* ren, svtkActor* act);
  virtual void RenderPieceDraw(svtkRenderer* ren, svtkActor* act);
  virtual void RenderPieceFinish(svtkRenderer* ren, svtkActor* act);
  //@}

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  svtkGetMacro(PopulateSelectionSettings, int);
  void SetPopulateSelectionSettings(int v) { this->PopulateSelectionSettings = v; }

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Used by svtkHardwareSelector to determine if the prop supports hardware
   * selection.
   */
  bool GetSupportsSelection() override { return true; }

  // used by RenderPiece and functions it calls to reduce
  // calls to get the input and allow for rendering of
  // other polydata (not the input)
  svtkPolyData* CurrentInput;

  //@{
  /**
   * By default, this class uses the dataset's point and cell ids during
   * rendering. However, one can override those by specifying cell and point
   * data arrays to use instead. Currently, only svtkIdType array is supported.
   * Set to NULL string (default) to use the point ids instead.
   */
  svtkSetStringMacro(PointIdArrayName);
  svtkGetStringMacro(PointIdArrayName);
  svtkSetStringMacro(CellIdArrayName);
  svtkGetStringMacro(CellIdArrayName);
  //@}

  //@{
  /**
   * If this class should override the process id using a data-array,
   * set this variable to the name of the array to use. It must be a
   * point-array.
   */
  svtkSetStringMacro(ProcessIdArrayName);
  svtkGetStringMacro(ProcessIdArrayName);
  //@}

  //@{
  /**
   * Generally, this class can render the composite id when iterating
   * over composite datasets. However in some cases (as in AMR), the rendered
   * structure may not correspond to the input data, in which case we need
   * to provide a cell array that can be used to render in the composite id in
   * selection passes. Set to NULL (default) to not override the composite id
   * color set by svtkCompositePainter if any.
   * The array *MUST* be a cell array and of type svtkUnsignedIntArray.
   */
  svtkSetStringMacro(CompositeIdArrayName);
  svtkGetStringMacro(CompositeIdArrayName);
  //@}

#ifndef SVTK_LEGACY_REMOVE
  //@{
  /**
   * This function enables you to apply your own substitutions
   * to the shader creation process. The shader code in this class
   * is created by applying a bunch of string replacements to a
   * shader template. Using this function you can apply your
   * own string replacements to add features you desire.
   *
   * @deprecated Replaced By svtkShaderProperty::{Add,Clear,ClearAll}ShaderReplacements as of
   * SVTK 9.0.
   */
  SVTK_LEGACY(void AddShaderReplacement(svtkShader::Type shaderType, // vertex, fragment, etc
    const std::string& originalValue,
    bool replaceFirst, // do this replacement before the default
    const std::string& replacementValue, bool replaceAll);)
  SVTK_LEGACY(void ClearShaderReplacement(svtkShader::Type shaderType, // vertex, fragment, etc
    const std::string& originalValue, bool replaceFirst);)
  SVTK_LEGACY(void ClearAllShaderReplacements(svtkShader::Type shaderType);)
  SVTK_LEGACY(void ClearAllShaderReplacements();)
  //@}

  //@{
  /**
   * Allow the program to set the shader codes used directly
   * instead of using the built in templates. Be aware, if
   * set, this template will be used for all cases,
   * primitive types, picking etc.
   *
   * @deprecated Replaced By svtkShaderProperty::Get*ShaderCode as of SVTK 9.0.
   */
  SVTK_LEGACY(virtual void SetVertexShaderCode(const char* code);)
  SVTK_LEGACY(virtual char* GetVertexShaderCode();)
  SVTK_LEGACY(virtual void SetFragmentShaderCode(const char* code);)
  SVTK_LEGACY(virtual char* GetFragmentShaderCode();)
  SVTK_LEGACY(virtual void SetGeometryShaderCode(const char* code);)
  SVTK_LEGACY(virtual char* GetGeometryShaderCode();)
  //@}
#endif

  /**
   * Make a shallow copy of this mapper.
   */
  void ShallowCopy(svtkAbstractMapper* m) override;

  /// Return the mapper's vertex buffer objects.
  svtkGetObjectMacro(VBOs, svtkOpenGLVertexBufferObjectGroup);

  /**\brief A convenience method for enabling/disabling
   *   the VBO's shift+scale transform.
   */
  void SetVBOShiftScaleMethod(int m);

  enum PrimitiveTypes
  {
    PrimitiveStart = 0,
    PrimitivePoints = 0,
    PrimitiveLines,
    PrimitiveTris,
    PrimitiveTriStrips,
    PrimitiveTrisEdges,
    PrimitiveTriStripsEdges,
    PrimitiveVertices,
    PrimitiveEnd
  };

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
   */
  void MapDataArrayToVertexAttribute(const char* vertexAttributeName, const char* dataArrayName,
    int fieldAssociation, int componentno = -1) override;

  // This method will Map the specified data array for use as
  // a texture coordinate for texture tname. The actual
  // attribute will be named tname_coord so as to not
  // conflict with the texture sampler definition which will
  // be tname.
  void MapDataArrayToMultiTextureAttribute(const char* tname, const char* dataArrayName,
    int fieldAssociation, int componentno = -1) override;

  /**
   * Remove a vertex attribute mapping.
   */
  void RemoveVertexAttributeMapping(const char* vertexAttributeName) override;

  /**
   * Remove all vertex attributes.
   */
  void RemoveAllVertexAttributeMappings() override;

  /**
   * allows a mapper to update a selections color buffers
   * Called from a prop which in turn is called from the selector
   */
  void ProcessSelectorPixelBuffers(
    svtkHardwareSelector* sel, std::vector<unsigned int>& pixeloffsets, svtkProp* prop) override;

protected:
  svtkOpenGLPolyDataMapper();
  ~svtkOpenGLPolyDataMapper() override;

  svtkGenericOpenGLResourceFreeCallback* ResourceCallback;

  void MapDataArray(const char* vertexAttributeName, const char* dataArrayName,
    const char* texturename, int fieldAssociation, int componentno);

  // what coordinate should be used for this texture
  std::string GetTextureCoordinateName(const char* tname);

  /**
   * helper function to get the appropriate coincident params
   */
  void GetCoincidentParameters(svtkRenderer* ren, svtkActor* actor, float& factor, float& offset);

  /**
   * Called in GetBounds(). When this method is called, the consider the input
   * to be updated depending on whether this->Static is set or not. This method
   * simply obtains the bounds from the data-object and returns it.
   */
  void ComputeBounds() override;

  /**
   * Make sure appropriate shaders are defined, compiled and bound.  This method
   * orchistrates the process, much of the work is done in other methods
   */
  virtual void UpdateShaders(svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* act);

  /**
   * Does the shader source need to be recomputed
   */
  virtual bool GetNeedToRebuildShaders(svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* act);

  /**
   * Build the shader source code, called by UpdateShader
   */
  virtual void BuildShaders(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act);

  /**
   * Create the basic shaders before replacement
   */
  virtual void GetShaderTemplate(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act);

  /**
   * Perform string replacements on the shader templates
   */
  virtual void ReplaceShaderValues(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act);

  //@{
  /**
   * Perform string replacements on the shader templates, called from
   * ReplaceShaderValues
   */
  virtual void ReplaceShaderRenderPass(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act, bool prePass);
  virtual void ReplaceShaderCustomUniforms(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkActor* act);
  virtual void ReplaceShaderColor(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act);
  virtual void ReplaceShaderLight(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act);
  virtual void ReplaceShaderTCoord(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act);
  virtual void ReplaceShaderPicking(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act);
  virtual void ReplaceShaderPrimID(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act);
  virtual void ReplaceShaderNormal(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act);
  virtual void ReplaceShaderClip(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act);
  virtual void ReplaceShaderPositionVC(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act);
  virtual void ReplaceShaderCoincidentOffset(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act);
  virtual void ReplaceShaderDepth(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act);
  //@}

  /**
   * Set the value of user-defined uniform variables, called by UpdateShader
   */
  virtual void SetCustomUniforms(svtkOpenGLHelper& cellBO, svtkActor* actor);

  /**
   * Set the shader parameters related to the mapper/input data, called by UpdateShader
   */
  virtual void SetMapperShaderParameters(svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* act);

  /**
   * Set the shader parameteres related to lighting, called by UpdateShader
   */
  virtual void SetLightingShaderParameters(
    svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* act);

  /**
   * Set the shader parameteres related to the Camera, called by UpdateShader
   */
  virtual void SetCameraShaderParameters(svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* act);

  /**
   * Set the shader parameteres related to the property, called by UpdateShader
   */
  virtual void SetPropertyShaderParameters(
    svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* act);

  /**
   * Update the VBO/IBO to be current
   */
  virtual void UpdateBufferObjects(svtkRenderer* ren, svtkActor* act);

  /**
   * Does the VBO/IBO need to be rebuilt
   */
  virtual bool GetNeedToRebuildBufferObjects(svtkRenderer* ren, svtkActor* act);

  /**
   * Build the VBO/IBO, called by UpdateBufferObjects
   */
  virtual void BuildBufferObjects(svtkRenderer* ren, svtkActor* act);

  /**
   * Build the IBO, called by BuildBufferObjects
   */
  virtual void BuildIBO(svtkRenderer* ren, svtkActor* act, svtkPolyData* poly);

  // The VBO and its layout.
  svtkOpenGLVertexBufferObjectGroup* VBOs;

  // Structures for the various cell types we render.
  svtkOpenGLHelper Primitives[PrimitiveEnd];
  svtkOpenGLHelper* LastBoundBO;
  bool DrawingEdgesOrVertices;

  // do we have wide lines that require special handling
  virtual bool HaveWideLines(svtkRenderer*, svtkActor*);

  // do we have textures that require special handling
  virtual bool HaveTextures(svtkActor* actor);

  // how many textures do we have
  virtual unsigned int GetNumberOfTextures(svtkActor* actor);

  // populate a vector with the textures we have
  // the order is always
  //  ColorInternalTexture
  //  Actors texture
  //  Properties textures
  virtual std::vector<std::pair<svtkTexture*, std::string> > GetTextures(svtkActor* actor);

  // do we have textures coordinates that require special handling
  virtual bool HaveTCoords(svtkPolyData* poly);

  // values we use to determine if we need to rebuild shaders
  std::map<const svtkOpenGLHelper*, int> LastLightComplexity;
  std::map<const svtkOpenGLHelper*, int> LastLightCount;
  std::map<const svtkOpenGLHelper*, svtkTimeStamp> LightComplexityChanged;

  int LastSelectionState;
  svtkTimeStamp SelectionStateChanged;

  // Caches the svtkOpenGLRenderPass::RenderPasses() information.
  // Note: Do not dereference the pointers held by this object. There is no
  // guarantee that they are still valid!
  svtkNew<svtkInformation> LastRenderPassInfo;

  // Check the renderpasses in actor's property keys to see if they've changed
  // render stages:
  svtkMTimeType GetRenderPassStageMTime(svtkActor* actor);

  bool UsingScalarColoring;
  svtkTimeStamp VBOBuildTime;     // When was the OpenGL VBO updated?
  svtkStateStorage VBOBuildState; // used for determining when to rebuild the VBO
  svtkStateStorage IBOBuildState; // used for determining whento rebuild the IBOs
  svtkStateStorage CellTextureBuildState;
  svtkStateStorage TempState; // can be used to avoid constant allocs/deallocs
  svtkOpenGLTexture* InternalColorTexture;

  int PopulateSelectionSettings;
  int PrimitiveIDOffset;

  svtkMatrix4x4* TempMatrix4;
  svtkMatrix3x3* TempMatrix3;
  svtkNew<svtkTransform> VBOInverseTransform;
  svtkNew<svtkMatrix4x4> VBOShiftScale;
  int ShiftScaleMethod; // for points

  // if set to true, tcoords will be passed to the
  // VBO even if the mapper knows of no texture maps
  // normally tcoords are only added to the VBO if the
  // mapper has identified a texture map as well.
  bool ForceTextureCoordinates;

  virtual void BuildCellTextures(
    svtkRenderer* ren, svtkActor*, svtkCellArray* prims[4], int representation);

  void AppendCellTextures(svtkRenderer* ren, svtkActor*, svtkCellArray* prims[4], int representation,
    std::vector<unsigned char>& colors, std::vector<float>& normals, svtkPolyData* pd,
    svtkOpenGLCellToSVTKCellMap* ccmap);

  svtkTextureObject* CellScalarTexture;
  svtkOpenGLBufferObject* CellScalarBuffer;
  bool HaveCellScalars;
  svtkTextureObject* CellNormalTexture;
  svtkOpenGLBufferObject* CellNormalBuffer;
  bool HaveCellNormals;

  // additional picking indirection
  char* PointIdArrayName;
  char* CellIdArrayName;
  char* ProcessIdArrayName;
  char* CompositeIdArrayName;

  class ExtraAttributeValue
  {
  public:
    std::string DataArrayName;
    int FieldAssociation;
    int ComponentNumber;
    std::string TextureName;
  };
  std::map<std::string, ExtraAttributeValue> ExtraAttributes;

  // Store shader properties on this class by legacy shader replacement functions
  // This should disappear when the functions are deprecated
#ifndef SVTK_LEGACY_REMOVE
  svtkOpenGLShaderProperty* GetLegacyShaderProperty();
  svtkSmartPointer<svtkOpenGLShaderProperty> LegacyShaderProperty;
#endif

  svtkOpenGLRenderTimer* TimerQuery;

  // are we currently drawing spheres/tubes
  bool DrawingSpheres(svtkOpenGLHelper& cellBO, svtkActor* actor);
  bool DrawingTubes(svtkOpenGLHelper& cellBO, svtkActor* actor);
  bool DrawingTubesOrSpheres(svtkOpenGLHelper& cellBO, svtkActor* actor);

  // get which opengl mode to use to draw the primitive
  int GetOpenGLMode(int representation, int primType);

  // get how big to make the points when doing point picking
  // typically 2 for points, 4 for lines, 6 for surface
  int GetPointPickingPrimitiveSize(int primType);

  // used to occasionally invoke timers
  unsigned int TimerQueryCounter;

  // stores the mapping from svtk cells to gl_PrimitiveId
  svtkNew<svtkOpenGLCellToSVTKCellMap> CellCellMap;

  // compute and set the maximum point and cell ID used in selection
  virtual void UpdateMaximumPointCellIds(svtkRenderer* ren, svtkActor* actor);

private:
  svtkOpenGLPolyDataMapper(const svtkOpenGLPolyDataMapper&) = delete;
  void operator=(const svtkOpenGLPolyDataMapper&) = delete;
};

#endif
