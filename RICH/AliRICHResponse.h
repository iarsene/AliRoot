#ifndef AliRICHResponse_h
#define AliRICHResponse_h


/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

#include "TObject.h"

class AliSegmentation;

class AliRICHResponse : public TObject
{
public:
    AliRICHResponse();                    // default ctor
    virtual ~AliRICHResponse(){}
    virtual void    SetSigmaIntegration(Float_t p1) {fSigmaIntegration=p1;}
    virtual Float_t SigmaIntegration() {return fSigmaIntegration;}    
    virtual void    SetChargeSlope(Float_t p1) {fChargeSlope=p1;}
    virtual Float_t ChargeSlope()      {return fChargeSlope;}
    virtual void    SetChargeSpread(Float_t p1, Float_t p2){fChargeSpreadX=p1; fChargeSpreadY=p2;}
    virtual Float_t ChargeSpreadX()    {return fChargeSpreadX;}    
    virtual Float_t ChargeSpreadY()    {return fChargeSpreadY;}        
    virtual void    SetMaxAdc(Float_t p1) {fMaxAdc=p1;}
    virtual Float_t MaxAdc()           {return fMaxAdc;}
    virtual Float_t Pitch()            {return fPitch;}
    virtual void    SetPitch(Float_t p1) {fPitch=p1;};
    virtual void    SetAlphaFeedback(Float_t alpha) {fAlphaFeedback=alpha;}
    virtual Float_t AlphaFeedback()  {return fAlphaFeedback;}
    virtual void    SetEIonisation(Float_t e) {fEIonisation=e;}
    virtual Float_t EIonisation() {return fEIonisation;}                            
    virtual void   SetSqrtKx3(Float_t p1) {fSqrtKx3=p1;};
    virtual void   SetKx2(Float_t p1) {fKx2=p1;};
    virtual void   SetKx4(Float_t p1) {fKx4=p1;};
    virtual void   SetSqrtKy3(Float_t p1) {fSqrtKy3=p1;};
    virtual void   SetKy2(Float_t p1) {fKy2=p1;};
    virtual void   SetKy4(Float_t p1) {fKy4=p1;};
    virtual Float_t IntPH(Float_t eloss, Float_t yhit);
    virtual Float_t IntPH(Float_t yhit);
    virtual Float_t IntXY(AliSegmentation * segmentation);
    virtual Int_t   FeedBackPhotons(Float_t *source, Float_t qtot);
    virtual void SetWireSag(Int_t p1) {fWireSag=p1;};
    virtual void SetVoltage(Int_t p1) {fVoltage=p1;};
    protected:
    Float_t fChargeSlope;              // Slope of the charge distribution
    Float_t fChargeSpreadX;            // Width of the charge distribution in x
    Float_t fChargeSpreadY;            // Width of the charge distribution in y
    Float_t fSigmaIntegration;         // Number of sigma's used for charge distribution
    Float_t fAlphaFeedback;            // Feedback photons coefficient
    Float_t fEIonisation;              // Mean ionisation energy
    Float_t fMaxAdc;                   // Maximum ADC channel
    Float_t fSqrtKx3;                  // Mathieson parameters for x
    Float_t fKx2;                      // Mathieson parameters for x
    Float_t fKx4;                      // Mathieson parameters for x
    Float_t fSqrtKy3;                  // Mathieson parameters for y
    Float_t fKy2;                      // Mathieson parameters for y 
    Float_t fKy4;                      // Mathieson parameters for y
    Float_t fPitch;                    // Anode-cathode pitch
    Int_t   fWireSag;                  // Flag to turn on/off (0/1) wire sag
    Int_t   fVoltage;                  // Working voltage (2000, 2050, 2100, 2150)
    ClassDef(AliRICHResponse,1)        // RICH Response model: Mathieson verion
};
    
#endif
