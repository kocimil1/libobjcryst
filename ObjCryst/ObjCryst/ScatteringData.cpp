/*  ObjCryst++ Object-Oriented Crystallographic Library
    (c) 2000-2002 Vincent Favre-Nicolin vincefn@users.sourceforge.net
        2000-2001 University of Geneva (Switzerland)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/*
*  source file for LibCryst++ ScatteringData class
*
*/

#include <cmath>

#include <typeinfo>

#include "ObjCryst/ScatteringData.h"
#include "Quirks/VFNDebug.h"
#include "Quirks/VFNStreamFormat.h"

#ifdef __WX__CRYST__
   #include "wxCryst/wxPowderPattern.h"
   #include "wxCryst/wxRadiation.h"
#endif

#include <fstream>
#include <iomanip>
#include <stdio.h> //for sprintf()

namespace ObjCryst
{

extern "C"
{
//XRay Tubes wavelengths from atominfo
//AtomInfo (c) 1994-96 Ralf W. Grosse-Kunstleve 
#include "atominfo/atominfo.h"
}
const RefParType *gpRefParTypeScattData
   = new RefParType(gpRefParTypeObjCryst,"Scattering Data");
const RefParType *gpRefParTypeScattDataScale
   = new RefParType(gpRefParTypeObjCryst,"Scale Factor");
const RefParType *gpRefParTypeScattDataProfile
   = new RefParType(gpRefParTypeScattData,"Profile");
const RefParType *gpRefParTypeScattDataProfileType
   = new RefParType(gpRefParTypeScattDataProfile,"Type");
const RefParType *gpRefParTypeScattDataProfileWidth
   = new RefParType(gpRefParTypeScattDataProfile,"Width");
const RefParType *gpRefParTypeScattDataProfileAsym
   = new RefParType(gpRefParTypeScattDataProfile,"Asymmetry");
const RefParType *gpRefParTypeScattDataCorr
   = new RefParType(gpRefParTypeScattData,"Correction");
const RefParType *gpRefParTypeScattDataCorrInt
   = new RefParType(gpRefParTypeScattDataCorr,"Intensities");
const RefParType *gpRefParTypeScattDataCorrIntAbsorp
   = new RefParType(gpRefParTypeScattDataCorrInt,"Absorption");
const RefParType *gpRefParTypeScattDataCorrIntPolar
   = new RefParType(gpRefParTypeScattDataCorrInt,"Polarization");
const RefParType *gpRefParTypeScattDataCorrIntExtinc
   = new RefParType(gpRefParTypeScattDataCorrInt,"Extinction");
const RefParType *gpRefParTypeScattDataCorrPos
   = new RefParType(gpRefParTypeScattDataCorr,"Reflections Positions");
const RefParType *gpRefParTypeScattDataBackground
   = new RefParType(gpRefParTypeScattData,"Background");

const RefParType *gpRefParTypeRadiation
   = new RefParType(gpRefParTypeObjCryst,"Radiation");
const RefParType *gpRefParTypeRadiationWavelength
   = new RefParType(gpRefParTypeRadiation,"Wavelength");
//######################################################################
//    Tabulated math functions for faster (&less precise) F(hkl) calculation
//These function are defined and used in cristallo-spacegroup.cpp
//Currently tabulating sine and cosine only
//######################################################################

static bool sLibCrystTabulCosineIsInit=false;
//conversion value
static REAL sLibCrystTabulCosineRatio;
//initialize tabulated values of cosine
void InitLibCrystTabulCosine();
// Number of tabulated values of cosine between [0;2pi]
// 100 000 is far enough for a model search, yielding a maximum
// error less than .05%... 10000 should be enough, too, with (probably) a higher cache hit
#define sLibCrystNbTabulSine 8192
#define sLibCrystNbTabulSineMASK 8191
//storage of tabulated values of cosine, and a table with interlaced csoine/sine values
static REAL *spLibCrystTabulCosine;
static REAL *spLibCrystTabulCosineSine;

void InitLibCrystTabulCosine()
{
   VFN_DEBUG_MESSAGE("InitLibCrystTabulCosine()",10)
   spLibCrystTabulCosine=new REAL[sLibCrystNbTabulSine];
   spLibCrystTabulCosineSine=new REAL[sLibCrystNbTabulSine*2];
   REAL *tmp=spLibCrystTabulCosine;
   sLibCrystTabulCosineRatio=sLibCrystNbTabulSine/2./M_PI;
   for(REAL i=0;i<sLibCrystNbTabulSine;i++) *tmp++ = cos(i/sLibCrystTabulCosineRatio);
   tmp=spLibCrystTabulCosineSine;
   for(REAL i=0;i<sLibCrystNbTabulSine;i++)
   {
      *tmp++ = cos(i/sLibCrystTabulCosineRatio);
      *tmp++ = sin(i/sLibCrystTabulCosineRatio);
   }
}

// Same for exponential calculations (used for global temperature factors)
static bool sLibCrystTabulExpIsInit=false;
void InitLibCrystTabulExp();
static const long sLibCrystNbTabulExp=10000;
static const REAL sLibCrystMinTabulExp=-5.;
static const REAL sLibCrystMaxTabulExp=10.;
static REAL *spLibCrystTabulExp;
void InitLibCrystTabulExp()
{
   VFN_DEBUG_MESSAGE("InitLibCrystTabulExp()",10)
   spLibCrystTabulExp=new REAL[sLibCrystNbTabulExp];
   REAL *tmp=spLibCrystTabulExp;
   for(REAL i=0;i<sLibCrystNbTabulExp;i++) 
      *tmp++ = exp(sLibCrystMinTabulExp+i*(sLibCrystMaxTabulExp-sLibCrystMinTabulExp)/sLibCrystNbTabulExp);
   sLibCrystTabulExpIsInit=true;
}

//:KLUDGE: The allocated memory for cos and sin table is never freed...
// This should be done after the last ScatteringData object is deleted.

////////////////////////////////////////////////////////////////////////
//
//    Radiation
//
////////////////////////////////////////////////////////////////////////
Radiation::Radiation():
mWavelength(1),mXRayTubeName(""),mXRayTubeDeltaLambda(0.),
mXRayTubeAlpha2Alpha1Ratio(0.5),mLinearPolarRate(0)
{
   mWavelength=1;
   this->InitOptions();
   mRadiationType.SetChoice(RAD_XRAY);
   mWavelengthType.SetChoice(WAVELENGTH_MONOCHROMATIC);
   mClockMaster.AddChild(mClockWavelength);
   mClockMaster.AddChild(mClockRadiation);
}

Radiation::Radiation(const RadiationType rad,const REAL wavelength)
{
   this->InitOptions();
   mRadiationType.SetChoice(rad);
   mWavelengthType.SetChoice(WAVELENGTH_MONOCHROMATIC);
   mWavelength=wavelength;
   mXRayTubeName="";
   mXRayTubeDeltaLambda=0.;//useless here
   mXRayTubeAlpha2Alpha1Ratio=0.5;//useless here
   mLinearPolarRate=0.95;//assume it's synchrotron ?
   mClockMaster.AddChild(mClockWavelength);
   mClockMaster.AddChild(mClockRadiation);
}

Radiation::Radiation(const string &XRayTubeElementName,const REAL alpha2Alpha2ratio)
{
   this->InitOptions();
   this->SetWavelength(XRayTubeElementName,alpha2Alpha2ratio);
   mClockMaster.AddChild(mClockWavelength);
   mClockMaster.AddChild(mClockRadiation);
}

Radiation::Radiation(const Radiation &old):
mRadiationType(old.mRadiationType),
mWavelengthType(old.mWavelengthType),
mWavelength(old.mWavelength),
mXRayTubeName(old.mXRayTubeName),
mXRayTubeDeltaLambda(old.mXRayTubeDeltaLambda),
mXRayTubeAlpha2Alpha1Ratio(old.mXRayTubeAlpha2Alpha1Ratio),
mLinearPolarRate(old.mLinearPolarRate)
{
   mClockWavelength.Click();
   mClockMaster.AddChild(mClockWavelength);
   mClockMaster.AddChild(mClockRadiation);
}

Radiation::~Radiation()
{}

const string& Radiation::GetClassName() const
{
   const static string className="Radiation";
   return className;
}

void Radiation::operator=(const Radiation &old)
{
   mRadiationType             =old.mRadiationType;
   mWavelengthType            =old.mWavelengthType;
   mWavelength                =old.mWavelength;
   mXRayTubeName              =old.mXRayTubeName;
   mXRayTubeDeltaLambda       =old.mXRayTubeDeltaLambda;
   mXRayTubeAlpha2Alpha1Ratio =old.mXRayTubeAlpha2Alpha1Ratio;
   mClockWavelength.Click();
   mRadiationType.SetChoice(old.mRadiationType.GetChoice());
}

RadiationType Radiation::GetRadiationType()const 
{return (RadiationType) mRadiationType.GetChoice();}

void Radiation::SetRadiationType(const RadiationType rad)
{
   mRadiationType.SetChoice(rad);
   if(rad == RAD_NEUTRON) mLinearPolarRate=0;
}

WavelengthType Radiation::GetWavelengthType()const
{return (WavelengthType) mWavelengthType.GetChoice();}

const CrystVector_REAL& Radiation::GetWavelength() const {return mWavelength;}
void Radiation::SetWavelength(const REAL l)
{
   mWavelength.resize(1);
   mWavelength=l;
   mClockWavelength.Click();
}
void Radiation::SetWavelength(const string &XRayTubeElementName,
                              const REAL alpha2Alpha2ratio)
{
   VFN_DEBUG_MESSAGE("Radiation::SetWavelength(tubeName,ratio):",5)
   mXRayTubeName=XRayTubeElementName;
   mRadiationType.SetChoice(RAD_XRAY);
   mWavelength.resize(1);
   mLinearPolarRate=0;
   
   if(XRayTubeElementName.length() >=3) //:KLUDGE:
   {
      mWavelengthType.SetChoice(WAVELENGTH_MONOCHROMATIC);
      if(XRayTubeElementName=="CoA1")
      {
         mWavelength=1.78901;
      }
      else
      {
         const T_ChXrayWaveLength *xrayWaveLength;
         xrayWaveLength=ChXrayWaveLengthOf(mXRayTubeName.c_str());
         if(xrayWaveLength==NULL)
         {
            cout << "WARNING: could not interpret X-Ray tube name:"<<XRayTubeElementName<<endl
                 << "         not modifying wavelength !"<<endl;
            return;
         }
         mWavelength=xrayWaveLength->Length;
      }
   }
   else
   {
      mWavelengthType.SetChoice(WAVELENGTH_ALPHA12);
      mXRayTubeAlpha2Alpha1Ratio=alpha2Alpha2ratio;
      REAL lambda1,lambda2;
      if(XRayTubeElementName=="Co")
      {
         lambda1=1.78901;
         lambda2=1.79290;
      }
      else
      {
         const T_ChXrayWaveLength *xrayWaveLength;
         xrayWaveLength=ChXrayWaveLengthOf((mXRayTubeName+"A1").c_str());
         if(xrayWaveLength==NULL)
         {
            cout << "WARNING: could not interpret X-Ray tube name:"<<XRayTubeElementName<<endl
                 << "         not modifying wavelength !"<<endl;
            return;
         }
         lambda1=xrayWaveLength->Length;
         xrayWaveLength=ChXrayWaveLengthOf((mXRayTubeName+"A2").c_str());
         if(xrayWaveLength==NULL)
         {
            cout << "WARNING: could not interpret X-Ray tube name:"<<XRayTubeElementName<<endl
                 << "         not modifying wavelength !"<<endl;
            return;
         }
         lambda2=xrayWaveLength->Length;
      }
      mXRayTubeDeltaLambda=lambda2-lambda1;
      mWavelength=lambda1
            +mXRayTubeDeltaLambda*mXRayTubeAlpha2Alpha1Ratio/(1.+mXRayTubeAlpha2Alpha1Ratio);
   }
   mClockWavelength.Click();
}

REAL Radiation::GetXRayTubeDeltaLambda()const {return mXRayTubeDeltaLambda;}

REAL Radiation::GetXRayTubeAlpha2Alpha1Ratio()const {return mXRayTubeAlpha2Alpha1Ratio;}
      

const RefinableObjClock& Radiation::GetClockWavelength() const {return mClockWavelength;}
const RefinableObjClock& Radiation::GetClockRadiation()const {return mRadiationType.GetClock();}

void Radiation::Print()const
{
   VFN_DEBUG_MESSAGE("Radiation::Print():"<<this->GetName(),5)
   cout << "Radiation:" << " " ;
   
   switch(mRadiationType.GetChoice())
   {
      case RAD_NEUTRON:  cout<< "Neutron,";break;
      case RAD_XRAY:     cout<< "X-Ray,";break;
      case RAD_ELECTRON: cout<< "Electron,";break;
   }
      
   cout << "Wavelength=" <<" ";
   switch(mWavelengthType.GetChoice())
   {
      case WAVELENGTH_MONOCHROMATIC: cout<< "monochromatic:"<<" "<<mWavelength(0) <<endl;break;
      case WAVELENGTH_ALPHA12:     cout  << "tube:"<<" "<<mXRayTubeName<<", Alpha1/Alpha2= "
                                       << mXRayTubeAlpha2Alpha1Ratio<<endl;break;
      case WAVELENGTH_MAD: cout<< "mad"<<" "<<endl;break;
      case WAVELENGTH_DAFS: cout<< "dafs"<<" "<<endl;break;
      case WAVELENGTH_LAUE: cout<< "laue"<<" "<<endl;break;
   }
}

REAL Radiation::GetLinearPolarRate()const{return mLinearPolarRate;}

void Radiation::SetLinearPolarRate(const REAL f){mLinearPolarRate=f;}

void Radiation::InitOptions()
{
   static string RadiationTypeName;
   static string RadiationTypeChoices[2];
   static string WavelengthTypeName;
   static string WavelengthTypeChoices[2];
   
   static bool needInitNames=true;
   if(true==needInitNames)
   {
      RadiationTypeName="Radiation";
      RadiationTypeChoices[0]="Neutron";
      RadiationTypeChoices[1]="X-Ray";
      
      WavelengthTypeName="Spectrum";
      WavelengthTypeChoices[0]="Monochromatic";
      WavelengthTypeChoices[1]="X-Ray Tube";
      //WavelengthTypeChoices[2]="MAD";
      //WavelengthTypeChoices[3]="DAFS";
      //WavelengthTypeChoices[4]="LAUE";
      
      needInitNames=false;//Only once for the class
   }
   mRadiationType.Init(2,&RadiationTypeName,RadiationTypeChoices);
   mWavelengthType.Init(2,&WavelengthTypeName,WavelengthTypeChoices);
   this->AddOption(&mRadiationType);
   this->AddOption(&mWavelengthType);
   
   {//Fixed by default
      RefinablePar tmp("Wavelength",mWavelength.data(),0.05,20.,
                        gpRefParTypeRadiationWavelength,REFPAR_DERIV_STEP_ABSOLUTE,
                        true,true,true,false,1.0);
      tmp.SetDerivStep(1e-4);
      tmp.AssignClock(mClockWavelength);
      this->AddPar(tmp);
   }
}

#ifdef __WX__CRYST__
WXCrystObjBasic* Radiation::WXCreate(wxWindow* parent)
{
   //:TODO: Check mpWXCrystObj==0
   mpWXCrystObj=new WXRadiation(parent,this);
   return mpWXCrystObj;
}
#endif

////////////////////////////////////////////////////////////////////////
//
//    ScatteringData
//
////////////////////////////////////////////////////////////////////////

ScatteringData::ScatteringData():
mNbRefl(0),
mpCrystal(0),mGlobalBiso(0),mUseFastLessPreciseFunc(false),
mIgnoreImagScattFact(false),mMaxSinThetaOvLambda(10)
{
   VFN_DEBUG_MESSAGE("ScatteringData::ScatteringData()",10)
   {//This should be done elsewhere...
      RefinablePar tmp("Global Biso",&mGlobalBiso,-1.,1.,
                        gpRefParTypeScattPowTemperatureIso,REFPAR_DERIV_STEP_ABSOLUTE,
                        true,true,true,false,1.0);
      tmp.SetDerivStep(1e-4);
      tmp.AssignClock(mClockGlobalBiso);
      this->AddPar(tmp);
   }
   mClockMaster.AddChild(mClockHKL);
   mClockMaster.AddChild(mClockGlobalBiso);
   mClockMaster.AddChild(mClockNbReflUsed);
}

ScatteringData::ScatteringData(const ScatteringData &old):
mNbRefl(old.mNbRefl),
mpCrystal(old.mpCrystal),mUseFastLessPreciseFunc(old.mUseFastLessPreciseFunc),
//Do not copy temporary arrays
mClockHKL(old.mClockHKL),
mIgnoreImagScattFact(old.mIgnoreImagScattFact),
mMaxSinThetaOvLambda(old.mMaxSinThetaOvLambda)
{
   VFN_DEBUG_MESSAGE("ScatteringData::ScatteringData(&old)",10)
   mClockStructFactor.Reset();
   mClockTheta.Reset();
   mClockScattFactor.Reset();
   mClockScattFactorResonant.Reset();
   mClockThermicFact.Reset();
   this->SetHKL(old.GetH(),old.GetK(),old.GetL());
   VFN_DEBUG_MESSAGE("ScatteringData::ScatteringData(&old):End",5)
   {//This should be done elsewhere...
      RefinablePar tmp("Global Biso",&mGlobalBiso,-1.,1.,
                        gpRefParTypeScattPowTemperatureIso,REFPAR_DERIV_STEP_ABSOLUTE,
                        true,true,true,false,1.0);
      tmp.SetDerivStep(1e-4);
      tmp.AssignClock(mClockGlobalBiso);
      this->AddPar(tmp);
   }
   mClockMaster.AddChild(mClockHKL);
   mClockMaster.AddChild(mClockGlobalBiso);
   mClockMaster.AddChild(mClockNbReflUsed);
}

ScatteringData::~ScatteringData()
{
   VFN_DEBUG_MESSAGE("ScatteringData::~ScatteringData()",10)
}

void ScatteringData::SetHKL(const CrystVector_REAL &h,
                            const CrystVector_REAL &k,
                            const CrystVector_REAL &l)
{
   VFN_DEBUG_ENTRY("ScatteringData::SetHKL(h,k,l)",5)
   mNbRefl=h.numElements();
   mH=h;
   mK=k;
   mL=l;
   mClockHKL.Click();
   this->PrepareHKLarrays();
   VFN_DEBUG_EXIT("ScatteringData::SetHKL(h,k,l):End",5)
}

void ScatteringData::GenHKLFullSpace2(const REAL maxSTOL,const bool useMultiplicity)
{
   (*fpObjCrystInformUser)("Generating Full HKL list...");
   VFN_DEBUG_ENTRY("ScatteringData::GenHKLFullSpace2()",5)
   TAU_PROFILE("ScatteringData::GenHKLFullSpace2()","void (REAL,bool)",TAU_DEFAULT);
   if(0==mpCrystal)
   {
      throw ObjCrystException("ScatteringData::GenHKLFullSpace2() \
      no crystal assigned yet to this ScatteringData object.");;
   }
   VFN_DEBUG_MESSAGE(" ->Max sin(theta)/lambda="<<maxSTOL \
   << " Using Multiplicity : "<<useMultiplicity,3)
   VFN_DEBUG_MESSAGE("a,b,c:"<<mpCrystal->GetLatticePar(0)\
                     <<","<<mpCrystal->GetLatticePar(1)<<","<<mpCrystal->GetLatticePar(2)<<",",3)
   long maxH,maxK,maxL;
   maxH=(int) (maxSTOL * mpCrystal->GetLatticePar(0)*2+1);
   maxK=(int) (maxSTOL * mpCrystal->GetLatticePar(1)*2+1);
   maxL=(int) (maxSTOL * mpCrystal->GetLatticePar(2)*2+1);
   VFN_DEBUG_MESSAGE("->maxH : " << maxH << "  maxK : " << maxK << "maxL : " << maxL,5)
   mNbRefl=(2*maxH+1)*(2*maxK+1)*(2*maxL+1);
   CrystVector_long H(mNbRefl);
   CrystVector_long K(mNbRefl);
   CrystVector_long L(mNbRefl);
   long i=0;
   for(int h = maxH ; h >= -maxH;h--)
      for(int k = maxK ; k >= -maxK;k--)
         for(int l = maxL ; l>= -maxL;l--)
         {
            H(i)=h;
            K(i)=k;
            L(i)=l;
            i++;
         }
   VFN_DEBUG_MESSAGE("ScatteringData::GenHKLFullSpace2():Finished setting h, k and l...",3)
   this->SetHKL(H,K,L);
   VFN_DEBUG_MESSAGE("ScatteringData::GenHKLFullSpace2()",1)
   this->SortReflectionBySinThetaOverLambda(maxSTOL);
   VFN_DEBUG_MESSAGE("ScatteringData::GenHKLFullSpace2()",1)
   #if 0
   {
      REAL h,k,l;
      for(int i=0;i<mNbRefl;i++)
      {
         h=mH(i);
         k=mK(i);
         l=mL(i);
         if(i<40) cout <<"Reflection:"<<h<<" "<<k<<" "<<l<<endl;
         if(this->GetCrystal().GetSpaceGroup().IsReflSystematicAbsent(h,k,l))
         {
            cout <<"Reflection:"<<h<<" "<<k<<" "<<l<<endl;
            cout <<"     is centric:"<<this->GetCrystal().GetSpaceGroup().IsReflCentric(h,k,l)<<endl;
            cout <<"     is systematiccaly absent:"
                 << this->GetCrystal().GetSpaceGroup().IsReflSystematicAbsent(h,k,l)<<endl;
            cout <<"      list of equivalent reflections:"<<endl
                 << this->GetCrystal().GetSpaceGroup().GetAllEquivRefl(h,k,l)<<endl;
            cout <<"      list of equivalent reflections (exclude Friedels):"<<endl
                 << this->GetCrystal().GetSpaceGroup().GetAllEquivRefl(h,k,l,true)<<endl;
            cout <<"      list of equivalent reflections (impose Friedel law):"<<endl
                 << this->GetCrystal().GetSpaceGroup().GetAllEquivRefl(h,k,l,false,true)<<endl;
         }
      }
   }
   #endif
   if(true==useMultiplicity)
   {
      VFN_DEBUG_MESSAGE("ScatteringData::GenHKLFullSpace2():Multiplicity...",3)
      //OK, now sort reflections to keep or remove
         long nbKeptRefl=0;
         CrystVector_long subscriptKeptRefl(mNbRefl);
         mMultiplicity.resize(mNbRefl);
         CrystVector_bool treatedRefl(mNbRefl);
         long currentBaseRefl=0,testedRefl=0;
         REAL currentSTOL=0;
         REAL h,k,l,h1,k1,l1;
         subscriptKeptRefl=0;
         mMultiplicity=0;
         treatedRefl=false;
      VFN_DEBUG_MESSAGE("ScatteringData::GenHKLFullSpace2():Multiplicity 1",2)
         do
         {
            VFN_DEBUG_MESSAGE("...Multiplicity 2",1)
            if(true==treatedRefl(currentBaseRefl)) continue;
            subscriptKeptRefl(nbKeptRefl)=currentBaseRefl;
            mMultiplicity(nbKeptRefl)=1;
            currentSTOL=mSinThetaLambda(currentBaseRefl);
            treatedRefl(currentBaseRefl)=true;
            h=mH(currentBaseRefl);
            k=mK(currentBaseRefl);
            l=mL(currentBaseRefl);
            testedRefl=currentBaseRefl+1;
            if(testedRefl==mNbRefl) break;
            bool test;
            int equiv;
            do
            {
               VFN_DEBUG_MESSAGE("...Multiplicity 3, IgnoreImagScattFact="<<mIgnoreImagScattFact,1)
               h1=mH(testedRefl);
               k1=mK(testedRefl);
               l1=mL(testedRefl);
               equiv=this->GetCrystal().GetSpaceGroup().AreReflEquiv(h,k,l,h1,k1,l1);
               if( (equiv==1) || ((equiv==2)&&(mIgnoreImagScattFact==true)))
               {
                  mMultiplicity(nbKeptRefl) +=1;
                  treatedRefl(testedRefl)=true;
                  
                  //keep the reflection with 0) max indices positive then 
                  //1)max H, 2)max K and 3) max L
                  VFN_DEBUG_MESSAGE("...Multiplicity 5",1)
                  VFN_DEBUG_MESSAGE(h1<<","<<k1<<","<<l1<<",",1)
                  VFN_DEBUG_MESSAGE(fabs(h1)<<","<<fabs(k1)<<","<<fabs(l1)<<",",1)
                  if( ((int)(h1/fabs(h1+.001)+k1/fabs(k1+.001)+l1/fabs(l1+.001)))
                        > ((int)(h/fabs(h+.001)+k/fabs(k+.001)+l/fabs(l+.001))) )
                  {
                     VFN_DEBUG_MESSAGE("...Multiplicity 6a",1)
                     subscriptKeptRefl(nbKeptRefl)=testedRefl;
                     h=h1;
                     k=k1;
                     l=l1;
                  } else
                  {
                     VFN_DEBUG_MESSAGE("...Multiplicity 6b",1)
                     if( (int)(h1/fabs(h1+.001)+k1/fabs(k1+.001)+l1/fabs(l1+.001))
                                 == (int)(h/fabs(h+.001)+k/fabs(k+.001)+l/fabs(l+.001)) )
                     {
                        if(  (mH(testedRefl) > mH(subscriptKeptRefl(nbKeptRefl)))  ||

                            ((mH(testedRefl) == mH(subscriptKeptRefl(nbKeptRefl))) &&
                             (mK(testedRefl) > mK(subscriptKeptRefl(nbKeptRefl)))) ||

                            ((mH(testedRefl) == mH(subscriptKeptRefl(nbKeptRefl))) &&
                             (mK(testedRefl) == mK(subscriptKeptRefl(nbKeptRefl))) &&
                             (mL(testedRefl) > mL(subscriptKeptRefl(nbKeptRefl)))) )
                        {
                           subscriptKeptRefl(nbKeptRefl)=testedRefl;
                           h=h1;
                           k=k1;
                           l=l1;
                        }
                     }
                  }
                  //cout << currentSTOL*RAD2DEG << "  " <<
                  //       mIntH(subscriptKeptRefl(nbKeptRefl))<<"  "<<
                  //       mIntK(subscriptKeptRefl(nbKeptRefl))<<"  "<<
                  //       mIntL(subscriptKeptRefl(nbKeptRefl))<<"  ";
                  VFN_DEBUG_MESSAGE(mIntH(testedRefl)<<"  "<< mIntK(testedRefl)<<"  "<<mIntL(testedRefl),1);
                  //cout << "   " << compare << "  " << h/fabs(h)+k/fabs(k)+l/fabs(l) << endl;
               }
               VFN_DEBUG_MESSAGE("...Multiplicity 5",1)
               testedRefl++;
               if(testedRefl<mNbRefl)
               {
                  if(fabs(currentSTOL-mSinThetaLambda(testedRefl)) < .002) test=true;
                  else test=false;
               }
               else test=false;
            } while(test);
            nbKeptRefl++;
         } while( ++currentBaseRefl < mNbRefl);
      VFN_DEBUG_MESSAGE("ScatteringData::GenHKLFullSpace2():Multiplicity 2",2)
      //Keep only the elected reflections
         mNbRefl=nbKeptRefl;
         {
            CrystVector_REAL oldH,oldK,oldL;
            CrystVector_REAL oldWeight;
            long subs;
            
            oldH=mH;
            oldK=mK;
            oldL=mL;
            
            mMultiplicity.resizeAndPreserve(mNbRefl);
            subscriptKeptRefl.resizeAndPreserve(mNbRefl);
            mH.resize(mNbRefl);
            mK.resize(mNbRefl);
            mL.resize(mNbRefl);
            for(long i=0;i<mNbRefl;i++)
            {
               subs=subscriptKeptRefl(i);
               mH(i)=oldH(subs);
               mK(i)=oldK(subs);
               mL(i)=oldL(subs);
            }
         }
         this->PrepareHKLarrays();
         //this->CalcSinThetaLambda(true);
      // Eliminate extinct reflections now
         this->EliminateExtinctReflections();
   } //true==useMultiplicity
   else
   {
      mMultiplicity.resize(mNbRefl);
      mMultiplicity=1;
      this->EliminateExtinctReflections();
   }
   mClockHKL.Click();
   {
      char buf [200];
      sprintf(buf,"Generating Full HKL list...Done (kept %d reflections)",(int)mNbRefl);
      (*fpObjCrystInformUser)((string)buf);
   }
   VFN_DEBUG_EXIT("ScatteringData::GenHKLFullSpace2():End",5)
}

void ScatteringData::GenHKLFullSpace(const REAL maxTheta,const bool useMultiplicity)
{
   VFN_DEBUG_ENTRY("ScatteringData::GenHKLFullSpace()",5)
   if(this->GetRadiation().GetWavelength()(0) <=.01)
   {
      throw ObjCrystException("ScatteringData::GenHKLFullSpace() \
      no wavelength assigned yet to this ScatteringData object.");;
   }
   this->GenHKLFullSpace2(sin(maxTheta)/this->GetRadiation().GetWavelength()(0),useMultiplicity);
   VFN_DEBUG_EXIT("ScatteringData::GenHKLFullSpace()",5)
}

RadiationType ScatteringData::GetRadiationType()const {return this->GetRadiation().GetRadiationType();}

void ScatteringData::SetCrystal(Crystal &crystal)
{
   VFN_DEBUG_MESSAGE("ScatteringData::SetCrystal()",5)
   mpCrystal=&crystal;
   this->AddSubRefObj(crystal);
   crystal.RegisterClient(*this);
   mClockMaster.AddChild(mpCrystal->GetClockLatticePar());
   mClockGeomStructFact.Reset();
   mClockStructFactor.Reset();
}
const Crystal& ScatteringData::GetCrystal()const {return *mpCrystal;}
Crystal& ScatteringData::GetCrystal() {return *mpCrystal;}

long ScatteringData::GetNbRefl() const {return mNbRefl;}

const CrystVector_REAL& ScatteringData::GetH() const {return mH;}
const CrystVector_REAL& ScatteringData::GetK() const {return mK;}
const CrystVector_REAL& ScatteringData::GetL() const {return mL;}

const CrystVector_REAL& ScatteringData::GetH2Pi() const {return mH2Pi;}
const CrystVector_REAL& ScatteringData::GetK2Pi() const {return mK2Pi;}
const CrystVector_REAL& ScatteringData::GetL2Pi() const {return mH2Pi;}

const CrystVector_REAL& ScatteringData::GetReflX() const
{
   VFN_DEBUG_ENTRY("ScatteringData::GetReflX()",1)
   this->CalcSinThetaLambda();
   VFN_DEBUG_EXIT("ScatteringData::GetReflX()",1)
   return mX;
}
const CrystVector_REAL& ScatteringData::GetReflY() const
{
   VFN_DEBUG_ENTRY("ScatteringData::GetReflY()",1)
   this->CalcSinThetaLambda();
   VFN_DEBUG_EXIT("ScatteringData::GetReflY()",1)
   return mY;
}
const CrystVector_REAL& ScatteringData::GetReflZ() const
{
   VFN_DEBUG_ENTRY("ScatteringData::GetReflZ()",1)
   this->CalcSinThetaLambda();
   VFN_DEBUG_EXIT("ScatteringData::GetReflZ()",1)
   return mZ;
}

const CrystVector_REAL& ScatteringData::GetSinThetaOverLambda()const
{
   VFN_DEBUG_ENTRY("ScatteringData::GetSinThetaOverLambda()",1)
   this->CalcSinThetaLambda();
   VFN_DEBUG_EXIT("ScatteringData::GetSinThetaOverLambda()",1)
   return mSinThetaLambda;
}

const CrystVector_REAL& ScatteringData::GetTheta()const
{
   VFN_DEBUG_ENTRY("ScatteringData::GetTheta()",1)
   this->CalcSinThetaLambda();
   VFN_DEBUG_EXIT("ScatteringData::GetTheta()",1)
   return mTheta;
}

const RefinableObjClock& ScatteringData::GetClockTheta()const
{
   return mClockTheta;
}

const CrystVector_REAL& ScatteringData::GetFhklCalcSq() const
{
   VFN_DEBUG_ENTRY("ScatteringData::GetFhklCalcSq()",2)
   this->CalcStructFactor();
   if(mClockStructFactorSq>mClockStructFactor) return mFhklCalcSq;
   #ifdef __LIBCRYST_VECTOR_USE_BLITZ__
   mFhklCalcSq=pow2(mFhklCalcReal)+pow2(mFhklCalcImag);
   #else
   const REAL *pr,*pi;
   REAL *p;
   pr=mFhklCalcReal.data();
   pi=mFhklCalcImag.data();
   p=mFhklCalcSq.data();
   for(long i=0;i<mNbReflUsed;i++)
   {
      *p++ = *pr * *pr + *pi * *pi;
      pr++;
      pi++;
   }
   #endif
   mClockStructFactorSq.Click();
   VFN_DEBUG_EXIT("ScatteringData::GetFhklCalcSq()",2)
   return mFhklCalcSq;
}
const CrystVector_REAL& ScatteringData::GetFhklCalcReal() const
{
   VFN_DEBUG_ENTRY("ScatteringData::GetFhklCalcReal()",2)
   this->CalcStructFactor();
   VFN_DEBUG_EXIT("ScatteringData::GetFhklCalcReal()",2)
   return mFhklCalcReal;
}

const CrystVector_REAL& ScatteringData::GetFhklCalcImag() const
{
   VFN_DEBUG_ENTRY("ScatteringData::GetFhklCalcImag()",2)
   this->CalcStructFactor();
   VFN_DEBUG_EXIT("ScatteringData::GetFhklCalcImag()",2)
   return mFhklCalcImag;
}

CrystVector_REAL ScatteringData::GetWavelength()const {return this->GetRadiation().GetWavelength();}
#if 0
void ScatteringData::SetUseFastLessPreciseFunc(const bool useItOrNot)
{
   mUseFastLessPreciseFunc=useItOrNot;
   mClockGeomStructFact.Reset();
   mClockStructFactor.Reset();
}
#endif
void ScatteringData::SetIsIgnoringImagScattFact(const bool b)
{
   mIgnoreImagScattFact=b;
   mClockGeomStructFact.Reset();
   mClockStructFactor.Reset();
}
bool ScatteringData::IsIgnoringImagScattFact() const {return mIgnoreImagScattFact;}

void ScatteringData::PrintFhklCalc(ostream &os)const
{
   VFN_DEBUG_ENTRY("ScatteringData::PrintFhklCalc()",5)
   this->GetFhklCalcSq();
   CrystVector_REAL theta;
   theta=mTheta;
   theta *= RAD2DEG;
   os <<" Number of reflections:"<<mNbRefl<<endl;
   os <<"       H        K        L     F(hkl)^2     Re(F)         Im(F)";
   os <<"        Theta       1/2d"<<endl;
   os << FormatVertVectorHKLFloats<REAL>
               (mH,mK,mL,mFhklCalcSq,mFhklCalcReal,mFhklCalcImag,theta,mSinThetaLambda,12,4);
   VFN_DEBUG_EXIT("ScatteringData::PrintFhklCalc()",5)
}

void ScatteringData::PrintFhklCalcDetail(ostream &os)const
{
   VFN_DEBUG_ENTRY("ScatteringData::PrintFhklCalcDetail()",5)
   this->GetFhklCalcSq();
   CrystVector_REAL theta;
   theta=mTheta;
   theta *= RAD2DEG;
   vector<const CrystVector_REAL *> v;
   v.push_back(&mH);
   v.push_back(&mK);
   v.push_back(&mL);
   v.push_back(&mSinThetaLambda);
   v.push_back(&theta);
   v.push_back(&mFhklCalcSq);
   v.push_back(&mFhklCalcReal);
   v.push_back(&mFhklCalcImag);
   os <<" Number of reflections:"<<mNbRefl<<endl;
   os <<"       H        K        L       1/2d        Theta       F(hkl)^2";
   os <<"     Re(F)         Im(F)       ";
   vector<CrystVector_REAL> sf;
   sf.resize(mvRealGeomSF.size()*2);
   map<const ScatteringPower*,CrystVector_REAL>::const_iterator pos=mvRealGeomSF.begin();
   long i=0;
   for(map<const ScatteringPower*,CrystVector_REAL>::const_iterator 
         pos=mvRealGeomSF.begin();pos!=mvRealGeomSF.end();++pos)
   {
      os << FormatString("Re(F)_"+pos->first->GetName(),14)
         << FormatString("Im(F)_"+pos->first->GetName(),14);
      cout<<pos->first->GetName()<<":"<<pos->first->GetForwardScatteringFactor(RAD_XRAY)<<endl;
      sf[2*i]  = mvRealGeomSF[pos->first];
      sf[2*i] *= mvScatteringFactor[pos->first];
      sf[2*i] *= mvTemperatureFactor[pos->first];
      sf[2*i+1]  = mvImagGeomSF[pos->first];
      sf[2*i+1] *= mvScatteringFactor[pos->first];
      sf[2*i+1] *= mvTemperatureFactor[pos->first];
      v.push_back(&(sf[2*i]));
      v.push_back(&(sf[2*i+1]));
      //v.push_back(mvRealGeomSF[pos->first]);
      //v.push_back(mvImagGeomSF[pos->first]);
      //v.push_back(mvScatteringFactor[pos->first]);
      i++;
   }
   os<<endl;
   os << FormatVertVectorHKLFloats<REAL>(v,12,4);
   VFN_DEBUG_EXIT("ScatteringData::PrintFhklCalcDetail()",5)
}

void ScatteringData::BeginOptimization(const bool allowApproximations,
                                       const bool enableRestraints)
{
   if(mUseFastLessPreciseFunc!=allowApproximations)
   {
      mClockGeomStructFact.Reset();
      mClockStructFactor.Reset();
   }
   mUseFastLessPreciseFunc=allowApproximations;
   this->RefinableObj::BeginOptimization(allowApproximations,enableRestraints);
}
void ScatteringData::EndOptimization()
{
   if(mUseFastLessPreciseFunc==true)
   {
      mClockGeomStructFact.Reset();
      mClockStructFactor.Reset();
   }
   mUseFastLessPreciseFunc=false;
   this->RefinableObj::EndOptimization();
}

void ScatteringData::PrepareHKLarrays()
{
   VFN_DEBUG_ENTRY("ScatteringData::PrepareHKLarrays()"<<mNbRefl<<" reflections",5)
   mFhklCalcReal.resize(mNbRefl);
   mFhklCalcImag.resize(mNbRefl);
   mFhklCalcSq.resize(mNbRefl);
   
   mIntH=mH;
   mIntK=mK;
   mIntL=mL;
   
   mH2Pi=mH;
   mK2Pi=mK;
   mL2Pi=mL;
   mH2Pi*=(2*M_PI);
   mK2Pi*=(2*M_PI);
   mL2Pi*=(2*M_PI);
   
   mNbReflUsed=mNbRefl;
   
   mExpectedIntensityFactor.resize(mNbRefl);
   for(long i=0;i<mNbRefl;i++)
   {
      mExpectedIntensityFactor(i)=
         mpCrystal->GetSpaceGroup().GetExpectedIntensityFactor(mH(i),mK(i),mL(i));
   }
   /*
   {
      mpCrystal->GetSpaceGroup().Print();
      for(long i=0;i<mNbRefl;i++)
      {
         cout<<mIntH(i)<<" "<<mIntK(i)<<" "<<mIntL(i)<<" "<<mExpectedIntensityFactor(i)<<endl;
      }
   }
   */

   mClockHKL.Click();
   VFN_DEBUG_EXIT("ScatteringData::PrepareHKLarrays()"<<mNbRefl<<" reflections",5)
}

void ScatteringData::SetMaxSinThetaOvLambda(const REAL max){mMaxSinThetaOvLambda=max;}
REAL ScatteringData::GetMaxSinThetaOvLambda()const{return mMaxSinThetaOvLambda;}
long ScatteringData::GetNbReflBelowMaxSinThetaOvLambda()const
{
   VFN_DEBUG_MESSAGE("ScatteringData::GetNbReflBelowMaxSinThetaOvLambda()",4)
   this->CalcSinThetaLambda();
   if((mNbReflUsed>0)&&(mNbReflUsed<mNbRefl))
   {
      if(  (mSinThetaLambda(mNbReflUsed  )>mMaxSinThetaOvLambda)
         &&(mSinThetaLambda(mNbReflUsed-1)<=mMaxSinThetaOvLambda)) return mNbReflUsed;
   }
   
   if((mNbReflUsed==mNbRefl)&&(mSinThetaLambda(mNbRefl-1)<=mMaxSinThetaOvLambda))
      return mNbReflUsed;
   
   long i;
   for(i=0;i<mNbRefl;i++) if(mSinThetaLambda(i)>mMaxSinThetaOvLambda) break;
   if(i!=mNbReflUsed)
   {
      mNbReflUsed=i;
      mClockNbReflUsed.Click();
      VFN_DEBUG_MESSAGE("->Changed Max sin(theta)/lambda="<<mMaxSinThetaOvLambda\
                        <<" nb refl="<<mNbReflUsed,4)
   }
   return mNbReflUsed;
}
const RefinableObjClock& ScatteringData::GetClockNbReflBelowMaxSinThetaOvLambda()const
{return mClockNbReflUsed;}

CrystVector_long ScatteringData::SortReflectionBySinThetaOverLambda(const REAL maxSTOL)
{
   TAU_PROFILE("ScatteringData::SortReflectionBySinThetaOverLambda()","void ()",TAU_DEFAULT);
   VFN_DEBUG_ENTRY("ScatteringData::SortReflectionBySinThetaOverLambda()",5)
   this->CalcSinThetaLambda();
   CrystVector_long sortedSubs;
   sortedSubs=SortSubs(mSinThetaLambda);
   CrystVector_long oldH,oldK,oldL;
   oldH=mH;
   oldK=mK;
   oldL=mL;
   long subs;
   long shift=0;
   
   //get rid of [0,0,0] reflection
   VFN_DEBUG_MESSAGE("ScatteringData::SortReflectionBySinThetaOverLambda() 1",2)
   if(0==mSinThetaLambda(sortedSubs(0)))
   {
      shift=1;
      mNbRefl -= 1;
      mH.resize(mNbRefl);
      mK.resize(mNbRefl);
      mL.resize(mNbRefl);
   }
   VFN_DEBUG_MESSAGE("ScatteringData::SortReflectionBySinThetaOverLambda() 2",2)
   for(long i=0;i<mNbRefl;i++)
   {
      subs=sortedSubs(i+shift);
      mH(i)=oldH(subs);
      mK(i)=oldK(subs);
      mL(i)=oldL(subs);
   }
   mClockHKL.Click();
   VFN_DEBUG_MESSAGE("ScatteringData::SortReflectionBySinThetaOverLambda() 3",2)
   this->PrepareHKLarrays();
   this->CalcSinThetaLambda();
   
   VFN_DEBUG_MESSAGE("ScatteringData::SortReflectionBySinThetaOverLambda() 4",2)
   if(0<maxSTOL)
   {
      VFN_DEBUG_MESSAGE("ScatteringData::SortReflectionBySinThetaOverLambda() 5"<<maxSTOL,2)
      long maxSubs=0;
      VFN_DEBUG_MESSAGE("  "<< mIntH(maxSubs)<<" "<< mIntK(maxSubs)<<" "<< mIntL(maxSubs)<<" "<<mSinThetaLambda(maxSubs),1)
      for(maxSubs=0;(mSinThetaLambda(maxSubs)<maxSTOL) && (maxSubs<mNbRefl) ;maxSubs++)
      {
         VFN_DEBUG_MESSAGE("  "<< mIntH(maxSubs)<<" "<< mIntK(maxSubs)<<" "<< mIntL(maxSubs)<<" "<<mSinThetaLambda(maxSubs),1)
      }
      if(maxSubs==mNbRefl)
      {
         VFN_DEBUG_EXIT("ScatteringData::SortReflectionBySinThetaOverLambda():"<<mNbRefl<<" reflections",5)
         return sortedSubs;
      }
      mNbRefl=maxSubs;
      mH.resizeAndPreserve(mNbRefl);
      mK.resizeAndPreserve(mNbRefl);
      mL.resizeAndPreserve(mNbRefl);
      sortedSubs.resizeAndPreserve(mNbRefl);
      mClockHKL.Click();
      this->PrepareHKLarrays();
   }
   VFN_DEBUG_EXIT("ScatteringData::SortReflectionBySinThetaOverLambda():"<<mNbRefl<<" reflections",5)
   return sortedSubs;
}

CrystVector_long ScatteringData::EliminateExtinctReflections()
{
   TAU_PROFILE("ScatteringData::EliminateExtinctReflections()","void ()",TAU_DEFAULT);
   VFN_DEBUG_ENTRY("ScatteringData::EliminateExtinctReflections()",7)
   
   long nbKeptRefl=0;
   CrystVector_long subscriptKeptRefl(mNbRefl);
   subscriptKeptRefl=0;
   for(long j=0;j<mNbRefl;j++)
   {
      if( this->GetCrystal().GetSpaceGroup().IsReflSystematicAbsent(mH(j),mK(j),mL(j))==false )
         subscriptKeptRefl(nbKeptRefl++)=j;
   }
   VFN_DEBUG_MESSAGE("ScatteringData::EliminateExtinctReflections():4",5)
   //Keep only the elected reflections
      mNbRefl=nbKeptRefl;
      {
         CrystVector_long oldH,oldK,oldL;
         CrystVector_int oldMulti;
         long subs;

         oldH=mH;
         oldK=mK;
         oldL=mL;
         oldMulti=mMultiplicity;
         
         mMultiplicity.resize(mNbRefl);
         mH.resize(mNbRefl);
         mK.resize(mNbRefl);
         mL.resize(mNbRefl);
         for(long i=0;i<mNbRefl;i++)
         {
            subs=subscriptKeptRefl(i);
            mH(i)=oldH(subs);
            mK(i)=oldK(subs);
            mL(i)=oldL(subs);
            mMultiplicity(i)=oldMulti(subs);
         }
      }
   this->PrepareHKLarrays();
   VFN_DEBUG_EXIT("ScatteringData::EliminateExtinctReflections():End",7)
   return subscriptKeptRefl;
}

void ScatteringData::CalcSinThetaLambda()const
{
   if(mClockTheta>mClockMaster) return;
   if( 0 == mpCrystal) throw ObjCrystException("ScatteringData::CalcSinThetaLambda() \
      Cannot compute sin(theta)/lambda : there is no crystal affected to this \
      ScatteringData object yet.");

   if( 0 == this->GetNbRefl()) throw ObjCrystException("ScatteringData::CalcSinThetaLambda() \
      Cannot compute sin(theta)/lambda : there are no reflections !");

   if(  (mClockTheta>this->GetRadiation().GetClockWavelength()) && (mClockTheta>mClockHKL)
      &&(mClockTheta>mpCrystal->GetClockLatticePar())) return;
   
   VFN_DEBUG_ENTRY("ScatteringData::CalcSinThetaLambda()",3)
   TAU_PROFILE("ScatteringData::CalcSinThetaLambda()","void (bool)",TAU_DEFAULT);
   mSinThetaLambda.resize(mNbRefl);
   
   const CrystMatrix_REAL bMatrix= mpCrystal->GetBMatrix();
   mX.resize(this->GetNbRefl());
   mY.resize(this->GetNbRefl());
   mZ.resize(this->GetNbRefl());
   for(int i=0;i<this->GetNbRefl();i++)
   {  //:TODO: faster,nicer 
      mX(i)=bMatrix(0,0)*mH(i)+bMatrix(0,1)*mK(i)+bMatrix(0,2)*mL(i);
      mY(i)=bMatrix(1,0)*mH(i)+bMatrix(1,1)*mK(i)+bMatrix(1,2)*mL(i);
      mZ(i)=bMatrix(2,0)*mH(i)+bMatrix(2,1)*mK(i)+bMatrix(2,2)*mL(i);
   }
   //cout << bMatrix << endl << xyz<<endl;
   for(int i=0;i< (this->GetNbRefl());i++)
      mSinThetaLambda(i)=sqrt(pow(mX(i),2)+pow(mY(i),2)+pow(mZ(i),2))/2;
   if(this->GetRadiation().GetWavelengthType()!=WAVELENGTH_TOF)
   {
      if(this->GetRadiation().GetWavelength()(0) > 0)
      {
         mTheta.resize(mNbRefl);
         for(int i=0;i< (this->GetNbRefl());i++) 
         {  
            if( (mSinThetaLambda(i)*this->GetRadiation().GetWavelength()(0))>1)
            {
               //:KLUDGE: :TODO:
               mTheta(i)=M_PI;
               /*
               ofstream out("log.txt");
               out << "Error when computing Sin(theta) :"
                   << "i="<<i<<" ,mSinThetaLambda(i)="<<mSinThetaLambda(i)
                   << " ,this->GetRadiation().GetWavelength()(0)="
                   << this->GetRadiation().GetWavelength()(0) 
                   << " ,H="<<mH(i)
                   << " ,K="<<mK(i)
                   << " ,L="<<mL(i)
                   <<endl;
               out.close();
               abort();
               */
            }
            else 
            {
               mTheta(i)=asin(mSinThetaLambda(i)*this->GetRadiation().GetWavelength()(0));
            }
         }
      } else 
      {
         cout << "Wavelength not given in ScatteringData::CalcSinThetaLambda() !" <<endl;
         throw 0;
      }
   }
   else mTheta.resize(0);
      
   mClockTheta.Click();
   VFN_DEBUG_EXIT("ScatteringData::CalcSinThetaLambda()",3)
}

void ScatteringData::CalcScattFactor()const
{
   //if(mClockScattFactor>mClockMaster) return;
   if(  (mClockScattFactor>this->GetRadiation().GetClockWavelength()) 
      &&(mClockScattFactor>mClockHKL)
      &&(mClockScattFactor>mpCrystal->GetClockLatticePar())
      &&(mClockThermicFact>mpCrystal->GetMasterClockScatteringPower())) return;
   TAU_PROFILE("ScatteringData::CalcScattFactor()","void (bool)",TAU_DEFAULT);
   VFN_DEBUG_ENTRY("ScatteringData::CalcScattFactor()",4)
   this->CalcResonantScattFactor();
   mvScatteringFactor.clear();
   for(int i=mpCrystal->GetScatteringPowerRegistry().GetNb()-1;i>=0;i--)
   {
      const ScatteringPower *pScattPow=&(mpCrystal->GetScatteringPowerRegistry().GetObj(i));
      mvScatteringFactor[pScattPow]=pScattPow->GetScatteringFactor(*this);
      //Directly add Fprime
      mvScatteringFactor[pScattPow]+= this->mvFprime[pScattPow];
      VFN_DEBUG_MESSAGE("->   H      K      L   sin(t/l)     f0+f'"
                        <<FormatVertVectorHKLFloats<REAL>(mH,mK,mL,mSinThetaLambda,
                                                          mvScatteringFactor[pScattPow]),1);
   }
   mClockScattFactor.Click();
   VFN_DEBUG_EXIT("ScatteringData::CalcScattFactor()",4)
}

void ScatteringData::CalcTemperatureFactor()const
{
   //if(mClockThermicFact>mClockMaster) return;
   if(  (mClockThermicFact>this->GetRadiation().GetClockWavelength())
      &&(mClockThermicFact>mClockHKL)
      &&(mClockThermicFact>mpCrystal->GetClockLatticePar())
      &&(mClockThermicFact>mpCrystal->GetMasterClockScatteringPower())) return;
   TAU_PROFILE("ScatteringData::CalcTemperatureFactor()","void (bool)",TAU_DEFAULT);
   VFN_DEBUG_ENTRY("ScatteringData::CalcTemperatureFactor()",4)
   mvTemperatureFactor.clear();
   for(int i=mpCrystal->GetScatteringPowerRegistry().GetNb()-1;i>=0;i--)
   {
      const ScatteringPower *pScattPow=&(mpCrystal->GetScatteringPowerRegistry().GetObj(i));
      mvTemperatureFactor[pScattPow]=pScattPow->GetTemperatureFactor(*this);
   }
   mClockThermicFact.Click();
   VFN_DEBUG_EXIT("ScatteringData::CalcTemperatureFactor()",4)
}

void ScatteringData::CalcResonantScattFactor()const
{
   if(  (mClockScattFactorResonant>mpCrystal->GetMasterClockScatteringPower())
      &&(mClockScattFactorResonant>this->GetRadiation().GetClockWavelength())) return;
   VFN_DEBUG_ENTRY("ScatteringData::CalcResonantScattFactor()",4)
   TAU_PROFILE("ScatteringData::CalcResonantScattFactor()","void (bool)",TAU_DEFAULT);
   
   mvFprime.clear();
   mvFsecond.clear();
   if(this->GetRadiation().GetWavelength()(0) == 0)
   {
      VFN_DEBUG_EXIT("ScatteringData::CalcResonantScattFactor()->Lambda=0. fprime=fsecond=0",4)
      return;
   }
   else
   {
      for(int i=mpCrystal->GetScatteringPowerRegistry().GetNb()-1;i>=0;i--)
      {
         const ScatteringPower *pScattPow=&(mpCrystal->GetScatteringPowerRegistry().GetObj(i));
         mvFprime [pScattPow]=pScattPow->GetResonantScattFactReal(*this)(0);
         mvFsecond[pScattPow]=pScattPow->GetResonantScattFactImag(*this)(0);
      }
   }
   mClockScattFactorResonant.Click();
   VFN_DEBUG_EXIT("ScatteringData::CalcResonantScattFactor()",4)
}

void ScatteringData::CalcGlobalTemperatureFactor() const
{
   this->GetNbReflBelowMaxSinThetaOvLambda();//update mNbReflUsed, also recalc sin(theta)/lambda
   if(mClockGlobalTemperatureFact>mClockMaster) return;
   if(  (mClockGlobalBiso<mClockGlobalTemperatureFact)
      &&(mClockTheta     <mClockGlobalTemperatureFact)
      &&(mClockHKL       <mClockGlobalTemperatureFact)
      &&(mClockNbReflUsed<mClockGlobalTemperatureFact)) return;
   VFN_DEBUG_MESSAGE("ScatteringData::CalcGlobalTemperatureFactor()",2)
   TAU_PROFILE("ScatteringData::CalcGlobalTemperatureFactor()","void ()",TAU_DEFAULT);
   
   mGlobalTemperatureFactor.resize(mNbRefl);
   //if(true==mUseFastLessPreciseFunc) //:TODO:
   {
   }
   //else
   {
      const REAL *stol=this->GetSinThetaOverLambda().data();
      REAL *fact=mGlobalTemperatureFactor.data();
      for(long i=0;i<mNbReflUsed;i++) {*fact++ = exp(-mGlobalBiso * *stol * *stol);stol++;}
   }
   mClockGlobalTemperatureFact.Click();
}

void ScatteringData::CalcStructFactor() const
{
   this->GetNbReflBelowMaxSinThetaOvLambda();//check mNbReflUsed, also recalc sin(theta)/lambda
   if(mClockStructFactor>mClockMaster) return;
   
   //:TODO: Anisotropic Thermic factors
   //TAU_PROFILE_TIMER(timer1,"ScatteringData::CalcStructFactor1:Prepare","", TAU_FIELD);
   //TAU_PROFILE_TIMER(timer2,"ScatteringData::CalcStructFactor2:GeomStructFact","", TAU_FIELD);
   //TAU_PROFILE_TIMER(timer3,"ScatteringData::CalcStructFactor3:Scatt.Factors","", TAU_FIELD);
   //TAU_PROFILE_TIMER(timer4,"ScatteringData::CalcStructFactor4:Finish,DynCorr","", TAU_FIELD);

   //TAU_PROFILE_START(timer1);
   
   const long nbRefl=this->GetNbRefl();
   this->CalcSinThetaLambda();
   //TAU_PROFILE_STOP(timer1);
   //TAU_PROFILE_START(timer2);
   this->CalcGeomStructFactor();
   //TAU_PROFILE_STOP(timer2);
   //TAU_PROFILE_START(timer3);
   this->CalcScattFactor();
   this->CalcResonantScattFactor();
   this->CalcTemperatureFactor();
   this->CalcGlobalTemperatureFactor();
   this->CalcLuzzatiFactor();
   this->CalcStructFactVariance();
   //TAU_PROFILE_STOP(timer3);
   
   //OK, really must recompute SFs?
   VFN_DEBUG_MESSAGE("ScatteringData::CalcStructFactor():Fhkl Recalc ?"<<endl
      <<"mClockStructFactor<mClockGlobalTemperatureFact"<<(bool)(mClockStructFactor<mClockGlobalTemperatureFact)<<endl
      <<"mClockStructFactor<mClockGeomStructFact"       <<(bool)(mClockStructFactor<mClockGeomStructFact)<<endl
      <<"mClockStructFactor<mClockScattFactorResonant"  <<(bool)(mClockStructFactor<mClockScattFactorResonant)<<endl
      <<"mClockStructFactor<mClockThermicFact"          <<(bool)(mClockStructFactor<mClockThermicFact)<<endl
      <<"mClockStructFactor<mClockLuzzatiFactor"        <<(bool)(mClockStructFactor<mClockLuzzatiFactor)<<endl      
      ,2)
   if(  (mClockStructFactor>mClockGlobalTemperatureFact)
      &&(mClockStructFactor>mClockGeomStructFact)
      &&(mClockStructFactor>mClockScattFactorResonant)
      &&(mClockStructFactor>mClockThermicFact)
      &&(mClockStructFactor>mClockLuzzatiFactor)) return;
   VFN_DEBUG_ENTRY("ScatteringData::CalcStructFactor()",3)
   TAU_PROFILE("ScatteringData::CalcStructFactor()","void ()",TAU_DEFAULT);
   //TAU_PROFILE_START(timer4);
   //reset Fcalc
      mFhklCalcReal.resize(nbRefl);
      mFhklCalcImag.resize(nbRefl);
      mFhklCalcReal=0;
      mFhklCalcImag=0;
   //Add all contributions
   for(map<const ScatteringPower*,CrystVector_REAL>::const_iterator pos=mvRealGeomSF.begin();
       pos!=mvRealGeomSF.end();++pos)
   {
      const ScatteringPower* pScattPow=pos->first;
      VFN_DEBUG_MESSAGE("ScatteringData::CalcStructFactor():Fhkl Recalc, "<<pScattPow->GetName(),2)
      const REAL *pGeomR=mvRealGeomSF[pScattPow].data();
      const REAL *pGeomI=mvImagGeomSF[pScattPow].data();
      const REAL *pScatt=mvScatteringFactor[pScattPow].data();
      const REAL *pTemp=mvTemperatureFactor[pScattPow].data();

      REAL *pReal=mFhklCalcReal.data();
      REAL *pImag=mFhklCalcImag.data();

      VFN_DEBUG_MESSAGE("->mvRealGeomSF[i] "
         <<mvRealGeomSF[pScattPow].numElements()<<"elements",2)
      VFN_DEBUG_MESSAGE("->mvImagGeomSF[i] "
         <<mvImagGeomSF[pScattPow].numElements()<<"elements",2)
      VFN_DEBUG_MESSAGE("->mvScatteringFactor[i]"
         <<mvScatteringFactor[pScattPow].numElements()<<"elements",1)
      VFN_DEBUG_MESSAGE("->mvTemperatureFactor[i]" 
         <<mvTemperatureFactor[pScattPow].numElements()<<"elements",1)
      VFN_DEBUG_MESSAGE("->mFhklCalcReal "<<mFhklCalcReal.numElements()<<"elements",2)
      VFN_DEBUG_MESSAGE("->mFhklCalcImag "<<mFhklCalcImag.numElements()<<"elements",2)
      VFN_DEBUG_MESSAGE("->   H      K      L   sin(t/l)     Re(F)      Im(F)      scatt      Temp",1)

      VFN_DEBUG_MESSAGE(FormatVertVectorHKLFloats<REAL>(mH,mK,mL,mSinThetaLambda,
                                                        mvRealGeomSF[pScattPow],
                                                        mvImagGeomSF[pScattPow],
                                                        mvScatteringFactor[pScattPow],
                                                        mvTemperatureFactor[pScattPow] 
                                                        ),1);
      if(mvLuzzatiFactor[pScattPow].numElements()>0)
      {// using maximum likelihood
         const REAL* pLuzzati=mvLuzzatiFactor[pScattPow].data();
         if(false==mIgnoreImagScattFact)
         {
            const REAL fsecond=mvFsecond[pScattPow];
            VFN_DEBUG_MESSAGE("->fsecond= "<<fsecond,2)
            for(long j=0;j<mNbReflUsed;j++)
            {
               VFN_DEBUG_MESSAGE("-->"<<j<<" "<<*pReal<<" "<<*pImag<<" "<<*pGeomR<<" "<<*pGeomI<<" "<<*pScatt<<" "<<*pTemp,1)
               *pReal++ += (*pGeomR   * *pScatt   - *pGeomI   * fsecond)* *pTemp * *pLuzzati;
               *pImag++ += (*pGeomI++ * *pScatt++ + *pGeomR++ * fsecond)* *pTemp++ * *pLuzzati++;
            }
         }
         else
         {
            for(long j=0;j<mNbReflUsed;j++)
            {
               *pReal++ += *pGeomR++  * *pTemp   * *pScatt   * *pLuzzati;
               *pImag++ += *pGeomI++  * *pTemp++ * *pScatt++ * *pLuzzati++;
            }
         }
         VFN_DEBUG_MESSAGE("ScatteringData::CalcStructFactor():"<<mIgnoreImagScattFact
                           <<",f\"="<<mvFsecond[pScattPow]<<endl<<
                           FormatVertVectorHKLFloats<REAL>(mH,mK,mL,mSinThetaLambda,
                                                           mvRealGeomSF[pScattPow],
                                                           mvImagGeomSF[pScattPow],
                                                           mvScatteringFactor[pScattPow],
                                                           mvTemperatureFactor[pScattPow], 
                                                           mvLuzzatiFactor[pScattPow],
                                                           mFhklCalcReal, 
                                                           mFhklCalcImag
                                                           ),2);
      }
      else
      {
         if(false==mIgnoreImagScattFact)
         {
            const REAL fsecond=mvFsecond[pScattPow];
            VFN_DEBUG_MESSAGE("->fsecond= "<<fsecond,2)
            for(long j=0;j<mNbReflUsed;j++)
            {
               *pReal += (*pGeomR   * *pScatt - *pGeomI * fsecond)* *pTemp;
               *pImag += (*pGeomI   * *pScatt + *pGeomR * fsecond)* *pTemp;
               VFN_DEBUG_MESSAGE("-->"<<j<<" "<<*pReal<<" "<<*pImag<<" "<<*pGeomR<<" "<<*pGeomI<<" "<<*pScatt<<" "<<*pTemp,1)
               pGeomR++;pGeomI++;pTemp++;pScatt++;pReal++;pImag++;
            }
         }
         else
         {
            for(long j=0;j<mNbReflUsed;j++)
            {
               *pReal++ += *pGeomR  * *pTemp * *pScatt;
               *pImag++ += *pGeomI  * *pTemp * *pScatt;
               pGeomR++;pGeomI++;pTemp++;pScatt++;
            }
         }
         VFN_DEBUG_MESSAGE(FormatVertVectorHKLFloats<REAL>(mH,mK,mL,mSinThetaLambda,
                                                            mvRealGeomSF[pScattPow],
                                                            mvImagGeomSF[pScattPow],
                                                            mvScatteringFactor[pScattPow],
                                                            mvTemperatureFactor[pScattPow], 
                                                            mFhklCalcReal, 
                                                            mFhklCalcImag
                                                            ),2);
      }
   }
   //TAU_PROFILE_STOP(timer4);
   {
      this->CalcGlobalTemperatureFactor();
      if(mGlobalTemperatureFactor.numElements()>0)
      {//else for some reason it's useless
         REAL *pReal=mFhklCalcReal.data();
         REAL *pImag=mFhklCalcImag.data();
         const REAL *pTemp=mGlobalTemperatureFactor.data();
         for(long j=0;j<mNbReflUsed;j++)
         {
            *pReal++ *= *pTemp;
            *pImag++ *= *pTemp++;
         }
      }
   }
   mClockStructFactor.Click();

   VFN_DEBUG_EXIT("ScatteringData::CalcStructFactor()",3)
}

void ScatteringData::CalcGeomStructFactor() const
{
   // This also updates the ScattCompList if necessary.
   const ScatteringComponentList *pScattCompList
      =&(this->GetCrystal().GetScatteringComponentList());
   if(  (mClockGeomStructFact>mpCrystal->GetClockScattCompList())
      &&(mClockGeomStructFact>mClockHKL)
      &&(mClockGeomStructFact<mpCrystal->GetMasterClockScatteringPower())) return;
   TAU_PROFILE("ScatteringData::GeomStructFactor()","void (Vx,Vy,Vz,data,M,M,bool)",TAU_DEFAULT);
   VFN_DEBUG_ENTRY("ScatteringData::GeomStructFactor(Vx,Vy,Vz,...)",3)
   VFN_DEBUG_MESSAGE("-->Using fast functions:"<<mUseFastLessPreciseFunc,2)
   VFN_DEBUG_MESSAGE("-->Number of translation vectors:"
      <<this->GetCrystal().GetSpaceGroup().GetNbTranslationVectors()-1,2)
   VFN_DEBUG_MESSAGE("-->Has an inversion Center:"
      <<this->GetCrystal().GetSpaceGroup().HasInversionCenter(),2)
   VFN_DEBUG_MESSAGE("-->Number of symetry operations (w/o transl&inv cent.):"\
                     <<this->GetCrystal().GetSpaceGroup().GetNbSymmetrics(true,true),2)
   VFN_DEBUG_MESSAGE("-->Number of Scattering Components :"
      <<this->GetCrystal().GetScatteringComponentList().GetNbComponent(),2)
   VFN_DEBUG_MESSAGE("-->Number of reflections:"
      <<this->GetNbRefl()<<" (actually used:"<<mNbReflUsed<<")",2)
   #ifdef __DEBUG__
   static long counter=0;
   VFN_DEBUG_MESSAGE("-->Number of GeomStructFactor calculations so far:"<<counter++,3)
   #endif
   
   //:TODO: implement for geometrical structure factor calculation
   //bool useGeomStructFactor=mUseGeomStructFactor;
   
   //if((mfpRealGeomStructFactor==0)||(mfpImagGeomStructFactor==0)) useGeomStructFactor=false ;
   
   //if(useGeomStructFactor==true)
   //{
   //   (*mfpRealGeomStructFactor)(x,y,z,data.H2Pi(),data.K2Pi(),data.L2Pi(),rsf);
   //   if(this->IsCentrosymmetric())return;
   //   (*mfpImagGeomStructFactor)(x,y,z,data.H2Pi(),data.K2Pi(),data.L2Pi(),isf);
   //   return;
   //}
   //else
   {  
      const SpaceGroup *pSpg=&(this->GetCrystal().GetSpaceGroup());
      
      const int nbSymmetrics=pSpg->GetNbSymmetrics(true,true);
      const int nbTranslationVectors=pSpg->GetNbTranslationVectors();
      const long nbComp=pScattCompList->GetNbComponent();
      const int nbRefl=this->GetNbRefl();
      CrystMatrix_REAL transVect(nbTranslationVectors,3);
      transVect=pSpg->GetTranslationVectors();
      CrystMatrix_REAL allCoords(nbSymmetrics,3);
      CrystVector_REAL tmpVect(nbRefl);
      
      if(mUseFastLessPreciseFunc==true)
      {
         if(sLibCrystTabulCosineIsInit==false )
         {
            cout << "Init tabulated sin and cosine functions."<<endl;
            InitLibCrystTabulCosine();
            sLibCrystTabulCosineIsInit=true;
         }
      }
      CrystVector_long intVect(nbRefl);//not used if mUseFastLessPreciseFunc==false
      
      // which scattering powers are actually used ?
      map<const ScatteringPower*,bool> vUsed;
      for(int i=mpCrystal->GetScatteringPowerRegistry().GetNb()-1;i>=0;i--)
         vUsed[&(mpCrystal->GetScatteringPowerRegistry().GetObj(i))]=false;
      for(long i=0;i<nbComp;i++)
         vUsed[(*pScattCompList)(i).mpScattPow]=true;
      //Resize all arrays and set them to 0
      for(map<const ScatteringPower*,bool>::const_iterator pos=vUsed.begin();pos!=vUsed.end();++pos)
      {
         if(pos->second)
         {// this will create the entry if it does not already exist
            mvRealGeomSF[pos->first].resize(nbRefl);
            mvImagGeomSF[pos->first].resize(nbRefl);
            mvRealGeomSF[pos->first]=0;
            mvImagGeomSF[pos->first]=0;
         }
         else
         {// erase entries that are not useful any more (e.g. ScatteringPower that were
          // used but are not any more).
            map<const ScatteringPower*,CrystVector_REAL>::iterator
               poubelle=mvRealGeomSF.find(pos->first);
            if(poubelle!=mvRealGeomSF.end()) mvRealGeomSF.erase(poubelle);
            poubelle=mvImagGeomSF.find(pos->first);
            if(poubelle!=mvImagGeomSF.end()) mvImagGeomSF.erase(poubelle);
         }
      }
      
      for(long i=0;i<nbComp;i++)
      {
         const REAL x=(*pScattCompList)(i).mX;
         const REAL y=(*pScattCompList)(i).mY;
         const REAL z=(*pScattCompList)(i).mZ;
         const ScatteringPower *pScattPow=(*pScattCompList)(i).mpScattPow;
         REAL centrMult=1.0;
         if(true==pSpg->HasInversionCenter()) centrMult=2.0;
         const REAL popu= (*pScattCompList)(i).mOccupancy 
                         *(*pScattCompList)(i).mDynPopCorr
                         *centrMult;
         
         allCoords=pSpg->GetAllSymmetrics(x,y,z,true,true);
         if((true==pSpg->HasInversionCenter()) && (false==pSpg->IsInversionCenterAtOrigin()))
         {
            for(int j=0;j<nbSymmetrics;j++)
            {
               //The phase of the structure factor will be wrong
               //This is fixed a bit further...
               allCoords(j,0) -= ((REAL)pSpg->GetSgOps().InvT[0])/STBF/2.;
               allCoords(j,1) -= ((REAL)pSpg->GetSgOps().InvT[1])/STBF/2.;
               allCoords(j,2) -= ((REAL)pSpg->GetSgOps().InvT[2])/STBF/2.;
            }
         }
         for(int j=0;j<nbSymmetrics;j++)
         {
            
            if(mUseFastLessPreciseFunc==true)
            {
               REAL *rrsf=mvRealGeomSF[pScattPow].data();
               REAL *iisf=mvImagGeomSF[pScattPow].data();
               
               const long intX=(long)(allCoords(j,0)*sLibCrystNbTabulSine);
               const long intY=(long)(allCoords(j,1)*sLibCrystNbTabulSine);
               const long intZ=(long)(allCoords(j,2)*sLibCrystNbTabulSine);

               const long *intH=mIntH.data();
               const long *intK=mIntK.data();
               const long *intL=mIntL.data();

               register long *tmpInt=intVect.data();
               // :KLUDGE: using a AND to bring back within [0;sLibCrystNbTabulSine[ may
               // not be portable, depending on the model used to represent signed integers
               // a test should be added to throw up in that case.
               //
               // This work if we are using "2's complement" to represent negative numbers,
               // but not with a "sign magnitude" approach
               for(int jj=0;jj<mNbReflUsed;jj++) 
                *tmpInt++ = (*intH++ * intX + *intK++ * intY + *intL++ *intZ)
                              &sLibCrystNbTabulSineMASK;
               if(false==pSpg->HasInversionCenter())
               {

                  tmpInt=intVect.data();
                  for(int jj=0;jj<mNbReflUsed;jj++)
                  {
                     const REAL *pTmp=&spLibCrystTabulCosineSine[*tmpInt++ <<1];
                     *rrsf++ += popu * *pTmp++;
                     *iisf++ += popu * *pTmp;
                  }

               }
               else
               {
                  tmpInt=intVect.data();
                  for(int jj=0;jj<mNbReflUsed;jj++) 
                     *rrsf++ += popu * spLibCrystTabulCosine[*tmpInt++];
               }
            }
            else
            {
               const REAL x=allCoords(j,0);
               const REAL y=allCoords(j,1);
               const REAL z=allCoords(j,2);
               const register REAL *hh=mH2Pi.data();
               const register REAL *kk=mK2Pi.data();
               const register REAL *ll=mL2Pi.data();
               register REAL *tmp=tmpVect.data();
               
               
               for(int jj=0;jj<mNbReflUsed;jj++) *tmp++ = *hh++ * x + *kk++ * y + *ll++ *z;
               
               REAL *sf=mvRealGeomSF[pScattPow].data();
               tmp=tmpVect.data();
               
               for(int jj=0;jj<mNbReflUsed;jj++) *sf++ += popu * cos(*tmp++);
               
               if(false==pSpg->HasInversionCenter()) 
               {
                  sf=mvImagGeomSF[pScattPow].data();
                  tmp=tmpVect.data();
                  for(int jj=0;jj<mNbReflUsed;jj++) *sf++ += popu * sin(*tmp++);
               }
            }
         }
      }//for all components...
      
      if(nbTranslationVectors > 1)
      {
         tmpVect=1;
         if( (pSpg->GetSpaceGroupNumber()>= 143) && (pSpg->GetSpaceGroupNumber()<= 167))
         {//Special case for trigonal groups R3,...
            REAL *p1=tmpVect.data();
            const register REAL *hh=mH2Pi.data();
            const register REAL *kk=mK2Pi.data();
            const register REAL *ll=mL2Pi.data();
            for(long j=0;j<mNbReflUsed;j++) *p1++ += 2*cos((*hh++ - *kk++ - *ll++)/3.);
         }
         else
         {
            for(int j=1;j<nbTranslationVectors;j++)
            {
               const REAL x=transVect(j,0);
               const REAL y=transVect(j,1);
               const REAL z=transVect(j,2);
               REAL *p1=tmpVect.data();
               const register REAL *hh=mH2Pi.data();
               const register REAL *kk=mK2Pi.data();
               const register REAL *ll=mL2Pi.data();
               for(long j=0;j<mNbReflUsed;j++) *p1++ += cos(*hh++ *x + *kk++ *y + *ll++ *z );
            }
         }
         for(map<const ScatteringPower*,CrystVector_REAL>::iterator
               pos=mvRealGeomSF.begin();pos!=mvRealGeomSF.end();++pos) 
                  pos->second *= tmpVect;
         
         if(false==pSpg->HasInversionCenter())
            for(map<const ScatteringPower*,CrystVector_REAL>::iterator
                  pos=mvImagGeomSF.begin();pos!=mvImagGeomSF.end();++pos)
                     pos->second *= tmpVect;
      }
      if(true==pSpg->HasInversionCenter())
      {
         // we already multiplied real geom struct factor by 2
         if(false==pSpg->IsInversionCenterAtOrigin())
         {
            VFN_DEBUG_MESSAGE("ScatteringData::GeomStructFactor(Vx,Vy,Vz):\
               Inversion Center not at the origin...",2)
            //fix the phase of each reflection when the inversion center is not
            //at the origin, using :
            // Re(F) = RSF*cos(2pi(h*Xc+k*Yc+l*Zc))
            // Re(F) = RSF*sin(2pi(h*Xc+k*Yc+l*Zc))
            //cout << "Glop Glop"<<endl;
            {
               const REAL xc=((REAL)pSpg->GetSgOps().InvT[0])/STBF/2.;
               const REAL yc=((REAL)pSpg->GetSgOps().InvT[1])/STBF/2.;
               const REAL zc=((REAL)pSpg->GetSgOps().InvT[2])/STBF/2.;
               #ifdef __LIBCRYST_VECTOR_USE_BLITZ__
               tmpVect = mH2Pi() * xc + mK2PI() * yc + mL2PI() * zc;
               #else
               {
                  const REAL *hh=mH2Pi.data();
                  const REAL *kk=mK2Pi.data();
                  const REAL *ll=mL2Pi.data();
                  REAL *ttmpVect=tmpVect.data();
                  for(long ii=0;ii<mNbReflUsed;ii++) 
                     *ttmpVect++ = *hh++ * xc + *kk++ * yc + *ll++ * zc;
               }
               #endif
            }
            CrystVector_REAL cosTmpVect;
            CrystVector_REAL sinTmpVect;
            cosTmpVect=cos(tmpVect);
            sinTmpVect=sin(tmpVect);
            
            map<const ScatteringPower*,CrystVector_REAL>::iterator posi=mvImagGeomSF.begin();
            map<const ScatteringPower*,CrystVector_REAL>::iterator posr=mvRealGeomSF.begin();
            for(;posi!=mvImagGeomSF.end();)
            {
               posi->second = posr->second;
               posi->second *= sinTmpVect;
               posr->second *= cosTmpVect;
               posi++;posr++;
            }
         }
      }

   }
   //cout << FormatVertVector<REAL>(*mvRealGeomSF,*mvImagGeomSF)<<endl;
   mClockGeomStructFact.Click();
   VFN_DEBUG_EXIT("ScatteringData::GeomStructFactor(Vx,Vy,Vz,...)",3)
}

void ScatteringData::CalcLuzzatiFactor()const
{
   // Assume this is  called by ScatteringData::CalcStructFactor()
   // and that we already have computed geometrical structure factors
   VFN_DEBUG_ENTRY("ScatteringData::CalcLuzzatiFactor",3)
   bool useLuzzati=false;
   for(map<const ScatteringPower*,CrystVector_REAL>::const_iterator
       pos=mvRealGeomSF.begin();pos!=mvRealGeomSF.end();++pos)
   {
      if(pos->first->GetMaximumLikelihoodPositionError()!=0)
      {
         useLuzzati=true;
         break;
      }
   }
   if(!useLuzzati)
   {
      mvLuzzatiFactor.clear();
      VFN_DEBUG_EXIT("ScatteringData::CalcLuzzatiFactor(): not needed, no positionnal errors",3)
      return;
   }
   bool recalc=false;
   if(  (mClockTheta     >mClockLuzzatiFactor)
      ||(mClockGeomStructFact>mClockLuzzatiFactor)//checks if occupancies changed
      ||(mClockNbReflUsed>mClockLuzzatiFactor))
   {
      //if(mClockTheta     >mClockLuzzatiFactor)cout<<"1"<<endl;
      //if(mClockGeomStructFact>mClockLuzzatiFactor)cout<<"2"<<endl;
      //if(mClockNbReflUsed>mClockLuzzatiFactor)cout<<"3"<<endl;
      recalc=true;
   }
   else
   {
      for(int i=mpCrystal->GetScatteringPowerRegistry().GetNb()-1;i>=0;i--)
      {
         if(mpCrystal->GetScatteringPowerRegistry().GetObj(i)
            .GetMaximumLikelihoodPositionErrorClock()>mClockLuzzatiFactor)
         {
            recalc=true;
            break;
         }
      }
   }
   
   if(false==recalc)
   {
      VFN_DEBUG_EXIT("ScatteringData::CalcLuzzatiFactor(): no recalc needed",3)
      return;
   }
   TAU_PROFILE("ScatteringData::CalcLuzzatiFactor()","void ()",TAU_DEFAULT);
   
   for(int i=mpCrystal->GetScatteringPowerRegistry().GetNb()-1;i>=0;i--)
   {
      const ScatteringPower* pScattPow=&(mpCrystal->GetScatteringPowerRegistry().GetObj(i));
      if(0 == pScattPow->GetMaximumLikelihoodPositionError())
      {
         mvLuzzatiFactor[pScattPow].resize(0);
      }
      else
      {
         mvLuzzatiFactor[pScattPow].resize(mNbRefl);
         const REAL b=-(8*M_PI*M_PI)* pScattPow->GetMaximumLikelihoodPositionError()
                                    * pScattPow->GetMaximumLikelihoodPositionError();
         const REAL *stol=this->GetSinThetaOverLambda().data();
         REAL *fact=mvLuzzatiFactor[pScattPow].data();
         for(long j=0;j<mNbReflUsed;j++) {*fact++ = exp(b * *stol * *stol);stol++;}
         VFN_DEBUG_MESSAGE("ScatteringData::CalcLuzzatiFactor():"<<pScattPow->GetName()<<endl<<
                           FormatVertVectorHKLFloats<REAL>(mH,mK,mL,mSinThetaLambda,
                           mvRealGeomSF[pScattPow],mvImagGeomSF[pScattPow],
                           mvScatteringFactor[pScattPow],mvLuzzatiFactor[pScattPow]
                           ),2);
      }
   }
   mClockLuzzatiFactor.Click();
   VFN_DEBUG_EXIT("ScatteringData::CalcLuzzatiFactor(): no recalc needed",3)
}

void ScatteringData::CalcStructFactVariance()const
{
   // this is called by CalcStructFactor(), after the calculation of the structure factors,
   // and the recomputation of Luzzati factors has already been asked
   // So we only recompute if these clocks have changed.
   if(  (mClockFhklCalcVariance>mClockLuzzatiFactor)
      &&(mClockFhklCalcVariance>mClockStructFactor)) return;
   if(0==mvLuzzatiFactor.size())
   {
      mFhklCalcVariance.resize(0);
      return;
   }
   VFN_DEBUG_ENTRY("ScatteringData::CalcStructFactVariance()",3)
   TAU_PROFILE("ScatteringData::CalcStructFactVariance()","void ()",TAU_DEFAULT);
   bool needVar=false;
   
   map<const ScatteringPower*,REAL> vComp;
   {
      const ScatteringComponentList *pList= & (this->GetCrystal().GetScatteringComponentList());
      const long nbComp=pList->GetNbComponent();
      const ScatteringComponent *pComp;
      for(long i=0;i<nbComp;i++)
      {
         pComp=&((*pList)(i));
         vComp[pComp->mpScattPow]=0;
      }
      for(long i=0;i<nbComp;i++)
      {
         pComp=&((*pList)(i));
         vComp[pComp->mpScattPow]+= pComp->mOccupancy * pComp->mDynPopCorr;
      }
      for(map<const ScatteringPower*,REAL>::iterator
          pos=vComp.begin();pos!=vComp.end();++pos)
            pos->second *= this->GetCrystal().GetSpaceGroup().GetNbSymmetrics();
   }

   if(mFhklCalcVariance.numElements() == mNbRefl)
   {
      REAL *pVar=mFhklCalcVariance.data();
      for(long j=0;j<mNbReflUsed;j++) *pVar++ = 0;
   }
   
   for(int i=mpCrystal->GetScatteringPowerRegistry().GetNb()-1;i>=0;i--)
   {
      const ScatteringPower* pScattPow=&(mpCrystal->GetScatteringPowerRegistry().GetObj(i));
      if(mvLuzzatiFactor[pScattPow].numElements()==0) continue;
      needVar=true;
      if(mFhklCalcVariance.numElements() != mNbRefl)
      {
         mFhklCalcVariance.resize(mNbRefl);
         REAL *pVar=mFhklCalcVariance.data();
         for(long j=0;j<mNbReflUsed;j++) *pVar++ = 0;
      }
      // variance on real & imag parts of the structure factor
      const REAL *pScatt=mvScatteringFactor[pScattPow].data();
      const REAL *pLuz=mvLuzzatiFactor[pScattPow].data();
      const int  *pExp=mExpectedIntensityFactor.data();
      REAL *pVar=mFhklCalcVariance.data();
      const REAL occ=vComp[pScattPow];
      for(long j=0;j<mNbReflUsed;j++)
      {
         *pVar++ +=  occ * *pExp++ * *pScatt * *pScatt * (1 - *pLuz * *pLuz);
         pScatt++; pLuz++;
      }
   }
   if(false == needVar) mFhklCalcVariance.resize(0);

   mClockFhklCalcVariance.Click();
   VFN_DEBUG_EXIT("ScatteringData::CalcStructFactVariance()",3)
}

}//namespace ObjCryst
