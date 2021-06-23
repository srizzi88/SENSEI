/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMoleculeMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMoleculeMapper
 * @brief   Mapper that draws svtkMolecule objects
 *
 *
 * svtkMoleculeMapper uses glyphs (display lists) to quickly render a
 * molecule.
 */

#ifndef svtkMoleculeMapper_h
#define svtkMoleculeMapper_h

#include "svtkDomainsChemistryModule.h" // For export macro
#include "svtkMapper.h"
#include "svtkNew.h" // For svtkNew

class svtkActor;
class svtkGlyph3DMapper;
class svtkIdTypeArray;
class svtkMolecule;
class svtkPeriodicTable;
class svtkPolyData;
class svtkPolyDataMapper;
class svtkRenderer;
class svtkSelection;
class svtkSphereSource;
class svtkTrivialProducer;

class SVTKDOMAINSCHEMISTRY_EXPORT svtkMoleculeMapper : public svtkMapper
{
public:
  static svtkMoleculeMapper* New();
  svtkTypeMacro(svtkMoleculeMapper, svtkMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the input svtkMolecule.
   */
  void SetInputData(svtkMolecule* in);
  svtkMolecule* GetInput();
  //@}

  /**
   * Set ivars to default ball-and-stick settings. This is equivalent
   * to the following:
   * - SetRenderAtoms( true )
   * - SetRenderBonds( true )
   * - SetAtomicRadiusType( VDWRadius )
   * - SetAtomicRadiusScaleFactor( 0.3 )
   * - SetBondColorMode( DiscreteByAtom )
   * - SetUseMultiCylindersForBonds( true )
   * - SetBondRadius( 0.075 )
   */
  void UseBallAndStickSettings();

  /**
   * Set ivars to default van der Waals spheres settings. This is
   * equivalent to the following:
   * - SetRenderAtoms( true )
   * - SetRenderBonds( true )
   * - SetAtomicRadiusType( VDWRadius )
   * - SetAtomicRadiusScaleFactor( 1.0 )
   * - SetBondColorMode( DiscreteByAtom )
   * - SetUseMultiCylindersForBonds( true )
   * - SetBondRadius( 0.075 )
   */
  void UseVDWSpheresSettings();

  /**
   * Set ivars to default liquorice stick settings. This is
   * equivalent to the following:
   * - SetRenderAtoms( true )
   * - SetRenderBonds( true )
   * - SetAtomicRadiusType( UnitRadius )
   * - SetAtomicRadiusScaleFactor( 0.1 )
   * - SetBondColorMode( DiscreteByAtom )
   * - SetUseMultiCylindersForBonds( false )
   * - SetBondRadius( 0.1 )
   */
  void UseLiquoriceStickSettings();

  /**
   * Set ivars to use fast settings that may be useful for rendering
   * extremely large molecules where the overall shape is more
   * important than the details of the atoms/bond. This is equivalent
   * to the following:
   * - SetRenderAtoms( true )
   * - SetRenderBonds( true )
   * - SetAtomicRadiusType( UnitRadius )
   * - SetAtomicRadiusScaleFactor( 0.60 )
   * - SetBondColorMode( SingleColor )
   * - SetBondColor( 50, 50, 50 )
   * - SetUseMultiCylindersForBonds( false )
   * - SetBondRadius( 0.075 )
   */
  void UseFastSettings();

  //@{
  /**
   * Get/Set whether or not to render atoms. Default: On.
   */
  svtkGetMacro(RenderAtoms, bool);
  svtkSetMacro(RenderAtoms, bool);
  svtkBooleanMacro(RenderAtoms, bool);
  //@}

  //@{
  /**
   * Get/Set whether or not to render bonds. Default: On.
   */
  svtkGetMacro(RenderBonds, bool);
  svtkSetMacro(RenderBonds, bool);
  svtkBooleanMacro(RenderBonds, bool);
  //@}

  //@{
  /**
   * Get/Set whether or not to render the unit cell lattice, if present.
   * Default: On.
   */
  svtkGetMacro(RenderLattice, bool);
  svtkSetMacro(RenderLattice, bool);
  svtkBooleanMacro(RenderLattice, bool);
  //@}

  enum
  {
    CovalentRadius = 0,
    VDWRadius,
    UnitRadius,
    CustomArrayRadius
  };

  //@{
  /**
   * Get/Set the type of radius used to generate the atoms. Default:
   * VDWRadius. If CustomArrayRadius is used, the VertexData array named
   * 'radii' is used for per-atom radii.
   */
  svtkGetMacro(AtomicRadiusType, int);
  svtkSetMacro(AtomicRadiusType, int);
  const char* GetAtomicRadiusTypeAsString();
  void SetAtomicRadiusTypeToCovalentRadius() { this->SetAtomicRadiusType(CovalentRadius); }
  void SetAtomicRadiusTypeToVDWRadius() { this->SetAtomicRadiusType(VDWRadius); }
  void SetAtomicRadiusTypeToUnitRadius() { this->SetAtomicRadiusType(UnitRadius); }
  void SetAtomicRadiusTypeToCustomArrayRadius() { this->SetAtomicRadiusType(CustomArrayRadius); }
  //@}

  //@{
  /**
   * Get/Set the uniform scaling factor applied to the atoms.
   * This is ignored when AtomicRadiusType == CustomArrayRadius.
   * Default: 0.3.
   */
  svtkGetMacro(AtomicRadiusScaleFactor, float);
  svtkSetMacro(AtomicRadiusScaleFactor, float);
  //@}

  //@{
  /**
   * Get/Set whether multicylinders will be used to represent multiple
   * bonds. Default: On.
   */
  svtkGetMacro(UseMultiCylindersForBonds, bool);
  svtkSetMacro(UseMultiCylindersForBonds, bool);
  svtkBooleanMacro(UseMultiCylindersForBonds, bool);
  //@}

  enum
  {
    SingleColor = 0,
    DiscreteByAtom
  };

  //@{
  /**
   * Get/Set the method by which bonds are colored.

   * If 'SingleColor' is used, all bonds will be the same color. Use
   * SetBondColor to set the rgb values used.

   * If 'DiscreteByAtom' is selected, each bond is colored using the
   * same lookup table as the atoms at each end, with a sharp color
   * boundary at the bond center.
   */
  svtkGetMacro(BondColorMode, int);
  svtkSetClampMacro(BondColorMode, int, SingleColor, DiscreteByAtom);
  void SetBondColorModeToSingleColor() { this->SetBondColorMode(SingleColor); }
  void SetBondColorModeToDiscreteByAtom() { this->SetBondColorMode(DiscreteByAtom); }
  const char* GetBondColorModeAsString();
  //@}

  //@{
  /**
   * Get/Set the method by which atoms are colored.
   *
   * If 'SingleColor' is used, all atoms will have the same color. Use
   * SetAtomColor to set the rgb values to be used.
   *
   * If 'DiscreteByAtom' is selected, each atom is colored using the
   * internal lookup table.
   */
  svtkGetMacro(AtomColorMode, int);
  svtkSetClampMacro(AtomColorMode, int, SingleColor, DiscreteByAtom);
  //@}

  //@{
  /**
   * Get/Set the color of the atoms as an rgb tuple.
   * Default: {150, 150, 150} (grey)
   */
  svtkGetVector3Macro(AtomColor, unsigned char);
  svtkSetVector3Macro(AtomColor, unsigned char);
  //@}

  //@{
  /**
   * Get/Set the color of the bonds as an rgb tuple.
   * Default: {50, 50, 50} (dark grey)
   */
  svtkGetVector3Macro(BondColor, unsigned char);
  svtkSetVector3Macro(BondColor, unsigned char);
  //@}

  //@{
  /**
   * Get/Set the radius of the bond cylinders. Default: 0.075
   */
  svtkGetMacro(BondRadius, float);
  svtkSetMacro(BondRadius, float);
  //@}

  //@{
  /**
   * Get/Set the color of the bonds as an rgb tuple.
   * Default: {255, 255, 255} (white)
   */
  svtkGetVector3Macro(LatticeColor, unsigned char);
  svtkSetVector3Macro(LatticeColor, unsigned char);
  //@}

  //@{
  /**
   * Extract the ids atoms and/or bonds rendered by this molecule from a
   * svtkSelection object. The svtkIdTypeArray
   */
  virtual void GetSelectedAtomsAndBonds(
    svtkSelection* selection, svtkIdTypeArray* atomIds, svtkIdTypeArray* bondIds);
  virtual void GetSelectedAtoms(svtkSelection* selection, svtkIdTypeArray* atomIds)
  {
    this->GetSelectedAtomsAndBonds(selection, atomIds, nullptr);
  }
  virtual void GetSelectedBonds(svtkSelection* selection, svtkIdTypeArray* bondIds)
  {
    this->GetSelectedAtomsAndBonds(selection, nullptr, bondIds);
  }
  //@}

  //@{
  /**
   * Reimplemented from base class
   */
  void Render(svtkRenderer*, svtkActor*) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  double* GetBounds() override;
  void GetBounds(double bounds[6]) override { svtkAbstractMapper3D::GetBounds(bounds); }
  int FillInputPortInformation(int port, svtkInformation* info) override;
  bool GetSupportsSelection() override { return true; }
  //@}

  //@{
  /**
   * Get/Set the atomic radius array name. Default: "radii"
   * It is only used when AtomicRadiusType is set to CustomArrayRadius.
   */
  svtkGetStringMacro(AtomicRadiusArrayName);
  svtkSetStringMacro(AtomicRadiusArrayName);
  //@}

  /**
   * Helper method to set ScalarMode on both AtomGlyphMapper and BondGlyphMapper.
   * true means SVTK_COLOR_MODE_MAP_SCALARS, false SVTK_COLOR_MODE_DIRECT_SCALARS.
   */
  virtual void SetMapScalars(bool map);

  /**
   * Accessor to internal structure. This is exposed to make it available for ray tracers.
   */
  svtkPeriodicTable* GetPeriodicTable() { return this->PeriodicTable; }

protected:
  svtkMoleculeMapper();
  ~svtkMoleculeMapper() override;

  //@{
  /**
   * Customize atom rendering
   */
  bool RenderAtoms;
  int AtomicRadiusType;
  float AtomicRadiusScaleFactor;
  char* AtomicRadiusArrayName;
  int AtomColorMode;
  unsigned char AtomColor[3];
  //@}

  //@{
  /**
   * Customize bond rendering
   */
  bool RenderBonds;
  int BondColorMode;
  bool UseMultiCylindersForBonds;
  float BondRadius;
  unsigned char BondColor[3];
  //@}

  bool RenderLattice;

  /**
   * Internal render methods
   */
  void GlyphRender(svtkRenderer* ren, svtkActor* act);

  //@{
  /**
   * Cached variables and update methods
   */
  svtkNew<svtkPolyData> AtomGlyphPolyData;
  svtkNew<svtkTrivialProducer> AtomGlyphPointOutput;
  svtkNew<svtkPolyData> BondGlyphPolyData;
  svtkNew<svtkTrivialProducer> BondGlyphPointOutput;
  bool GlyphDataInitialized;
  virtual void UpdateGlyphPolyData();
  virtual void UpdateAtomGlyphPolyData();
  virtual void UpdateBondGlyphPolyData();
  //@}

  //@{
  /**
   * Internal mappers
   */
  svtkNew<svtkGlyph3DMapper> AtomGlyphMapper;
  svtkNew<svtkGlyph3DMapper> BondGlyphMapper;
  //@}

  unsigned char LatticeColor[3];
  svtkNew<svtkPolyData> LatticePolyData;
  svtkNew<svtkPolyDataMapper> LatticeMapper;
  virtual void UpdateLatticePolyData();

  /**
   * Periodic table for lookups
   */
  svtkNew<svtkPeriodicTable> PeriodicTable;

private:
  svtkMoleculeMapper(const svtkMoleculeMapper&) = delete;
  void operator=(const svtkMoleculeMapper&) = delete;
};

#endif
