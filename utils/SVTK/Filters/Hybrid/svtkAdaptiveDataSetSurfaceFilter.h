/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAdaptiveDataSetSurfaceFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAdaptiveDataSetSurfaceFilter
 * @brief   Adaptively extract dataset surface
 *
 * svtkAdaptiveDataSetSurfaceFilter uses view and dataset properties to
 * create the outside surface mesh with the minimum minimorum of facets
 * @warning
 * Only implemented currently for 2-dimensional svtkHyperTreeGrid objects
 * @sa
 * svtkHyperTreeGrid svtkDataSetSurfaceFilter
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien, 2014
 * This class was rewritten by Philippe Pebay, 2016
 * This class was modified by Rogeli Grima, 2016
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef svtkAdaptiveDataSetSurfaceFilter_h
#define svtkAdaptiveDataSetSurfaceFilter_h

#include "svtkFiltersHybridModule.h" // For export macro
#include "svtkGeometryFilter.h"

class svtkBitArray;
class svtkCamera;
class svtkHyperTreeGrid;
class svtkRenderer;

class svtkHyperTreeGridNonOrientedGeometryCursor;
class svtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight;

class SVTKFILTERSHYBRID_EXPORT svtkAdaptiveDataSetSurfaceFilter : public svtkGeometryFilter
{
public:
  static svtkAdaptiveDataSetSurfaceFilter* New();
  svtkTypeMacro(svtkAdaptiveDataSetSurfaceFilter, svtkGeometryFilter);
  void PrintSelf(ostream&, svtkIndent) override;

  //@{
  /**
   * Set/Get the renderer attached to this adaptive surface extractor
   */
  void SetRenderer(svtkRenderer* ren);
  svtkGetObjectMacro(Renderer, svtkRenderer);
  //@}

  /**
   * Get the mtime of this object.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Set/Get for active the circle selection viewport (defaut true)
   */
  svtkSetMacro(CircleSelection, bool);
  svtkGetMacro(CircleSelection, bool);
  //@}

  //@{
  /**
   * Set/Get for active the bounding box selection viewport (defaut false)
   * JB C'est un facteur supplementaire d'acceleration possible
   * JB uniquement si l'on ne peut faire de rotation dans la vue.
   */
  svtkSetMacro(BBSelection, bool);
  svtkGetMacro(BBSelection, bool);
  //@}

  //@{
  /**
   * JB Activation de la dependance au point de vue. Par defaut a True.
   */
  svtkSetMacro(ViewPointDepend, bool);
  svtkGetMacro(ViewPointDepend, bool);
  //@}

  //@{
  /**
   * Set/Get for forced a fixed the level max (lost dynamicity) (defaut -1)
   */
  svtkSetMacro(FixedLevelMax, int);
  svtkGetMacro(FixedLevelMax, int);
  //@}

  //@{
  /**
   * JB Set/Get the scale factor influence le calcul de l'adaptive view.
   * JB Pour un raffinement de 2, donner Scale=2*X revient a faire un
   * JB appel a DynamicDecimateLevelMax avec la valeur X. (defaut 1)
   */
  svtkSetMacro(Scale, double);
  svtkGetMacro(Scale, double);
  //@}

  //@{
  /**
   * JB Set/Get reduit de autant le niveau max de profondeur, calcule
   * JB dynamiquement a parcourir dans la
   * JB representation HTG. (defaut 0)
   */
  svtkSetMacro(DynamicDecimateLevelMax, int);
  svtkGetMacro(DynamicDecimateLevelMax, int);
  //@}

protected:
  svtkAdaptiveDataSetSurfaceFilter();
  ~svtkAdaptiveDataSetSurfaceFilter() override;

  int RequestData(svtkInformation* svtkNotUsed(request), svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int DataSetExecute(svtkDataObject* input, svtkPolyData* output) /*override*/;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  /**
   * Main routine to generate external boundary
   */
  void ProcessTrees(svtkHyperTreeGrid* input, svtkPolyData* output);

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTreeNot3D(svtkHyperTreeGridNonOrientedGeometryCursor*, int);
  void RecursivelyProcessTree3D(svtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight*, int);

  /**
   * Process 1D leaves and issue corresponding edges (lines)
   */
  void ProcessLeaf1D(svtkHyperTreeGridNonOrientedGeometryCursor*);

  /**
   * Process 2D leaves and issue corresponding faces (quads)
   */
  void ProcessLeaf2D(svtkHyperTreeGridNonOrientedGeometryCursor*);

  /**
   * Process 3D leaves and issue corresponding cells (voxels)
   */
  void ProcessLeaf3D(svtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight*);

  /**
   * Helper method to generate a face based on its normal and offset from cursor origin
   */
  void AddFace(svtkIdType, const double*, const double*, int, unsigned int);

  svtkDataSetAttributes* InData;
  svtkDataSetAttributes* OutData;

  /**
   * Dimension of input grid
   */
  unsigned int Dimension;

  /**
   * Orientation of input grid when dimension < 3
   */
  unsigned int Orientation;

  /**
   * Visibility Mask
   */
  svtkBitArray* Mask;

  /**
   * Storage for points of output unstructured mesh
   */
  svtkPoints* Points;

  /**
   * Storage for cells of output unstructured mesh
   */
  svtkCellArray* Cells;

  /**
   * Pointer to the renderer in use
   */
  svtkRenderer* Renderer;

  /**
   * First axis parameter for adaptive view
   */
  unsigned int Axis1;

  /**
   * Second axis parameter for adaptive view
   */
  unsigned int Axis2;

  /**
   * Maximum depth parameter for adaptive view
   */
  int LevelMax;

  /**
   * Parallel projection parameter for adaptive view
   */
  bool ParallelProjection;

  /**
   * Last renderer size parameters for adaptive view
   */
  int LastRendererSize[2];

  /**
   * JB Activation de la dependance au point de vue
   */
  bool ViewPointDepend;

  /**
   * Last camera focal point coordinates for adaptive view
   */
  double LastCameraFocalPoint[3];

  /**
   * Last camera parallel scale for adaptive view
   */
  double LastCameraParallelScale;

  /**
   * Bounds windows in the real coordinates
   */
  double WindowBounds[4];

  /**
   * Product cell when in circle selection
   */
  bool CircleSelection;

  /**
   * Radius parameter for adaptive view
   */
  double Radius;

  /**
   * Product cell when in nounding box selection
   */
  bool BBSelection;

#ifndef NDEBUG
  /**
   * Effect of options selection
   */
  long int NbRejectByCircle;
  long int NbRejectByBB;
#endif

  /**
   * JB Forced, fixed the level depth, ignored automatic determination
   */
  int FixedLevelMax;

  /**
   * Scale factor for adaptive view
   */
  double Scale;

  /**
   * JB Decimate level max after automatic determination
   */
  int DynamicDecimateLevelMax;

private:
  svtkAdaptiveDataSetSurfaceFilter(const svtkAdaptiveDataSetSurfaceFilter&) = delete;
  void operator=(const svtkAdaptiveDataSetSurfaceFilter&) = delete;
};

#endif // svtkAdaptiveDataSetSurfaceFilter_h
