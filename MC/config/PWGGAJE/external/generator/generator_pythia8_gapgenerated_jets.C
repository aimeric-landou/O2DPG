#include "FairGenerator.h"
#include "Generators/GeneratorPythia8.h"
#include "Pythia8/Pythia.h"
#include "TRandom.h"
#include <fairlogger/Logger.h>

#include <string>
#include <vector>

// Example of an implementation of an event generator
// that alternates between 2 gun generators. Serves as example
// to construct any meta-generator (such as cocktails) consisting
// or using a pool of underlying o2::eventgen::Generators.

// Test is using #o2-sim-dpl-eventgen --nEvents 10 --generator external --configKeyValues "GeneratorExternal.fileName=${O2DPG_ROOT}/MC/config/PWGGAJE/external/generator/generator_pythia8_gapgenerated_jets.C;GeneratorExternal.funcName=GeneratorPythia8GapGenJE()"

namespace o2
{
namespace eventgen
{


/// A very simple gap generator alternating between 2 different particle guns
class GeneratorPythia8GapGenJE : public Generator
{
public:
  /// default constructor
  GeneratorPythia8GapGenJE(int inputTriggerRatio = 5) {
    // put 2 different generators in the cocktail of generators
    gens.push_back(new o2::eventgen::GeneratorPythia8);
    gens.push_back(new o2::eventgen::GeneratorPythia8);

    mGeneratedEvents = 0;
    mInverseTriggerRatio = inputTriggerRatio;
  }


  ///  Destructor
  ~GeneratorPythia8GapGenJE() = default;

  Bool_t Init() override
  {
    // init all sub-gens
    gens[0].config=${O2DPG_ROOT}/MC/config/PWGGAJE/pythia8/generator/pythia8_jet.cfg;
    gens[1].config=${O2DPG_ROOT}/MC/config/PWGGAJE/pythia8/generator/pythia8_minBias.cfg; //gotta keep this one identical with pythia8_jet, maybe have them be generated once we understand how it works and have more time
    for (auto gen : gens) {
      gen->Init();
    }
    addSubGenerator(0, "Minimum bias"); // name the generators
    addSubGenerator(1, "High jet pt biased gen");
    return Generator::Init();
  }


  void setUsedSeed(unsigned int seed)
  {
    mUsedSeed = seed;
  };
  unsigned int getUsedSeed() const
  {
    return mUsedSeed;
  };

  // Bool_t generateEvent() override
  // {
  //   // here we call the individual gun generators in turn
  //   // (but we could easily call all of them to have cocktails)
  //   currentindex++;
  //   currentgen = gens[currentindex % 2];
  //   currentgen->generateEvent();
  //   // notify the sub event generator
  //   notifySubGenerator(currentindex % 2);
  //   return true;
  // }
  bool generateEvent() override
  {
    

    // Simple straightforward check to alternate generators
    if (mGeneratedEvents % mInverseTriggerRatio == 0)
    {
      currentgen = gens[1];
      currentgen->generateEvent();
      notifySubGenerator(1); // this gives the id 1 to the high pt jet biased pythia generator
    }
    else
    {
      currentgen = gens[0];
      currentgen->generateEvent(); //GeneratorPythia8::generateEvent()
      notifySubGenerator(0); // this gives the id 0 to the minimum-bias pythia generator
    }
    mGeneratedEvents++;
    return true;
  }

  // We override this function to import the particles from the
  // underlying generators into **this** generator instance
  Bool_t importParticles() override // not sure about what this does
  {
    mParticles.clear(); // clear container of mother class
    currentgen->importParticles();
    std::copy(currentgen->getParticles().begin(), currentgen->getParticles().end(), std::back_insert_iterator(mParticles));

    // we need to fix particles statuses --> need to enforce this on the importParticles level of individual generators
    for (auto& p : mParticles) {
      auto st = o2::mcgenstatus::MCGenStatusEncoding(p.GetStatusCode(), p.GetStatusCode()).fullEncoding;
      p.SetStatusCode(st);
      p.SetBit(ParticleStatus::kToBeDone, true);
    }

    return true;
  }

private:
  // Interface to override import particles
  Pythia8::Event mOutputEvent;
  
  // Properties of selection
  unsigned int mUsedSeed;

  // Control gap-triggering
  unsigned long long mGeneratedEvents;
  int mInverseTriggerRatio;

  // Handling generators
  std::vector<o2::eventgen::GeneratorPythia8*> gens;
  o2::eventgen::GeneratorPythia8* currentgen = nullptr;

};

} // namespace eventgen
} // namespace o2

/** generator instance and settings **/

FairGenerator* getGeneratorPythia8GapGenJE() {
  auto myGen = new GeneratorPythia8GapTriggeredJE(inputTriggerRatio);
  auto seed = (gRandom->TRandom::GetSeed() % 900000000);
  myGen->setUsedSeed(seed);
  myGen->readString("Random:setSeed on");
  myGen->readString("Random:seed " + std::to_string(seed));
  return new o2::eventgen::GeneratorPythia8GapGenJE();
}
