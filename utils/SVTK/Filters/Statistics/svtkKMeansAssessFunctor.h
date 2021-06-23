#ifndef svtkKMeansAssessFunctor_h
#define svtkKMeansAssessFunctor_h

class svtkKMeansAssessFunctor : public svtkStatisticsAlgorithm::AssessFunctor
{
  svtkDoubleArray* Distances;
  svtkIdTypeArray* ClusterMemberIDs;
  int NumRuns;

public:
  static svtkKMeansAssessFunctor* New();
  svtkKMeansAssessFunctor() {}
  ~svtkKMeansAssessFunctor() override;
  void operator()(svtkDoubleArray* result, svtkIdType row) override;
  bool Initialize(svtkTable* inData, svtkTable* reqModel, svtkKMeansDistanceFunctor* distFunc);
  int GetNumberOfRuns() { return NumRuns; }
};

#endif // svtkKMeansAssessFunctor_h
// SVTK-HeaderTest-Exclude: svtkKMeansAssessFunctor.h
