/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXdmf3DataSet.h
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXdmf3DataSet
 * @brief   dataset level translation between xdmf3 and svtk
 *
 * This class holds static methods that translate the five atomic data
 * types between svtk and xdmf3.
 *
 * This file is a helper for the svtkXdmf3Reader and svtkXdmf3Writer and
 * not intended to be part of SVTK public API
 */

#ifndef svtkXdmf3DataSet_h
#define svtkXdmf3DataSet_h

#include "svtkIOXdmf3Module.h" // For export macro

// clang-format off
#include "svtk_xdmf3.h"
#include SVTKXDMF3_HEADER(core/XdmfSharedPtr.hpp)
// clang-format on

#include <string> //Needed only for XdmfArray::getName :(

class svtkXdmf3ArraySelection;
class svtkXdmf3ArrayKeeper;
class XdmfArray;
class XdmfAttribute;
class svtkDataArray;
class XdmfGrid;
class svtkDataObject;
class XdmfSet;
class svtkDataSet;
class XdmfTopologyType;
class XdmfRegularGrid;
class svtkImageData;
class XdmfRectilinearGrid;
class svtkRectilinearGrid;
class XdmfCurvilinearGrid;
class svtkStructuredGrid;
class XdmfUnstructuredGrid;
class svtkUnstructuredGrid;
class svtkPointSet;
class XdmfGraph;
class svtkMutableDirectedGraph;
class svtkDirectedGraph;
class XdmfDomain;

class SVTKIOXDMF3_EXPORT svtkXdmf3DataSet
{
public:
  // Common

  /**
   * Returns a SVTK array corresponding to the Xdmf array it is given.
   */
  static svtkDataArray* XdmfToSVTKArray(XdmfArray* xArray,
    std::string attrName, // TODO: needed because XdmfArray::getName() misbehaves
    unsigned int preferredComponents = 0, svtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Populates and Xdmf array corresponding to the SVTK array it is given
   */
  static bool SVTKToXdmfArray(
    svtkDataArray* vArray, XdmfArray* xArray, unsigned int rank = 0, unsigned int* dims = nullptr);

  /**
   * Populates the given SVTK DataObject's attribute arrays with the selected
   * arrays from the Xdmf Grid
   */
  static void XdmfToSVTKAttributes(svtkXdmf3ArraySelection* fselection,
    svtkXdmf3ArraySelection* cselection, svtkXdmf3ArraySelection* pselection, XdmfGrid* grid,
    svtkDataObject* dObject, svtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Populates the given Xdmf Grid's attribute arrays with the selected
   * arrays from the SVTK DataObject
   */
  static void SVTKToXdmfAttributes(svtkDataObject* dObject, XdmfGrid* grid);

  //@{
  /**
   * Helpers for Unstructured Grid translation
   */
  static unsigned int GetNumberOfPointsPerCell(int svtk_cell_type, bool& fail);
  static int GetSVTKCellType(shared_ptr<const XdmfTopologyType> topologyType);
  static int GetXdmfCellType(int svtkType);
  //@}

  //@{
  /**
   * Helper used in SVTKToXdmf to set the time in a Xdmf grid
   */
  static void SetTime(XdmfGrid* grid, double hasTime, double time);
  static void SetTime(XdmfGraph* graph, double hasTime, double time);
  //@}

  // svtkXdmf3RegularGrid

  /**
   * Populates the SVTK data set with the contents of the Xdmf grid
   */
  static void XdmfToSVTK(svtkXdmf3ArraySelection* fselection, svtkXdmf3ArraySelection* cselection,
    svtkXdmf3ArraySelection* pselection, XdmfRegularGrid* grid, svtkImageData* dataSet,
    svtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Helper that does topology for XdmfToSVTK
   */
  static void CopyShape(
    XdmfRegularGrid* grid, svtkImageData* dataSet, svtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Populates the Xdmf Grid with the contents of the SVTK data set
   */
  static void SVTKToXdmf(
    svtkImageData* dataSet, XdmfDomain* domain, bool hasTime, double time, const char* name = 0);

  // svtkXdmf3RectilinearGrid
  /**
   * Populates the SVTK data set with the contents of the Xdmf grid
   */
  static void XdmfToSVTK(svtkXdmf3ArraySelection* fselection, svtkXdmf3ArraySelection* cselection,
    svtkXdmf3ArraySelection* pselection, XdmfRectilinearGrid* grid, svtkRectilinearGrid* dataSet,
    svtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Helper that does topology for XdmfToSVTK
   */
  static void CopyShape(
    XdmfRectilinearGrid* grid, svtkRectilinearGrid* dataSet, svtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Populates the Xdmf Grid with the contents of the SVTK data set
   */
  static void SVTKToXdmf(svtkRectilinearGrid* dataSet, XdmfDomain* domain, bool hasTime, double time,
    const char* name = 0);

  // svtkXdmf3CurvilinearGrid
  /**
   * Populates the SVTK data set with the contents of the Xdmf grid
   */
  static void XdmfToSVTK(svtkXdmf3ArraySelection* fselection, svtkXdmf3ArraySelection* cselection,
    svtkXdmf3ArraySelection* pselection, XdmfCurvilinearGrid* grid, svtkStructuredGrid* dataSet,
    svtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Helper that does topology for XdmfToSVTK
   */
  static void CopyShape(
    XdmfCurvilinearGrid* grid, svtkStructuredGrid* dataSet, svtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Populates the Xdmf Grid with the contents of the SVTK data set
   */
  static void SVTKToXdmf(svtkStructuredGrid* dataSet, XdmfDomain* domain, bool hasTime, double time,
    const char* name = 0);

  // svtkXdmf3UnstructuredGrid
  /**
   * Populates the SVTK data set with the contents of the Xdmf grid
   */
  static void XdmfToSVTK(svtkXdmf3ArraySelection* fselection, svtkXdmf3ArraySelection* cselection,
    svtkXdmf3ArraySelection* pselection, XdmfUnstructuredGrid* grid, svtkUnstructuredGrid* dataSet,
    svtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Helper that does topology for XdmfToSVTK
   */
  static void CopyShape(XdmfUnstructuredGrid* grid, svtkUnstructuredGrid* dataSet,
    svtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Populates the Xdmf Grid with the contents of the SVTK data set
   */
  static void SVTKToXdmf(
    svtkPointSet* dataSet, XdmfDomain* domain, bool hasTime, double time, const char* name = 0);

  // svtkXdmf3Graph
  /**
   * Populates the SVTK graph with the contents of the Xdmf grid
   */
  static void XdmfToSVTK(svtkXdmf3ArraySelection* fselection, svtkXdmf3ArraySelection* cselection,
    svtkXdmf3ArraySelection* pselection, XdmfGraph* grid, svtkMutableDirectedGraph* dataSet,
    svtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Populates the Xdmf Grid with the contents of the SVTK data set
   */
  static void SVTKToXdmf(
    svtkDirectedGraph* dataSet, XdmfDomain* domain, bool hasTime, double time, const char* name = 0);

  // Side Sets

  /**
   * Populates the given SVTK DataObject's attribute arrays with the selected
   * arrays from the Xdmf Grid
   */
  static void XdmfToSVTKAttributes(
    /*
        svtkXdmf3ArraySelection *fselection,
        svtkXdmf3ArraySelection *cselection,
        svtkXdmf3ArraySelection *pselection,
    */
    XdmfSet* grid, svtkDataObject* dObject, svtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Extracts numbered subset out of grid (grid corresponds to dataSet),
   * and fills in subSet with it.
   */
  static void XdmfSubsetToSVTK(XdmfGrid* grid, unsigned int setnum, svtkDataSet* dataSet,
    svtkUnstructuredGrid* subSet, svtkXdmf3ArrayKeeper* keeper = nullptr);

  /**
   * Converts XDMF topology type, finite element family and degree
   * into an equivalent (or approximative) representation via SVTK cell
   * type.
   */
  static int GetSVTKFiniteElementCellType(unsigned int element_degree,
    const std::string& element_family, shared_ptr<const XdmfTopologyType> topologyType);

  /**
   * Parses finite element function defined in Attribute.
   *
   * This method changes geometry stored in svtkDataObject
   * and adds Point/Cell data field.
   *
   * XdmfAttribute must contain 2 arrays - one is the XdmfAttribute itself and
   * remaining one the auxiliary array. Interpretation of the arrays is
   * described in XDMF wiki page http://www.xdmf.org/index.php/XDMF_Model_and_Format#Attribute
   *
   */
  static void ParseFiniteElementFunction(svtkDataObject* dObject,
    shared_ptr<XdmfAttribute> xmfAttribute, svtkDataArray* array, XdmfGrid* grid,
    svtkXdmf3ArrayKeeper* keeper = nullptr);
};

#endif
// SVTK-HeaderTest-Exclude: svtkXdmf3DataSet.h
