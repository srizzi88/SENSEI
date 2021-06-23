/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositePolyDataMapper2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCompositePolyDataMapper2
 * @brief   mapper for composite dataset consisting
 * of polygonal data.
 *
 * svtkCompositePolyDataMapper2 is similar to svtkCompositePolyDataMapper except
 * that instead of creating individual mapper for each block in the composite
 * dataset, it iterates over the blocks internally.
 */

#ifndef svtkCompositePolyDataMapper2_h
#define svtkCompositePolyDataMapper2_h

#include "svtkOpenGLPolyDataMapper.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro
#include "svtkSmartPointer.h"           // for svtkSmartPointer

#include "svtkColor.h" // used for ivars
#include <map>        // use for ivars
#include <stack>      // used for ivars
#include <vector>     // used for ivars

class svtkCompositeDataDisplayAttributes;
class svtkCompositeMapperHelper2;
class svtkCompositeMapperHelperData;

class SVTKRENDERINGOPENGL2_EXPORT svtkCompositePolyDataMapper2 : public svtkOpenGLPolyDataMapper
{
public:
  static svtkCompositePolyDataMapper2* New();
  svtkTypeMacro(svtkCompositePolyDataMapper2, svtkOpenGLPolyDataMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Some introspection on the type of data the mapper will render
   * used by props to determine if they should invoke the mapper
   * on a specific rendering pass.
   */
  bool HasOpaqueGeometry() override;
  bool HasTranslucentPolygonalGeometry() override;
  //@}

  //@{
  /**
   * Set/get the composite data set attributes.
   */
  void SetCompositeDataDisplayAttributes(svtkCompositeDataDisplayAttributes* attributes);
  svtkCompositeDataDisplayAttributes* GetCompositeDataDisplayAttributes();
  //@}

  //@{
  /**
   * Set/get the visibility for a block given its flat index.
   */
  void SetBlockVisibility(unsigned int index, bool visible);
  bool GetBlockVisibility(unsigned int index);
  void RemoveBlockVisibility(unsigned int index);
  void RemoveBlockVisibilities();
  // This method is deprecated and will be removed in SVTK 8.2. It is misspelled.
  SVTK_LEGACY(void RemoveBlockVisibilites());
  //@}

  //@{
  /**
   * Set/get the color for a block given its flat index.
   */
  void SetBlockColor(unsigned int index, double color[3]);
  void SetBlockColor(unsigned int index, double r, double g, double b)
  {
    double color[3] = { r, g, b };
    this->SetBlockColor(index, color);
  }
  double* GetBlockColor(unsigned int index);
  void RemoveBlockColor(unsigned int index);
  void RemoveBlockColors();
  //@}

  //@{
  /**
   * Set/get the opacity for a block given its flat index.
   */
  void SetBlockOpacity(unsigned int index, double opacity);
  double GetBlockOpacity(unsigned int index);
  void RemoveBlockOpacity(unsigned int index);
  void RemoveBlockOpacities();
  //@}

  /**
   * If the current 'color by' array is missing on some datasets, color these
   * dataset by the LookupTable's NaN color, if the lookup table supports it.
   * Default is false.
   * @{
   */
  svtkSetMacro(ColorMissingArraysWithNanColor, bool);
  svtkGetMacro(ColorMissingArraysWithNanColor, bool);
  svtkBooleanMacro(ColorMissingArraysWithNanColor, bool);
  /**@}*/

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  /**
   * This calls RenderPiece (in a for loop if streaming is necessary).
   */
  void Render(svtkRenderer* ren, svtkActor* act) override;

  //@{
  /**
   * Call SetInputArrayToProcess on helpers.
   */
  using svtkAlgorithm::SetInputArrayToProcess;
  void SetInputArrayToProcess(
    int idx, int port, int connection, int fieldAssociation, const char* name) override;
  void SetInputArrayToProcess(
    int idx, int port, int connection, int fieldAssociation, int fieldAttributeType) override;
  void SetInputArrayToProcess(int idx, svtkInformation* info) override;
  //@}

  /**
   * Accessor to the ordered list of PolyData that we end last drew.
   */
  std::vector<svtkPolyData*> GetRenderedList() { return this->RenderedList; }

  /**
   * allows a mapper to update a selections color buffers
   * Called from a prop which in turn is called from the selector
   */
  void ProcessSelectorPixelBuffers(
    svtkHardwareSelector* sel, std::vector<unsigned int>& pixeloffsets, svtkProp* prop) override;

protected:
  svtkCompositePolyDataMapper2();
  ~svtkCompositePolyDataMapper2() override;

  /**
   * We need to override this method because the standard streaming
   * demand driven pipeline is not what we want - we are expecting
   * hierarchical data as input
   */
  svtkExecutive* CreateDefaultExecutive() override;

  /**
   * Need to define the type of data handled by this mapper.
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

  /**
   * Need to loop over the hierarchy to compute bounds
   */
  void ComputeBounds() override;

  /**
   * This method is called before RenderPiece is called on helpers.
   * One can override it to initialize the helpers.
   */
  virtual void InitializeHelpersBeforeRendering(
    svtkRenderer* svtkNotUsed(ren), svtkActor* svtkNotUsed(act))
  {
  }

  /**
   * Time stamp for computation of bounds.
   */
  svtkTimeStamp BoundsMTime;

  // what "index" are we currently rendering, -1 means none
  int CurrentFlatIndex;
  std::map<const std::string, svtkCompositeMapperHelper2*> Helpers;
  std::map<svtkPolyData*, svtkCompositeMapperHelperData*> HelperDataMap;
  svtkTimeStamp HelperMTime;

  virtual svtkCompositeMapperHelper2* CreateHelper();

  // copy values to the helpers
  virtual void CopyMapperValuesToHelper(svtkCompositeMapperHelper2* helper);

  class RenderBlockState
  {
  public:
    std::stack<bool> Visibility;
    std::stack<bool> Pickability;
    std::stack<double> Opacity;
    std::stack<svtkColor3d> AmbientColor;
    std::stack<svtkColor3d> DiffuseColor;
    std::stack<svtkColor3d> SpecularColor;
  };

  bool RecursiveHasTranslucentGeometry(svtkDataObject* dobj, unsigned int& flat_index);
  svtkStateStorage TranslucentState;
  bool HasTranslucentGeometry;

  void BuildRenderValues(
    svtkRenderer* renderer, svtkActor* actor, svtkDataObject* dobj, unsigned int& flat_index);
  svtkStateStorage RenderValuesState;

  RenderBlockState BlockState;
  void RenderBlock(
    svtkRenderer* renderer, svtkActor* actor, svtkDataObject* dobj, unsigned int& flat_index);

  /**
   * Composite data set attributes.
   */
  svtkSmartPointer<svtkCompositeDataDisplayAttributes> CompositeAttributes;

  friend class svtkCompositeMapperHelper2;

  /**
   * If the current 'color by' array is missing on some datasets, color these
   * dataset by the LookupTable's NaN color, if the lookup table supports it.
   */
  bool ColorMissingArraysWithNanColor;

  std::vector<svtkPolyData*> RenderedList;

private:
  double ColorResult[3];

  svtkCompositePolyDataMapper2(const svtkCompositePolyDataMapper2&) = delete;
  void operator=(const svtkCompositePolyDataMapper2&) = delete;
};

#endif
