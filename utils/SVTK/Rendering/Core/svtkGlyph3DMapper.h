/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGlyph3DMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGlyph3DMapper
 * @brief   svtkGlyph3D on the GPU.
 *
 * Do the same job than svtkGlyph3D but on the GPU. For this reason, it is
 * a mapper not a svtkPolyDataAlgorithm. Also, some methods of svtkGlyph3D
 * don't make sense in svtkGlyph3DMapper: GeneratePointIds, old-style
 * SetSource, PointIdsName, IsPointVisible.
 *
 * @sa
 * svtkGlyph3D
 */

#ifndef svtkGlyph3DMapper_h
#define svtkGlyph3DMapper_h

#include "svtkGlyph3D.h" // for the constants (SVTK_SCALE_BY_SCALAR, ...).
#include "svtkMapper.h"
#include "svtkRenderingCoreModule.h" // For export macro
#include "svtkWeakPointer.h"         // needed for svtkWeakPointer.

class svtkCompositeDataDisplayAttributes;
class svtkDataObjectTree;

class SVTKRENDERINGCORE_EXPORT svtkGlyph3DMapper : public svtkMapper
{
public:
  static svtkGlyph3DMapper* New();
  svtkTypeMacro(svtkGlyph3DMapper, svtkMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum ArrayIndexes
  {
    SCALE = 0,
    SOURCE_INDEX = 1,
    MASK = 2,
    ORIENTATION = 3,
    SELECTIONID = 4
  };

  /**
   * Specify a source object at a specified table location. New style.
   * Source connection is stored in port 1. This method is equivalent
   * to SetInputConnection(1, id, outputPort).
   */
  void SetSourceConnection(int idx, svtkAlgorithmOutput* algOutput);
  void SetSourceConnection(svtkAlgorithmOutput* algOutput)
  {
    this->SetSourceConnection(0, algOutput);
  }

  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void SetInputData(svtkDataObject*);

  /**
   * Specify a source object at a specified table location.
   */
  void SetSourceData(int idx, svtkPolyData* pd);

  /**
   * Specify a data object tree that will be used for the source table. Requires
   * UseSourceTableTree to be true. The top-level nodes of the tree are mapped
   * to the source data inputs.
   *
   * Must only contain svtkPolyData instances on the OpenGL backend. May contain
   * svtkCompositeDataSets with svtkPolyData leaves on OpenGL2.
   */
  void SetSourceTableTree(svtkDataObjectTree* tree);

  /**
   * Set the source to use for he glyph.
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetSourceConnection for connecting the pipeline.
   */
  void SetSourceData(svtkPolyData* pd);

  /**
   * Get a pointer to a source object at a specified table location.
   */
  svtkPolyData* GetSource(int idx = 0);

  /**
   * Convenience method to get the source table tree, if it exists.
   */
  svtkDataObjectTree* GetSourceTableTree();

  //@{
  /**
   * Turn on/off scaling of source geometry. When turned on, ScaleFactor
   * controls the scale applied. To scale with some data array, ScaleMode should
   * be set accordingly.
   */
  svtkSetMacro(Scaling, bool);
  svtkBooleanMacro(Scaling, bool);
  svtkGetMacro(Scaling, bool);
  //@}

  //@{
  /**
   * Either scale by individual components (SCALE_BY_COMPONENTS) or magnitude
   * (SCALE_BY_MAGNITUDE) of the chosen array to SCALE with or disable scaling
   * using data array all together (NO_DATA_SCALING). Default is
   * NO_DATA_SCALING.
   */
  svtkSetMacro(ScaleMode, int);
  svtkGetMacro(ScaleMode, int);
  //@}

  //@{
  /**
   * Specify scale factor to scale object by. This is used only when Scaling is
   * On.
   */
  svtkSetMacro(ScaleFactor, double);
  svtkGetMacro(ScaleFactor, double);
  //@}

  enum ScaleModes
  {
    NO_DATA_SCALING = 0,
    SCALE_BY_MAGNITUDE = 1,
    SCALE_BY_COMPONENTS = 2
  };

  void SetScaleModeToScaleByMagnitude() { this->SetScaleMode(SCALE_BY_MAGNITUDE); }
  void SetScaleModeToScaleByVectorComponents() { this->SetScaleMode(SCALE_BY_COMPONENTS); }
  void SetScaleModeToNoDataScaling() { this->SetScaleMode(NO_DATA_SCALING); }
  const char* GetScaleModeAsString();

  //@{
  /**
   * Specify range to map scalar values into.
   */
  svtkSetVector2Macro(Range, double);
  svtkGetVectorMacro(Range, double, 2);
  //@}

  //@{
  /**
   * Turn on/off orienting of input geometry.
   * When turned on, the orientation array specified
   * using SetOrientationArray() will be used.
   */
  svtkSetMacro(Orient, bool);
  svtkGetMacro(Orient, bool);
  svtkBooleanMacro(Orient, bool);
  //@}

  //@{
  /**
   * Orientation mode indicates if the OrientationArray provides the direction
   * vector for the orientation or the rotations around each axes. Default is
   * DIRECTION
   */
  svtkSetClampMacro(OrientationMode, int, DIRECTION, QUATERNION);
  svtkGetMacro(OrientationMode, int);
  void SetOrientationModeToDirection() { this->SetOrientationMode(svtkGlyph3DMapper::DIRECTION); }
  void SetOrientationModeToRotation() { this->SetOrientationMode(svtkGlyph3DMapper::ROTATION); }
  void SetOrientationModeToQuaternion() { this->SetOrientationMode(svtkGlyph3DMapper::QUATERNION); }
  const char* GetOrientationModeAsString();
  //@}

  enum OrientationModes
  {
    DIRECTION = 0,
    ROTATION = 1,
    QUATERNION = 2
  };

  //@{
  /**
   * Turn on/off clamping of data values to scale with to the specified range.
   */
  svtkSetMacro(Clamping, bool);
  svtkGetMacro(Clamping, bool);
  svtkBooleanMacro(Clamping, bool);
  //@}

  //@{
  /**
   * Enable/disable indexing into table of the glyph sources. When disabled,
   * only the 1st source input will be used to generate the glyph. Otherwise the
   * source index array will be used to select the glyph source. The source
   * index array can be specified using SetSourceIndexArray().
   */
  svtkSetMacro(SourceIndexing, bool);
  svtkGetMacro(SourceIndexing, bool);
  svtkBooleanMacro(SourceIndexing, bool);
  //@}

  //@{
  /**
   * If true, and the glyph source dataset is a subclass of svtkDataObjectTree,
   * the top-level members of the tree will be mapped to the glyph source table
   * used for SourceIndexing.
   */
  svtkSetMacro(UseSourceTableTree, bool);
  svtkGetMacro(UseSourceTableTree, bool);
  svtkBooleanMacro(UseSourceTableTree, bool);

  //@{
  /**
   * Turn on/off custom selection ids. If enabled, the id values set with
   * SetSelectionIdArray are returned from pick events.
   */
  svtkSetMacro(UseSelectionIds, bool);
  svtkBooleanMacro(UseSelectionIds, bool);
  svtkGetMacro(UseSelectionIds, bool);
  //@}

  /**
   * Redefined to take into account the bounds of the scaled glyphs.
   */
  double* GetBounds() override;

  /**
   * Same as superclass. Appear again to stop warnings about hidden method.
   */
  void GetBounds(double bounds[6]) override;

  /**
   * All the work is done is derived classes.
   */
  void Render(svtkRenderer* ren, svtkActor* act) override;

  //@{
  /**
   * Tells the mapper to skip glyphing input points that haves false values
   * in the mask array. If there is no mask array (id access mode is set
   * and there is no such id, or array name access mode is set and
   * the there is no such name), masking is silently ignored.
   * A mask array is a svtkBitArray with only one component.
   * Initial value is false.
   */
  svtkSetMacro(Masking, bool);
  svtkGetMacro(Masking, bool);
  svtkBooleanMacro(Masking, bool);
  //@}

  /**
   * Set the name of the point array to use as a mask for generating the glyphs.
   * This is a convenience method. The same effect can be achieved by using
   * SetInputArrayToProcess(svtkGlyph3DMapper::MASK, 0, 0,
   * svtkDataObject::FIELD_ASSOCIATION_POINTS, maskarrayname)
   */
  void SetMaskArray(const char* maskarrayname);

  /**
   * Set the point attribute to use as a mask for generating the glyphs.
   * \c fieldAttributeType is one of the following:
   * \li svtkDataSetAttributes::SCALARS
   * \li svtkDataSetAttributes::VECTORS
   * \li svtkDataSetAttributes::NORMALS
   * \li svtkDataSetAttributes::TCOORDS
   * \li svtkDataSetAttributes::TENSORS
   * This is a convenience method. The same effect can be achieved by using
   * SetInputArrayToProcess(svtkGlyph3DMapper::MASK, 0, 0,
   * svtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType)
   */
  void SetMaskArray(int fieldAttributeType);

  /**
   * Tells the mapper to use an orientation array if Orient is true.
   * An orientation array is a svtkDataArray with 3 components. The first
   * component is the angle of rotation along the X axis. The second
   * component is the angle of rotation along the Y axis. The third
   * component is the angle of rotation along the Z axis. Orientation is
   * specified in X,Y,Z order but the rotations are performed in Z,X an Y.
   * This definition is compliant with SetOrientation method on svtkProp3D.
   * By using vector or normal there is a degree of freedom or rotation
   * left (underconstrained). With the orientation array, there is no degree of
   * freedom left.
   * This is convenience method. The same effect can be achieved by using
   * SetInputArrayToProcess(svtkGlyph3DMapper::ORIENTATION, 0, 0,
   * svtkDataObject::FIELD_ASSOCIATION_POINTS, orientationarrayname);
   */
  void SetOrientationArray(const char* orientationarrayname);

  /**
   * Tells the mapper to use an orientation array if Orient is true.
   * An orientation array is a svtkDataArray with 3 components. The first
   * component is the angle of rotation along the X axis. The second
   * component is the angle of rotation along the Y axis. The third
   * component is the angle of rotation along the Z axis. Orientation is
   * specified in X,Y,Z order but the rotations are performed in Z,X an Y.
   * This definition is compliant with SetOrientation method on svtkProp3D.
   * By using vector or normal there is a degree of freedom or rotation
   * left (underconstrained). With the orientation array, there is no degree of
   * freedom left.
   * \c fieldAttributeType is one of the following:
   * \li svtkDataSetAttributes::SCALARS
   * \li svtkDataSetAttributes::VECTORS
   * \li svtkDataSetAttributes::NORMALS
   * \li svtkDataSetAttributes::TCOORDS
   * \li svtkDataSetAttributes::TENSORS
   * This is convenience method. The same effect can be achieved by using
   * SetInputArrayToProcess(svtkGlyph3DMapper::ORIENTATION, 0, 0,
   * svtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType);
   */
  void SetOrientationArray(int fieldAttributeType);

  /**
   * Convenience method to set the array to scale with. This is same as calling
   * SetInputArrayToProcess(svtkGlyph3DMapper::SCALE, 0, 0,
   * svtkDataObject::FIELD_ASSOCIATION_POINTS, scalarsarrayname).
   */
  void SetScaleArray(const char* scalarsarrayname);

  /**
   * Convenience method to set the array to scale with. This is same as calling
   * SetInputArrayToProcess(svtkGlyph3DMapper::SCALE, 0, 0,
   * svtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType).
   */
  void SetScaleArray(int fieldAttributeType);

  /**
   * Convenience method to set the array to use as index within the sources.
   * This is same as calling
   * SetInputArrayToProcess(svtkGlyph3DMapper::SOURCE_INDEX, 0, 0,
   * svtkDataObject::FIELD_ASSOCIATION_POINTS, arrayname).
   */
  void SetSourceIndexArray(const char* arrayname);

  /**
   * Convenience method to set the array to use as index within the sources.
   * This is same as calling
   * SetInputArrayToProcess(svtkGlyph3DMapper::SOURCE_INDEX, 0, 0,
   * svtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType).
   */
  void SetSourceIndexArray(int fieldAttributeType);

  /**
   * Convenience method to set the array used for selection IDs. This is same
   * as calling
   * SetInputArrayToProcess(svtkGlyph3DMapper::SELECTIONID, 0, 0,
   * svtkDataObject::FIELD_ASSOCIATION_POINTS, selectionidarrayname).

   * If no selection id array is specified, the index of the glyph point is
   * used.
   */
  void SetSelectionIdArray(const char* selectionIdArrayName);

  /**
   * Convenience method to set the array used for selection IDs. This is same
   * as calling
   * SetInputArrayToProcess(svtkGlyph3DMapper::SELECTIONID, 0, 0,
   * svtkDataObject::FIELD_ASSOCIATION_POINTS, fieldAttributeType).

   * If no selection id array is specified, the index of the glyph point is
   * used.
   */
  void SetSelectionIdArray(int fieldAttributeType);

  //@{
  /**
   * For selection by color id mode (not for end-user, called by
   * svtkGlyphSelectionRenderMode). 0 is reserved for miss. it has to
   * start at 1. Initial value is 1.
   */
  svtkSetMacro(SelectionColorId, unsigned int);
  svtkGetMacro(SelectionColorId, unsigned int);
  //@}

  //@{
  /**
   * When the input data object (not the source) is composite data,
   * it is possible to control visibility and pickability on a per-block
   * basis by passing the mapper a svtkCompositeDataDisplayAttributes instance.
   * The color and opacity in the display-attributes instance are ignored
   * for now. By default, the mapper does not own a display-attributes
   * instance. The value of BlockAttributes has no effect when the input
   * is a poly-data object.
   */
  virtual void SetBlockAttributes(svtkCompositeDataDisplayAttributes* attr);
  svtkGetObjectMacro(BlockAttributes, svtkCompositeDataDisplayAttributes);
  //@}

  //@{
  /**
   * Enable or disable frustum culling and LOD of the instances.
   * When enabled, an OpenGL driver supporting GL_ARB_gpu_shader5 extension is mandatory.
   */
  svtkSetMacro(CullingAndLOD, bool);
  svtkGetMacro(CullingAndLOD, bool);

  /**
   * Get the maximum number of LOD. OpenGL context must be bound.
   * The maximum number of LOD depends on GPU capabilities.
   * This method is intended to be reimplemented in inherited classes, current implementation
   * always returns zero.
   */
  virtual svtkIdType GetMaxNumberOfLOD();

  /**
   * Set the number of LOD.
   * This method is intended to be reimplemented in inherited classes, current implementation
   * does nothing.
   */
  virtual void SetNumberOfLOD(svtkIdType svtkNotUsed(nb)) {}

  /**
   * Configure LODs. Culling must be enabled.
   * distance have to be a positive value, it is the distance to the camera scaled by
   * the instanced geometry bounding box.
   * targetReduction have to be between 0 and 1, 0 disable decimation, 1 draw a point.
   * This method is intended to be reimplemented in inherited classes, current implementation
   * does nothing.
   *
   * @sa svtkDecimatePro::SetTargetReduction
   */
  virtual void SetLODDistanceAndTargetReduction(
    svtkIdType svtkNotUsed(index), float svtkNotUsed(distance), float svtkNotUsed(targetReduction))
  {
  }

  /**
   * Enable LOD coloring. It can be useful to configure properly the LODs.
   * Each LOD have a unique color, based on its index.
   */
  svtkSetMacro(LODColoring, bool);
  svtkGetMacro(LODColoring, bool);
  //@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Used by svtkHardwareSelector to determine if the prop supports hardware
   * selection.
   */
  bool GetSupportsSelection() override { return true; }

protected:
  svtkGlyph3DMapper();
  ~svtkGlyph3DMapper() override;

  virtual int RequestUpdateExtent(
    svtkInformation* request, svtkInformationVector** inInfo, svtkInformationVector* outInfo);

  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkPolyData* GetSource(int idx, svtkInformationVector* sourceInfo);
  svtkPolyData* GetSourceTable(int idx, svtkInformationVector* sourceInfo);

  //@{
  /**
   * Convenience methods to get each of the arrays.
   */
  svtkDataArray* GetMaskArray(svtkDataSet* input);
  svtkDataArray* GetSourceIndexArray(svtkDataSet* input);
  svtkDataArray* GetOrientationArray(svtkDataSet* input);
  svtkDataArray* GetScaleArray(svtkDataSet* input);
  svtkDataArray* GetSelectionIdArray(svtkDataSet* input);
  svtkUnsignedCharArray* GetColors(svtkDataSet* input);
  //@}

  svtkCompositeDataDisplayAttributes* BlockAttributes;
  bool Scaling;       // Determine whether scaling of geometry is performed
  double ScaleFactor; // Scale factor to use to scale geometry
  int ScaleMode;      // Scale by scalar value or vector magnitude

  double Range[2];      // Range to use to perform scalar scaling
  bool Orient;          // boolean controls whether to "orient" data
  bool Clamping;        // whether to clamp scale factor
  bool SourceIndexing;  // Enable/disable indexing into the glyph table
  bool UseSelectionIds; // Enable/disable custom pick ids
  bool Masking;         // Enable/disable masking.
  int OrientationMode;

  bool UseSourceTableTree; // Map DataObjectTree glyph source into table

  unsigned int SelectionColorId;

  bool CullingAndLOD = false; // Disable culling
  std::vector<std::pair<float, float> > LODs;
  bool LODColoring = false;

private:
  svtkGlyph3DMapper(const svtkGlyph3DMapper&) = delete;
  void operator=(const svtkGlyph3DMapper&) = delete;

  /**
   * Returns true when valid bounds are returned.
   */
  bool GetBoundsInternal(svtkDataSet* ds, double ds_bounds[6]);
};

#endif
