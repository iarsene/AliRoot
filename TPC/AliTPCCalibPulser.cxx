/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

/* $Id$ */

//Root includes
#include <TH1F.h>
#include <TH2S.h>
#include <TString.h>
#include <TVectorF.h>
#include <TMath.h>

#include <TDirectory.h>
#include <TSystem.h>
#include <TFile.h>

//AliRoot includes
#include "AliRawReader.h"
#include "AliRawReaderRoot.h"
#include "AliRawReaderDate.h"
#include "AliTPCRawStream.h"
#include "AliTPCCalROC.h"
#include "AliTPCCalPad.h"
#include "AliTPCROC.h"
#include "AliTPCParam.h"
#include "AliTPCCalibPulser.h"
#include "AliTPCcalibDB.h"
#include "AliMathBase.h"
#include "TTreeStream.h"

//date
#include "event.h"


///////////////////////////////////////////////////////////////////////////////////////
//          Implementation of the TPC pulser calibration
//
//   Origin: Jens Wiechula, Marian Ivanov   J.Wiechula@gsi.de, Marian.Ivanov@cern.ch
// 
// 
/***************************************************************************
 *                      Class Description                                  *
 ***************************************************************************

 The AliTPCCalibPulser class is used to get calibration data concerning the FEE using
 runs performed with the calibration pulser.

 The information retrieved is
 - Time0 differences
 - Signal width differences
 - Amplification variations

 the seen differences arise from the manufacturing tolerances of the PASAs and are very small within
 one chip and somewhat large between different chips.

 Histograms:
   For each ROC three TH2S histos 'Reference Histograms'  (ROC channel vs. [Time0, signal width, Q sum]) is created when
   it is filled for the first time (GetHisto[T0,RMS,Q](ROC,kTRUE)). The histos are stored in the
   TObjArrays fHistoT0Array, fHistoRMSArray and fHistoQArray.


 Working principle:
 ------------------
 Raw calibration pulser data is processed by calling one of the ProcessEvent(...) functions
 (see below). These in the end call the Update(...) function.

 - the Update(...) function:
   In this function the array fPadSignal is filled with the adc signals between the specified range
   fFirstTimeBin and fLastTimeBin for the current pad.
   before going to the next pad the ProcessPad() function is called, which analyses the data for one pad
   stored in fPadSignal.

   - the ProcessPad() function:
     Find Pedestal and Noise information
     - use database information which has to be set by calling
       SetPedestalDatabase(AliTPCCalPad *pedestalTPC, AliTPCCalPad *padNoiseTPC)
     - if no information from the pedestal data base
       is available the informaion is calculated on the fly ( see FindPedestal() function )

     Find the Pulser signal information
     - calculate  mean = T0, RMS = signal width and Q sum in a range of -2+7 timebins around Q max
       the Q sum is scaled by pad area
       (see FindPulserSignal(...) function)

     Fill a temprary array for the T0 information (GetPadTimesEvent(fCurrentSector,kTRUE)) (why see below)
     Fill the Q sum and RMS values in the histograms (GetHisto[RMS,Q](ROC,kTRUE)),

 At the end of each event the EndEvent() function is called

 - the EndEvent() function:
   calculate the mean T0 for each ROC and fill the Time0 histogram with Time0-<Time0 for ROC>
   This is done to overcome syncronisation problems between the trigger and the fec clock.

 After accumulating the desired statistics the Analyse() function has to be called.
 - the Analyse() function
   Whithin this function the mean values of T0, RMS, Q are calculated for each pad, using
   the AliMathBase::GetCOG(...) function, and the calibration
   storage classes (AliTPCCalROC) are filled for each ROC.
   The calibration information is stored in the TObjArrays fCalRocArrayT0, fCalRocArrayRMS and
   fCalRocArrayQ;



 User interface for filling data:
 --------------------------------

 To Fill information one of the following functions can be used:

 Bool_t ProcessEvent(eventHeaderStruct *event);
   - process Date event
   - use AliTPCRawReaderDate and call ProcessEvent(AliRawReader *rawReader)

 Bool_t ProcessEvent(AliRawReader *rawReader);
   - process AliRawReader event
   - use AliTPCRawStream to loop over data and call ProcessEvent(AliTPCRawStream *rawStream)

 Bool_t ProcessEvent(AliTPCRawStream *rawStream);
   - process event from AliTPCRawStream
   - call Update function for signal filling

 Int_t Update(const Int_t isector, const Int_t iRow, const Int_t
              iPad,  const Int_t iTimeBin, const Float_t signal);
   - directly  fill signal information (sector, row, pad, time bin, pad)
     to the reference histograms

 It is also possible to merge two independently taken calibrations using the function

 void Merge(AliTPCCalibPulser *sig)
   - copy histograms in 'sig' if the do not exist in this instance
   - Add histograms in 'sig' to the histograms in this instance if the allready exist
   - After merging call Analyse again!



 -- example: filling data using root raw data:
 void fillSignal(Char_t *filename)
 {
    rawReader = new AliRawReaderRoot(fileName);
    if ( !rawReader ) return;
    AliTPCCalibPulser *calib = new AliTPCCalibPulser;
    while (rawReader->NextEvent()){
      calib->ProcessEvent(rawReader);
    }
    calib->Analyse();
    calib->DumpToFile("SignalData.root");
    delete rawReader;
    delete calib;
 }


 What kind of information is stored and how to retrieve them:
 ------------------------------------------------------------

 - Accessing the 'Reference Histograms' (Time0, signal width and Q sum information pad by pad):

   TH2F *GetHistoT0(Int_t sector);
   TH2F *GetHistoRMS(Int_t sector);
   TH2F *GetHistoQ(Int_t sector);

 - Accessing the calibration storage objects:

   AliTPCCalROC *GetCalRocT0(Int_t sector);   // for the Time0 values
   AliTPCCalROC *GetCalRocRMS(Int_t sector);  // for the signal width values
   AliTPCCalROC *GetCalRocQ(Int_t sector);    // for the Q sum values

   example for visualisation:
   if the file "SignalData.root" was created using the above example one could do the following:

   TFile fileSignal("SignalData.root")
   AliTPCCalibPulser *sig = (AliTPCCalibPulser*)fileSignal->Get("AliTPCCalibPulser");
   sig->GetCalRocT0(0)->Draw("colz");
   sig->GetCalRocRMS(0)->Draw("colz");

   or use the AliTPCCalPad functionality:
   AliTPCCalPad padT0(ped->GetCalPadT0());
   AliTPCCalPad padSigWidth(ped->GetCalPadRMS());
   padT0->MakeHisto2D()->Draw("colz");       //Draw A-Side Time0 Information
   padSigWidth->MakeHisto2D()->Draw("colz"); //Draw A-Side signal width Information
*/


ClassImp(AliTPCCalibPulser) /*FOLD00*/

AliTPCCalibPulser::AliTPCCalibPulser() : /*FOLD00*/
    TObject(),
    fFirstTimeBin(60),
    fLastTimeBin(120),
    fNbinsT0(200),
    fXminT0(-2),
    fXmaxT0(2),
    fNbinsQ(200),
    fXminQ(1),
    fXmaxQ(40),
    fNbinsRMS(100),
    fXminRMS(0.1),
    fXmaxRMS(5.1),
    fLastSector(-1),
    fOldRCUformat(kTRUE),
    fROC(AliTPCROC::Instance()),
    fParam(new AliTPCParam),
    fPedestalTPC(0x0),
    fPadNoiseTPC(0x0),
    fPedestalROC(0x0),
    fPadNoiseROC(0x0),
    fCalRocArrayT0(72),
    fCalRocArrayQ(72),
    fCalRocArrayRMS(72),
    fCalRocArrayOutliers(72),
    fHistoQArray(72),
    fHistoT0Array(72),
    fHistoRMSArray(72),
    fPadTimesArrayEvent(72),
    fPadQArrayEvent(72),
    fPadRMSArrayEvent(72),
    fPadPedestalArrayEvent(72),
    fCurrentChannel(-1),
    fCurrentSector(-1),
    fCurrentRow(-1),
    fMaxPadSignal(-1),
    fMaxTimeBin(-1),
    fPadSignal(1024),
    fPadPedestal(0),
    fPadNoise(0),
    fVTime0Offset(72),
    fVTime0OffsetCounter(72),
    fEvent(-1),
    fDebugStreamer(0x0),
    fDebugLevel(0)
{
    //
    // AliTPCSignal default constructor
    //

}
//_____________________________________________________________________
AliTPCCalibPulser::AliTPCCalibPulser(const AliTPCCalibPulser &sig) :
    TObject(sig),
    fFirstTimeBin(sig.fFirstTimeBin),
    fLastTimeBin(sig.fLastTimeBin),
    fNbinsT0(sig.fNbinsT0),
    fXminT0(sig.fXminT0),
    fXmaxT0(sig.fXmaxT0),
    fNbinsQ(sig.fNbinsQ),
    fXminQ(sig.fXminQ),
    fXmaxQ(sig.fXmaxQ),
    fNbinsRMS(sig.fNbinsRMS),
    fXminRMS(sig.fXminRMS),
    fXmaxRMS(sig.fXmaxRMS),
    fLastSector(-1),
    fOldRCUformat(kTRUE),
    fROC(AliTPCROC::Instance()),
    fParam(new AliTPCParam),
    fPedestalTPC(0x0),
    fPadNoiseTPC(0x0),
    fPedestalROC(0x0),
    fPadNoiseROC(0x0),
    fCalRocArrayT0(72),
    fCalRocArrayQ(72),
    fCalRocArrayRMS(72),
    fCalRocArrayOutliers(72),
    fHistoQArray(72),
    fHistoT0Array(72),
    fHistoRMSArray(72),
    fPadTimesArrayEvent(72),
    fPadQArrayEvent(72),
    fPadRMSArrayEvent(72),
    fPadPedestalArrayEvent(72),
    fCurrentChannel(-1),
    fCurrentSector(-1),
    fCurrentRow(-1),
    fMaxPadSignal(-1),
    fMaxTimeBin(-1),
    fPadSignal(1024),
    fPadPedestal(0),
    fPadNoise(0),
    fVTime0Offset(72),
    fVTime0OffsetCounter(72),
    fEvent(-1),
    fDebugStreamer(0x0),
    fDebugLevel(sig.fDebugLevel)
{
    //
    // AliTPCSignal default constructor
    //

    for (Int_t iSec = 0; iSec < 72; ++iSec){
	const AliTPCCalROC *calQ   = (AliTPCCalROC*)sig.fCalRocArrayQ.UncheckedAt(iSec);
	const AliTPCCalROC *calT0  = (AliTPCCalROC*)sig.fCalRocArrayT0.UncheckedAt(iSec);
	const AliTPCCalROC *calRMS = (AliTPCCalROC*)sig.fCalRocArrayRMS.UncheckedAt(iSec);
        const AliTPCCalROC *calOut = (AliTPCCalROC*)sig.fCalRocArrayOutliers.UncheckedAt(iSec);

	const TH2S *hQ   = (TH2S*)sig.fHistoQArray.UncheckedAt(iSec);
	const TH2S *hT0  = (TH2S*)sig.fHistoT0Array.UncheckedAt(iSec);
        const TH2S *hRMS = (TH2S*)sig.fHistoRMSArray.UncheckedAt(iSec);

	if ( calQ   != 0x0 ) fCalRocArrayQ.AddAt(new AliTPCCalROC(*calQ), iSec);
	if ( calT0  != 0x0 ) fCalRocArrayT0.AddAt(new AliTPCCalROC(*calT0), iSec);
	if ( calRMS != 0x0 ) fCalRocArrayRMS.AddAt(new AliTPCCalROC(*calRMS), iSec);
        if ( calOut != 0x0 ) fCalRocArrayOutliers.AddAt(new AliTPCCalROC(*calOut), iSec);

	if ( hQ != 0x0 ){
	    TH2S *hNew = new TH2S(*hQ);
	    hNew->SetDirectory(0);
	    fHistoQArray.AddAt(hNew,iSec);
	}
	if ( hT0 != 0x0 ){
	    TH2S *hNew = new TH2S(*hT0);
	    hNew->SetDirectory(0);
	    fHistoQArray.AddAt(hNew,iSec);
	}
	if ( hRMS != 0x0 ){
	    TH2S *hNew = new TH2S(*hRMS);
	    hNew->SetDirectory(0);
	    fHistoQArray.AddAt(hNew,iSec);
	}
    }

}
//_____________________________________________________________________
AliTPCCalibPulser& AliTPCCalibPulser::operator = (const  AliTPCCalibPulser &source)
{
  //
  // assignment operator
  //
  if (&source == this) return *this;
  new (this) AliTPCCalibPulser(source);

  return *this;
}
//_____________________________________________________________________
AliTPCCalibPulser::~AliTPCCalibPulser()
{
    //
    // destructor
    //

    fCalRocArrayT0.Delete();
    fCalRocArrayQ.Delete();
    fCalRocArrayRMS.Delete();
    fCalRocArrayOutliers.Delete();

    fHistoQArray.Delete();
    fHistoT0Array.Delete();
    fHistoRMSArray.Delete();

    fPadTimesArrayEvent.Delete();
    fPadQArrayEvent.Delete();
    fPadRMSArrayEvent.Delete();
    fPadPedestalArrayEvent.Delete();

    if ( fDebugStreamer) delete fDebugStreamer;
    delete fParam;
}
//_____________________________________________________________________
Int_t AliTPCCalibPulser::Update(const Int_t icsector, /*FOLD00*/
				const Int_t icRow,
				const Int_t icPad,
				const Int_t icTimeBin,
				const Float_t csignal)
{
    //
    // Signal filling methode on the fly pedestal and Time offset correction if necessary.
    // no extra analysis necessary. Assumes knowledge of the signal shape!
    // assumes that it is looped over consecutive time bins of one pad
    //
    if ( (icTimeBin>fLastTimeBin) || (icTimeBin<fFirstTimeBin)   ) return 0;

    Int_t iChannel  = fROC->GetRowIndexes(icsector)[icRow]+icPad; //  global pad position in sector

    //init first pad and sector in this event
    if ( fCurrentChannel == -1 ) {
	fCurrentChannel = iChannel;
	fCurrentSector  = icsector;
        fCurrentRow     = icRow;
    }

    //process last pad if we change to a new one
    if ( iChannel != fCurrentChannel ){
        ProcessPad();
	fCurrentChannel = iChannel;
	fCurrentSector  = icsector;
        fCurrentRow     = icRow;
    }

    //fill signals for current pad
    fPadSignal[icTimeBin]=csignal;
    if ( csignal > fMaxPadSignal ){
	fMaxPadSignal = csignal;
	fMaxTimeBin   = icTimeBin;
    }
    return 0;
}
//_____________________________________________________________________
void AliTPCCalibPulser::FindPedestal(Float_t part)
{
    //
    // find pedestal and noise for the current pad. Use either database or
    // truncated mean with part*100%
    //
    Bool_t noPedestal = kTRUE;;
    if (fPedestalTPC&&fPadNoiseTPC){
        //use pedestal database
        //only load new pedestals if the sector has changed
	if ( fCurrentSector!=fLastSector ){
	    fPedestalROC = fPedestalTPC->GetCalROC(fCurrentSector);
            fPadNoiseROC = fPadNoiseTPC->GetCalROC(fCurrentSector);
	    fLastSector=fCurrentSector;
	}

	if ( fPedestalROC&&fPadNoiseROC ){
	    fPadPedestal = fPedestalROC->GetValue(fCurrentChannel);
	    fPadNoise    = fPadNoiseROC->GetValue(fCurrentChannel);
            noPedestal   = kFALSE;
	}

    }

    //if we are not running with pedestal database, or for the current sector there is no information
    //available, calculate the pedestal and noise on the fly
    if ( noPedestal ) {
	const Int_t kPedMax = 100;  //maximum pedestal value
	Float_t  max    =  0;
	Float_t  maxPos =  0;
	Int_t    median =  -1;
	Int_t    count0 =  0;
	Int_t    count1 =  0;
	//
	Float_t padSignal=0;
        //
	UShort_t histo[kPedMax];
	memset(histo,0,kPedMax*sizeof(UShort_t));

	for (Int_t i=fFirstTimeBin; i<=fLastTimeBin; ++i){
            padSignal = fPadSignal.GetMatrixArray()[i];
	    if (padSignal<=0) continue;
	    if (padSignal>max && i>10) {
		max = padSignal;
		maxPos = i;
	    }
	    if (padSignal>kPedMax-1) continue;
	    histo[Int_t(padSignal+0.5)]++;
	    count0++;
	}
	    //
	for (Int_t i=1; i<kPedMax; ++i){
	    if (count1<count0*0.5) median=i;
	    count1+=histo[i];
	}
	// truncated mean
	//
	Float_t count=histo[median] ,mean=histo[median]*median,  rms=histo[median]*median*median ;
	//
	for (Int_t idelta=1; idelta<10; ++idelta){
	    if (median-idelta<=0) continue;
	    if (median+idelta>kPedMax) continue;
	    if (count<part*count1){
		count+=histo[median-idelta];
		mean +=histo[median-idelta]*(median-idelta);
		rms  +=histo[median-idelta]*(median-idelta)*(median-idelta);
		count+=histo[median+idelta];
		mean +=histo[median+idelta]*(median+idelta);
		rms  +=histo[median+idelta]*(median+idelta)*(median+idelta);
	    }
	}
	fPadPedestal = 0;
	fPadNoise    = 0;
	if ( count > 0 ) {
	    mean/=count;
	    rms    = TMath::Sqrt(TMath::Abs(rms/count-mean*mean));
	    fPadPedestal = mean;
	    fPadNoise    = rms;
	} 
    }
}
//_____________________________________________________________________
void AliTPCCalibPulser::FindPulserSignal(TVectorD &param, Float_t &qSum)
{
    //
    //  Find position, signal width and height of the CE signal (last signal)
    //  param[0] = Qmax, param[1] = mean time, param[2] = rms;
    //  maxima: array of local maxima of the pad signal use the one closest to the mean CE position
    //

    Float_t ceQmax  =0, ceQsum=0, ceTime=0, ceRMS=0;
    Int_t   cemaxpos       = fMaxTimeBin;
    Float_t ceSumThreshold = 8.*fPadNoise;  // threshold for the signal sum
    const Int_t    kCemin  = 2;             // range for the analysis of the ce signal +- channels from the peak
    const Int_t    kCemax  = 7;

    if (cemaxpos!=0){
        ceQmax = fPadSignal.GetMatrixArray()[cemaxpos]-fPadPedestal;
	for (Int_t i=cemaxpos-kCemin; i<cemaxpos+kCemax; ++i){
            Float_t signal = fPadSignal.GetMatrixArray()[i]-fPadPedestal;
	    if ( (i>fFirstTimeBin) && (i<fLastTimeBin) && (signal>0) ){
		ceTime+=signal*(i+0.5);
                ceRMS +=signal*(i+0.5)*(i+0.5);
		ceQsum+=signal;
	    }
	}
    }
    if (ceQmax&&ceQsum>ceSumThreshold) {
	ceTime/=ceQsum;
	ceRMS  = TMath::Sqrt(TMath::Abs(ceRMS/ceQsum-ceTime*ceTime));
	fVTime0Offset.GetMatrixArray()[fCurrentSector]+=ceTime;   // mean time for each sector
	fVTime0OffsetCounter.GetMatrixArray()[fCurrentSector]++;

	//Normalise Q to the pad area
	Float_t norm = fParam->GetPadPitchWidth(fCurrentSector)*fParam->GetPadPitchLength(fCurrentSector,fCurrentRow);

	ceQsum/=norm;
    } else {
	ceQmax=0;
	ceTime=0;
	ceRMS =0;
	ceQsum=0;
    }
    param[0] = ceQmax;
    param[1] = ceTime;
    param[2] = ceRMS;
    qSum     = ceQsum;
}
//_____________________________________________________________________
void AliTPCCalibPulser::ProcessPad() /*FOLD00*/
{
    //
    //  Process data of current pad
    //

    FindPedestal();
    TVectorD param(3);
    Float_t  Qsum;
    FindPulserSignal(param, Qsum);

    Double_t meanT  = param[1];
    Double_t sigmaT = param[2];

    //Fill Event T0 counter
    (*GetPadTimesEvent(fCurrentSector,kTRUE)).GetMatrixArray()[fCurrentChannel] = meanT;

    //Fill Q histogram
    GetHistoQ(fCurrentSector,kTRUE)->Fill( TMath::Sqrt(Qsum), fCurrentChannel );

    //Fill RMS histogram
    GetHistoRMS(fCurrentSector,kTRUE)->Fill( sigmaT, fCurrentChannel );


    //Fill debugging info
    if ( fDebugLevel>0 ){
	(*GetPadPedestalEvent(fCurrentSector,kTRUE))[fCurrentChannel]=fPadPedestal;
	(*GetPadRMSEvent(fCurrentSector,kTRUE))[fCurrentChannel]=sigmaT;
	(*GetPadQEvent(fCurrentSector,kTRUE))[fCurrentChannel]=Qsum;
    }

    ResetPad();
}
//_____________________________________________________________________
void AliTPCCalibPulser::EndEvent() /*FOLD00*/
{
    //
    //  Process data of current event
    //

    //check if last pad has allready been processed, if not do so
    if ( fMaxTimeBin>-1 ) ProcessPad();

    //loop over all ROCs, fill Time0 histogram corrected for the mean Time0 of each ROC
    for ( Int_t iSec = 0; iSec<72; ++iSec ){
	TVectorF *vTimes = GetPadTimesEvent(iSec);
        if ( !vTimes ) continue;

	for ( UInt_t iChannel=0; iChannel<fROC->GetNChannels(iSec); ++iChannel ){
	    Float_t Time0 = fVTime0Offset[iSec]/fVTime0OffsetCounter[iSec];
	    Float_t Time  = (*vTimes).GetMatrixArray()[iChannel];

            GetHistoT0(iSec,kTRUE)->Fill( Time-Time0,iChannel );


	    //Debug start
	    if ( fDebugLevel>0 ){
		if ( !fDebugStreamer ) {
                        //debug stream
		    TDirectory *backup = gDirectory;
		    fDebugStreamer = new TTreeSRedirector("deb2.root");
		    if ( backup ) backup->cd();  //we don't want to be cd'd to the debug streamer
		}

		Int_t row=0;
		Int_t pad=0;
		Int_t padc=0;

		Float_t Q   = (*GetPadQEvent(iSec)).GetMatrixArray()[iChannel];
                Float_t RMS = (*GetPadRMSEvent(iSec)).GetMatrixArray()[iChannel];

		UInt_t channel=iChannel;
		Int_t sector=iSec;

		while ( channel > (fROC->GetRowIndexes(sector)[row]+fROC->GetNPads(sector,row)-1) ) row++;
		pad = channel-fROC->GetRowIndexes(sector)[row];
		padc = pad-(fROC->GetNPads(sector,row)/2);

		TH1F *h1 = new TH1F(Form("hSignalD%d.%d.%d",sector,row,pad),
				    Form("hSignalD%d.%d.%d",sector,row,pad),
				    fLastTimeBin-fFirstTimeBin,
				    fFirstTimeBin,fLastTimeBin);
		h1->SetDirectory(0);

		for (Int_t i=fFirstTimeBin; i<fLastTimeBin+1; ++i)
		    h1->Fill(i,fPadSignal(i));

		(*fDebugStreamer) << "DataPad" <<
		    "Event=" << fEvent <<
		    "Sector="<< sector <<
		    "Row="   << row<<
		    "Pad="   << pad <<
		    "PadC="  << padc <<
		    "PadSec="<< channel <<
		    "Time0="  << Time0 <<
		    "Time="  << Time <<
		    "RMS="   << RMS <<
		    "Sum="   << Q <<
		    "hist.=" << h1 <<
		    "\n";

		delete h1;
	    }
	    //Debug end

	}
    }

}
//_____________________________________________________________________
Bool_t AliTPCCalibPulser::ProcessEvent(AliTPCRawStream *rawStream) /*FOLD00*/
{
  //
  // Event Processing loop - AliTPCRawStream
  //

  rawStream->SetOldRCUFormat(fOldRCUformat);

  ResetEvent();

  Bool_t withInput = kFALSE;

  while (rawStream->Next()) {

      Int_t isector  = rawStream->GetSector();                       //  current sector
      Int_t iRow     = rawStream->GetRow();                          //  current row
      Int_t iPad     = rawStream->GetPad();                          //  current pad
      Int_t iTimeBin = rawStream->GetTime();                         //  current time bin
      Float_t signal = rawStream->GetSignal();                       //  current ADC signal

      Update(isector,iRow,iPad,iTimeBin,signal);
      withInput = kTRUE;
  }

  if (withInput){
      EndEvent();
  }

  return withInput;
}
//_____________________________________________________________________
Bool_t AliTPCCalibPulser::ProcessEvent(AliRawReader *rawReader)
{
  //
  //  Event processing loop - AliRawReader
  //


  AliTPCRawStream rawStream(rawReader);

  rawReader->Select("TPC");

  return ProcessEvent(&rawStream);
}
//_____________________________________________________________________
Bool_t AliTPCCalibPulser::ProcessEvent(eventHeaderStruct *event)
{
  //
  //  Event processing loop - date event
  //
    AliRawReader *rawReader = new AliRawReaderDate((void*)event);
    Bool_t result=ProcessEvent(rawReader);
    delete rawReader;
    return result;

}
//_____________________________________________________________________
TH2S* AliTPCCalibPulser::GetHisto(Int_t sector, TObjArray *arr, /*FOLD00*/
				  Int_t nbinsY, Float_t ymin, Float_t ymax,
				  Char_t *type, Bool_t force)
{
    //
    // return pointer to Q histogram
    // if force is true create a new histogram if it doesn't exist allready
    //
    if ( !force || arr->UncheckedAt(sector) )
	return (TH2S*)arr->UncheckedAt(sector);

    // if we are forced and histogram doesn't yes exist create it
    Char_t name[255], title[255];

    sprintf(name,"hCalib%s%.2d",type,sector);
    sprintf(title,"%s calibration histogram sector %.2d",type,sector);

    // new histogram with Q calib information. One value for each pad!
    TH2S* hist = new TH2S(name,title,
			  nbinsY, ymin, ymax,
			  fROC->GetNChannels(sector),0,fROC->GetNChannels(sector));
    hist->SetDirectory(0);
    arr->AddAt(hist,sector);
    return hist;
}
//_____________________________________________________________________
TH2S* AliTPCCalibPulser::GetHistoT0(Int_t sector, Bool_t force) /*FOLD00*/
{
    //
    // return pointer to T0 histogram
    // if force is true create a new histogram if it doesn't exist allready
    //
    TObjArray *arr = &fHistoT0Array;
    return GetHisto(sector, arr, fNbinsT0, fXminT0, fXmaxT0, "T0", force);
}
//_____________________________________________________________________
TH2S* AliTPCCalibPulser::GetHistoQ(Int_t sector, Bool_t force) /*FOLD00*/
{
    //
    // return pointer to Q histogram
    // if force is true create a new histogram if it doesn't exist allready
    //
    TObjArray *arr = &fHistoQArray;
    return GetHisto(sector, arr, fNbinsQ, fXminQ, fXmaxQ, "Q", force);
}
//_____________________________________________________________________
TH2S* AliTPCCalibPulser::GetHistoRMS(Int_t sector, Bool_t force) /*FOLD00*/
{
    //
    // return pointer to Q histogram
    // if force is true create a new histogram if it doesn't exist allready
    //
    TObjArray *arr = &fHistoRMSArray;
    return GetHisto(sector, arr, fNbinsRMS, fXminRMS, fXmaxRMS, "RMS", force);
}
//_____________________________________________________________________
TVectorF* AliTPCCalibPulser::GetPadInfoEvent(Int_t sector, TObjArray *arr, Bool_t force) /*FOLD00*/
{
    //
    // return pointer to Pad Info from 'arr' for the current event and sector
    // if force is true create it if it doesn't exist allready
    //
    if ( !force || arr->UncheckedAt(sector) )
	return (TVectorF*)arr->UncheckedAt(sector);

    TVectorF *vect = new TVectorF(fROC->GetNChannels(sector));
    arr->AddAt(vect,sector);
    return vect;
}
//_____________________________________________________________________
TVectorF* AliTPCCalibPulser::GetPadTimesEvent(Int_t sector, Bool_t force) /*FOLD00*/
{
    //
    // return pointer to Pad Times Array for the current event and sector
    // if force is true create it if it doesn't exist allready
    //
    TObjArray *arr = &fPadTimesArrayEvent;
    return GetPadInfoEvent(sector,arr,force);
}
//_____________________________________________________________________
TVectorF* AliTPCCalibPulser::GetPadQEvent(Int_t sector, Bool_t force) /*FOLD00*/
{
    //
    // return pointer to Pad Q Array for the current event and sector
    // if force is true create it if it doesn't exist allready
    // for debugging purposes only
    //

    TObjArray *arr = &fPadQArrayEvent;
    return GetPadInfoEvent(sector,arr,force);
}
//_____________________________________________________________________
TVectorF* AliTPCCalibPulser::GetPadRMSEvent(Int_t sector, Bool_t force) /*FOLD00*/
{
    //
    // return pointer to Pad RMS Array for the current event and sector
    // if force is true create it if it doesn't exist allready
    // for debugging purposes only
    //
    TObjArray *arr = &fPadRMSArrayEvent;
    return GetPadInfoEvent(sector,arr,force);
}
//_____________________________________________________________________
TVectorF* AliTPCCalibPulser::GetPadPedestalEvent(Int_t sector, Bool_t force) /*FOLD00*/
{
    //
    // return pointer to Pad RMS Array for the current event and sector
    // if force is true create it if it doesn't exist allready
    // for debugging purposes only
    //
    TObjArray *arr = &fPadPedestalArrayEvent;
    return GetPadInfoEvent(sector,arr,force);
}
//_____________________________________________________________________
AliTPCCalROC* AliTPCCalibPulser::GetCalRoc(Int_t sector, TObjArray* arr, Bool_t force) /*FOLD00*/
{
    //
    // return pointer to ROC Calibration
    // if force is true create a new histogram if it doesn't exist allready
    //
    if ( !force || arr->UncheckedAt(sector) )
	return (AliTPCCalROC*)arr->UncheckedAt(sector);

    // if we are forced and histogram doesn't yes exist create it

    // new AliTPCCalROC for T0 information. One value for each pad!
    AliTPCCalROC *croc = new AliTPCCalROC(sector);
    arr->AddAt(croc,sector);
    return croc;
}
//_____________________________________________________________________
AliTPCCalROC* AliTPCCalibPulser::GetCalRocT0(Int_t sector, Bool_t force) /*FOLD00*/
{
    //
    // return pointer to Carge ROC Calibration
    // if force is true create a new histogram if it doesn't exist allready
    //
    TObjArray *arr = &fCalRocArrayT0;
    return GetCalRoc(sector, arr, force);
}
//_____________________________________________________________________
AliTPCCalROC* AliTPCCalibPulser::GetCalRocQ(Int_t sector, Bool_t force) /*FOLD00*/
{
    //
    // return pointer to T0 ROC Calibration
    // if force is true create a new histogram if it doesn't exist allready
    //
    TObjArray *arr = &fCalRocArrayQ;
    return GetCalRoc(sector, arr, force);
}
//_____________________________________________________________________
AliTPCCalROC* AliTPCCalibPulser::GetCalRocRMS(Int_t sector, Bool_t force) /*FOLD00*/
{
    //
    // return pointer to signal width ROC Calibration
    // if force is true create a new histogram if it doesn't exist allready
    //
    TObjArray *arr = &fCalRocArrayRMS;
    return GetCalRoc(sector, arr, force);
}
//_____________________________________________________________________
AliTPCCalROC* AliTPCCalibPulser::GetCalRocOutliers(Int_t sector, Bool_t force)
{
    //
    // return pointer to Outliers
    // if force is true create a new histogram if it doesn't exist allready
    //
    TObjArray *arr = &fCalRocArrayOutliers;
    return GetCalRoc(sector, arr, force);
}
//_____________________________________________________________________
void AliTPCCalibPulser::ResetEvent() /*FOLD00*/
{
    //
    //  Reset global counters  -- Should be called before each event is processed
    //
    fLastSector=-1;
    fCurrentSector=-1;
    fCurrentRow=-1;
    fCurrentChannel=-1;

    ResetPad();

    fPadTimesArrayEvent.Delete();
    fPadQArrayEvent.Delete();
    fPadRMSArrayEvent.Delete();
    fPadPedestalArrayEvent.Delete();

    for ( Int_t i=0; i<72; ++i ){
	fVTime0Offset[i]=0;
	fVTime0OffsetCounter[i]=0;
    }
}
//_____________________________________________________________________
void AliTPCCalibPulser::ResetPad() /*FOLD00*/
{
    //
    //  Reset pad infos -- Should be called after a pad has been processed
    //
    for (Int_t i=fFirstTimeBin; i<fLastTimeBin+1; ++i)
	fPadSignal[i] = 0;
    fMaxTimeBin = -1;
    fMaxPadSignal = -1;
    fPadPedestal  = -1;
    fPadNoise     = -1;
}
//_____________________________________________________________________
void AliTPCCalibPulser::Merge(AliTPCCalibPulser *sig)
{
    //
    //  Merge reference histograms of sig to the current AliTPCCalibPulser
    //

    //merge histograms
    for (Int_t iSec=0; iSec<72; ++iSec){
	TH2S *hRefQmerge   = sig->GetHistoQ(iSec);
	TH2S *hRefT0merge  = sig->GetHistoT0(iSec);
	TH2S *hRefRMSmerge = sig->GetHistoRMS(iSec);


	if ( hRefQmerge ){
	    TDirectory *dir = hRefQmerge->GetDirectory(); hRefQmerge->SetDirectory(0);
	    TH2S *hRefQ   = GetHistoQ(iSec);
	    if ( hRefQ ) hRefQ->Add(hRefQmerge);
	    else {
		TH2S *hist = new TH2S(*hRefQmerge);
                hist->SetDirectory(0);
		fHistoQArray.AddAt(hist, iSec);
	    }
	    hRefQmerge->SetDirectory(dir);
	}
	if ( hRefT0merge ){
	    TDirectory *dir = hRefT0merge->GetDirectory(); hRefT0merge->SetDirectory(0);
	    TH2S *hRefT0  = GetHistoT0(iSec);
	    if ( hRefT0 ) hRefT0->Add(hRefT0merge);
	    else {
		TH2S *hist = new TH2S(*hRefT0merge);
                hist->SetDirectory(0);
		fHistoT0Array.AddAt(hist, iSec);
	    }
	    hRefT0merge->SetDirectory(dir);
	}
	if ( hRefRMSmerge ){
	    TDirectory *dir = hRefRMSmerge->GetDirectory(); hRefRMSmerge->SetDirectory(0);
	    TH2S *hRefRMS = GetHistoRMS(iSec);
	    if ( hRefRMS ) hRefRMS->Add(hRefRMSmerge);
	    else {
		TH2S *hist = new TH2S(*hRefRMSmerge);
                hist->SetDirectory(0);
		fHistoRMSArray.AddAt(hist, iSec);
	    }
	    hRefRMSmerge->SetDirectory(dir);
	}

    }
}
//_____________________________________________________________________
void AliTPCCalibPulser::Analyse() /*FOLD00*/
{
    //
    //  Calculate calibration constants
    //

    TVectorD paramQ(3);
    TVectorD paramT0(3);
    TVectorD paramRMS(3);
    TMatrixD dummy(3,3);

    for (Int_t iSec=0; iSec<72; ++iSec){
	TH2S *hT0 = GetHistoT0(iSec);
        if (!hT0 ) continue;

	AliTPCCalROC *rocQ   = GetCalRocQ  (iSec,kTRUE);
	AliTPCCalROC *rocT0  = GetCalRocT0 (iSec,kTRUE);
	AliTPCCalROC *rocRMS = GetCalRocRMS(iSec,kTRUE);
        AliTPCCalROC *rocOut = GetCalRocOutliers(iSec,kTRUE);

	TH2S *hQ   = GetHistoQ(iSec);
	TH2S *hRMS = GetHistoRMS(iSec);

	Short_t *array_hQ   = hQ->GetArray();
	Short_t *array_hT0  = hT0->GetArray();
	Short_t *array_hRMS = hRMS->GetArray();

        UInt_t nChannels = fROC->GetNChannels(iSec);

	//debug
	Int_t row=0;
	Int_t pad=0;
	Int_t padc=0;
	//! debug

	for (UInt_t iChannel=0; iChannel<nChannels; ++iChannel){


	    Float_t cogTime0 = -1000;
	    Float_t cogQ     = -1000;
	    Float_t cogRMS   = -1000;
            Float_t cogOut   = 0;


	    Int_t offsetQ = (fNbinsQ+2)*(iChannel+1)+1;
	    Int_t offsetT0 = (fNbinsT0+2)*(iChannel+1)+1;
	    Int_t offsetRMS = (fNbinsRMS+2)*(iChannel+1)+1;

/*
	    AliMathBase::FitGaus(array_hQ+offsetQ,fNbinsQ,fXminQ,fXmaxQ,&paramQ,&dummy);
	    AliMathBase::FitGaus(array_hT0+offsetT0,fNbinsT0,fXminT0,fXmaxT0,&paramT0,&dummy);
            AliMathBase::FitGaus(array_hRMS+offsetRMS,fNbinsRMS,fXminRMS,fXmaxRMS,&paramRMS,&dummy);
	    cogQ     = paramQ[1];
	    cogTime0 = paramT0[1];
	    cogRMS   = paramRMS[1];
*/
	    cogQ     = AliMathBase::GetCOG(array_hQ+offsetQ,fNbinsQ,fXminQ,fXmaxQ);
	    cogTime0 = AliMathBase::GetCOG(array_hT0+offsetT0,fNbinsT0,fXminT0,fXmaxT0);
            cogRMS   = AliMathBase::GetCOG(array_hRMS+offsetRMS,fNbinsRMS,fXminRMS,fXmaxRMS);



	    /*
	    if ( (cogQ < ??) && (cogTime0 > ??) && (cogTime0<??) && ( cogRMS>??) ){
		cogOut = 1;
		cogTime0 = 0;
		cogQ     = 0;
		cogRMS   = 0;
	    }
*/
       	    rocQ->SetValue(iChannel, cogQ*cogQ);
	    rocT0->SetValue(iChannel, cogTime0);
	    rocRMS->SetValue(iChannel, cogRMS);
	    rocOut->SetValue(iChannel, cogOut);


	    //debug
	    if ( fDebugLevel > 0 ){
		if ( !fDebugStreamer ) {
                        //debug stream
		    TDirectory *backup = gDirectory;
		    fDebugStreamer = new TTreeSRedirector("deb2.root");
		    if ( backup ) backup->cd();  //we don't want to be cd'd to the debug streamer
		}

		while ( iChannel > (fROC->GetRowIndexes(iSec)[row]+fROC->GetNPads(iSec,row)-1) ) row++;
		pad = iChannel-fROC->GetRowIndexes(iSec)[row];
		padc = pad-(fROC->GetNPads(iSec,row)/2);

		(*fDebugStreamer) << "DataEnd" <<
		    "Sector="  << iSec      <<
		    "Pad="     << pad       <<
		    "PadC="    << padc      <<
		    "Row="     << row       <<
		    "PadSec="  << iChannel   <<
		    "Q="       << cogQ      <<
		    "T0="      << cogTime0  <<
		    "RMS="     << cogRMS    <<
		    "\n";
	    }
	    //! debug

	}

    }
    delete fDebugStreamer;
    fDebugStreamer = 0x0;
}
//_____________________________________________________________________
void AliTPCCalibPulser::DumpToFile(const Char_t *filename, const Char_t *dir, Bool_t append)
{
    //
    //  Write class to file
    //

    TString sDir(dir);
    TString option;

    if ( append )
	option = "update";
    else
        option = "recreate";

    TDirectory *backup = gDirectory;
    TFile f(filename,option.Data());
    f.cd();
    if ( !sDir.IsNull() ){
	f.mkdir(sDir.Data());
	f.cd(sDir);
    }
    this->Write();
    f.Close();

    if ( backup ) backup->cd();
}
//_____________________________________________________________________
//_________________________  Test Functions ___________________________
//_____________________________________________________________________
TObjArray* AliTPCCalibPulser::TestBinning()
{
    //
    //  Function to test the binning of the reference histograms
    //  type: T0, Q or RMS
    //  mode: 0 - number of filled bins per channel
    //        1 - number of empty bins between filled bins in one ROC
    //  returns TObjArray with the test histograms type*2+mode:
    //  position 0 = T0,0 ; 1 = T0,1 ; 2 = Q,0 ...


    TObjArray *histArray = new TObjArray(6);
    const Char_t *type[] = {"T0","Q","RMS"};
    Int_t fNbins[3] = {fNbinsT0,fNbinsQ,fNbinsRMS};

    for (Int_t itype = 0; itype<3; ++itype){
	for (Int_t imode=0; imode<2; ++imode){
            Int_t icount = itype*2+imode;
	    histArray->AddAt(new TH1F(Form("hTestBinning%s%d",type[itype],imode),
				      Form("Test Binning of '%s', mode - %d",type[itype],imode),
				      72,0,72),
                             icount);
	}
    }


    TH2S *hRef=0x0;
    Short_t *array=0x0;
    for (Int_t itype = 0; itype<3; ++itype){
	for (Int_t iSec=0; iSec<72; ++iSec){
	    if ( itype == 0 ) hRef = GetHistoT0(iSec);
	    if ( itype == 1 ) hRef = GetHistoQ(iSec);
            if ( itype == 2 ) hRef = GetHistoRMS(iSec);
	    if ( hRef == 0x0 ) continue;
	    array = (hRef->GetArray());
	    UInt_t nChannels = fROC->GetNChannels(iSec);

	    Int_t nempty=0;
	    for (UInt_t iChannel=0; iChannel<nChannels; ++iChannel){
		Int_t nfilled=0;
		Int_t offset = (fNbins[itype]+2)*(iChannel+1)+1;
		Int_t c1 = 0;
		Int_t c2 = 0;
		for (Int_t iBin=0; iBin<fNbins[itype]; ++iBin){
		    if ( array[offset+iBin]>0 ) {
			nfilled++;
			if ( c1 && c2 ) nempty++;
			else c1 = 1;
		    }
		    else if ( c1 ) c2 = 1;

		}
		((TH1F*)histArray->At(itype*2))->Fill(nfilled);
	    }
	    ((TH1F*)histArray->At(itype*2+1))->Fill(iSec,nempty);
	}
    }
    return histArray;
}
