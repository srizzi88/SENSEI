#ifndef svtkExodusIIReaderVariableCheck_h
#define svtkExodusIIReaderVariableCheck_h
#ifndef __SVTK_WRAP__
#ifndef SVTK_WRAPPING_CXX

#include "svtkExodusIIReaderPrivate.h" // for ArrayInfoType

#include <set>                          // STL Header for integration point names
#include <string>                       // STL Header for Start/StartInternal/Add
#include <vector>                       // STL Header for glommed array names
#include <svtksys/RegularExpression.hxx> // for integration point names

/**\brief Abstract base class for glomming arrays of variable names.
 *
 * Subclasses check whether variable names listed in an array of names
 * are related to each other (and should thus be glommed into a single
 * SVTK array).
 */
class svtkExodusIIReaderVariableCheck
{
public:
  /// Initialize a sequence of names. Returns true if any more names are acceptable.
  virtual bool Start(std::string name, const int* truth, int numTruth);
  /// Subclasses implement this and returns true if any more names are acceptable.
  virtual bool StartInternal(std::string name, const int* truth, int numTruth) = 0;
  /// Add a name to the sequence. Returns true if any more names may be added.
  virtual bool Add(std::string name, const int* truth) = 0;
  /// Returns the length of the sequence (or 0 if the match is incorrect or incomplete).
  virtual std::vector<std::string>::size_type Length();
  /// Accept this sequence. (Add an entry to the end of \a arr.) Must return Length().
  virtual int Accept(std::vector<svtkExodusIIReaderPrivate::ArrayInfoType>& arr, int startIndex,
    svtkExodusIIReaderPrivate* priv, int objtyp);

protected:
  svtkExodusIIReaderVariableCheck();
  virtual ~svtkExodusIIReaderVariableCheck();
  /** Utility that subclasses may call from within Add() to verify that
   * the new variable is defined on the same objects as other variables in the sequence.
   */
  bool CheckTruth(const int* truth);
  bool UniquifyName(svtkExodusIIReaderPrivate::ArrayInfoType& ainfo,
    std::vector<svtkExodusIIReaderPrivate::ArrayInfoType>& arrays);

  int GlomType;
  std::vector<int> SeqTruth;
  std::string Prefix;
  std::vector<std::string> OriginalNames;
};

/// This always accepts a single array name as a scalar. It is the fallback for all other checkers.
class svtkExodusIIReaderScalarCheck : public svtkExodusIIReaderVariableCheck
{
public:
  svtkExodusIIReaderScalarCheck();
  bool StartInternal(std::string name, const int*, int) override;
  bool Add(std::string, const int*) override;
};

/// This looks for n-D vectors whose names are identical except for a single final character.
class svtkExodusIIReaderVectorCheck : public svtkExodusIIReaderVariableCheck
{
public:
  svtkExodusIIReaderVectorCheck(const char* seq, int n);
  bool StartInternal(std::string name, const int*, int) override;
  bool Add(std::string name, const int* truth) override;
  std::vector<std::string>::size_type Length() override;

protected:
  std::string Endings;
  bool StillAdding;
};

/**\brief This looks for symmetric tensors of a given rank and dimension.
 *
 * All array names must be identical except for the last \a rank characters
 * which must be taken from the \a dim -length character array \a seq, specified
 * as dimension indicators.
 */
class svtkExodusIIReaderTensorCheck : public svtkExodusIIReaderVariableCheck
{
public:
  svtkExodusIIReaderTensorCheck(const char* seq, int n, int rank, int dim);
  bool StartInternal(std::string name, const int*, int) override;
  bool Add(std::string name, const int* truth) override;
  std::vector<std::string>::size_type Length() override;

protected:
  std::string Endings;
  svtkTypeUInt64 NumEndings;
  int Dimension;
  int Rank;
  bool StillAdding;
};

/// This looks for integration-point variables whose names contain an element shape and digits
/// specifying an integration point.
class svtkExodusIIReaderIntPointCheck : public svtkExodusIIReaderVariableCheck
{
public:
  svtkExodusIIReaderIntPointCheck();
  bool StartInternal(std::string name, const int*, int) override;
  bool Add(std::string name, const int*) override;
  std::vector<std::string>::size_type Length() override;
  /*
  virtual int Accept(
    std::vector<svtkExodusIIReaderPrivate::ArrayInfoType>& arr, int startIndex,
  svtkExodusIIReaderPrivate* priv, int objtyp )
    {
    }
    */
protected:
  bool StartIntegrationPoints(std::string cellType, std::string iptName);
  bool AddIntegrationPoint(std::string iptName);

  svtksys::RegularExpression RegExp;
  std::string VarName;
  std::string CellType;
  std::vector<int> IntPtMin;
  std::vector<int> IntPtMax;
  std::set<std::string> IntPtNames;
  svtkTypeUInt64 Rank;
  bool StillAdding;
};

#endif
#endif
#endif // svtkExodusIIReaderVariableCheck_h
// SVTK-HeaderTest-Exclude: svtkExodusIIReaderVariableCheck.h
