/*
* ObjCryst++ : a Crystallographic computing library in C++
*
*  (c) 2000 Vincent FAVRE-NICOLIN
*           Laboratoire de Cristallographie
*           24, quai Ernest-Ansermet, CH-1211 Geneva 4, Switzerland
*  Contact: Vincent.Favre-Nicolin@cryst.unige.ch
*           Radovan.Cerny@cryst.unige.ch
*
*/
/*
*  source file for LibCryst++ DiffractionData class
*
*/

#include <cmath>

#include <typeinfo>

#include "ObjCryst/DiffractionDataSingleCrystal.h"
#include "Quirks/VFNDebug.h"
#include "Quirks/VFNStreamFormat.h"

#include <fstream>
#include <iomanip>

namespace ObjCryst
{
//######################################################################
//    DiffractionDataSingleCrystal
//######################################################################
ObjRegistry<DiffractionDataSingleCrystal> 
   gDiffractionDataSingleCrystalRegistry("Global DiffractionDataSingleCrystal Registry");

DiffractionDataSingleCrystal::DiffractionDataSingleCrystal():mScaleFactor(1.)
{
   VFN_DEBUG_MESSAGE("DiffractionDataSingleCrystal::DiffractionDataSingleCrystal()",5)
   this->InitRefParList();
   gDiffractionDataSingleCrystalRegistry.Register(*this);
   gTopRefinableObjRegistry.Register(*this);
}

DiffractionDataSingleCrystal::~DiffractionDataSingleCrystal()
{
   VFN_DEBUG_MESSAGE("DiffractionDataSingleCrystal::~DiffractionDataSingleCrystal()",5)
   gDiffractionDataSingleCrystalRegistry.DeRegister(*this);
   gTopRefinableObjRegistry.DeRegister(*this);
}

DiffractionDataSingleCrystal* DiffractionDataSingleCrystal::CreateCopy()const
{
   VFN_DEBUG_MESSAGE("DiffractionDataSingleCrystal::CreateCopy()",5)
   return new DiffractionDataSingleCrystal(*this);
}

const string DiffractionDataSingleCrystal::GetClassName() const
{return "DiffractionDataSingleCrystal";}

void DiffractionDataSingleCrystal::SetHklIobs(CrystVector_long const &h,
                                              CrystVector_long const &k,
                                              CrystVector_long const &l,
                                              CrystVector_double const &iObs,
                                              CrystVector_double const &sigma)
{
   VFN_DEBUG_ENTRY("DiffractionDataSingleCrystal::SetHklIobs(h,k,l,i,s)",5)
   mNbRefl=h.numElements();
   mH.resize(mNbRefl);
   mK.resize(mNbRefl);
   mL.resize(mNbRefl);
   mObsIntensity.resize(mNbRefl);
   mObsSigma.resize(mNbRefl);
   mWeight.resize(mNbRefl);
   mMultiplicity.resize(mNbRefl);
   
   mH=h;
   mK=k;
   mL=l;
   mObsIntensity=iObs;
   mObsSigma=sigma;
   mWeight=0;
   mMultiplicity=1;
   
   this->PrepareHKLarrays();
   
   this->CalcSinThetaLambda();
   
   mHasObservedData=true;
   VFN_DEBUG_EXIT("DiffractionDataSingleCrystal::SetHklIobs(h,k,l,i,s)",5)
}

const CrystVector_double& DiffractionDataSingleCrystal::GetIcalc()const
{
   this->CalcIcalc();
   return mCalcIntensity;
}
const CrystVector_double& DiffractionDataSingleCrystal::GetIobs()const
{
   //if(mHasObservedData==false) DoSomething
   return mObsIntensity;
}

void DiffractionDataSingleCrystal::SetIobs(const CrystVector_double &obs) {mObsIntensity=obs;}

const CrystVector_double& DiffractionDataSingleCrystal::GetSigma()const
{
   //if(mHasObservedData=false) DoSomething
   return mObsSigma;
}

void DiffractionDataSingleCrystal::SetSigma(const CrystVector_double& sigma) {mObsSigma=sigma;}

const CrystVector_double& DiffractionDataSingleCrystal::GetWeight()const
{
   //if(mHasObservedData=false) DoSomething
   return mWeight;
}

void DiffractionDataSingleCrystal::SetWeight(const CrystVector_double& weight)
{
   VFN_DEBUG_MESSAGE("DiffractionDataSingleCrystal::SetWeight(w)",5)
   mWeight=weight;
}

void DiffractionDataSingleCrystal::SetIobsToIcalc()
{
   VFN_DEBUG_MESSAGE("DiffractionDataSingleCrystal::SetIobsToIcalc()",5)
   mObsIntensity=this->GetIcalc();
   mObsSigma.resize(mNbRefl);
   mWeight.resize(mNbRefl);
   mWeight=1;
   mObsSigma=0;
   mHasObservedData=true;
}


void DiffractionDataSingleCrystal::ImportHklIobs(const string &fileName,
                                    const long nbRefl,
                                    const int skipLines)
{
   //configure members
      mNbRefl=nbRefl;
      mH.resize(mNbRefl);
      mK.resize(mNbRefl);
      mL.resize(mNbRefl);
      mObsIntensity.resize(mNbRefl);
      mObsSigma.resize(mNbRefl);
      mWeight.resize(mNbRefl);
      mWeight=1;
   
   //Import data
   {	
      //:TODO: Skip the lines if required !!!
   	cout << "inputing reflections from file : "+fileName<<endl;
		ifstream fin (fileName.c_str());
      if(!fin)
      {
         throw ObjCrystException("DiffractionDataSingleCrystal::ImportHklIobs() : \
Error opening file for input:"+fileName);
      }
		cout << "Number of reflections to import : " << nbRefl << endl ;
      for(long i=0;i<nbRefl;i++)
      { // :TODO: A little faster....
         //:TODO: Check for the end of the stream if(!fin.good()) {throw...}
         fin >> mH(i);
         fin >> mK(i);
         fin >> mL(i);
         fin >> mObsIntensity(i);
         mWeight(i)=1./mObsIntensity(i);
         mObsSigma(i)=sqrt(mObsIntensity(i));
         //cout << mObsIntensity(i) <<endl;
      }
      cout << "Finished reading data>"<< endl;
      fin.close();
   }
  //Finish
   mWeight.resize(mNbRefl);
   mWeight=1;
   mObsSigma.resize(mNbRefl);
   mObsSigma=0;
   mHasObservedData=true;
   
   mMultiplicity.resize(mNbRefl);
   mMultiplicity=1;
   
   this->PrepareHKLarrays();
   
   cout << "Finished storing data..."<< endl ;
}

void DiffractionDataSingleCrystal::ImportHklIobsSigma(const string &fileName,
                                         const long nbRefl,
                                         const int skipLines)
{
   //configure members
      mNbRefl=nbRefl;
      mH.resize(mNbRefl);
      mK.resize(mNbRefl);
      mL.resize(mNbRefl);
      mObsIntensity.resize(mNbRefl);
      mObsSigma.resize(mNbRefl);
      mWeight.resize(mNbRefl);
      mWeight=1;
   
   //Import data
   {	
   	cout << "inputing reflections from file : "+fileName<<endl;
		ifstream fin (fileName.c_str());
      if(!fin)
      {
         throw ObjCrystException("DiffractionDataSingleCrystal::ImportHklIobsSigma() : \
Error opening file for input:"+fileName);
      }
      if(skipLines>0)
      {//Get rid of first lines if required
         char tmpComment[200];
         for(int i=0;i<skipLines;i++) fin.getline(tmpComment,150);
      }
		cout << "Number of reflections to import : " << nbRefl << endl ;
      for(long i=0;i<nbRefl;i++)
      { // :TODO: A little faster....
         //:TODO: Check for the end of the stream if(!fin.good()) {throw...}
         fin >> mH(i);
         fin >> mK(i);
         fin >> mL(i);
         fin >> mObsIntensity(i);
         fin >> mObsSigma(i);
        //cout << mH(i)<<" "<<mK(i)<<" "<<mL(i)<<" "<<mObsIntensity(i)<<" "<<mObsSigma(i)<<endl;
      }
      cout << "Finished reading data>"<< endl;
      fin.close();
   }
  //Finish
   mWeight.resize(mNbRefl);
   mWeight=1;
   mHasObservedData=true;

   mMultiplicity.resize(mNbRefl);
   mMultiplicity=1;

   this->PrepareHKLarrays();

   cout << "Finished storing data..."<< endl ;

}

void DiffractionDataSingleCrystal::ImportHklIobsSigmaJanaM91(const string &fileName)
{
   //configure members
      mNbRefl=1000;//reasonable beginning value ?
      mH.resize(mNbRefl);
      mK.resize(mNbRefl);
      mL.resize(mNbRefl);
      mObsIntensity.resize(mNbRefl);
      mObsSigma.resize(mNbRefl);
   
   //Import data
   {	
   	cout << "inputing reflections from Jana98 file : "+fileName<<endl;
		ifstream fin (fileName.c_str());
      if(!fin)
      {
         throw ObjCrystException("DiffractionDataSingleCrystal::ImportHklIobsSigmaJanaM91() : \
Error opening file for input:"+fileName);
      }
      long i=0;
      double tmpH;
      double junk;
      fin >> tmpH;
      while(tmpH != 999)
      { // :TODO: A little faster....
         //:TODO: Check for the end of the stream if(!fin.good()) {throw...}
         i++;
         if(i>=mNbRefl)
         {
            cout << mNbRefl << " reflections imported..." << endl;
            mNbRefl+=1000;
            mH.resizeAndPreserve(mNbRefl);
            mK.resizeAndPreserve(mNbRefl);
            mL.resizeAndPreserve(mNbRefl);
            mObsIntensity.resizeAndPreserve(mNbRefl);
            mObsSigma.resizeAndPreserve(mNbRefl);
         }
         mH(i)=tmpH;
         fin >> mK(i);
         fin >> mL(i);
         fin >> mObsIntensity(i);
         fin >> mObsSigma(i);
         fin >> junk;
         fin >> junk;
         fin >> junk;
         fin >> junk;
         fin >> tmpH;
      }
      fin.close();
      mNbRefl=i;
      cout << mNbRefl << " reflections imported." << endl;
      mH.resizeAndPreserve(mNbRefl);
      mK.resizeAndPreserve(mNbRefl);
      mL.resizeAndPreserve(mNbRefl);
      mObsIntensity.resizeAndPreserve(mNbRefl);
      mObsSigma.resizeAndPreserve(mNbRefl);
   }
  //Finish
   mWeight.resize(mNbRefl);
   mWeight=1;
   
   mMultiplicity.resize(mNbRefl);
   mMultiplicity=1;

   this->PrepareHKLarrays();
   
   cout << "Finished storing data..."<< endl ;

   mHasObservedData=true;
}
double DiffractionDataSingleCrystal::GetRw()const
{
   TAU_PROFILE("DiffractionData::Rw()"," double()",TAU_DEFAULT);
   VFN_DEBUG_MESSAGE("DiffractionData::Rw()",3);
   if(mHasObservedData==false)
   {
      throw ObjCrystException("DiffractionData::Rw() Cannot compute Rw ! \
         There is no observed data !");
   }
   double tmp1=0;
   double tmp2=0;
   const double *p1=mCalcIntensity.data();
   const double *p2=mObsIntensity.data();
   const double *p3=mWeight.data();
  for(long i=0;i<this->GetNbRefl();i++)
   {
      //avoid pow(f,i); too SLOW !
      //tmp1 += *p3 * pow( sqrt(*p1) - mScaleFactor*sqrt(*p2),2);
      
      tmp1 += *p3 * ( *p1 - *p2) * ( *p1 - *p2);
      tmp2 += *p3 * *p2 * *p2;
      p1++;p2++;p3++;
   }
   tmp1=sqrt(tmp1/tmp2);
   VFN_DEBUG_MESSAGE("DiffractionData::Rw()="<<tmp1,3);
   return tmp1 ;// /this->GetNbRefl();
}

double DiffractionDataSingleCrystal::GetR()const
{
   TAU_PROFILE("DiffractionData::R()"," double()",TAU_DEFAULT);
   VFN_DEBUG_MESSAGE("DiffractionData::R()",3);
   if(mHasObservedData==false)
   {
      throw ObjCrystException("DiffractionData::R() Cannot compute R ! \
         There is no observed data !");
   }
   
   double tmp1=0;
   double tmp2=0;
   const double *p1=mCalcIntensity.data();
   const double *p2=mObsIntensity.data();
   for(long i=0;i<this->GetNbRefl();i++)
   {
      //avoid pow(f,i); too SLOW !
      //tmp1 += *p3 * pow( sqrt(*p1) - mScaleFactor*sqrt(*p2),2);
      
      tmp1 += ( *p1 - *p2) * ( *p1 - *p2);
      tmp2 += *p2 * *p2;
      p1++;p2++;
   }
   tmp1=sqrt(tmp1/tmp2);
   
   VFN_DEBUG_MESSAGE("DiffractionData::R()="<<tmp1,3);
   return tmp1 ;
}

double DiffractionDataSingleCrystal::GetChi2()const
{
   TAU_PROFILE("DiffractionData::Chi2()"," double()",TAU_DEFAULT);
   VFN_DEBUG_MESSAGE("DiffractionData::Chi2()",3);
   if(mHasObservedData==false)
   {
      throw ObjCrystException("DiffractionData::Chi2() Cannot compute Chi^2 ! \
         There is no observed data !");
   }
   double tmp1=0;
   const double *p1=mCalcIntensity.data();
   const double *p2=mObsIntensity.data();
   const double *p3=mWeight.data();
  for(long i=0;i<this->GetNbRefl();i++)
   {
      //avoid pow(f,i); too SLOW !
      //tmp1 += *p3 * pow( sqrt(*p1) - mScaleFactor*sqrt(*p2),2);
      
      tmp1 += *p3++ * ( *p1 - *p2) * ( *p1 - *p2);
      p1++;p2++;
   }
   VFN_DEBUG_MESSAGE("DiffractionData::Chi2()="<<tmp1,3);
   return tmp1;
}

void DiffractionDataSingleCrystal::FitScaleFactorForRw()
{
   TAU_PROFILE("DiffractionData::FitScaleFactorForRw()","void ()",TAU_DEFAULT);
   VFN_DEBUG_MESSAGE("DiffractionData::FitScaleFactorForRw()",3);
   if(mHasObservedData==false)
   {//throw exception here ?
      throw ObjCrystException("DiffractionData::FitScaleFactorForRw() Cannot compute Rw \
         or scale factor: there is no observed data !");
   }
   double tmp1=0;
   double tmp2=0;
   const double *p1=mCalcIntensity.data();
   const double *p2=mObsIntensity.data();
   const double *p3=mWeight.data();
   for(long i=0;i<this->GetNbRefl();i++)
   {
      tmp1 += *p3 * (*p1) * (*p2++);
      tmp2 += *p3++ * (*p1) * (*p1);
      p1++;
   }
   mScaleFactor      *= tmp1/tmp2;
   mCalcIntensity *= tmp1/tmp2;
   
}

void DiffractionDataSingleCrystal::FitScaleFactorForR()
{
   TAU_PROFILE("DiffractionData::FitScaleFactorForR()","void ()",TAU_DEFAULT);
   VFN_DEBUG_MESSAGE("DiffractionData::FitScaleFactorForR()",3);
   if(mHasObservedData==false)
   {//throw exception here ?
      throw ObjCrystException("DiffractionData::FitScaleFactorForR() Cannot compute R \
         or scale factor: there is no observed data !");
   }
   double tmp1=0;
   double tmp2=0;
   const double *p1=mCalcIntensity.data();
   const double *p2=mObsIntensity.data();
   for(long i=0;i<this->GetNbRefl();i++)
   {
      tmp1 += (*p1) * (*p2++);
      tmp2 += (*p1) * (*p1);
      p1++;
   }
   mScaleFactor      *= tmp1/tmp2;
   mCalcIntensity *= tmp1/tmp2;
}

double DiffractionDataSingleCrystal::GetBestRFactor()
{
   TAU_PROFILE("DiffractionData::GetBestRFactor()","void ()",TAU_DEFAULT);
   VFN_DEBUG_MESSAGE("DiffractionData::GetBestRFactor()",3);
   if(mHasObservedData==false)
   {
      throw ObjCrystException("DiffractionData::GetBestRFactor() Cannot compute R \
         or scale factor: there is no observed data !");
   }
   this->FitScaleFactorForR();
   return this->GetR();
}

void DiffractionDataSingleCrystal::SetSigmaToSqrtIobs()
{
   for(long i=0;i<mObsIntensity.numElements();i++) mObsSigma(i)=sqrt(fabs(mObsIntensity(i)));
}

void DiffractionDataSingleCrystal::SetWeightToInvSigma2(const double minRelatSigma)
{
   //:KLUDGE: If less than 1e-6*max, set to 0.... Do not give weight to unobserved points
   const double min=MaxAbs(mObsSigma)*minRelatSigma;
   for(long i=0;i<mObsSigma.numElements();i++)
   {
      if(mObsSigma(i)<min) mWeight(i)=0 ; else  mWeight(i) =1./mObsSigma(i)/mObsSigma(i);
   }
}

double DiffractionDataSingleCrystal::GetScaleFactor()const {return mScaleFactor;}

//void DiffractionDataSingleCrystal::SetScaleFactor(const double s) {mScaleFactor=s;}

void DiffractionDataSingleCrystal::PrintObsData()const
{
   this->CalcSinThetaLambda();
   cout << "DiffractionData : " << mName <<endl;
   cout << "Number of observed reflections : " << mNbRefl << endl;
   cout << "       H        K        L     Iobs        Sigma       sin(theta)/lambda)" <<endl;
   cout << mH.numElements()<<endl;
   cout << mK.numElements()<<endl;
   cout << mL.numElements()<<endl;
   cout << mObsIntensity.numElements()<<endl;
   cout << mObsSigma.numElements()<<endl;
   cout << mSinThetaLambda.numElements()<<endl;
   cout << FormatVertVectorHKLFloats<double>
               (mH,mK,mL,mObsIntensity,mObsSigma,mSinThetaLambda,12,4);
}

void DiffractionDataSingleCrystal::PrintObsCalcData()const
{
   this->CalcIcalc();
   CrystVector_double tmpTheta=mTheta;
   tmpTheta*= RAD2DEG;
   /*
   CrystVector_double tmp=mObsIntensity;
   CrystVector_double tmpS=mObsSigma;
   if(true==mHasObservedData)
   {
      cout << "Scale factor : " << mScaleFactor <<endl;
      tmp*= mScaleFactor*mScaleFactor;//the scale factor is computed for F's
      tmpS*=mScaleFactor;
   } else cout << "No observed data. Default scale factor =1." <<endl;
   */
   cout << "DiffractionData : " << mName <<endl;
   cout << " Scale Factor : " << mScaleFactor <<endl;
   cout << "Number of observed reflections : " << mNbRefl << endl;
   
   cout << "       H        K        L     Iobs        Sigma       Icalc  ";
   cout << "      multiplicity     Theta      SiThSL       Re(F)     Im(F)    Weight" <<endl;
   cout << FormatVertVectorHKLFloats<double>(mH,mK,mL,
               mObsIntensity,mObsSigma,mCalcIntensity,
               mMultiplicity,tmpTheta,mSinThetaLambda,
               mFhklCalcReal,mFhklCalcImag,mWeight,12,4);
}

void DiffractionDataSingleCrystal::SetUseOnlyLowAngleData(
                     const bool useOnlyLowAngle,const double angle)
{
   throw ObjCrystException("DiffractionDataSingleCrystal::SetUseOnlyLowAngleData() :\
 not yet implemented for DiffractionDataSingleCrystal.");
}

void DiffractionDataSingleCrystal::SaveHKLIobsIcalc(const string &filename)
{
   VFN_DEBUG_MESSAGE("DiffractionDataSingleCrystal::SaveHKLIobsIcalc",5)
   //:TODO: test if everything is ready...
   ofstream out(filename.c_str());
   CrystVector_double theta;
   theta=mTheta;
   theta *= RAD2DEG;
   
   if(false == mHasObservedData)
   {
      out << "#    H        K        L      Icalc    theta  sin(theta)/lambda"
          <<"  Re(F)   Im(F)" << endl;
      out << FormatVertVectorHKLFloats<double>(mH,mK,mL,mCalcIntensity,
                           theta,mSinThetaLambda,mFhklCalcReal,mFhklCalcImag,12,4);
   }
   else
   {
      out << "#    H        K        L      Iobs   Icalc    theta"
          <<" sin(theta)/lambda  Re(F)   Im(F)" << endl;
      out << FormatVertVectorHKLFloats<double>(mH,mK,mL,mObsIntensity,mCalcIntensity,
                           theta,mSinThetaLambda,mFhklCalcReal,mFhklCalcImag,12,4);
   }
   out.close();
   VFN_DEBUG_MESSAGE("DiffractionDataSingleCrystal::SaveHKLIobsIcalc:End",3)
}

unsigned int DiffractionDataSingleCrystal::GetNbCostFunction()const {return 2;}

const string& DiffractionDataSingleCrystal::GetCostFunctionName(const unsigned int id)const
{
   static string costFunctionName[2];
   if(0==costFunctionName[0].length())
   {
      costFunctionName[0]="R()";
      costFunctionName[1]="Rw()";
   }
   switch(id)
   {
      case 0: return costFunctionName[0];
      case 1: return costFunctionName[1];
      default:
      {
         cout << "DiffractionDataSingleCrystal::GetCostFunctionName(): Not Found !" <<endl;
         throw 0;
      }
   }
}

const string& DiffractionDataSingleCrystal::GetCostFunctionDescription(const unsigned int id)const
{
   static string costFunctionDescription[2];
   if(0==costFunctionDescription[0].length())
   {
      costFunctionDescription[0]="Crystallographic, unweighted R-factor";
      costFunctionDescription[1]="Crystallographic, weigthed R-factor";
   }
   switch(id)
   {
      case 0: return costFunctionDescription[0];
      case 1: return costFunctionDescription[1];
      default:
      {
         cout << "DiffractionDataSingleCrystal::GetCostFunctionDescription(): Not Found !" 
              <<endl;
         throw 0;
      }
   }
}

double DiffractionDataSingleCrystal::GetCostFunctionValue(const unsigned int n)
{
   VFN_DEBUG_MESSAGE("DiffractionDataSingleCrystal::GetCostFunctionValue():"<<mName,4)
   this->CalcIcalc();
   switch(n)
   {
      case 0: this->FitScaleFactorForR()  ;return this->GetR();
      case 1: this->FitScaleFactorForRw() ;return this->GetRw();
      default:
      {
         cout << "DiffractionDataSingleCrystal::GetCostFunctionValue(): Not Found !" <<endl;
         throw 0;
      }
   }
}

void DiffractionDataSingleCrystal::InitRefParList()
{
   VFN_DEBUG_MESSAGE("DiffractionDataSingleCrystal::InitRefParList()",5)
   //:TODO:
//   throw ObjCrystException("DiffractionDataSingleCrystal::InitRefParList() :
// not yet implemented !");
   this->ResetParList();
   cout << "DiffractionDataSingleCrystal::InitRefParList():no parameters !" <<endl;
}

void DiffractionDataSingleCrystal::CalcIcalc() const
{
   TAU_PROFILE("DiffractionData::CalcIcalc()","void ()",TAU_DEFAULT);
   VFN_DEBUG_MESSAGE("DiffractionData::CalcIcalc():"<<this->GetName(),3)
   mCalcIntensity=this->GetFhklCalcSq();
   mCalcIntensity*=mScaleFactor;
}

}//namespace
