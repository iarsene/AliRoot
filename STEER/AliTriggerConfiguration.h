#ifndef ALITRIGGERCONFIGURATION_H
#define ALITRIGGERCONFIGURATION_H

/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// This class represents the CTP configuration                               //
//                                                                           //
//                                                                           //
// A Trigger Configuration define a trigger setup for particula run          //
// We have default one for different running modes                           //
// (Pb-Pb, p-p, p-A, Calibration, etc).                                      //
// It keep:                                                                  //
//   All the information contained in the real CTP configuration file        //
//   used online during the data taking.                                     //
//                                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <TNamed.h>
#include <TObjArray.h>
class TString;

class AliTriggerCluster;
class AliTriggerDescriptor;
class AliTriggerInput;
class AliTriggerInteraction;
class AliTriggerPFProtection;
class AliTriggerBCMask;
class AliTriggerDescriptor;
class AliTriggerClass;

class AliTriggerConfiguration : public TNamed {

public:
                          AliTriggerConfiguration();
                          AliTriggerConfiguration( TString & name, TString & description );
               virtual   ~AliTriggerConfiguration();
   //  Setters
       AliTriggerInput*   AddInput(TString &name, TString &det, UChar_t level, UInt_t signature, UChar_t number);
		Bool_t    AddInput(AliTriggerInput *input);

 AliTriggerInteraction*   AddInteraction(TString &name, TString &logic);
                Bool_t    AddInteraction(AliTriggerInteraction *interact);

 AliTriggerInteraction*   AddFunction(TString &name, TString &logic);
                Bool_t    AddFunction(AliTriggerInteraction *func);

		Bool_t    AddPFProtection( AliTriggerPFProtection* pf );

      AliTriggerBCMask*   AddMask( TString &name, TString &mask );
		Bool_t    AddMask( AliTriggerBCMask* mask );

     AliTriggerCluster*   AddCluster( TString &name, UChar_t index, TString &detectors );
                Bool_t    AddCluster( AliTriggerCluster* cluster );

  AliTriggerDescriptor*   AddDescriptor( TString & name, TString & cond);
                Bool_t    AddDescriptor( AliTriggerDescriptor* desc );

       AliTriggerClass*   AddClass( TString &name, UChar_t index,
				    AliTriggerDescriptor *desc, AliTriggerCluster *clus,
				    AliTriggerPFProtection *pfp, AliTriggerBCMask *mask,
				    UInt_t prescaler, Bool_t allrare);
       AliTriggerClass*   AddClass( TString &name, UChar_t index,
				    TString &desc, TString &clus,
				    TString &pfp, TString &mask,
				    UInt_t prescaler, Bool_t allrare);
                 Bool_t   AddClass( AliTriggerClass *trclass);

		   void   Reset();

  //  Getters
               TString    GetActiveDetectors() const;
               TString    GetTriggeringDetectors() const;
               TString    GetTriggeringModules() const;

       const TObjArray&   GetInputs() const { return fInputs; }
       const TObjArray&   GetInteractions() const { return fInteractions; }
       const TObjArray&   GetFunctions() const { return fFunctions; }
       const TObjArray&   GetPFProtections() const { return fPFProtections; }
       const TObjArray&   GetMasks() const { return fMasks; }
       const TObjArray&   GetClusters() const { return fClusters; }
       const TObjArray&   GetDescriptors() const { return fDescriptors; }
       const TObjArray&   GetClasses() const { return fClasses; }

                  Int_t   GetVersion() const { return fVersion; }

       //     AliTriggerCluster*   GetTriggerCluster(UInt_t index)
       //       { return (index < kNMaxClusters) ? (AliTriggerCluster*)fClusters[index] : NULL; }

       //AliTriggerPFProtection*   GetPFProtection(UInt_t index)
       //	       { return (index < kNMaxPFProtections) ? (AliTriggerPFProtection*)fPFProtections[index] : NULL; }
                Bool_t    CheckConfiguration( TString & configfile );
                  void    Print( const Option_t* opt ="" ) const;

  //  Configurations Database (root file)
                  void    WriteConfiguration( const char* filename="" );
      static TObjArray*   GetAvailableConfigurations( const char* filename="" );

      static AliTriggerConfiguration* LoadConfiguration(TString & des);
      static AliTriggerConfiguration* LoadConfigurationFromString(const char* configuration);

      enum {kNMaxInputs = 60}; // CTP handles up to 60 trigger detector inputs
      enum {kNMaxInteractions = 2}; // CTP handles up to two different interactions
      enum {kNMaxFunctions = 2}; // CTP handles up to two different logical functions
      enum {kNMaxClasses = 50}; // Maximum number of trigger classes = 50
      enum {kNMaxClusters = 6}; // Maximum number of different detector clusters that can be handled by CTP
      enum {kNMaxPFProtections = 4}; // Maximum number of different past-future protections that can be handled by CTP
      enum {kNMaxMasks = 4};  // CTP handles up to 4 different BC masks

private:
      Bool_t ProcessConfigurationLine(const char* line, Int_t& level);

      TObjArray            fInputs;                           // Array with active CTP inputs
      TObjArray            fInteractions;                     // Array with the trigger interactions
      TObjArray            fFunctions;                        // Array with the logical functions of the first 4 inputs
      TObjArray            fPFProtections;                    // Array of Past-Future Protections
      TObjArray            fMasks;                            // Array with BC masks
      TObjArray            fDescriptors;                      // Array with trigger descriptors
      TObjArray            fClusters;                         // Array of Detector Trigger Clusters
      TObjArray            fClasses;                          // Array of Trigger Classes

      Int_t                fVersion;                          // Configuration format version

                 Bool_t    IsSelected( TString detName, TString & detectors ) const;
   static const TString    fgkConfigurationFileName;        //! name of default configurations file

   AliTriggerConfiguration&   operator=(const AliTriggerConfiguration& des);
   AliTriggerConfiguration( const AliTriggerConfiguration& des );

   ClassDef( AliTriggerConfiguration, 2 )  // Define a trigger configuration
};

#endif
