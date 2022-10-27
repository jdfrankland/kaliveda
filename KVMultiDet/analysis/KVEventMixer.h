#ifndef KVEventMixer_H
#define KVEventMixer_H

#include <vector>
#include <deque>

/**
   \class KVEventMixer
   \brief Generic event mixing algorithm for two-particle correlation studies
   \ingroup Analysis

   \tparam ParticleInfoStruct user-provided structure which will be used to store any required information on each particle to be used in the event mixing
   \tparam NumBins number of different event classes to be treated separately in the event mixing

   This class is a generic implementation of the event mixing technique used to estimate the uncorrelated
   background for two-particle correlation functions.
   Events belonging to different user-defined event classes can be treated separately at the same time.

   The generic problem adressed here is the construction of 2-particle correlation functions using an event mixing technique.
   Given events containing particles of type A and particles type B, it is assumed the user wants to construct
   some quantity which can be calculated from the properties of any pair (A,B), using both correlated
   and uncorrelated (mixed) pairs. The algorithm requires the user to provide:
      + iterators to perform loops over A and B particles in each event;
      + functions to treat each correlated and uncorrelated pair (A,B)

   For each analysed event, the function for correlated pairs will be called for each pair in the event,
   and then each previous event stored in buffers will be used to generate uncorrelated pairs for which the
   corresponding function will be called. The particles of each type of the \f$N\f$ previous events
   (by default \f$N=10\f$: can be changed with an argument to the class constructor) are kept in memory:
   decorrelating begins when each buffer is full, after this each new event which is added causes the previous
   'oldest' event in the same buffer to be discarded.

   ### Defining storage for particle properties to be used in event mixing buffers
   The user must define a class/struct which will be used to store any necessary information on each
   particle in the buffers used in the event mixing. This struct must define a constructor whose single argument
   is compatible with whatever type of reference to the particles in the event is returned by the iterators
   provided by the user (see ProcessEvent() method below).
   In the case of events stored in KVEvent (or derived) objects,
   this class/struct must have a constructor which can be called
   with a reference to a KVNucleus object (or derived class) and which will extract and store the required information.

   Here are some examples:
~~~~{.cpp}
   struct azimuthal_cor
   {
      double phi;
      azimuthal_cor(const KVNucleus& nuc) : phi{nuc.GetPhi()} {}
   };
~~~~
   This could be used for azimuthal correlations, supposing that all is required is the azimuthal angle of
   each particle.

   In experimental cases (i.e. analysing reconstructed data), you probably need to store some information on which detector
   was hit by the particle, in order to avoid generating uncorrelated pairs which could not exist experimentally. For example:
~~~~{.cpp}
   struct azimuthal_cor
   {
      double phi;
      int det_index;
      azimuthal_cor(const KVReconstructedNucleus& nuc)
         : phi{nuc.GetPhi()}, det_index{nuc.GetStoppingDetector()->GetIndex()} {}
   };
~~~~

   If more detailed informations on each particle are required, the most simple (but not the most lightweight in
   terms of memory) way may be to store the informations in a KVNucleus object in your structure. In this case,
   rather than trying to do a direct 'copy' of the nucleus passed to the constructor (especially when dealing
   with KVReconstructedNucleus objects), you should use the appropriate 'Set' methods of KVNucleus in order to
   copy the required characteristics for each nucleus. In this example, we show the minimum required in order to
   keep all kinematic information (in a given reference frame - not all frames can be copied) on each nucleus:
~~~~{.cpp}
   struct my_nucleus
   {
      KVNucleus nuc;
      my_nucleus(const KVReconstructedNucleus& n)
      {
         nuc.SetZandA(n.GetZ(),n.GetA());
         nuc.SetMomentum(n.GetMomentum()); // this is momentum in whatever 'default' frame is defined for n
      }
   };
~~~~

   In all cases, your class/struct needs to have a default constructor, default copy constructor, and default move constructor:
   just add them as shown:
~~~~{.cpp}
   struct my_part_struct
   {
      // add compiler-generated default constructors
      my_part_struct() = default;
      my_part_struct(const azimuthal_cor&) = default;
      my_part_struct(azimuthal_cor&&) = default;
   };
~~~~

   ### Using the event mixer in data analysis
   Once the class/struct for particle info storage is defined, the event mixer can be declared as a
   member variable of your analysis class:
~~~~{.cpp}
   struct azimuthal_cor { ... };
   KVEventMixer<azimuthal_cor,5> event_mixer;
~~~~

   The 2 template parameters are:
      + the particle storage class;
      + the number of different classes/bins that events are sorted into (default=1: no binning).

   The constructor optionally takes an argument defining the number of events to use for the mixing
   (if not given, default value 10 is used).

   In your event by event analysis, you need to call the ProcessEvent() method, providing the required
   iterators and functions to treat each pair of particles, along with the 'bin' number corresponding to
   the class of the current event (e.g. events could be classed according to total multiplicity into
   bins of centrality - it is up to you how you sort your events):

~~~~{.cpp}
   event_mixer.ProcessEvent(bin,IterA,IterB,CorrelatedPairFunction,UnCorrelatedPairFunction);
~~~~

   #### User-provided iterators for each particle type
   The 'iterators' IterA and IterB are not in fact strictly iterators: they are any expression which can be
   used on the right-hand side of a range-for loop, i.e. 'X' in the following pseudo-code:
~~~~{.cpp}
for(auto& p : X){ ... }
~~~~
   For example, if the particles in your event are stored in a std::vector, X would be a reference to the vector:
   ~~~~{.cpp}
   std::vector<KVNucleus> my_event;
   for(auto& n : my_event){ ... loop over particles in event ... }

   event_mixer.ProcessEvent(bin, my_event, ...);
   ~~~~

   KaliVeda provides a wide range of pseudo-iterators for KVEvent to be used in range-for loops,
   which are commonly used in KVEventSelector analysis classes:
   ~~~~{.cpp}
   for(auto& n : EventOKIterator(GetEvent())) { ... loop over 'OK' particles of event, 'n' is a KVNucleus& ... }

   for(auto& p : ReconEventIterator(GetEvent(),
                 {"alpha",[](const KVReconstructedNucleus* x){ return x->IsAMeasured() && x->IsIsotope(2,4); }})
      )
       { ... loop over isotopically-identified alpha particles of event, 'p' is a KVReconstructedNucleus& ... }
   ~~~~

   Therefore to use KVEventMixer in a KVEventSelector analysis class, IterA and IterB will probably be one of
   these pseudo-iterators, as in this (incomplete) example for correlating \f$\alpha\f$-particles and deuterons:
   ~~~~{.cpp}
   event_mixer.ProcessEvent(bin,
                  ReconEventIterator(GetEvent(),
                 {"alpha",[](const KVReconstructedNucleus* x){ return x->IsAMeasured() && x->IsIsotope(2,4); }}),
                  ReconEventIterator(GetEvent(),
                 {"deuton",[](const KVReconstructedNucleus* x){ return x->IsAMeasured() && x->IsIsotope(1,2); }}),
                 ...);
   ~~~~

   #### User-provided functions for correlated and uncorrelated pairs of particles
   Finally you have to provide two functions in order to treat each pair of correlated and uncorrelated particles.
   This is conveniently done using lambda functions. Each function takes three arguments, the 'bin' corresponding
   to the event class (this is the value passed as first argumet to ProcessEvent) and then one argument for each particle in the
   pair, with a slight signature difference between correlated and uncorrelated pairs:
       + correlated pair signature:   `void(int, IteratorReferenceType, IteratorReferenceType)`
       + uncorrelated pair signature: `void(int, IteratorReferenceType, ParticleInfoStruct&)`

   where `IteratorReferenceType` refers to the type of reference to the particles in the event returned by the
   pseudo-iterators given as 2nd and 3rd argument:
       + with a ReconEventIterator as in previous example, this would be KVReconstructedNucleus& (or base reference KVNucleus&)
       + etc.

   and `ParticleInfoStruct` is the structure you provided to store the informations on particles from different events
   for the decorrelation.

   The correlated pair function CorrelatedPairFunction will be called for every pair of particles (A,B) generated by coupling every A particle
   in the event with every B particle. If A and B particles are in fact of the same type, it is the user's responsibility
   to avoid correlating each particle with itself in the given CorrelatedPairFunction (e.g. by comparing the memory
   address of each particle object).

   The uncorrelated pair function UnCorrelatedPairFunction will be called for every pair generated by coupling:
      + every A particle of the current event with every B particle of the previous \f$N\f$ events stored in buffers;
      + every B particle of the current event with every A particle of the previous \f$N\f$ events stored in buffers;

   #### Full example of event mixing correlation study
   All will hopefully become clear with a full example. Let us assume that in your experimental data analysis
   you want to calculate azimuthal correlations
   for alpha particles for different bins in centrality, using the `azimuthal_cor` structure we presented above to
   store relevant informations (\f$\phi\f$ angle and detector index for each  \f$\alpha\f$-particle).

   Declaration and initialisation of the structure and the event mixing object as member variables of the analysis
   class (we only specify the parts of the code concerning the event mixing):

   ~~~~{.cpp}
   class MyCorrelationAnalysis : public KVReconEventSelector {

     // ... some other stuff here ...

   struct azimuthal_cor
   {
      double phi;
      int det_index;

      azimuthal_cor() = default;
      azimuthal_cor(azimuthal_cor&&) = default;
      azimuthal_cor(const azimuthal_cor&) = default;
      azimuthal_cor(const KVReconstructedNucleus& nuc)
         : phi{nuc.GetPhi()}, det_index{nuc.GetStoppingDetector()->GetIndex()} {}
   };
   // 5 centrality bins in this example
   KVEventMixer<azimuthal_cor,5> event_mixer;

     // ... lots more stuff here ...
  };
  ~~~~

  Next, the use of the event mixing object in the event by event analysis:
  ~~~~{.cpp}
  Bool_t MyCorrelationAnalysis::Analysis()
  {
     // ... lots of other analysis stuff, such as determining the 'bin' this event belongs to ...

     event_mixer.ProcessEvent(bin,

       ReconEventIterator(GetEvent(),         // iterator for first alpha
           {"alpha",[](const KVReconstructedNucleus* n){return n->IsOK() && n->IsAMeasured() && n->IsIsotope(2,4);}}),

       ReconEventIterator(GetEvent(),         // iterator for second alpha
           {"alpha",[](const KVReconstructedNucleus* n){return n->IsOK() && n->IsAMeasured() && n->IsIsotope(2,4);}}),

   [](int bin, KVNucleus& alpha, KVNucleus& other_alpha) // function to treat correlated pairs
   {
      // treat pair from same event
      // avoid same alpha!!
      if(&alpha == &other_alpha) return; // function will be called with same alpha as both arguments - warning!
      KVPosition pos;
      auto dphi = pos.GetAzimuthalWidth(alpha.GetPhi(),other_alpha.GetPhi());
      if(dphi>180) dphi=360-dphi;
      // ... probably fill a histogram or something here ...
   },

   [](int bin, KVReconstructedNucleus& alpha, azimuthal_cor& other_alpha) // function to treat uncorrelated pairs from mixed events
   {
      // treat pair from different events
      if(other_alpha.det_index != alpha.GetStoppingDetector()->GetIndex()) // here we avoid 'pairs' hitting same detector - impossible experimentally
      {
         KVPosition pos;
         auto dphi = pos.GetAzimuthalWidth(alpha.GetPhi(),other_alpha.phi);
         if(dphi>180) dphi=360-dphi;
         // ... probably fill a histogram or something here ...
      }
   }
  }
  ~~~~

  Things to note in this example:
     1. The correlated pair function uses KVNucleus& arguments although the provided iterators return KVReconstructedNucleus
        references. As KVNucleus is a base class of KVReconstructedNucleus and the only method used in the function
        (KVParticle::GetPhi()) is defined for KVNucleus, this is not a problem. KVReconstructedNucleus& arguments
        could be used equally well, without any other change to the code.
     2. We avoid correlating each alpha particle with itself (which would give a huge peak at \f$\Delta\phi=0^{o}\f$)
        by comparing the memory addresses of the two KVNucleus objects passed to the function (`if(&alpha == &other_alpha) return;`)
     3. In the uncorrelated pair function we use a KVReconstructedNucleus& argument for the particle from the current event,
        as we need to acces the KVReconstructedNucleus::GetStoppingDetector() method in order to avoid generating uncorrelated
        pairs which could not exist experimentally (particles hitting the same detector).

*/
template<typename ParticleInfoStruct, int NumBins = 1>
class KVEventMixer {
   struct event {
      std::vector<ParticleInfoStruct> particles;//!

      event() = default;
      event(event&&) = default;
      event(const event&) = default;
      event(const ParticleInfoStruct& p)
      {
         add(p);
      }
      void add(const ParticleInfoStruct& p)
      {
         particles.push_back(p);
      }
   };

   struct bin_data {
      std::deque<event> partA_events;//!
      std::deque<event> partB_events;//!
   };
   bin_data bins[NumBins];//!

   // number of events used for decorrelation (event mixing)
   typename std::deque<event>::size_type decor_events = 10;

public:
   /**
   \tparam ParticleInfoStruct user-provided structure which will be used to store any required information on each particle to be used in the event mixing
   \tparam NumBins number of different event classes to be treated separately in the event mixing [default:1]
      @param number_of_events_to_mix number of previous events to store in order to do event mixing [default:10]
    */
   KVEventMixer(typename std::deque<event>::size_type number_of_events_to_mix = 10)
      : decor_events{number_of_events_to_mix}
   {}

   /**
   @brief Event-by-event processing function
   @param bin_number user-defined 'event class number'; particles from different classes are not mixed
   @param iter_A iterator over particle type A
   @param iter_B iterator over particle type B
   @param TreatCorPair function called for each pair of (A,B) particles from the same event
   @param TreatNCorPair function called for each pair of (A,B) particles built from different events

   In your event by event analysis, you need to call the ProcessEvent() method, providing the required
   iterators and functions to treat each pair of particles, along with the 'bin' number corresponding to
   the class of the current event (e.g. events could be classed according to total multiplicity into
   bins of centrality - it is up to you how you sort your events):

   ~~~~{.cpp}
   event_mixer.ProcessEvent(bin,IterA,IterB,CorrelatedPairFunction,UnCorrelatedPairFunction);
   ~~~~

   #### User-provided iterators for each particle type
   The 'iterators' IterA and IterB are not in fact strictly iterators: they are any expression which can be
   used on the right-hand side of a range-for loop, i.e. 'X' in the following pseudo-code:
   ~~~~{.cpp}
   for(auto& p : X){ ... }
   ~~~~
   For example, if the particles in your event are stored in a std::vector, X would be a reference to the vector:
   ~~~~{.cpp}
   std::vector<KVNucleus> my_event;
   for(auto& n : my_event){ ... loop over particles in event ... }

   event_mixer.ProcessEvent(bin, my_event, ...);
   ~~~~

   KaliVeda provides a wide range of pseudo-iterators for KVEvent to be used in range-for loops,
   which are commonly used in KVEventSelector analysis classes:
   ~~~~{.cpp}
   for(auto& n : EventOKIterator(GetEvent())) { ... loop over 'OK' particles of event, 'n' is a KVNucleus& ... }

   for(auto& p : ReconEventIterator(GetEvent(),
                 {"alpha",[](const KVReconstructedNucleus* x){ return x->IsAMeasured() && x->IsIsotope(2,4); }})
      )
       { ... loop over isotopically-identified alpha particles of event, 'p' is a KVReconstructedNucleus& ... }
   ~~~~

   Therefore to use KVEventMixer in a KVEventSelector analysis class, IterA and IterB will probably be one of
   these pseudo-iterators, as in this (incomplete) example for correlating \f$\alpha\f$-particles and deuterons:
   ~~~~{.cpp}
   event_mixer.ProcessEvent(bin,
                  ReconEventIterator(GetEvent(),
                 {"alpha",[](const KVReconstructedNucleus* x){ return x->IsAMeasured() && x->IsIsotope(2,4); }}),
                  ReconEventIterator(GetEvent(),
                 {"deuton",[](const KVReconstructedNucleus* x){ return x->IsAMeasured() && x->IsIsotope(1,2); }}),
                 ...);
   ~~~~

   #### User-provided functions for correlated and uncorrelated pairs of particles
   Finally you have to provide two functions in order to treat each pair of correlated and uncorrelated particles.
   This is conveniently done using lambda functions. Each function takes three arguments, the 'bin' corresponding
   to the event class (this is the value passed as first argumet to ProcessEvent) and then one argument for each particle in the
   pair, with a slight signature difference between correlated and uncorrelated pairs:
       + correlated pair signature:   `void(int, IteratorReferenceType, IteratorReferenceType)`
       + uncorrelated pair signature: `void(int, IteratorReferenceType, ParticleInfoStruct&)`

   where `IteratorReferenceType` refers to the type of reference to the particles in the event returned by the
   pseudo-iterators given as 2nd and 3rd argument:
       + with a ReconEventIterator as in previous example, this would be KVReconstructedNucleus& (or base reference KVNucleus&)
       + etc.

   and `ParticleInfoStruct` is the structure you provided to store the informations on particles from different events
   for the decorrelation.

   The correlated pair function CorrelatedPairFunction will be called for every pair of particles (A,B) generated by coupling every A particle
   in the event with every B particle. If A and B particles are in fact of the same type, it is the user's responsibility
   to avoid correlating each particle with itself in the given CorrelatedPairFunction (e.g. by comparing the memory
   address of each particle object).

   The uncorrelated pair function UnCorrelatedPairFunction will be called for every pair generated by coupling:
      + every A particle of the current event with every B particle of the previous \f$N\f$ events stored in buffers;
      + every B particle of the current event with every A particle of the previous \f$N\f$ events stored in buffers;
    */
   template<typename TreatCorPairFunc, typename TreatNCorPairFunc, typename partA_iterator, typename partB_iterator>
   void ProcessEvent(int bin_number, partA_iterator iter_A, partB_iterator iter_B, TreatCorPairFunc TreatCorPair, TreatNCorPairFunc TreatNCorPair)
   {
      int n_partA(0), n_partB(0);

      for (auto& partA : iter_A) {
         // store partA in new correlated event in list
         ++n_partA;
         if (n_partA == 1) bins[bin_number].partA_events.push_back(ParticleInfoStruct(partA));
         else bins[bin_number].partA_events.back().add(ParticleInfoStruct(partA));

         for (auto& partB : iter_B) {
            if (n_partA == 1) { // each partA will loop over all partB, only add partB event once!
               // store partB in new correlated event in list
               ++n_partB;
               if (n_partB == 1) bins[bin_number].partB_events.push_back(ParticleInfoStruct(partB));
               else bins[bin_number].partB_events.back().add(ParticleInfoStruct(partB));
            }
            TreatCorPair(bin_number, partA, partB);
         }
      }
      if (!n_partA) {
         // treat events with no A particles, which may still have B particles
         // in this case the B particles should be added to the mixing buffers
         for (auto& partB : iter_B) {
            // store partB in new correlated event in list
            ++n_partB;
            if (n_partB == 1) bins[bin_number].partB_events.push_back(ParticleInfoStruct(partB));
            else bins[bin_number].partB_events.back().add(ParticleInfoStruct(partB));
         }
      }

      // keep only last decor_events events in lists
      if (bins[bin_number].partA_events.size() > decor_events) {
         // remove oldest (first) event
         bins[bin_number].partA_events.pop_front();
      }
      if (bins[bin_number].partB_events.size() > decor_events) {
         // remove oldest (first) event
         bins[bin_number].partB_events.pop_front();
      }

      // DECORRELATION
      if (n_partA && bins[bin_number].partB_events.size() == decor_events) {
         // uncorrelated spectra using each partA of this event and all partB in each of last decor_events
         for (auto& partA : iter_A) {
            for (auto& e : bins[bin_number].partB_events) {
               for (auto& partB : e.particles) {
                  TreatNCorPair(bin_number, partA, partB);
               }
            }
         }
      }
      if (n_partB && bins[bin_number].partA_events.size() == decor_events) {
         // uncorrelated spectra using each partB of this event and all partA in each of last decor_events
         for (auto& partB : iter_B) {
            for (auto& e : bins[bin_number].partA_events) {
               for (auto& partA : e.particles) {
                  TreatNCorPair(bin_number, partB, partA);
               }
            }
         }
      }

   }
};
#endif // KVEventMixer_H

/** \example ExampleCorrelationAnalysis.cpp
# Build \f$\alpha\f$-\f$\alpha\f$ azimuthal correlations using event mixing technique

Example of use of KVEventMixer which is a generic implementation of event mixing for two-particle correlation studies.

\include ExampleCorrelationAnalysis.h
*/
