#ifndef ALIITSPLANEEFFSPD_H
#define ALIITSPLANEEFFSPD_H
/* Copyright(c) 2007-2009, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */


#include "AliITSPlaneEff.h"

///////////////////////////////////////////
//                                       //
// ITS Plane Efficiency class            //
//       for SPD                         //
// Origin: Giuseppe.Bruno@ba.infn.it     //
///////////////////////////////////////////

/* $Id$ */
  
class AliITSPlaneEffSPD :  public AliITSPlaneEff {
 public:
    AliITSPlaneEffSPD(); // default constructor
    virtual ~AliITSPlaneEffSPD(); // destructror
    // copy constructor
    AliITSPlaneEffSPD(const AliITSPlaneEffSPD &source);
    // ass. operator
    AliITSPlaneEffSPD& operator=(const AliITSPlaneEffSPD &s);
    AliITSPlaneEff& operator=(const AliITSPlaneEff &source);
    //AliPlaneEff& operator=(const AliPlaneEff &source);
    // Simple way to add another class (i.e. statistics). 
    AliITSPlaneEffSPD& operator +=( const AliITSPlaneEffSPD &add);
    // Getters for average Plane efficiency (icluding dead/noisy)
    Double_t PlaneEff(const UInt_t mod, const UInt_t chip) const;
    Double_t ErrPlaneEff(const UInt_t mod, const UInt_t chip) const;
    Double_t PlaneEff(const UInt_t key) const 
        {return PlaneEff(GetModFromKey(key),GetChipFromKey(key));};
    Double_t ErrPlaneEff(const UInt_t key) const 
        {return ErrPlaneEff(GetModFromKey(key),GetChipFromKey(key));};
    // Methods to update the Plane efficiency (specific of the SPD segmentation) 
    Bool_t UpDatePlaneEff(const Bool_t Kfound, const UInt_t mod, const UInt_t chip);
    virtual Bool_t UpDatePlaneEff(const Bool_t Kfound, const UInt_t key)
        {return UpDatePlaneEff(Kfound,GetModFromKey(key),GetChipFromKey(key));};
    //
    enum {kNModule = 240}; // The number of modules
    enum {kNChip = 5}; // The number of chips per module
    enum {kNCol = 32}; // The number of columns per chip
    enum {kNRow = 256}; // The number of rows per chip (and per module)
//
//  UInt_t GetChip(const UInt_t col) const; // get the chip number (from 0 to kNChip)
//  Plane efficiency for active  detector (excluding dead/noisy channels)
//  access to DB is needed
    virtual Double_t LivePlaneEff(UInt_t key) const;
    Double_t LivePlaneEff(const UInt_t mod, const UInt_t chip) const
       {return LivePlaneEff(GetKey(mod,chip));};
    virtual Double_t ErrLivePlaneEff(UInt_t key) const;
    Double_t ErrLivePlaneEff(const UInt_t mod, const UInt_t chip) const
       {return ErrLivePlaneEff(GetKey(mod,chip));};
    // Compute the fraction of Live area (of the CHIP for the SPD)
    virtual Double_t GetFracLive(const UInt_t key) const;
    // Compute the fraction of bad (i.e. dead and noisy) area (of the CHIP for the SPD)
    virtual Double_t GetFracBad(const UInt_t key) const;
    virtual Bool_t WriteIntoCDB() const;
    virtual Bool_t ReadFromCDB(); // this method reads Data Members (statistics) from DataBase
    virtual Bool_t AddFromCDB()   // this method updates Data Members (statistics) from DataBase
      {AliError("AddFromCDB: Still To be implemented"); return kFALSE;}
   // method to locate a basic block from Detector Local coordinate (to be used in tracking)
   // see file cxx for numbering convention.
   // here idet runs from 0 to 79 for layer 0 and from 0 to 159 for layer 1
    UInt_t GetKeyFromDetLocCoord(Int_t ilay,Int_t idet, Float_t, Float_t locz) const;
    UInt_t Nblock() const; // return the number of basic blocks
    // compute the geometrical limit of a basic block (chip) in detector local coordinate system 
    Bool_t GetBlockBoundaries(const UInt_t key,Float_t& xmn,Float_t& xmx,Float_t& zmn,Float_t& zmx) const;

 protected:
    virtual void Copy(TObject &obj) const;
    Int_t GetMissingTracksForGivenEff(Double_t eff, Double_t RelErr, UInt_t im, UInt_t ic) const;

// 
    Int_t fFound[kNModule*kNChip];  // number of associated clusters in a given chip
    Int_t fTried[kNModule*kNChip];  // number of tracks used for chip efficiency evaluation
 private:
    UInt_t GetKey(const UInt_t mod, const UInt_t chip) const; // unique key to locate the basic 
                                                              // block of the SPD 
    UInt_t GetModFromKey(const UInt_t key) const;
    UInt_t GetChipFromKey(const UInt_t key) const;
    UInt_t GetChipFromCol(const UInt_t col) const;  // get the chip number (from 0 to kNChip)
    UInt_t GetColFromLocZ(Float_t zloc) const;      // get the Column from the local z
    Float_t GetLocZFromCol(const UInt_t col) const; // get the local Z from the column number,  
                                                    // the latter in the range [0,kNChip*kNCol]
    Float_t GetLocXFromRow(const UInt_t row) const; // get the local X from the row number  
                                                    // the latter in the range [0,kNRow]
    void GetModAndChipFromKey(const UInt_t key, UInt_t& mod, UInt_t& chip) const;
    void GetDeadAndNoisyInChip(const UInt_t key, UInt_t& dead, UInt_t& noisy) const;

    ClassDef(AliITSPlaneEffSPD,1) // SPD Plane Efficiency class
};
//
inline UInt_t AliITSPlaneEffSPD::Nblock() const {return kNModule*kNChip;}
//
#endif

