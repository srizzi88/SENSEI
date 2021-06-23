/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXdmfWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkXdmfWriter
 * @brief   write eXtensible Data Model and Format files
 *
 * svtkXdmfWriter converts svtkDataObjects to XDMF format. This is intended to
 * replace svtkXdmfWriter, which is not up to date with the capabilities of the
 * newer XDMF2 library. This writer understands SVTK's composite data types and
 * produces full trees in the output XDMF files.
 */

#ifndef svtkXdmfWriter_h
#define svtkXdmfWriter_h

#include "svtkIOXdmf2Module.h" // For export macro

#include "svtkDataObjectAlgorithm.h"

#include <string> // Needed for private members
#include <vector> //

class svtkExecutive;

class svtkCompositeDataSet;
class svtkDataArray;
class svtkDataSet;
class svtkDataObject;
class svtkFieldData;
class svtkInformation;
class svtkInformationVector;
class svtkXdmfWriterDomainMemoryHandler;

namespace xdmf2
{
class XdmfArray;
class XdmfDOM;
class XdmfElement;
class XdmfGrid;
class XdmfGeometry;
class XdmfTopology;
}

class SVTKIOXDMF2_EXPORT svtkXdmfWriter : public svtkDataObjectAlgorithm
{
public:
  static svtkXdmfWriter* New();
  svtkTypeMacro(svtkXdmfWriter, svtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Set the input data set.
   */
  virtual void SetInputData(svtkDataObject* dobj);

  //@{
  /**
   * Set or get the file name of the xdmf file.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Set or get the file name of the hdf5 file.
   * Note that if the File name is not specified, then the group name is ignore
   */
  svtkSetStringMacro(HeavyDataFileName);
  svtkGetStringMacro(HeavyDataFileName);
  //@}

  //@{
  /**
   * Set or get the group name into which data will be written
   * it may contain nested groups as in "/Proc0/Block0"
   */
  svtkSetStringMacro(HeavyDataGroupName);
  svtkGetStringMacro(HeavyDataGroupName);
  //@}

  /**
   * Write data to output. Method executes subclasses WriteData() method, as
   * well as StartMethod() and EndMethod() methods.
   * Returns 1 on success and 0 on failure.
   */
  virtual int Write();

  //@{
  /**
   * Topology Geometry and Attribute arrays smaller than this are written in line into the XML.
   * Default is 100.
   * Node: LightDataLimit is forced to 1 when MeshStaticOverTime is TRUE.
   */
  svtkSetMacro(LightDataLimit, int);
  svtkGetMacro(LightDataLimit, int);
  //@}

  //@{
  /**
   * Controls whether writer automatically writes all input time steps, or
   * just the timestep that is currently on the input.
   * Default is OFF.
   */
  svtkSetMacro(WriteAllTimeSteps, int);
  svtkGetMacro(WriteAllTimeSteps, int);
  svtkBooleanMacro(WriteAllTimeSteps, int);
  //@}

  //@{
  /**
   * Set of get the flag that specify if input mesh is static over time.
   * If so, the mesh topology and geometry heavy data will be written only once.
   * Default if FALSE.
   * Note: this mode requires that all data is dumped in the heavy data file.
   */
  svtkSetMacro(MeshStaticOverTime, bool);
  svtkGetMacro(MeshStaticOverTime, bool);
  svtkBooleanMacro(MeshStaticOverTime, bool);
  //@}

  //@{
  /**
   * Called in parallel runs to identify the portion this process is responsible for
   * TODO: respect this
   */
  svtkSetMacro(Piece, int);
  svtkSetMacro(NumberOfPieces, int);
  //@}

  // TODO: control choice of heavy data format (xml, hdf5, sql, raw)

  // TODO: These controls are available in svtkXdmfWriter, but are not used here.
  // GridsOnly
  // Append to Domain

protected:
  svtkXdmfWriter();
  ~svtkXdmfWriter() override;

  // Choose composite executive by default for time.
  svtkExecutive* CreateDefaultExecutive() override;

  // Can take any one data object
  int FillInputPortInformation(int port, svtkInformation* info) override;

  // Overridden to ...
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  // Overridden to ...
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  // Overridden to ...
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  // These do the work: recursively parse down input's structure all the way to arrays,
  // use XDMF lib to dump everything to file.

  virtual int CreateTopology(svtkDataSet* ds, xdmf2::XdmfGrid* grid, svtkIdType PDims[3],
    svtkIdType CDims[3], svtkIdType& PRank, svtkIdType& CRank, void* staticdata);
  virtual int CreateGeometry(svtkDataSet* ds, xdmf2::XdmfGrid* grid, void* staticdata);

  virtual int WriteDataSet(svtkDataObject* dobj, xdmf2::XdmfGrid* grid);
  virtual int WriteCompositeDataSet(svtkCompositeDataSet* dobj, xdmf2::XdmfGrid* grid);
  virtual int WriteAtomicDataSet(svtkDataObject* dobj, xdmf2::XdmfGrid* grid);
  virtual int WriteArrays(svtkFieldData* dsa, xdmf2::XdmfGrid* grid, int association, svtkIdType rank,
    svtkIdType* dims, const char* name);
  virtual void ConvertVToXArray(svtkDataArray* vda, xdmf2::XdmfArray* xda, svtkIdType rank,
    svtkIdType* dims, int AllocStrategy, const char* heavyprefix);

  virtual void SetupDataArrayXML(xdmf2::XdmfElement*, xdmf2::XdmfArray*) const;

  char* FileName;
  char* HeavyDataFileName;
  char* HeavyDataGroupName;
  std::string WorkingDirectory;
  std::string BaseFileName;

  int LightDataLimit;

  int WriteAllTimeSteps;
  int NumberOfTimeSteps;
  double CurrentTime;
  int CurrentTimeIndex;
  int CurrentBlockIndex;
  int UnlabelledDataArrayId;

  int Piece;
  int NumberOfPieces;

  bool MeshStaticOverTime;

  xdmf2::XdmfDOM* DOM;
  xdmf2::XdmfGrid* TopTemporalGrid;

  svtkXdmfWriterDomainMemoryHandler* DomainMemoryHandler;

  std::vector<xdmf2::XdmfTopology*> TopologyAtT0;
  std::vector<xdmf2::XdmfGeometry*> GeometryAtT0;

private:
  svtkXdmfWriter(const svtkXdmfWriter&) = delete;
  void operator=(const svtkXdmfWriter&) = delete;
};

#endif /* svtkXdmfWriter_h */
