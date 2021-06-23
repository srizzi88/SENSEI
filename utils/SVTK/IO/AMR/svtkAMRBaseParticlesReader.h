/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkAMRBaseParticlesReader.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkAMRBaseParticlesReader
 *
 *
 *  An abstract base class that implements all the common functionality for
 *  all particle readers.
 */

#ifndef svtkAMRBaseParticlesReader_h
#define svtkAMRBaseParticlesReader_h

#include "svtkIOAMRModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

class svtkInformation;
class svtkInformationVector;
class svtkIndent;
class svtkMultiProcessController;
class svtkPolyData;
class svtkDataArraySelection;
class svtkCallbackCommand;

class SVTKIOAMR_EXPORT svtkAMRBaseParticlesReader : public svtkMultiBlockDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkAMRBaseParticlesReader, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set & Get the frequency.
   */
  svtkGetMacro(Frequency, int);
  svtkSetMacro(Frequency, int);
  //@}

  //@{
  /**
   * Set & Get the multi-process controller.
   */
  svtkGetMacro(Controller, svtkMultiProcessController*);
  svtkSetMacro(Controller, svtkMultiProcessController*);
  //@}

  //@{
  /**
   * Set & Get for filter location and boolean macro
   */
  svtkSetMacro(FilterLocation, svtkTypeBool);
  svtkGetMacro(FilterLocation, svtkTypeBool);
  svtkBooleanMacro(FilterLocation, svtkTypeBool);
  //@}

  //@{
  /**
   * Get the data array selection tables used to configure which data
   * arrays are loaded by the reader.
   */
  svtkGetObjectMacro(ParticleDataArraySelection, svtkDataArraySelection);
  //@}

  /**
   * Get the number of particles arrays available in the input.
   */
  int GetNumberOfParticleArrays();

  /**
   * Get the particle array name of the array associated with the given
   * index.
   */
  const char* GetParticleArrayName(int index);

  //@{
  /**
   * Get/Set whether the particle array status.
   */
  int GetParticleArrayStatus(const char* name);
  void SetParticleArrayStatus(const char* name, int status);
  //@}

  virtual void SetFileName(const char* fileName);
  svtkGetStringMacro(FileName);

  //@{
  /**
   * Sets the min location
   */
  inline void SetMinLocation(const double minx, const double miny, const double minz)
  {
    this->MinLocation[0] = minx;
    this->MinLocation[1] = miny;
    this->MinLocation[2] = minz;
  }
  //@}

  //@{
  /**
   * Sets the max location
   */
  inline void SetMaxLocation(const double maxx, const double maxy, const double maxz)
  {
    this->MaxLocation[0] = maxx;
    this->MaxLocation[1] = maxy;
    this->MaxLocation[2] = maxz;
  }
  //@}

  /**
   * Returns the total number of particles
   */
  virtual int GetTotalNumberOfParticles() = 0;

protected:
  svtkAMRBaseParticlesReader();
  ~svtkAMRBaseParticlesReader() override;

  /**
   * Reads the metadata, e.g., the number of blocks in the file.
   * After the metadata is read, this->Initialized is set to true.
   * Furthermore, to limit I/O, all concrete classes must make sure
   * that this method returns immediately if this->Initialized is true.
   */
  virtual void ReadMetaData() = 0;

  /**
   * Reads the particles corresponding to the block associated with the
   * given supplied block index.
   */
  virtual svtkPolyData* ReadParticles(const int blkIdx) = 0;

  /**
   * Filters particles by their location. If FilterLocation is ON, this
   * method returns whether or not the particle with the supplied xyz
   * coordinates class within the bounding box specified by the user using
   * the SetMinLocation & SetMaxLocation.
   */
  bool CheckLocation(const double x, const double y, const double z);

  /**
   * Determines whether this reader instance is running in parallel or not.
   */
  bool IsParallel();

  /**
   * Determines if the block associated with the given block index belongs
   * to the process that executes the current instance of the reader.
   */
  bool IsBlockMine(const int blkIdx);

  /**
   * Given the block index, this method determines the process Id.
   * If the reader instance is serial this method always returns 0.
   * Otherwise, static block-cyclic-distribution is assumed and each
   * block is assigned to a process according to blkIdx%N, where N is
   * the total number of processes.
   */
  int GetBlockProcessId(const int blkIdx);

  /**
   * Initializes the AMR Particles reader
   * NOTE: must be called in the constructor of concrete classes.
   */
  void Initialize();

  //@{
  /**
   * Standard Array selection variables & methods
   */
  svtkDataArraySelection* ParticleDataArraySelection;
  svtkCallbackCommand* SelectionObserver;
  //@}

  /**
   * Initializes the ParticleDataArraySelection object. This method
   * only executes for an initial request in which case all arrays are
   * deselected.
   */
  void InitializeParticleDataSelections();

  /**
   * Sets up the ParticleDataArraySelection. Implemented
   * by concrete classes.
   */
  virtual void SetupParticleDataSelections() = 0;

  /**
   * Call-back registered with the SelectionObserver for selecting/deselecting
   * particles
   */
  static void SelectionModifiedCallback(
    svtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  //@{
  /**
   * Standard pipeline operations
   */
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;
  //@}

  int NumberOfBlocks;

  svtkTypeBool FilterLocation;
  double MinLocation[3];
  double MaxLocation[3];

  int Frequency;
  svtkMultiProcessController* Controller;

  bool InitialRequest;
  bool Initialized;
  char* FileName;

private:
  svtkAMRBaseParticlesReader(const svtkAMRBaseParticlesReader&) = delete;
  void operator=(const svtkAMRBaseParticlesReader&) = delete;
};

#endif /* svtkAMRBaseParticlesReader_h */
