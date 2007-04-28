/*  ObjCryst++ Object-Oriented Crystallographic Library
    (c) 2006- Vincent Favre-Nicolin vincefn@users.sourceforge.net

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
*  source file for Indexing classes & functions
*
*/
#include <algorithm>

#include "ObjCryst/Indexing.h"
#include "Quirks/VFNDebug.h"
#include "Quirks/VFNStreamFormat.h"

using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

#ifndef DEG2RAD
#define DEG2RAD (M_PI/180.)
#endif
#ifndef RAD2DEG
#define RAD2DEG (180./M_PI)
#endif

namespace ObjCryst
{

float EstimateCellVolume(const float dmin, const float dmax, const float nbrefl, 
                         const CrystalSystem system,const CrystalCentering centering,const float kappa)
{
   const float q1=dmin*dmin*dmin-dmax*dmax*dmax;
   const float q2=dmin*dmin     -dmax*dmax;
   float D0,C0;
   if(system==TRICLINIC)
   {
      C0=2.095;
      return nbrefl/(C0*kappa*q1);
   }
   if(system==CUBIC)
   {
      if(centering==LATTICE_P) D0=0.862;
      if(centering==LATTICE_I) D0=0.475;
      if(centering==LATTICE_F) D0=0.354;
      return pow(nbrefl/(D0*kappa*q2),(float)1.5);
   }
   // "*.85" means using D0_min rather than D0
   if(system==MONOCLINIC) {C0=1.047;D0=0.786*.85;}
   if(system==ORTHOROMBIC){C0=0.524;D0=1.36 *.85;}
   if(system==HEXAGONAL)  {C0=0.150;D0=1.04 *.85;}
   if(system==RHOMBOEDRAL){C0=0.230;D0=1.04 *.85;}
   if(system==TETRAGONAL) {C0=0.214;D0=1.25 *.85;}
   if((centering==LATTICE_I)||(centering==LATTICE_A)||(centering==LATTICE_B)||(centering==LATTICE_C)) {C0/=2;D0/=2;}
   if(centering==LATTICE_F){C0/=4;D0/=4;}
   const double alpha=D0*q2/(3*C0*q1), beta=nbrefl/(2*kappa*C0*q1);
   const double eta=beta-alpha*alpha*alpha,gamma=sqrt(beta*beta-2*beta*alpha*alpha*alpha);
   const float v=pow(pow(eta+gamma,(double)1/3.)+pow((eta-gamma),(double)1/3.)-alpha,(double)3);
   return v;
}

/** light-weight class storing the reciprocal space unitcell
*/

RecUnitCell::RecUnitCell(const float zero,const float p0,const float p1,const float p2,
                         const float p3,const float p4,const float p5,CrystalSystem lattice):
mlattice(lattice)
{
   par[0]=zero;
   par[1]=p0;
   par[2]=p1;
   par[3]=p2;
   par[4]=p3;
   par[5]=p4;
   par[6]=p5;
}

RecUnitCell::RecUnitCell(const RecUnitCell &old)
{
   *this=old;
}

void RecUnitCell::operator=(const RecUnitCell &rhs)
{
   for(unsigned int i=0;i<7;++i) par[i]=rhs.par[i];
   mlattice=rhs.mlattice;
}

float RecUnitCell::hkl2d(const float h,const float k,const float l,REAL *derivpar,const unsigned int derivhkl) const
{
   if((derivpar==NULL)&&(derivhkl==0))
   {
      switch(mlattice)
      {
         case TRICLINIC:
         {
            return par[0]+par[1]*par[1]*h*h + par[2]*par[2]*k*k + par[3]*par[3]*l*l
                  + 2*par[1]*par[2]*par[4]*h*k + 2*par[2]*par[3]*par[5]*k*l + 2*par[1]*par[3]*par[6]*h*l;
            break;
         }
         case MONOCLINIC:
         {
            return par[0]+par[1]*par[1]*h*h + par[2]*par[2]*k*k + par[3]*par[3]*l*l + 2*par[1]*par[3]*par[4]*h*l;
            break;
         }
         case ORTHOROMBIC:
         {
            return par[0]+par[1]*par[1]*h*h + par[2]*par[2]*k*k + par[3]*par[3]*l*l;
            break;
         }
         case HEXAGONAL:
         {
            return par[0]+par[1]*par[1]*(h*h + k*k + h*k)+ par[2]*par[2]*l*l ;
            break;
         }
         case RHOMBOEDRAL:
         {
            return par[0]+par[1]*par[1]*(h*h + k*k + l*l + 2*par[2]*(h*k + k*l + h*l));
            break;
         }
         case TETRAGONAL:
         {
            return par[0]+par[1]*par[1]*(h*h + k*k) + par[2]*par[2]*l*l;
            break;
         }
         case CUBIC:
         {
            return par[0]+par[1]*par[1]*(h*h+k*k+l*l);
            break;
         }
      }
   }
   if(derivhkl==1)
   {
      switch(mlattice)
      {
         case TRICLINIC:
         {
            return 2*par[1]*par[1]*h + 2*par[1]*par[2]*par[4]*k + 2*par[1]*par[3]*par[6]*l;
            break;
         }
         case MONOCLINIC:
         {
            return 2*par[1]*par[1]*h + 2*par[1]*par[3]*par[4]*l;
            break;
         }
         case ORTHOROMBIC:
         {
            return 2*par[1]*par[1]*h;
            break;
         }
         case HEXAGONAL:
         {
            return par[1]*par[1]*(2*h + k);
            break;
         }
         case RHOMBOEDRAL:
         {
            return par[1]*par[1]*(2*h + 2*par[2]*(k + l));
            break;
         }
         case TETRAGONAL:
         {
            return 2*par[1]*par[1]*h;
            break;
         }
         case CUBIC:
         {
            return 2*par[1]*par[1]*h;
            break;
         }
      }
   }
   if(derivhkl==2)
   {
      switch(mlattice)
      {
         case TRICLINIC:
         {
            return 2*par[2]*par[2]*k + 2*par[1]*par[2]*par[4]*h + 2*par[2]*par[3]*par[5]*l;
            break;
         }
         case MONOCLINIC:
         {
            return 2*par[2]*par[2]*k;
            break;
         }
         case ORTHOROMBIC:
         {
            return 2*par[2]*par[2]*k;
            break;
         }
         case HEXAGONAL:
         {
            return par[1]*par[1]*(2*k + h);
            break;
         }
         case RHOMBOEDRAL:
         {
            return par[1]*par[1]*(2*k + l*l + 2*par[2]*(h + l));
            break;
         }
         case TETRAGONAL:
         {
            return 2*par[1]*par[1]*k;
            break;
         }
         case CUBIC:
         {
            return 2*par[1]*par[1]*k;
            break;
         }
      }
   }
   if(derivhkl==3)
   {
      switch(mlattice)
      {
         case TRICLINIC:
         {
            return 2*par[3]*par[3]*l + 2*par[2]*par[3]*par[5]*k + 2*par[1]*par[3]*par[6]*h;
            break;
         }
         case MONOCLINIC:
         {
            return 2*par[3]*par[3]*l + 2*par[1]*par[3]*par[4]*h;
            break;
         }
         case ORTHOROMBIC:
         {
            return 2*par[3]*par[3]*l;
            break;
         }
         case HEXAGONAL:
         {
            return 2*par[2]*par[2]*l;
            break;
         }
         case RHOMBOEDRAL:
         {
            return par[1]*par[1]*(2*l + 2*par[2]*(k + h));
            break;
         }
         case TETRAGONAL:
         {
            return 2*par[2]*par[2]*l;
            break;
         }
         case CUBIC:
         {
            return 2*par[1]*par[1]*l;
            break;
         }
      }
   }

   if(derivpar==&par[0]) return 1.0;
   if(derivpar==(par+1))
   {
      switch(mlattice)
      {
         case TRICLINIC:
         {
            return 2*par[1]*h*h + 2*par[2]*par[4]*h*k + 2*par[3]*par[6]*h*l;
            break;
         }
         case MONOCLINIC:
         {
            return 2*par[1]*h*h + 2*par[3]*par[4]*h*l;
            break;
         }
         case ORTHOROMBIC:
         {
            return 2*par[1]*h*h;
            break;
         }
         case HEXAGONAL:
         {
            return 2*par[1]*(h*h + k*k + h*k);
            break;
         }
         case RHOMBOEDRAL:
         {
            return 2*par[1]*(h*h + k*k + l*l + 2*par[2]*(h*k + k*l + h*l));
            break;
         }
         case TETRAGONAL:
         {
            return 2*par[1]*(h*h + k*k);
            break;
         }
         case CUBIC:
         {
            return 2*par[1]*(h*h+k*k+l*l);
            break;
         }
      }
   }
   if(derivpar==(par+2))
   {
      switch(mlattice)
      {
         case TRICLINIC:
         {
            return 2*par[2]*k*k + 2*par[1]*par[4]*h*k + 2*par[2]*par[5]*k*l;
            break;
         }
         case MONOCLINIC:
         {
            return 2*par[2]*k*k;
            break;
         }
         case ORTHOROMBIC:
         {
            return 2*par[2]*k*k;
            break;
         }
         case HEXAGONAL:
         {
            return 2*par[2]*l*l ;
            break;
         }
         case RHOMBOEDRAL:
         {
            return par[1]*par[1]*(h*h + k*k + l*l + 2*(h*k + k*l + h*l));
            break;
         }
         case TETRAGONAL:
         {
            return 2*par[2]*l*l;
            break;
         }
         case CUBIC:
         {
            throw 0;
            break;
         }
      }
   }
   if(derivpar==(par+3))
   {
      switch(mlattice)
      {
         case TRICLINIC:
         {
            return 2*par[3]*l*l + 2*par[2]*par[5]*k*l + 2*par[1]*par[6]*h*l;
            break;
         }
         case MONOCLINIC:
         {
            return 2*par[3]*l*l + 2*par[1]*par[4]*h*l;
            break;
         }
         case ORTHOROMBIC:
         {
            return 2*par[3]*l*l;
            break;
         }
         case HEXAGONAL:
         {
            throw 0;
            break;
         }
         case RHOMBOEDRAL:
         {
            throw 0;
            break;
         }
         case TETRAGONAL:
         {
            throw 0;
            break;
         }
         case CUBIC:
         {
            throw 0;
            break;
         }
      }
   }
   if(derivpar==(par+4))
   {
      switch(mlattice)
      {
         case TRICLINIC:
         {
            return 2*par[1]*par[2]*h*k;
            break;
         }
         case MONOCLINIC:
         {
            return 2*par[1]*par[3]*h*l;
            break;
         }
         default:
         {
            throw 0;
            break;
         }
      }
   }
   if(derivpar==(par+5))
   {
      switch(mlattice)
      {
         case TRICLINIC:
         {
            return 2*par[2]*par[3]*k*l;
            break;
         }
         default:
         {
            throw 0;
            break;
         }
      }
   }
   if(derivpar==(par+6))
   {
      switch(mlattice)
      {
         case TRICLINIC:
         {
            return 2*par[1]*par[3]*h*l;
            break;
         }
         default:
         {
            throw 0;
            break;
         }
      }
   }
   throw 0;
   return 0.0;
}

void RecUnitCell::hkl2d_delta(const float h,const float k,const float l,
                              const RecUnitCell &delta, float & dmin, float &dmax) const
{
   const float p0m=par[0]-delta.par[0] , p0p=par[0]+delta.par[0],
               p1m=par[1]-delta.par[1] , p1p=par[1]+delta.par[1],
               p2m=par[2]-delta.par[2] , p2p=par[2]+delta.par[2],
               p3m=par[3]-delta.par[3] , p3p=par[3]+delta.par[3],
               p4m=par[4]-delta.par[4] , p4p=par[4]+delta.par[4],
               p5m=par[5]-delta.par[5] , p5p=par[5]+delta.par[5],
               p6m=par[6]-delta.par[6] , p6p=par[6]+delta.par[6];
   switch(mlattice)
   {
      case TRICLINIC://TODO
      {
         float p0mm,p1mm,p2mm,p3mm,p4mm,p5mm,p6mm,p0pp,p1pp,p2pp,p3pp,p4pp,p5pp,p6pp;
         if((h*(par[1]*h+par[2]*par[4]*k+par[3]*par[6]*l))>0) {p1mm=p1m;p1pp=p1p;}else{p1mm=p1p;p1pp=p1m;}
         if((k*(par[2]*k+par[1]*par[4]*h+par[3]*par[5]*l))>0) {p2mm=p2m;p2pp=p2p;}else{p2mm=p2p;p2pp=p2m;}
         if((l*(par[3]*l+par[2]*par[5]*k+par[1]*par[6]*h))>0) {p3mm=p3m;p3pp=p3p;}else{p3mm=p3p;p3pp=p3m;}
         if((h*k)>0){p4mm=p4m;p4pp=p4p;}else{p4mm=p4p;p4pp=p4m;}
         if((k*l)>0){p5mm=p5m;p5pp=p5p;}else{p5mm=p5p;p5pp=p5m;}
         if((h*l)>0){p6mm=p6m;p6pp=p6p;}else{p6mm=p6p;p6pp=p6m;}
         dmin=p0m+p1mm*p1mm*h*h+p2mm*p2mm*k*k+p3mm*p3mm*l*l+2*p1mm*p2mm*p4mm*h*k+2*p2mm*p3mm*p5mm*k*l+2*p1mm*p3mm*p6mm*h*l;
         dmax=p0p+p1pp*p1pp*h*h+p2pp*p2pp*k*k+p3pp*p3pp*l*l+2*p1pp*p2pp*p4pp*h*k+2*p2pp*p3pp*p5pp*k*l+2*p1pp*p3pp*p6pp*h*l;
         return;
      }
      case MONOCLINIC: //OK
      {
         if((h*l)>0)
         {
            dmin = p0m + p1m*p1m*h*h + p2m*p2m*k*k + p3m*p3m*l*l + 2*p1m*p3m*p4m*h*l;
            dmax = p0p + p1p*p1p*h*h + p2p*p2p*k*k + p3p*p3p*l*l + 2*p1p*p3p*p4p*h*l;
            return;
         }
         else
         {
            const bool b1=(h*(par[1]*h+par[3]*par[4]*l))>0;// d(d*^2)/dp1
            const bool b3=(l*(par[3]*l+par[1]*par[4]*h))>0;// d(d*^2)/dp2
            if(b1 && b3)
            {
               dmin = p0m + p1m*p1m*h*h + p2m*p2m*k*k + p3m*p3m*l*l + 2*p1m*p3m*p4p*h*l;
               dmax = p0p + p1p*p1p*h*h + p2p*p2p*k*k + p3p*p3p*l*l + 2*p1p*p3p*p4m*h*l;
               return;
            }
            else if(b1 && (!b3))
               {
                  dmin = p0m + p1m*p1m*h*h + p2m*p2m*k*k + p3p*p3p*l*l + 2*p1m*p3p*p4p*h*l;
                  dmax = p0p + p1p*p1p*h*h + p2p*p2p*k*k + p3m*p3m*l*l + 2*p1p*p3m*p4m*h*l;
                  return;
               }
               else if((!b1) && b3)
                  {
                     dmin = p0m + p1p*p1p*h*h + p2m*p2m*k*k + p3m*p3m*l*l + 2*p1p*p3m*p4p*h*l;
                     dmax = p0p + p1m*p1m*h*h + p2p*p2p*k*k + p3p*p3p*l*l + 2*p1m*p3p*p4m*h*l;
                     return;
                  }
                  else
                  {
                     dmin = p0m + p1p*p1p*h*h + p2m*p2m*k*k + p3p*p3p*l*l + 2*p1p*p3p*p4p*h*l;
                     dmax = p0p + p1m*p1m*h*h + p2p*p2p*k*k + p3m*p3m*l*l + 2*p1m*p3m*p4m*h*l;
                     return;
                  }
            }
      }
      case ORTHOROMBIC: //OK
      {
         dmin= p0m + p1m*p1m*h*h + p2m*p2m*k*k + p3m*p3m*l*l;
         dmax= p0p + p1p*p1p*h*h + p2p*p2p*k*k + p3p*p3p*l*l;
         return;
      }
      case HEXAGONAL: //OK
      {
         dmin=p0m+p1m*p1m*(h*h + k*k + h*k)+ p2m*p2m*l*l ;
         dmax=p0p+p1p*p1p*(h*h + k*k + h*k)+ p2p*p2p*l*l ;
         return;
      }
      case RHOMBOEDRAL:
      {
         if((h*k + k*l + h*l)>=0)
         {
            dmin= p0m+p1m*p1m*(h*h + k*k + l*l + 2*p2m*(h*k + k*l + h*l));
            dmax= p0p+p1p*p1p*(h*h + k*k + l*l + 2*p2p*(h*k + k*l + h*l));
         }
         else
         {
            dmin= p0m+p1m*p1m*(h*h + k*k + l*l + 2*p2p*(h*k + k*l + h*l));
            dmax= p0p+p1p*p1p*(h*h + k*k + l*l + 2*p2m*(h*k + k*l + h*l));
         }
         return;
      }
      case TETRAGONAL: //OK
      {
         dmin= p0m + p1m*p1m*(h*h + k*k) + p2m*p2m*l*l;
         dmax= p0p + p1p*p1p*(h*h + k*k) + p2p*p2p*l*l;
         return;
      }
      case CUBIC: //OK
      {
         dmin=p0m + p1m*p1m*(h*h+k*k+l*l);
         dmax=p0p + p1p*p1p*(h*h+k*k+l*l);
         return;
      }
   }
}

vector<float> RecUnitCell::DirectUnitCell()const
{
   // reciprocal unit cell parameters
   float aa,bb,cc,calphaa,cbetaa,cgammaa,salphaa,sbetaa,sgammaa;
   switch(mlattice)
   {
      case TRICLINIC:
      {
         aa=par[1];
         bb=par[2];
         cc=par[3];
         calphaa=par[5];
         cbetaa=par[6];
         cgammaa=par[4];
         salphaa=sqrt(abs(1-calphaa*calphaa));
         sbetaa =sqrt(abs(1-cbetaa *cbetaa));
         sgammaa=sqrt(abs(1-cgammaa*cgammaa));
         break;
      }
      case MONOCLINIC:
      {
         aa=par[1];
         bb=par[2];
         cc=par[3];
         calphaa=0;
         cbetaa=par[4];
         cgammaa=0;
         salphaa=1;
         sbetaa =sqrt(abs(1-cbetaa *cbetaa));
         sgammaa=1;
         break;
      }
      case ORTHOROMBIC:
      {
         aa=par[1];
         bb=par[2];
         cc=par[3];
         calphaa=0;
         cbetaa =0;
         cgammaa=0;
         salphaa=1;
         sbetaa =1;
         sgammaa=1;
         break;
      }
      case HEXAGONAL:
      {
         aa=par[1];
         bb=par[1];
         cc=par[2];
         calphaa=0;
         cbetaa =0;
         cgammaa=0.5;
         salphaa=1;
         sbetaa =1;
         sgammaa=0.8660254037844386;
         break;
      }
      case RHOMBOEDRAL:
      {
         aa=par[1];
         bb=par[1];
         cc=par[1];
         calphaa=par[4];
         cbetaa =par[4];
         cgammaa=par[4];
         salphaa=sqrt(abs(1-calphaa *calphaa));
         sbetaa =salphaa;
         sgammaa=salphaa;
         break;
      }
      case TETRAGONAL:
      {
         aa=par[1];
         bb=par[1];
         cc=par[2];
         calphaa=0;
         cbetaa =0;
         cgammaa=0;
         salphaa=1;
         sbetaa =1;
         sgammaa=1;
         break;
      }
      case CUBIC:
      {
         aa=par[1];
         bb=par[1];
         cc=par[1];
         calphaa=0;
         cbetaa =0;
         cgammaa=0;
         salphaa=1;
         sbetaa =1;
         sgammaa=1;
         break;
      }
   }
   // Volume of reciprocal unit cell
   const float vv=sqrt(abs(1-calphaa*calphaa-cbetaa*cbetaa-cgammaa*cgammaa+2*calphaa*cbetaa*cgammaa));

   const float a=salphaa/(aa*vv);
   const float b=sbetaa /(bb*vv);
   const float c=sgammaa/(cc*vv);

   const float calpha=(cbetaa *cgammaa-calphaa)/(sbetaa *sgammaa);
   const float cbeta =(calphaa*cgammaa-cbetaa )/(salphaa*sgammaa);
   const float cgamma=(calphaa*cbetaa -cgammaa)/(salphaa*sbetaa );
   
   const float alpha=acos( calpha );
   const float beta =acos( cbeta );
   const float gamma=acos( cgamma );

   const float v=a*b*c*sqrt(1-calpha*calpha-cbeta*cbeta-cgamma*cgamma+2*calpha*cbeta*cgamma);
   
   vector<float> uc(7);
   uc[0]=a;
   uc[1]=b;
   uc[2]=c;
   uc[3]=alpha;
   uc[4]=beta;
   uc[5]=gamma;
   uc[6]=v;
   return uc;
}
///////////////////////////////////////////////// PEAKLIST:HKL0 /////////////////////
PeakList::hkl0::hkl0(const int h0,const int k0, const int l0):
h(h0),k(k0),l(l0)
{}

///////////////////////////////////////////////// PEAKLIST:HKL /////////////////////
PeakList::hkl::hkl(const float d,const float i,const float ds,const float is,
                   const int h0,const int k0, const int l0,const float dc0):
dobs(d),dobssigma(ds),d2obs(d*d),d2obsmin((d-ds/2)*(d-ds/2)),d2obsmax((d+ds/2)*(d+ds/2)),iobs(i),iobssigma(is),
h(h0),k(k0),l(l0),isIndexed(false),isSpurious(false),stats(0),
d2calc(dc0),d2diff(0)
{}

bool compareHKL_d(const PeakList::hkl &d1, const PeakList::hkl &d2)
{
   return d1.dobs < d2.dobs;
}


///////////////////////////////////////////////// PEAKLIST /////////////////////

PeakList::PeakList()
{}

PeakList::PeakList(const PeakList &old)
{
   *this=old;
}

void PeakList::operator=(const PeakList &rhs)
{
   VFN_DEBUG_ENTRY("PeakList::operator=(PeakList &old)",10);
   mvHKL=rhs.mvHKL;
   VFN_DEBUG_EXIT("PeakList::operator=(PeakList &old)",10);
}

PeakList::~PeakList()
{}

void PeakList::ImportDhklDSigmaIntensity(istream &is,float defaultsigma)
{
   float d,sigma,iobs;
   while(true)
   {// :TODO: use readline to make sure when the end is reached
      is >>d;
      if(is.eof()) break;
      is>>sigma;
      if(is.eof()) break;
      is>>iobs;
      if(sigma<=0) sigma=d*defaultsigma;
      if(iobs<=0) iobs=1.0;
      mvHKL.push_back(hkl(1/d,iobs,1/(d-sigma/2)-1/(d+sigma/2)));
      cout<<__FILE__<<":"<<__LINE__<<"  "<<mvHKL.size()<<":d="<<d<<"+/-"<<sigma<<", I="<<iobs<<" 1/d="<<1/d<<endl;
      if(is.eof()) break;
   }
   sort(mvHKL.begin(),mvHKL.end(),compareHKL_d);
   cout<<"Imported "<<mvHKL.size()<<" observed reflection positions."<<endl;
}

void PeakList::ImportDhklIntensity(istream &is)
{
   float d,iobs;
   while(true)
   {// :TODO: use readline to make sure when the end is reached
      is >>d;
      if(is.eof()) break;
      is>>iobs;
      mvHKL.push_back(hkl(1/d,iobs));
      cout<<__FILE__<<":"<<__LINE__<<"  "<<mvHKL.size()<<":d="<<d<<", I="<<iobs<<" 1/d="<<1/d<<endl;
      if(is.eof()) break;
   }
   sort(mvHKL.begin(),mvHKL.end(),compareHKL_d);
   cout<<"Imported "<<mvHKL.size()<<" observed reflection positions."<<endl;
}

void PeakList::ImportDhkl(istream &is)
{
   std::vector<std::pair<float,float> > v;
   float d;
   while(true)
   {// :TODO: use readline to make sure when the end is reached
      is >>d;
      if(is.eof()) break;
      mvHKL.push_back(hkl(1/d));
      cout<<__FILE__<<":"<<__LINE__<<"  "<<mvHKL.size()<<":d="<<d<<" 1/d="<<1/d<<endl;
      if(is.eof()) break;
   }
   sort(mvHKL.begin(),mvHKL.end(),compareHKL_d);
   cout<<"Imported "<<v.size()<<" observed reflection positions."<<endl;
}


template<class T,class U> bool comparePairFirst(std::pair<T,U> &p1, std::pair<T,U> &p2)
{
   return p1.first < p2.first;
}

void PeakList::Import2ThetaIntensity(istream &is, const float wavelength)
{
   std::list<std::pair<float,float> > v;
   float d,iobs;
   while(true)
   {// :TODO: use readline to make sure when the end is reached
      is >>d;
      if(is.eof()) break;
      is>>iobs;
      d=2*sin(d/2*DEG2RAD)/wavelength;
      mvHKL.push_back(hkl(1/d,iobs));
      cout<<__FILE__<<":"<<__LINE__<<"  "<<mvHKL.size()<<":d="<<1/d<<", I="<<iobs<<" 1/d="<<d<<endl;
      if((is.eof())||v.size()>=20) break;
   }
   sort(mvHKL.begin(),mvHKL.end(),compareHKL_d);
   cout<<"Imported "<<v.size()<<" observed reflection positions."<<endl;
}

void PeakList::ExportDhklDSigmaIntensity(std::ostream &os)const
{
   char buf[100];
   for(vector<PeakList::hkl>::const_iterator pos=mvHKL.begin();pos!=mvHKL.end();++pos)
   {
      const float sigma=1/(pos->dobs-pos->dobssigma/2)-1/(pos->dobs+pos->dobssigma/2);
      sprintf(buf,"%6.3f %6.3f %f",1/pos->dobs,sigma,pos->iobs);
      os<<buf<<endl;
   }
}

void PeakList::AddPeak(const float d, const float iobs,const float dobssigma,const float iobssigma,
                       const int h,const int k, const int l,const float d2calc)
{
   if(dobssigma<=0)
   {// Manually added peak ? Use other reflection's sigmas to evaluate sigma for this reflection
      float s=0;
      for(vector<hkl>::const_iterator pos=mvHKL.begin();pos!=mvHKL.end();++pos)
         s+= pos->dobssigma;
      s/=mvHKL.size();
      mvHKL.push_back(hkl(d,iobs,s,iobssigma,h,k,l,d2calc));
   }
   else mvHKL.push_back(hkl(d,iobs,dobssigma,iobssigma,h,k,l,d2calc));
   sort(mvHKL.begin(),mvHKL.end(),compareHKL_d);
   this->Print(cout);
}

void PeakList::RemovePeak(unsigned int idx)
{
   for(unsigned int i=idx;i<(mvHKL.size()-1);++i) mvHKL[i]=mvHKL[i+1];
   mvHKL.pop_back();
}

void PeakList::Print(std::ostream &os) const
{
   unsigned int i=0;
   char buf[200];
   os<<"PeakList, with "<<mvHKL.size()<<" peaks"<<endl;
   for(vector<PeakList::hkl>::const_iterator pos=mvHKL.begin();pos!=mvHKL.end();++pos)
   {
      const float sigma=1/(pos->dobs-pos->dobssigma/2)-1/(pos->dobs+pos->dobssigma/2);
      if(pos->isIndexed)
         sprintf(buf,"#%3d d=%6.3f+/-%7.4f dcalc=%6.3f, diff=%7.4f, iobs=%6.3f HKL=%2d %2d %2d Spurious=%1d stats=%6d",
                 i++,1/pos->dobs,sigma,
                 1/sqrt(pos->d2calc),1/sqrt(pos->d2calc)-1/pos->dobs,
                 pos->iobs,pos->h,pos->k,pos->l,pos->isSpurious,pos->stats);
      else
         sprintf(buf,"#%3d d=%6.3f+/-%6.3f              iobs=%6.3f  UNINDEXED   Spurious=%1d stats=%6d",
                 i++,1/pos->dobs,1/(pos->dobs-pos->dobssigma/2)-1/(pos->dobs+pos->dobssigma/2),
                 pos->iobs,pos->isSpurious,pos->stats);
      os<<buf<<endl;
   }
}

vector<PeakList::hkl> & PeakList::GetPeakList(){return mvHKL;}

const vector<PeakList::hkl> & PeakList::GetPeakList()const {return mvHKL;}

/////////////////////////////////////////////////////// SCORE ///////////////////////////////////////

float Score(const PeakList &dhkl, const RecUnitCell &ruc, const unsigned int nbSpurious,
            const bool verbose,const bool storehkl,const bool storePredictedHKL)
{
   const bool autozero=false;
   vector<PeakList::hkl>::const_iterator pos,first,last;
   for(pos=dhkl.GetPeakList().begin();pos!=dhkl.GetPeakList().end();++pos)
   {
      if(storehkl) pos->isIndexed=false;
      pos->d2calc=0;
      pos->d2diff=1000;
   }
   const unsigned long nb=dhkl.GetPeakList().size();
   if(storePredictedHKL) dhkl.mvPredictedHKL.clear();
   
   unsigned long nbCalc=0;
   int h,k,l;
   float predict_coeff=1;
   if(storePredictedHKL)predict_coeff=2;
   const float dmax=dhkl.mvHKL[nb-1].d2obs*predict_coeff*1.05;
   int sk0,sl0;// do we need >0 *and* <0 indices for k,l ?
   switch(ruc.mlattice)
   {
      case TRICLINIC:
         sk0=-1;sl0=-1;
	 break;
      case MONOCLINIC:
         sk0=1;sl0=-1;
         break;
      case ORTHOROMBIC:
         sk0=1;sl0=1;
         break;
      case HEXAGONAL:
         sk0=-1;sl0=1;
         break;
      case RHOMBOEDRAL:
         sk0=-1;sl0=-1;
         break;
      case TETRAGONAL:
         sk0=1;sl0=1;
         break;
      case CUBIC:
         sk0=1;sl0=1;
         break;
   }
   first=dhkl.GetPeakList().begin();last=dhkl.GetPeakList().end();
   unsigned long nbCalcH,nbCalcK;// Number of calculated lines below dmax for each h,k
   for(h=0;;++h)
   {
      nbCalcH=0;
      for(int sk=sk0;sk<=1;sk+=2)
      {
         if(h==0) sk=1;// no need to explore 0kl with both sk -1 and 1
         for(k=0;;++k)
         {
            nbCalcK=0;
            for(int sl=sl0;sl<=1;sl+=2)
            {
               if((h+k)==0)
               {
                  sl=1;// No need to list 0 0 l with l<0
                  l=1;
               }
               else
               {
                  if(h==0)
                  {
                     if(ruc.mlattice==MONOCLINIC) sl=1;// 0 k l and 0 k -l are equivalent
                     if((sk<0)||(sl<0)) l=1;// Do not list 0 k 0 with k<0
                     else l=0;// h==k==0 already covered
                  }
                  else
                  {
                     if(sl<0) l=1;// Do not list h k 0 twice
                     else l=0;
                  }
               }
               for(;;++l)
               {
                  const float d2=ruc.hkl2d(h,sk*k,sl*l);
                  if(d2>dmax)
                  {
                     //cout<<__FILE__<<":"<<__LINE__<<" hkl: "<<h<<" "<<sk*k<<" "<<sl*l<<":"<<sqrt(d2)<<" deriv="<<sl*ruc.hkl2d(h,sk*k,sl*l,NULL,3)<<endl;
                     // Only break if d is increasing with l 
                     if((sl*ruc.hkl2d(h,sk*k,sl*l,NULL,3))>=0) break;
                     else continue;
                  }
                  nbCalc++;nbCalcK++;nbCalcH++;
                  if(storePredictedHKL)
                  {
                     dhkl.mvPredictedHKL.push_back(PeakList::hkl(0,0,0,0,h,sk*k,sl*l,d2));
                     //continue;
                  }
                  for(pos=first;pos!=last;++pos)
                  {
                     const float tmp=d2-pos->d2obs;
                     if(tmp<.1)
                     {
                        if(tmp<-.1) break;
                        if(fabs(tmp)<fabs(pos->d2diff))
                        {  
                           pos->d2diff=tmp;
                           if(storehkl)
                           {
                              pos->h=h;
                              pos->k=sk*k;
                              pos->l=sl*l;
                              pos->isIndexed=true;
                              pos->d2calc=d2;
                           }
                        }
                        /*
                        if((verbose)&&(fabs(tmp)<.01))
                           cout<<__FILE__<<":"<<__LINE__<<"      hkl: "<<h<<" "<<k<<" "<<l
                              <<"#"<<i<<": calc="<<sqrt(d2)<<", obs="<<sqrt(*pd2)<<", min_epsilon="<<*pdq2<<", tmp="<<tmp<<endl;
                        */
                     }
                  }
               }
            }
            if(nbCalcK==0) //d(hk0)>dmax
            {
               //cout<<__FILE__<<":"<<__LINE__<<" hkl: "<<h<<" "<<sk*k<<" "<<0<<" deriv="<<sk*ruc.hkl2d(h,sk*k,0,NULL,2)<<endl;
               if((sk*ruc.hkl2d(h,sk*k,0,NULL,2))>=0) break;
            }
         }
      }
      if(nbCalcH==0) break;//h00 beyond limit
   }
   float epsilon=0.0,zero=0.0;
   if(autozero)
   {
      for(pos=dhkl.GetPeakList().begin();pos!=dhkl.GetPeakList().end();++pos) zero+=pos->d2diff;
      zero/=nb;
   }
   for(pos=dhkl.GetPeakList().begin();pos!=dhkl.GetPeakList().end();++pos)
   {
      epsilon +=fabs(pos->d2diff-zero);
   }
   if(nbSpurious>0)
   {// find worst fitting lines and remove them from epsilon calculation
      list<pair<float,unsigned int> > vdiff_idx;
      unsigned int i=0;
      for(pos=dhkl.GetPeakList().begin();pos!=dhkl.GetPeakList().end();++pos)
         vdiff_idx.push_back(make_pair(fabs(pos->d2diff),i++));
      vdiff_idx.sort(comparePairFirst<float,unsigned int>);
      i=0;
      for(list<pair<float,unsigned int> >::reverse_iterator rpos=vdiff_idx.rbegin();rpos!=vdiff_idx.rend();++rpos)
      {// :TODO: correct zero after removing spurious lines
         epsilon -= fabs(rpos->first-zero);
         if(storehkl) dhkl.GetPeakList()[rpos->second].isIndexed=false;
         dhkl.GetPeakList()[rpos->second].stats++;
         if(++i==nbSpurious) break;
      }
   }
   if(verbose)
   {
      float epstmp=0;
      unsigned long i=0;
      for(pos=dhkl.GetPeakList().begin();pos!=dhkl.GetPeakList().end();++pos)
      {
         epstmp+=fabs(pos->d2diff-zero);
         //cout<<"Line #"<<i++<<": obs="<<pos->d2obs<<", diff="<<pos->d2diff<<" -> epsilon="<<epstmp<<endl;
      }
      cout<<"epsilon="<<epstmp<<", dmax="<<dmax<<" ,nb="<<nb<<" ,nbcalc="<<nbCalc<<endl;
   }
   /*
   else
   {//Only stat+ the worst
      float max=-1;
      unsigned int worst=0;
      unsigned int i=0;
      for(pos=dhkl.GetPeakList().begin();pos!=dhkl.GetPeakList().end();++pos)
         if(abs(pos->d2diff)>max) {worst=i++;max=abs(pos->d2diff);}
         else i++;
      dhkl.GetPeakList()[worst].stats++;
   }
   */
   if(nbCalc==0) return 0;
   const float score=sqrt(dmax)*nb/(2*epsilon*nbCalc);
   if(verbose)
   {
      dhkl.Print(cout);
      cout<<"Final score:"<<score<<", nbCalc="<<nbCalc<<" ,<epsilon>="<<epsilon<<" nb="<<nb<<" Qn="<<sqrt(dmax)<<endl;
   }
   return score;
}

/////////////////////////////////////////////////////// CellExplorer ///////////////////////////////////////

CellExplorer::CellExplorer(const PeakList &dhkl, const CrystalSystem lattice, const unsigned int nbSpurious):
mnpar(3),mpPeakList(&dhkl),
mLengthMin(4),mLengthMax(25),
mAngleMin(M_PI),mAngleMax(2*M_PI/3),
mVolumeMin(0),mVolumeMax(1600),
mZeroShiftMin(0),mZeroShiftMax(0),
mlattice(lattice),mNbSpurious(nbSpurious),
mObs(0),mCalc(0),mWeight(0),mDeriv(0),mBestScore(0.0),
mMinScoreReport(10),mMaxDicVolDepth(7),mDicVolDepthReport(6)
{
   this->Init();
}

void CellExplorer::Evolution(unsigned int ng,const bool randomize,const float f,const float cr,unsigned int np)
{
   this->Init();
   const bool autozero=true;
   //cout<<__FILE__<<":"<<__LINE__<<"<CellExplorer::Evolution(...): randomizing,ng="<<ng
   //    <<"random="<<randomize<<"f="<<f<<"cr="<<cr<<"np="<<np<<endl;
   vector<pair<RecUnitCell,float> > vRUC(np);
   vector<pair<RecUnitCell,float> > vTrial(np);
   float bestScore=-1e20;
   vector<pair<RecUnitCell,float> >::iterator bestpos=vRUC.begin();

   const clock_t mTime0=clock();
   
   if(randomize)
   {
      for(unsigned int i=0;i<vRUC.size();++i)
      {
         vRUC[i].first.mlattice=mlattice;
         vTrial[i].first.mlattice=mlattice;
         for(unsigned int k=0;k<mnpar;++k) vRUC[i].first.par[k]=mMin[k]+mAmp[k]*rand()/(float)RAND_MAX;
         vRUC[i].second=Score(*mpPeakList,vRUC[i].first,mNbSpurious);
      }
   }

   for(unsigned long i=ng;i>0;--i)
   {
      for(unsigned j=0;j<np;j++)
      {
         if(true)
         {// DE/rand/1/exp
            unsigned int r1=j,r2=j,r3=j;
            while(r1==j)r1=rand()%np;
            while((r2==j)||(r1==r2))r2=rand()%np;
            while((r3==j)||(r3==r1)||(r3==r2))r3=rand()%np;
            unsigned int ncr=1;//+(int)(cr*mnpar*rand()/(float)RAND_MAX);
            unsigned int ncr0=rand()%mnpar;
            RecUnitCell *t0=&(vTrial[j].first);
            RecUnitCell *c0=&(vRUC[j].first);
            RecUnitCell *c1=&(vRUC[r1].first);
            RecUnitCell *c2=&(vRUC[r2].first);
            RecUnitCell *c3=&(vRUC[r3].first);
            for(unsigned int k=0;k<6;++k)t0->par[k] = c0->par[k];
            for(unsigned int k=0;k<ncr;++k)
            {
               const unsigned l=(ncr0+k)%mnpar;
               const float v1=c1->par[l]-mMin[l];
               const float v2=c2->par[l]-mMin[l];
               const float v3=c3->par[l]-mMin[l];
               t0->par[l]=mMin[l]+fmod(v1+f*(v2-v3)+3*mAmp[l],mAmp[l]);
            }
         }
         if(false)
         {// DE/rand-to-best/1/exp
            unsigned int r1=j,r2=j,r3=j;
            while(r1==j)r1=rand()%np;
            while((r2==j)||(r1==r2))r2=rand()%np;
            while((r3==j)||(r3==r1)||(r3==r2))r3=rand()%np;
            unsigned int ncr=1+(int)(cr*(mnpar-1)*rand()/(float)RAND_MAX);
            unsigned int ncr0=rand()%mnpar;
            RecUnitCell *t0=&(vTrial[j].first);
            RecUnitCell *c0=&(vRUC[j].first);
            //RecUnitCell *c1=&(vRUC[r1].first);
            RecUnitCell *c2=&(vRUC[r2].first);
            RecUnitCell *c3=&(vRUC[r3].first);
            RecUnitCell *best=&(bestpos->first);
            for(unsigned int k=0;k<6;++k)t0->par[k] = c0->par[k];//mMin[k]+mAmp[k]*rand()/(float)RAND_MAX;
            for(unsigned int k=0;k<ncr;++k)
            {
               const unsigned l=(ncr0+k)%mnpar;
               const float v0=c0->par[l]-mMin[l];
               //const float v1=c1->par[l]-mMin[l];
               const float v2=c2->par[l]-mMin[l];
               const float v3=c3->par[l]-mMin[l];
               const float vb=best->par[l]-mMin[l];
               t0->par[l]=mMin[l]+fmod(vb+f*(vb-v0)+f*(v2-v3)+5*mAmp[l],mAmp[l]);
            }
         }
         if(false)
         {// MC
            const float amp=.05/(1+i*.01);
            RecUnitCell *t0=&(vTrial[j].first);
            for(unsigned int k=0;k<6;++k)
            {
               
               t0->par[k] = mMin[k]+ fmod((float)(amp*mAmp[k]*(rand()/(float)RAND_MAX-0.5)+5*mAmp[k]),(float)mAmp[k]);
            }
         }
      }
      // Compute cost for all trials and select best
      vector<pair<RecUnitCell,float> >::iterator posTrial,pos;
      posTrial=vTrial.begin();
      pos=vRUC.begin();
      for(;posTrial!=vTrial.end();)
      {
         // If using auto-zero, fix zero parameter
         if(autozero) posTrial->first.par[0]=0;
         // Did we go beyond allowed volume ?
         switch(mlattice)
         {
            case TRICLINIC:
               break;
            case MONOCLINIC:
            {
               float v0=posTrial->first.par[1]*posTrial->first.par[2]*posTrial->first.par[3];
               while(v0<1/mVolumeMax)
               {
                  const unsigned int i=rand()%3+1;
                  posTrial->first.par[i]*=1/(mVolumeMax*v0)+1e-4;
                  if(posTrial->first.par[i]>(mMin[i]+mAmp[i])) posTrial->first.par[i]=mMin[i]+mAmp[i];
                  v0=posTrial->first.par[1]*posTrial->first.par[2]*posTrial->first.par[3];
               }
               break;
            }
            case ORTHOROMBIC:
               break;
            case HEXAGONAL:
               break;
            case RHOMBOEDRAL:
               break;
            case TETRAGONAL:
               break;
            case CUBIC:
               break;
         }

         const float score=Score(*mpPeakList,posTrial->first,mNbSpurious);
         if(score > pos->second)
         {
            pos->second=score;
            const float *p0=posTrial->first.par;
            float *p1=pos->first.par;
            for(unsigned int k=0;k<mnpar;++k) *p1++ = *p0++;
            if(score>bestScore)
            {
               bestScore=score;
               bestpos=pos;
            }
         }
         /*
         else
         {
            if(log(rand()/(float)RAND_MAX)>(-(score-pos->second)))
            {
               pos->second=score;
               const float *p0=posTrial->first.par;
               float *p1=pos->first.par;
               for(unsigned int k=0;k<mnpar;++k) *p1++ = *p0++;
            }
         }
         */
         ++pos;++posTrial;
      }
      if((i%100000)==0)
      {
         vector<float> uc=bestpos->first.DirectUnitCell();
         cout<<"Generation #"<<ng-i<<", Best score="<<bestScore
             <<" Trial: a="<<uc[0]<<", b="<<uc[1]<<", c="<<uc[2]<<", alpha="
             <<uc[3]*RAD2DEG<<", beta="<<uc[4]*RAD2DEG<<", gamma="<<uc[5]*RAD2DEG<<", V="<<uc[6]
             <<"   "<<(ng-i)*np/((clock()-mTime0)/(float)CLOCKS_PER_SEC)<<" trials/s"
             <<endl;
      }
      if(false)//((i%10000)==0)
      {// Randomize periodically
         for(vector<pair<RecUnitCell,float> >::iterator pos=vRUC.begin();pos!=vRUC.end();++pos)
         {
            if(pos==bestpos) continue;
            for(unsigned int k=0;k<mnpar;++k) pos->first.par[k]=mMin[k]+mAmp[k]*rand()/(float)RAND_MAX;
         }
      }
   }
   /*
   for(vector<pair<RecUnitCell,float> >::iterator pos=vRUC.begin();pos!=vRUC.end();++pos)
   {
      // Final cost
      vector<float> uc=pos->first.DirectUnitCell();
      cout<<__FILE__<<":"<<__LINE__<<" Trial: a="<<uc[0]<<", b="<<uc[1]<<", c="<<uc[2]<<", alpha="
       <<uc[3]*RAD2DEG<<", beta="<<uc[4]*RAD2DEG<<", gamma="<<uc[5]*RAD2DEG<<", V="<<uc[6]
          <<", score="<<pos->second<<endl;
   }
   Score(*mpPeakList,bestpos->first,mNbSpurious,true);
   */
   
   //this->ReduceSolutions();

   mRecUnitCell=bestpos->first;
   float score=Score(*mpPeakList,mRecUnitCell,mNbSpurious,false,true);
   vector<float> uc=mRecUnitCell.DirectUnitCell();
   cout<<__FILE__<<":"<<__LINE__<<" Best-DE : a="<<uc[0]<<", b="<<uc[1]<<", c="<<uc[2]<<", alpha="
       <<uc[3]*RAD2DEG<<", beta="<<uc[4]*RAD2DEG<<", gamma="<<uc[5]*RAD2DEG<<", V="<<uc[6]
       <<", score="<<bestpos->second
       <<"     ("<<ng*np/((clock()-mTime0)/(float)CLOCKS_PER_SEC)<<" trials/s)"<<endl;
   if(score>mMinScoreReport*.5)
   {
      // Now, do a least-squares refinement on best
      mRecUnitCell=bestpos->first;
      this->LSQRefine(10,true,true);
      uc=mRecUnitCell.DirectUnitCell();
      score=Score(*mpPeakList,mRecUnitCell,mNbSpurious,false,true);
      cout<<__FILE__<<":"<<__LINE__<<" Best-LSQ: a="<<uc[0]<<", b="<<uc[1]<<", c="<<uc[2]<<", alpha="
         <<uc[3]*RAD2DEG<<", beta="<<uc[4]*RAD2DEG<<", gamma="<<uc[5]*RAD2DEG<<", V="<<uc[6]
         <<", score="<<score<<endl;
      if(score>mMinScoreReport)
      {
         if(score>mBestScore) mBestScore=score;
         mvSolution.push_back(make_pair(mRecUnitCell,score));
         this->ReduceSolutions();// We may have solutions from previous runs
      }
   }
}

void CellExplorer::SetLengthMinMax(const float min,const float max)
{
   mLengthMin=min;
   mLengthMax=max;
}
void CellExplorer::SetAngleMinMax(const float min,const float max)
{
   mAngleMin=min;
   mAngleMax=max;
}
void CellExplorer::SetVolumeMinMax(const float min,const float max)
{
   mVolumeMin=min;
   mVolumeMax=max;
}
void CellExplorer::SetNbSpurious(const unsigned int nb)
{
   mNbSpurious=nb;
}
void CellExplorer::SetMinMaxZeroShift(const float min,const float max)
{
   mZeroShiftMin=min;
   mZeroShiftMax=max;
}

void CellExplorer::SetCrystalSystem(const CrystalSystem system)
{
   mlattice=system;
}

void CellExplorer::SetD2Error(const float err){mD2Error=err;}

const string& CellExplorer::GetClassName() const
{
   const static string className="CellExplorer";
   return className;
}
const string& CellExplorer::GetName() const
{
   const static string name="Some CellExplorer Object";
   return name;
}
void CellExplorer::Print() const
{
   this->RefinableObj::Print();
}
unsigned int CellExplorer::GetNbLSQFunction() const
{return 1;}

const CrystVector_REAL& CellExplorer::GetLSQCalc(const unsigned int) const
{
   VFN_DEBUG_ENTRY("CellExplorer::GetLSQCalc()",2)
   unsigned int j=0;
   for(vector<PeakList::hkl>::const_iterator pos=mpPeakList->GetPeakList().begin();pos!=mpPeakList->GetPeakList().end();++pos)
   {
      if(pos->isIndexed)
         mCalc(j++)=mRecUnitCell.hkl2d(pos->h,pos->k,pos->l);
   }
   //cout<<__FILE__<<":"<<__LINE__<<"LSQCalc : Score:"<<Score(*mpPeakList,mRecUnitCell,mNbSpurious,false,true,false)<<endl;
   VFN_DEBUG_EXIT("CellExplorer::GetLSQCalc()",2)
   return mCalc;
}
const CrystVector_REAL& CellExplorer::GetLSQObs(const unsigned int) const
{
   VFN_DEBUG_MESSAGE("CellExplorer::GetLSQObs()",2)
   return mObs;
}
const CrystVector_REAL& CellExplorer::GetLSQWeight(const unsigned int) const
{
   VFN_DEBUG_MESSAGE("CellExplorer::GetLSQWeight()",2)
   //:TODO: exclude the worst points (user-chosen number)
   return mWeight;
}
const CrystVector_REAL& CellExplorer::GetLSQDeriv(const unsigned int, RefinablePar &refpar)
{
   VFN_DEBUG_ENTRY("CellExplorer::GetLSQDeriv()",2)
   float *par=NULL;
   if(refpar.GetName()=="Reciprocal unit cell par #0") par=mRecUnitCell.par+1;
   else
      if(refpar.GetName()=="Reciprocal unit cell par #1") par=mRecUnitCell.par+2;
      else
         if(refpar.GetName()=="Reciprocal unit cell par #2") par=mRecUnitCell.par+3;
         else
            if(refpar.GetName()=="Reciprocal unit cell par #3") par=mRecUnitCell.par+4;
            else
               if(refpar.GetName()=="Reciprocal unit cell par #4") par=mRecUnitCell.par+5;
               else
                  if(refpar.GetName()=="Reciprocal unit cell par #5") par=mRecUnitCell.par+6;
                  else 
                     if(refpar.GetName()=="Zero") par=mRecUnitCell.par+0;
                     else cout<<__FILE__<<":"<<__LINE__<<":Parameter not found:"<<refpar.GetName()<<endl;
   unsigned int j=0;
   for(vector<PeakList::hkl>::const_iterator pos=mpPeakList->GetPeakList().begin();pos!=mpPeakList->GetPeakList().end();++pos)
   {
      VFN_DEBUG_MESSAGE("CellExplorer::GetLSQDeriv():"<<j<<"/"<<mpPeakList->GetPeakList().size(),2)
      VFN_DEBUG_MESSAGE("CellExplorer::GetLSQDeriv():"<<pos->h<<","<<pos->k<<","<<pos->l,2)
      if(pos->isIndexed) 
         mDeriv(j++)=mRecUnitCell.hkl2d(pos->h,pos->k,pos->l,par);
   }
   VFN_DEBUG_EXIT("CellExplorer::GetLSQDeriv()",2)
   return mDeriv;
}

void CellExplorer::BeginOptimization(const bool allowApproximations, const bool enableRestraints)
{
   VFN_DEBUG_ENTRY("CellExplorer::BeginOptimization()",10)
   Score(*mpPeakList,mRecUnitCell,mNbSpurious,false,true,false);
   const unsigned int nb=mpPeakList->GetPeakList().size();
   mCalc.resize(nb-mNbSpurious);
   mObs.resize(nb-mNbSpurious);
   mWeight.resize(nb-mNbSpurious);
   mDeriv.resize(nb-mNbSpurious);
   int j=0;
   float thres=0.0;
   for(vector<PeakList::hkl>::const_iterator pos=mpPeakList->GetPeakList().begin();pos!=mpPeakList->GetPeakList().end();++pos)
      if(thres<pos->iobs) thres=pos->iobs;
   thres/=10;// weight=1 for intensities up to Imax/10

   //cout <<"Beginning optimization with reflexions:"<<endl;
   //char buf[100];
   for(vector<PeakList::hkl>::const_iterator pos=mpPeakList->GetPeakList().begin();pos!=mpPeakList->GetPeakList().end();++pos)
   {
      if(pos->isIndexed)
      {
         mObs(j)=pos->d2obs;
         if(mObs(j)>thres) mWeight(j)=1;
         else mWeight(j)=mObs(j)/thres;
         /*
         sprintf(buf,"#%2d  (%3d %3d %3d) dobs=%6.3f dcalc=%6.3f iobs=%6.3f weight=%6.4f",
                 i,mpPeakList->mvHKL[i].h,mpPeakList->mvHKL[i].k,mpPeakList->mvHKL[i].l,
                 1/mpPeakList->mvdobs[i],1/sqrt(mRecUnitCell.hkl2d(mpPeakList->mvHKL[i].h,mpPeakList->mvHKL[i].k,mpPeakList->mvHKL[i].l)),
                 mpPeakList->mviobs[i],mWeight(j));
         */
         j++;
      }
      /*
      else
      {
         sprintf(buf,"#%2d  (%3d %3d %3d) dobs=%6.3f dcalc=%6.3f iobs=%6.3f               SPURIOUS",
                 i,mpPeakList->mvHKL[i].h,mpPeakList->mvHKL[i].k,mpPeakList->mvHKL[i].l,
                 1/mpPeakList->mvdobs[i],1/sqrt(mRecUnitCell.hkl2d(mpPeakList->mvHKL[i].h,mpPeakList->mvHKL[i].k,mpPeakList->mvHKL[i].l)),
                 mpPeakList->mviobs[i]);
      }
      cout<<buf<<endl;
      */
   }
   this->RefinableObj::BeginOptimization(allowApproximations,enableRestraints);
   VFN_DEBUG_EXIT("CellExplorer::BeginOptimization()",10)
}

void CellExplorer::LSQRefine(int nbCycle, bool useLevenbergMarquardt, const bool silent)
{
   VFN_DEBUG_ENTRY("CellExplorer::LSQRefine()",5)
   mLSQObj.SetRefinedObj(*this);
   //this->BeginOptimization();
   //cout<<FormatVertVector<REAL>(this->GetLSQObs(0),this->GetLSQCalc(0),this->GetLSQWeight(0),this->GetLSQDeriv(0,this->GetPar((long)0)))<<endl;
   mLSQObj.Refine(nbCycle,useLevenbergMarquardt,silent);
   if(!silent) mpPeakList->Print(cout);
   VFN_DEBUG_EXIT("CellExplorer::LSQRefine()",5)
}

/** Number of reflexions found in the intervals calculated between uc+duc and uc-duc
*
* \param useStoredHKL:
*    - if equal to 0, explore all possible hkl values to find possible Miller indices.
*    - if useStoredHKL=1, use the Miller indices already stored in hkl.vDicVolHKL
*    for each observed line as the only possible indices.
*    - if useStoredHKL=2, search all the possible Miller indices for all reflections
*    and store them in hkl.vDicVolHKL for each observed line.
*/
bool DichoIndexed(const PeakList &dhkl, const RecUnitCell &uc,const RecUnitCell &duc,
                 const unsigned int nbUnindexed=0,const bool verbose=false,unsigned int useStoredHKL=0)
{
   const unsigned int nb=dhkl.GetPeakList().size();
   int nbIndexed=nb-nbUnindexed;// Number of reflections we require to be indexed
   // List of indexed reflections
   vector<PeakList::hkl>::const_iterator pos,first,last,end;
   if(useStoredHKL==1)
   {// We already now possible Miller indices for all reflections
      int nbUnIx=0;
      for(pos=dhkl.GetPeakList().begin();pos!=dhkl.GetPeakList().end();++pos)
      {
         pos->isIndexed=false;
         for(list<PeakList::hkl0>::const_iterator phkl0=pos->vDicVolHKL.begin();phkl0!=pos->vDicVolHKL.end();++phkl0)
         {
            float d0,d1;
            uc.hkl2d_delta(phkl0->h,phkl0->k,phkl0->l,duc,d0,d1);
            if((pos->d2obsmax>=d0) && (d1>=pos->d2obsmin))
            {
               pos->d2calc=(d0+d1)/2;
               pos->isIndexed=true;
               if(--nbIndexed==0) return true;
               break;
            }
         }
         if(!(pos->isIndexed)) if(++nbUnIx>nbUnindexed) return false;
      }
      return false;
   }
   const bool storePossibleHKL=(useStoredHKL==2);
   
   if(storePossibleHKL)
      for(pos=dhkl.GetPeakList().begin();pos!=dhkl.GetPeakList().end();++pos)
      {
         pos->isIndexed=false;
         pos->vDicVolHKL.clear();
      }
   else
      for(pos=dhkl.GetPeakList().begin();pos!=dhkl.GetPeakList().end();++pos) pos->isIndexed=false;
   
   int h,k,l;
   float dmax=dhkl.GetPeakList()[nb-1].d2obs;
   float dmin=dhkl.GetPeakList()[0   ].d2obs;
   
   
   int sk0,sl0;// do we need >0 *and* <0 indices for k,l ?
   switch(uc.mlattice)
   {
      case TRICLINIC:
         sk0=-1;sl0=-1;
	 break;
      case MONOCLINIC:
      {
         sk0=1;sl0=-1;
         break;
      }
      case ORTHOROMBIC:
         sk0=1;sl0=1;
         break;
      case HEXAGONAL:
         sk0=-1;sl0=1;
         break;
      case RHOMBOEDRAL:
         sk0=-1;sl0=-1;
         break;
      case TETRAGONAL:
         sk0=1;sl0=1;
         break;
      case CUBIC:
         sk0=1;sl0=1;
         break;
   }
   RecUnitCell uc0(uc),uc1(uc);
   for(unsigned int i=0;i<7;++i) {uc0.par[i]-=duc.par[i];uc1.par[i]+=duc.par[i];}
   
   //currently first & last unindexed dhkl
   first=dhkl.GetPeakList().begin(),last=dhkl.GetPeakList().end(),end=dhkl.GetPeakList().end();
   
   bool break_k,break_l;//0: continue, 1:break if only >0 to be explored, 2: break
   for(h=0;;++h)
   {
      break_k=false;
      for(int sk=sk0;sk<=1;sk+=2)
      {
         if(h==0) sk=1;
         for(k=0;;++k)
         {
            break_l=false;
            for(int sl=sl0;sl<=1;sl+=2)
            {
               int l0;
               if((h+k)==0)
               {
                  sl=1;// No need to list 0 0 l with l<0
                  l0=1;
               }
               else
               {
                  if(h==0)
                  {
                     if(uc.mlattice==MONOCLINIC) sl=1;// 0 k l and 0 k -l are equivalent
                     if((sk<0)||(sl<0)) l0=1;// Do not list 0 k 0 with k<0
                     else l0=0;// h==k==0 already covered
                  }
                  else
                  {
                     if(sl<0) l0=1;// Do not list h k 0 twice
                     else l0=0;
                  }
               }
               if((h+k)==0) l0=1;
               else l0=0;
               for(l=l0;;++l)
               {
                  float d0,d1;
                  uc.hkl2d_delta(h,sk*k,sl*l,duc,d0,d1);
                  if(d1<dmin) continue;
                  if(d0>dmax)
                  {
                     if(l==l0) break_l=true;
                     break;
                  }
                  for(pos=first;pos!=end;++pos)
                  {
                     if(pos==last) break;
                     if((!storePossibleHKL)&&(pos->isIndexed)) continue;
                     const float d2obs=pos->d2obs,d2obsmin=pos->d2obsmin, d2obsmax=pos->d2obsmax;
                     if((d2obsmax>=d0) && (d1>=d2obsmin))
                     {
                        if(!(pos->isIndexed))
                        {
                           pos->d2calc=(d0+d1)/2;
                           --nbIndexed;
                           pos->isIndexed=true;
                        }
                        if(verbose) cout<<d1<<" < ? <"<<d0<<"("<<h<<","<<sk*k<<","<<sl*l<<"): "<<d2obs<<" (remaining to index:"<<nbIndexed<<")"<<endl;
                        if(storePossibleHKL)
                           pos->vDicVolHKL.push_back(PeakList::hkl0(h,sk*k,sl*l));
                        else
                        {
                           if((!storePossibleHKL)&&(nbIndexed==0)) return true;
                           if(pos==first){first++;dmin=first->d2obsmin;}
                           if(pos==last){last--;dmax=last->d2obsmax;}
                        }
                     }
                  }
               }
            }
            if(break_l)
            {
               if(k==0) break_k=true;
               break; // hk0 beyond limit
            }
         }
      }
      if(break_k) break;//h00 beyond limit
   }
   if(verbose)
   {
      dhkl.Print(cout);
   }
   return nbIndexed<=0;
}

float CellExplorer::GetBestScore()const{return mBestScore;}
const std::list<std::pair<RecUnitCell,float> >& CellExplorer::GetSolutions()const {return mvSolution;}
std::list<std::pair<RecUnitCell,float> >& CellExplorer::GetSolutions() {return mvSolution;}

unsigned int CellExplorer::RDicVol(RecUnitCell uc0,RecUnitCell duc, unsigned int depth,unsigned long &nbCalc,const float minV,const float maxV,vector<unsigned int> vdepth)
{
   if((depth>0)&&(depth<=2))// test if volume is within range
   {
      RecUnitCell ucm=uc0,ucp=uc0;
      for(unsigned int i=0;i<6;++i) {ucm.par[i]-=duc.par[i];ucp.par[i]+=duc.par[i];}
      vector<float> ucmd=ucm.DirectUnitCell();
      vector<float> ucpd=ucp.DirectUnitCell();
      if((ucpd[6]>maxV)||(ucmd[6]<minV))return 0;
   }
   unsigned int useStoredHKL=1;//Use already stored hkl
   if(depth==0) useStoredHKL=2; //Store possible hkl for all observed lines
   
   bool indexed=DichoIndexed(*mpPeakList,uc0,duc,mNbSpurious,false,useStoredHKL);
   // if we can zoom in for one parameter directly, we need per-parameter depth
   if(vdepth.size()==0)
   {
      vdepth.resize(mnpar-1);
      for(vector<unsigned int>::iterator pos=vdepth.begin();pos!=vdepth.end();) *pos++=depth;
   }
   else
      for(vector<unsigned int>::iterator pos=vdepth.begin();pos!=vdepth.end();++pos) if(*pos<depth)*pos=depth;
   if((useStoredHKL==2)&&(mNbSpurious==0))
   {  // If high-d lines have been associated to a single reflection which is either h00, 0k0 or 00l,
      // jump the corresponding parameter to higher depth (mDicVolDepthReport, lowest depth report) immediately
      vector<pair<unsigned int,float> > vq0(3);
      for(unsigned int i=0;i<3;++i) {vq0[i].first=0;vq0[i].second=0.0;}
      RecUnitCell uc0orig=uc0,ducorig=duc;
      for(vector<PeakList::hkl>::const_iterator pos=mpPeakList->GetPeakList().begin();pos!=mpPeakList->GetPeakList().end();++pos)
      {
         if(pos->vDicVolHKL.size()==1)
         {
            const PeakList::hkl0 h0=pos->vDicVolHKL.front();
            if((h0.k==0)&&(h0.l==0)) {vq0[0].first+=1 ; vq0[0].second+=pos->dobs/h0.h;}
            else
               if((h0.h==0)&&(h0.l==0)) {vq0[1].first+=1 ; vq0[1].second+=pos->dobs/h0.k;}
               else
                  if((h0.h==0)&&(h0.k==0)) {vq0[2].first+=1 ; vq0[2].second+=pos->dobs/h0.l;}
         }
      }
      switch(mlattice)
      {
         case TRICLINIC:
         {
            if(vq0[0].first>0) {uc0.par[1]=vq0[0].second/vq0[0].first ; vdepth[1]=mDicVolDepthReport;duc.par[1]*=.0625;}
            if(vq0[1].first>0) {uc0.par[2]=vq0[1].second/vq0[1].first ; vdepth[2]=mDicVolDepthReport;duc.par[2]*=.0625;}
            if(vq0[2].first>0) {uc0.par[3]=vq0[2].second/vq0[2].first ; vdepth[3]=mDicVolDepthReport;duc.par[3]*=.0625;}
            break;
         }
         case MONOCLINIC:
         {
            if(vq0[0].first>0) {uc0.par[1]=vq0[0].second/vq0[0].first ; vdepth[0]=mDicVolDepthReport;duc.par[1]*=.0625;}//pow((float)0.5,(int)(mDicVolDepthReport-depth));}
            if(vq0[1].first>0) {uc0.par[2]=vq0[1].second/vq0[1].first ; vdepth[1]=mDicVolDepthReport;duc.par[2]*=.0625;}//pow((float)0.5,(int)(mDicVolDepthReport-depth));}
            if(vq0[2].first>0) {uc0.par[3]=vq0[2].second/vq0[2].first ; vdepth[2]=mDicVolDepthReport;duc.par[3]*=.0625;}//pow((float)0.5,(int)(mDicVolDepthReport-depth));}
            break;                                           
         }                                                   
         case ORTHOROMBIC:
         {
            if(vq0[0].first>0) {uc0.par[1]=vq0[0].second/vq0[0].first ; vdepth[0]=mDicVolDepthReport;duc.par[1]*=.0625;}//pow((float)0.5,(int)(mDicVolDepthReport-depth));}
            if(vq0[1].first>0) {uc0.par[2]=vq0[1].second/vq0[1].first ; vdepth[1]=mDicVolDepthReport;duc.par[2]*=.0625;}//pow((float)0.5,(int)(mDicVolDepthReport-depth));}
            if(vq0[2].first>0) {uc0.par[3]=vq0[2].second/vq0[2].first ; vdepth[2]=mDicVolDepthReport;duc.par[3]*=.0625;}//pow((float)0.5,(int)(mDicVolDepthReport-depth));}
            break;
         }
         case HEXAGONAL:break;
         case RHOMBOEDRAL:break;
         case TETRAGONAL:break;
         case CUBIC:break;
      }
      // If all parameters are at a higher depth, jump the global depth immediately
      unsigned int newdepth=40;
      for(vector<unsigned int>::iterator pos=vdepth.begin();pos!=vdepth.end();++pos) if(*pos<newdepth) newdepth=*pos;
      if(newdepth>depth) depth=newdepth;
      if((vq0[0].first>0)||(vq0[1].first>0)||(vq0[2].first>0))
      {
         indexed=DichoIndexed(*mpPeakList,uc0,duc,mNbSpurious,false,1);
         if(false)
         {
            {
               RecUnitCell ucm=uc0orig,ucp=uc0;
               for(unsigned int i=0;i<6;++i) {ucm.par[i]-=ducorig.par[i];ucp.par[i]+=ducorig.par[i];}
               vector<float> ucmd=ucm.DirectUnitCell();
               vector<float> ucpd=ucp.DirectUnitCell();
               char buf[200];
               sprintf(buf,"orig:   a=%5.2f-%5.2f b=%5.2f-%5.2f c=%5.2f-%5.2f alpha=%5.2f-%5.2f beta=%5.2f-%5.2f gamma=%5.2f-%5.2f V=%5.2f-%5.2f",
                        ucpd[0],ucmd[0],ucpd[1],ucmd[1],ucpd[2],ucmd[2],ucpd[3]*RAD2DEG,ucmd[3]*RAD2DEG,
                        ucpd[4]*RAD2DEG,ucmd[4]*RAD2DEG,ucpd[5]*RAD2DEG,ucmd[5]*RAD2DEG,ucpd[6],ucmd[6]);
               for(int i=0;i<depth;++i) cout<<" ";
               cout<<buf<<"level="<<depth<<", indexed="<<indexed<<endl;
            }
            {
               RecUnitCell ucm=uc0,ucp=uc0;
               for(unsigned int i=0;i<6;++i) {ucm.par[i]-=duc.par[i];ucp.par[i]+=duc.par[i];}
               vector<float> ucmd=ucm.DirectUnitCell();
               vector<float> ucpd=ucp.DirectUnitCell();
               char buf[200];
               sprintf(buf,"bypass: a=%5.2f-%5.2f b=%5.2f-%5.2f c=%5.2f-%5.2f alpha=%5.2f-%5.2f beta=%5.2f-%5.2f gamma=%5.2f-%5.2f V=%5.2f-%5.2f",
                        ucpd[0],ucmd[0],ucpd[1],ucmd[1],ucpd[2],ucmd[2],ucpd[3]*RAD2DEG,ucmd[3]*RAD2DEG,
                        ucpd[4]*RAD2DEG,ucmd[4]*RAD2DEG,ucpd[5]*RAD2DEG,ucmd[5]*RAD2DEG,ucpd[6],ucmd[6]);
               for(int i=0;i<depth;++i) cout<<" ";
               cout<<buf<<"level="<<depth<<", indexed="<<indexed<<endl;
            }
         }
      }
   }
   if(false)//(depth==1)&&(rand()%10==0))
   {
      RecUnitCell ucm=uc0,ucp=uc0;
      for(unsigned int i=0;i<4;++i) {ucm.par[i]-=duc.par[i];ucp.par[i]+=duc.par[i];}
      for(unsigned int i=4;i<7;++i) {ucm.par[i]+=duc.par[i];ucp.par[i]-=duc.par[i];}
      vector<float> ucmd=ucm.DirectUnitCell();
      vector<float> ucpd=ucp.DirectUnitCell();
      char buf[200];
      sprintf(buf,"a=%5.2f-%5.2f b=%5.2f-%5.2f c=%5.2f-%5.2f alpha=%5.2f-%5.2f beta=%5.2f-%5.2f gamma=%5.2f-%5.2f V=%5.2f-%5.2f",
               ucpd[0],ucmd[0],ucpd[1],ucmd[1],ucpd[2],ucmd[2],ucpd[3]*RAD2DEG,ucmd[3]*RAD2DEG,
               ucpd[4]*RAD2DEG,ucmd[4]*RAD2DEG,ucpd[5]*RAD2DEG,ucmd[5]*RAD2DEG,ucpd[6],ucmd[6]);
      for(int i=0;i<depth;++i) cout<<" ";
      cout<<buf<<"level="<<depth<<", indexed="<<indexed<<endl;
   }
   nbCalc++;
   // :TODO: if we failed the dichotomy and reached some depth, try guessing a zero shift from the indexed reflections
   /*
   if((!indexed)&&(depth>=2))
   {
      vector<float> shifts(mpPeakList->GetPeakList().size());
      vector<PeakList::hkl>::const_iterator peakpos=mpPeakList->GetPeakList().begin();
      for(vector<float>::iterator spos=shifts.begin();spos!=shifts.end();)
      {   *spos++ = peakpos->d2diff * (float)(peakpos->isIndexed&&(!peakpos->isSpurious));peakpos++;}
      sort(shifts.begin(),shifts.end());
      uc0.par[0]=shifts[mpPeakList->GetPeakList().size()/2];//use median value
      indexed=DichoIndexed(*mpPeakList,uc0,duc,mNbSpurious);
      if(indexed) cout<<"Failed Dicho ? Trying auto-zero shifting :Worked !"<<endl;
   }
   */
   if(indexed)
   {
      unsigned int deeperSolutions=0;
      if(depth<mMaxDicVolDepth)
      {
         if(false)//depth>=3)
         {
            RecUnitCell ucm=uc0,ucp=uc0;
            for(unsigned int i=0;i<6;++i) {ucm.par[i]-=duc.par[i];ucp.par[i]+=duc.par[i];}
            vector<float> ucmd=ucm.DirectUnitCell();
            vector<float> ucpd=ucp.DirectUnitCell();
            char buf[200];
            sprintf(buf,"a=%5.2f-%5.2f b=%5.2f-%5.2f c=%5.2f-%5.2f alpha=%5.2f-%5.2f beta=%5.2f-%5.2f gamma=%5.2f-%5.2f V=%5.2f-%5.2f",
                     ucpd[0],ucmd[0],ucpd[1],ucmd[1],ucpd[2],ucmd[2],ucpd[3]*RAD2DEG,ucmd[3]*RAD2DEG,
                     ucpd[4]*RAD2DEG,ucmd[4]*RAD2DEG,ucpd[5]*RAD2DEG,ucmd[5]*RAD2DEG,ucpd[6],ucmd[6]);
            cout<<"Depth="<<depth<<buf<<endl;
         }
         RecUnitCell uc=uc0;
         // zero (if used...)
         duc.par[0]=0.5*duc.par[0];
         // Divide interval by 2, except if this parameter is already at a higher depth 
         // because a main axis has been indexed already.
         for(unsigned int i=1;i<mnpar;++i) duc.par[i]*=(0.5+0.5*(vdepth[i-1]>depth));
         
         for(int i0=-1;i0<=1;i0+=2)
         {
            //:TODO: dichotomy on zero shift ?
            
            // Don't change parameter if it is already determined at a higher depth
            if(vdepth[0]==depth) {uc.par[1]=uc0.par[1]+i0*duc.par[1];}
            else {i0=2;}// no need to dicho this parameter which is already at higher depth
            if(mnpar==2)
               deeperSolutions+=RDicVol(uc,duc, depth+1,nbCalc,minV,maxV,vdepth);
            else
               for(int i1=-1;i1<=1;i1+=2)
               {
                  if(vdepth[1]==depth) {uc.par[2]=uc0.par[2]+i1*duc.par[2];}
                  else {i1=2;}// no need to dicho this parameter which is already at higher depth
                  if(mnpar==3)
                     deeperSolutions+=RDicVol(uc,duc, depth+1,nbCalc,minV,maxV,vdepth);
                  else
                     for(int i2=-1;i2<=1;i2+=2)
                     {
                        if(vdepth[2]==depth) {uc.par[3]=uc0.par[3]+i2*duc.par[3];}
                        else {i2=2;}// no need to dicho this parameter which is already at higher depth
                        if(mnpar==4)
                           deeperSolutions+=RDicVol(uc,duc, depth+1,nbCalc,minV,maxV,vdepth);
                        else
                           for(int i3=-1;i3<=1;i3+=2)
                           {
                              uc.par[4]=uc0.par[4]+i3*duc.par[4];
                              if(mnpar==5)
                                 deeperSolutions+=RDicVol(uc,duc, depth+1,nbCalc,minV,maxV,vdepth);
                              else
                                 for(int i4=-1;i4<=1;i4+=2)
                                 {
                                    uc.par[5]=uc0.par[5]+i4*duc.par[5];
                                    if(mnpar==7)
                                       deeperSolutions+=RDicVol(uc,duc, depth+1,nbCalc,minV,maxV,vdepth);
                                    else
                                       for(int i5=-1;i5<=1;i5+=2)
                                       {
                                          uc.par[6]=uc0.par[6]+i5*duc.par[6];
                                          deeperSolutions+=RDicVol(uc,duc, depth+1,nbCalc,minV,maxV,vdepth);
                                       }
                                 }
                           }
                     }
               }
          }
      }
      if((deeperSolutions==0) &&(depth>=mDicVolDepthReport))
      {
         mRecUnitCell=uc0;
         vector<float> uc=mRecUnitCell.DirectUnitCell();
         float score=Score(*mpPeakList,mRecUnitCell,mNbSpurious,false,true,false);
         if(((score>(mMinScoreReport*.5))&&(depth>=mDicVolDepthReport))||(depth>=mMaxDicVolDepth))
         {
            if(score>mBestScore) //((score>mMinScoreReport)||(depth>=mDicVolDepthReport))
               cout<<__FILE__<<":"<<__LINE__<<" Depth="<<depth<<" (DIC) ! a="<<uc[0]<<", b="<<uc[1]<<", c="<<uc[2]<<", alpha="
                  <<uc[3]*RAD2DEG<<", beta="<<uc[4]*RAD2DEG<<", gamma="<<uc[5]*RAD2DEG<<", V="<<uc[6]
                  <<", score="<<score<<endl;
            this->LSQRefine(10,true,true);
            uc=mRecUnitCell.DirectUnitCell();
            score=Score(*mpPeakList,mRecUnitCell,mNbSpurious,false,true,false);
            if((score>mMinScoreReport)||(depth>=mDicVolDepthReport))
            {
               if(score>mBestScore)
               {
                  cout<<__FILE__<<":"<<__LINE__<<" Depth="<<depth<<" (LSQ) ! a="<<uc[0]<<", b="<<uc[1]<<", c="<<uc[2]<<", alpha="
                     <<uc[3]*RAD2DEG<<", beta="<<uc[4]*RAD2DEG<<", gamma="<<uc[5]*RAD2DEG<<", V="<<uc[6]
                     <<", score="<<score<<endl;
                  mBestScore=score;
               }
               mvSolution.push_back(make_pair(mRecUnitCell,score));
               mvNbSolutionDepth[depth]+=1;
            }
         }
      }
      return deeperSolutions+1;
   }
   return 0;
}

void CellExplorer::DicVol(const float minScore,const unsigned int minDepth,const float stopOnScore,const unsigned int stopOnDepth)
{
   mDicVolDepthReport=minDepth;
   mMinScoreReport=minScore;
   this->Init();
   if(minDepth>mMaxDicVolDepth) mMaxDicVolDepth=minDepth;
   mvNbSolutionDepth.resize(mMaxDicVolDepth+1);
   for(unsigned int i=0;i<=mMaxDicVolDepth;++i) mvNbSolutionDepth[i]=0;
   
   float latstep=0.5,
         cosangmax=abs(cos(mAngleMax)),
         vstep=(mVolumeMax-mVolumeMin)/(ceil((mVolumeMax-mVolumeMin)/500)-0.0001);
   const float cosangstep=cosangmax/(ceil(cosangmax/.08)-.0001);
   if(((mVolumeMax-mVolumeMin)/vstep)>10) vstep=(mVolumeMax-mVolumeMin)/9.999;
   if(((mLengthMax-mLengthMin)/latstep)>25) latstep=(mLengthMax-mLengthMin)/24.9999;
   
   cout<<mLengthMin<<"->"<<mLengthMax<<","<<latstep<<","<<(mLengthMax-mLengthMin)/latstep<<endl;
   cout<<mAngleMin<<"->"<<mAngleMax<<","<<cosangstep<<","<<cosangmax<<","<<(mAngleMax-mAngleMin)/cosangstep<<endl;
   cout<<mVolumeMin<<"->"<<mVolumeMax<<","<<vstep<<","<<(mVolumeMax-mVolumeMin)/vstep<<endl;
   RecUnitCell uc0,duc;
   uc0.mlattice=mlattice;
   duc.mlattice=mlattice;
   //Zero shift parameter - not used for dicvol right now ? :TODO:
   uc0.par[0]=0.0;
   duc.par[0]=0.0;
   unsigned long nbCalc=0;
   const clock_t mTime0=clock();
   float bestscore=0;
   list<pair<RecUnitCell,float> >::iterator bestpos;
   for(float minv=mVolumeMin;minv<mVolumeMax;minv+=vstep)
   {
      float maxv=minv+vstep;
      if(maxv>mVolumeMax)maxv=mVolumeMax;
      cout<<"Starting: V="<<minv<<"->"<<maxv<<endl;
      switch(mlattice)
      {
         case TRICLINIC:
         {
            RecUnitCell uclarge,//ucm: smallest reciprocal, largest direct cell
                        ucsmall;//ucp: largest reciprocal, smallest direct cell
            vector<float> uclarged,ucsmalld;
            for(float calpha=0;calpha>=-(cosangmax-cosangstep);calpha-=cosangstep)
            {
               const float alpha=acos(calpha);
               const float salpha=sqrt(abs(1-calpha*calpha));
               cout<<"alpha="<<acos(calpha)*RAD2DEG<<endl;
               for(float cbeta =0;cbeta >=-(cosangmax-cosangstep);cbeta -=cosangstep)
               {
                  const float beta=acos(cbeta);
                  const float sbeta=sqrt(abs(1-cbeta*cbeta));
                  cout<<" beta="<<acos(cbeta)*RAD2DEG<<endl;
                  for(float cgamma=0;cgamma>=-(cosangmax-cosangstep);cgamma-=cosangstep)
                  {
                     const float gamma=acos(cgamma);
                     if((alpha+beta+gamma)>6) break;// alpha+beta+gamma < 2*pi
                     const float sgamma=sqrt(abs(1-cgamma*cgamma));
                     const float vv0=sqrt(abs(1-calpha*calpha-cbeta*cbeta-cgamma*cgamma+2*calpha*cbeta*cgamma));
                     const float vv=1/vv0;
                     
                     const float calphar=(cbeta *cgamma-calpha)/(sbeta *sgamma);
                     const float cbetar =(calpha*cgamma-cbeta )/(salpha*sgamma);
                     const float cgammar=(calpha*cbeta -cgamma)/(salpha*sbeta );
                     
                     float a=mLengthMin;
                     cout<<"  gamma="<<acos(cgamma)*RAD2DEG<<endl;
                     for(;a<(mLengthMax-latstep);a+=latstep)
                     {
                        float b=a;// CHECK ?
                        for(;b<(mLengthMax-latstep);b+=latstep)
                        {
                           if(abs(b*cgamma)>a) break;
                           
                           const float v0=a*b*vv0;
                           float cmin=minv/v0;
                           if(cmin<b) cmin=b;// CHECK ?
                           float cmax=maxv/v0-latstep;
                           if(cmax>(mLengthMax-latstep))cmax=(mLengthMax-latstep);
                           if(cmax<=cmin)continue;
                           float c=cmin;
                           const float cstep=(cmax-cmin)/(ceil((cmax-cmin)/latstep)-0.0001);
                           for(;c<cmax;c+=cstep)
                           {
                              if((abs(c*cbeta)>a) || (abs(c*calpha)>b)) break;
                              duc.par[1]=(1/(a)-1/(a+latstep))*0.5*salpha*vv;
                              duc.par[2]=(1/(b)-1/(b+latstep))*0.5*sbeta *vv;
                              duc.par[3]=(1/(c)-1/(c+latstep))*0.5*sgamma*vv;
                              duc.par[4]=cosangstep*0.5/(salpha*sbeta );
                              duc.par[5]=cosangstep*0.5/(sbeta *sgamma);
                              duc.par[6]=cosangstep*0.5/(salpha*sgamma);
                              
                              uc0.par[0]=0;
                              uc0.par[1]=(1/(a)+1/(a+latstep))*0.5*salpha*vv;
                              uc0.par[2]=(1/(b)+1/(b+latstep))*0.5*sbeta *vv;
                              uc0.par[3]=(1/(c)+1/(c+latstep))*0.5*sgamma*vv;
                              uc0.par[4]=cgammar+cosangstep*.5/(salpha*sbeta );
                              uc0.par[5]=calphar+cosangstep*.5/(sbeta *sgamma);
                              uc0.par[6]=cbetar +cosangstep*.5/(salpha*sgamma);
                              /*
                              uclarge=uc0;
                              ucsmall=uc0;
                              for(unsigned int i=0;i<4;++i) {uclarge.par[i]-=duc.par[i];ucsmall.par[i]+=duc.par[i];}
                              for(unsigned int i=4;i<7;++i) {uclarge.par[i]+=duc.par[i];ucsmall.par[i]-=duc.par[i];}
                              
                              //if( (uclarge.par[4]>1)||(uclarge.par[5]>1)(uclarge.par[6]>1)) break;
                              uclarged=uclarge.DirectUnitCell();
                              ucsmalld =ucsmall.DirectUnitCell();
                              
                              if(true)//uclarged[0]<ucsmalld[0])
                              {
                                 char buf[200];
                                 sprintf(buf,"a=%5.2f-%5.2f b=%5.2f-%5.2f c=%5.2f-%5.2f alpha=%5.2f-%5.2f beta=%5.2f-%5.2f gamma=%5.2f-%5.2f V=%5.2f-%5.2f",
                                          ucsmalld[0],uclarged[0],ucsmalld[1],uclarged[1],ucsmalld[2],uclarged[2],ucsmalld[3]*RAD2DEG,uclarged[3]*RAD2DEG,
                                          ucsmalld[4]*RAD2DEG,uclarged[4]*RAD2DEG,ucsmalld[5]*RAD2DEG,uclarged[5]*RAD2DEG,ucsmalld[6],uclarged[6]);
                                 cout<<buf<<"   VM="<<maxv<<endl;
                                 //cout<<a<<","<<b<<","<<c<<","<<calpha<<","<<cbeta<<","<<cgamma<<endl;
                                 //cout<<uclarge.par[1]<<","<<uclarge.par[2]<<","<<uclarge.par[3]<<","<<uclarge.par[4]<<","<<uclarge.par[5]<<","<<uclarge.par[6]<<endl;
                                 //cout<<ucsmall.par[1]<<","<<ucsmall.par[2]<<","<<ucsmall.par[3]<<","<<ucsmall.par[4]<<","<<ucsmall.par[5]<<","<<ucsmall.par[6]<<endl;
                                 //cout<<calphar<<","<<cbetar<<","<<cgammar<<","<<uc0.par[4]<<","<<uc0.par[5]<<","<<uc0.par[6]<<endl;
                              }
                              */
                              RDicVol(uc0,duc,0,nbCalc,minv,maxv);
                           }
                           if(c==cmin) break;//still c==a, so no iteration passed
                        }
                        if(b==a) break;//still b==a, so no iteration passed
                     }
                     if(a==mLengthMin) break;
                  }
               }
            }
            break;
         }
         case MONOCLINIC:
         {
            RecUnitCell uclarge,//ucm: smallest reciprocal, largest direct cell
                        ucsmall;//ucp: largest reciprocal, smallest direct cell
            vector<float> uclarged,ucsmalld;
            for(float x4=0;x4<cosangmax+cosangstep;x4+=cosangstep)
            {
               const float sinbeta=sqrt(1-x4*x4);
               float x1=mLengthMin;
               for(;x1<mLengthMax;x1+=latstep)
               {
                  float x2=mLengthMin;
                  for(;x2<mLengthMax;x2+=latstep)
                  {
                     float x3=x1;
                     for(;x3<mLengthMax;x3+=latstep) //x3+=(latstep+x3*sin4)
                     {
                        if((x3*x4)>x1) break;// | c * cos(beta) | <a
                        duc.par[1]=(1/(x1)-1/(x1+latstep))*0.5/sinbeta;
                        duc.par[2]=(1/(x2)-1/(x2+latstep))*0.5/sinbeta;
                        duc.par[3]=(1/(x3)-1/(x3+latstep))*0.5/sinbeta;
                        duc.par[4]=cosangstep*0.5;
                        
                        uc0.par[0]=0;
                        uc0.par[1]=(1/(x1)+1/(x1+latstep))*0.5/sinbeta;
                        uc0.par[2]=(1/(x2)+1/(x2+latstep))*0.5/sinbeta;
                        uc0.par[3]=(1/(x3)+1/(x3+latstep))*0.5/sinbeta;
                        uc0.par[4]=x4+cosangstep*.5;
                        
                        const float smallv=x1*x2*x3*sinbeta;
                        if(smallv>maxv) break;
                        const float largev=(x1+latstep)*(x2+latstep)*(x3+latstep)*(sinbeta+cosangstep);
                        if(largev<minv) continue;
                        /*
                        char buf[200];
                        sprintf(buf,"a=%5.2f-%5.2f b=%5.2f-%5.2f c=%5.2f-%5.2f alpha=%5.2f-%5.2f beta=%5.2f-%5.2f gamma=%5.2f-%5.2f V=%5.2f-%5.2f",
                                 ucsmalld[0],uclarged[0],ucsmalld[1],uclarged[1],ucsmalld[2],uclarged[2],ucsmalld[3]*RAD2DEG,uclarged[3]*RAD2DEG,
                                 ucsmalld[4]*RAD2DEG,uclarged[4]*RAD2DEG,ucsmalld[5]*RAD2DEG,uclarged[5]*RAD2DEG,ucsmalld[6],uclarged[6]);
                        cout<<buf<<"   VM="<<maxv<<", x3="<<x3<<endl;
                        */
                        RDicVol(uc0,duc,0,nbCalc,minv,maxv);
                     }//x3
                     //if(((ucsmalld[6]>maxv)&&(x3==x1))||(uclarged[1]>mLengthMax)) break;
                  }//x2
               }//x1
               // Test if we have one solution before going to the next angle range
               for(list<pair<RecUnitCell,float> >::iterator pos=mvSolution.begin();pos!=mvSolution.end();++pos)
               {
                  const float score=pos->second;//Score(*mpPeakList,pos->first,mNbSpurious);
                  if(score>bestscore) {bestscore=score;bestpos=pos;}
               }
               bool breakDepth=false;
               if(stopOnDepth>0)
                  for(unsigned int i=stopOnDepth; i<mvNbSolutionDepth.size();++i)
                     if(mvNbSolutionDepth[i]>1) {breakDepth=true;break;}
               if((bestscore>stopOnScore)&&(breakDepth)) break;
            }//x4
            break;
         }
         case ORTHOROMBIC:
         {
            if(false)
            {
               // Test 7.677350  5.803770  10.313160   V=480
               //const float a=7.75,b=5.75,c=10.25;
               // Test 6.062000 16.779400 16.8881 v=1750
               const float a=6.25,b=16.75,c=16.75;
               duc.par[1]=(1/(a-.25)-1/(a+.25))*0.5;
               duc.par[2]=(1/(b-.25)-1/(b+.25))*0.5;
               duc.par[3]=(1/(c-.25)-1/(c+.25))*0.5;
               uc0.par[0]=0;
               uc0.par[1]=1/a;
               uc0.par[2]=1/b;
               uc0.par[3]=1/c;
               RDicVol(uc0,duc,0,nbCalc,minv,maxv);
               break;
            }
            for(float x1=mLengthMin;x1<mLengthMax;x1+=latstep)
            {
               for(float x2=x1;x2<mLengthMax;x2+=latstep)
               {
                  for(float x3=x2;x3<mLengthMax;x3+=latstep)
                  {
                     duc.par[1]=(1/(x1)-1/(x1+latstep))*0.5;
                     duc.par[2]=(1/(x2)-1/(x2+latstep))*0.5;
                     duc.par[3]=(1/(x3)-1/(x3+latstep))*0.5;
                     
                     uc0.par[0]=0;
                     uc0.par[1]=(1/(x1)+1/(x1+latstep))*0.5;
                     uc0.par[2]=(1/(x2)+1/(x2+latstep))*0.5;
                     uc0.par[3]=(1/(x3)+1/(x3+latstep))*0.5;
                     
                     vector<float> uc=uc0.DirectUnitCell();
                     RecUnitCell ucm=uc0,ucp=uc0;
                     for(unsigned int i=0;i<6;++i) {ucm.par[i]-=duc.par[i];ucp.par[i]+=duc.par[i];}
                     vector<float> ucmd=ucm.DirectUnitCell();
                     vector<float> ucpd=ucp.DirectUnitCell();
                     char buf[200];
                     sprintf(buf,"a=%5.2f-%5.2f b=%5.2f-%5.2f c=%5.2f-%5.2f alpha=%5.2f-%5.2f beta=%5.2f-%5.2f gamma=%5.2f-%5.2f V=%5.2f-%5.2f",
                              ucpd[0],ucmd[0],ucpd[1],ucmd[1],ucpd[2],ucmd[2],ucpd[3]*RAD2DEG,ucmd[3]*RAD2DEG,
                              ucpd[4]*RAD2DEG,ucmd[4]*RAD2DEG,ucpd[5]*RAD2DEG,ucmd[5]*RAD2DEG,ucpd[6],ucmd[6]);
                     if((ucpd[6]<maxv)&&(ucmd[6]>minv))
                     {
                        //cout<<buf<<"   VM="<<maxv<<endl;
                        RDicVol(uc0,duc,0,nbCalc,minv,maxv);
                     }
                     else
                     {
                        //cout<<buf<<"BREAK"<<endl;
                        if(ucpd[6]>maxv) break;
                     }
                  }
                  if((x1*x2*x2)>maxv) break;
               }
               if((x1*x1*x1)>maxv) break;
            }
            break;
         }
         case HEXAGONAL:
         {
            vector<float> uclarged,ucsmalld;// Small & large UC in direct space
            for(float x1=mLengthMin;;x1+=latstep)
            {
               for(float x2=mLengthMin;x2<(mLengthMax+latstep);x2+=latstep)
               {
                  duc.par[1]=(1/(x1)-1/(x1+latstep))*0.5;
                  duc.par[2]=(1/(x2)-1/(x2+latstep))*0.5;
                     
                  uc0.par[0]=0;
                  uc0.par[1]=(1/(x1)+1/(x1+latstep))*0.5;
                  uc0.par[2]=(1/(x2)+1/(x2+latstep))*0.5;
                  
                  RecUnitCell uclarge=uc0,ucsmall=uc0;
                  for(unsigned int i=0;i<6;++i) {uclarge.par[i]-=duc.par[i];ucsmall.par[i]+=duc.par[i];}
                  uclarged=uclarge.DirectUnitCell();
                  ucsmalld=ucsmall.DirectUnitCell();
                  /*
                  char buf[200];
                  sprintf(buf,"a=%5.2f-%5.2f b=%5.2f-%5.2f c=%5.2f-%5.2f alpha=%5.2f-%5.2f beta=%5.2f-%5.2f gamma=%5.2f-%5.2f V=%5.2f-%5.2f",
                           ucsmalld[0],uclarged[0],ucsmalld[1],uclarged[1],ucsmalld[2],uclarged[2],ucsmalld[3]*RAD2DEG,uclarged[3]*RAD2DEG,
                           ucsmalld[4]*RAD2DEG,uclarged[4]*RAD2DEG,ucsmalld[5]*RAD2DEG,uclarged[5]*RAD2DEG,ucsmalld[6],uclarged[6]);
                  */
                  if((ucsmalld[6]<maxv)&&(uclarged[6]>minv))
                  {
                     //cout<<buf<<endl;
                     RDicVol(uc0,duc,0,nbCalc,minv,maxv);
                  }
                  //else cout<<buf<<" BREAK"<<endl;
               }
               if(uclarged[0]>mLengthMax) break;
            }
            break;
         }
         case RHOMBOEDRAL:   //:TODO:
         {
            for(float x1=mLengthMin;x1<(mLengthMax+latstep);x1+=latstep)
            {
               for(float x2=0;x2<cosangmax+cosangstep;x2+=cosangstep)
               {
                  duc.par[1]=latstep/2*1.1;
                  duc.par[2]=cosangstep/2*1.1;
                  
                  uc0.par[0]=0;
                  uc0.par[1]=x1-latstep/2*1.1;
                  uc0.par[2]=x2-cosangstep/2*1.1;
                  vector<float> uc=uc0.DirectUnitCell();
                  if((uc[6]<maxv)&&(uc[6]>minv))
                  {
                     RDicVol(uc0,duc,0,nbCalc,minv,maxv);
                  }
               }
            }
            break;
         }
         case TETRAGONAL:
         {
            vector<float> uclarged,ucsmalld;// Small & large UC in direct space
            for(float x1=mLengthMin;x1<mLengthMax;x1+=latstep)
            {
               for(float x2=mLengthMin;x2<mLengthMax;x2+=latstep)
               {
                  duc.par[1]=(1/(x1)-1/(x1+latstep))*0.5;
                  duc.par[2]=(1/(x2)-1/(x2+latstep))*0.5;
                     
                  uc0.par[0]=0;
                  uc0.par[1]=(1/(x1)+1/(x1+latstep))*0.5;
                  uc0.par[2]=(1/(x2)+1/(x2+latstep))*0.5;
                  
                  RecUnitCell uclarge=uc0,ucsmall=uc0;
                  for(unsigned int i=0;i<6;++i) {uclarge.par[i]-=duc.par[i];ucsmall.par[i]+=duc.par[i];}
                  uclarged=uclarge.DirectUnitCell();
                  ucsmalld=ucsmall.DirectUnitCell();
                  /*
                  char buf[200];
                  sprintf(buf,"a=%5.2f-%5.2f b=%5.2f-%5.2f c=%5.2f-%5.2f alpha=%5.2f-%5.2f beta=%5.2f-%5.2f gamma=%5.2f-%5.2f V=%5.2f-%5.2f",
                           ucsmalld[0],uclarged[0],ucsmalld[1],uclarged[1],ucsmalld[2],uclarged[2],ucsmalld[3]*RAD2DEG,uclarged[3]*RAD2DEG,
                           ucsmalld[4]*RAD2DEG,uclarged[4]*RAD2DEG,ucsmalld[5]*RAD2DEG,uclarged[5]*RAD2DEG,ucsmalld[6],uclarged[6]);
                  */
                  if((ucsmalld[6]<maxv)&&(uclarged[6]>minv))
                  {
                     RDicVol(uc0,duc,0,nbCalc,minv,maxv);
                  }
                  if(ucsmalld[6]>maxv) break;
               }
               if((x1*mLengthMin*mLengthMin)>maxv) break;
            }
            break;
         }
         case CUBIC:
         {
            vector<float> uclarged,ucsmalld;// Small & large UC in direct space
            for(float x1=mLengthMin;x1<(mLengthMax+latstep);x1+=latstep)
            {
               duc.par[1]=(1/(x1)-1/(x1+latstep))*0.5;
                  
               uc0.par[0]=0;
               uc0.par[1]=(1/(x1)+1/(x1+latstep))*0.5;
               
               RecUnitCell uclarge=uc0,ucsmall=uc0;
               uclarge.par[1]-=duc.par[1];
               ucsmall.par[1]+=duc.par[1];
               
               uclarged=uclarge.DirectUnitCell();
               ucsmalld=ucsmall.DirectUnitCell();
               /*
               char buf[200];
               sprintf(buf,"a=%5.2f-%5.2f b=%5.2f-%5.2f c=%5.2f-%5.2f alpha=%5.2f-%5.2f beta=%5.2f-%5.2f gamma=%5.2f-%5.2f V=%5.2f-%5.2f",
                        ucsmalld[0],uclarged[0],ucsmalld[1],uclarged[1],ucsmalld[2],uclarged[2],ucsmalld[3]*RAD2DEG,uclarged[3]*RAD2DEG,
                        ucsmalld[4]*RAD2DEG,uclarged[4]*RAD2DEG,ucsmalld[5]*RAD2DEG,uclarged[5]*RAD2DEG,ucsmalld[6],uclarged[6]);
               */
               if((ucsmalld[6]<maxv)&&(uclarged[6]>minv))
               {
                  //cout<<buf<<endl;
                  RDicVol(uc0,duc,0,nbCalc,minv,maxv);
               }
               //else cout<<buf<<" BREAK"<<endl;
            }
            break;
         }
      }
      for(list<pair<RecUnitCell,float> >::iterator pos=mvSolution.begin();pos!=mvSolution.end();++pos)
      {
         const float score=pos->second;//Score(*mpPeakList,pos->first,mNbSpurious);
         if(score>bestscore) {bestscore=score;bestpos=pos;}
      }
      bool breakDepth=false;
      if(stopOnDepth>0)
         for(unsigned int i=stopOnDepth; i<mvNbSolutionDepth.size();++i)
            if(mvNbSolutionDepth[i]>1) {breakDepth=true;break;}
      if((bestscore>stopOnScore)&&(breakDepth)) break;
   }
   /*
   {// Tag spurious lines
      vector<int> vSpuriousScore;
      for(vector<PeakList::hkl>::const_iterator pos=mpPeakList->GetPeakList().begin();pos!=mpPeakList->GetPeakList().end();++pos)
         vSpuriousScore.push_back(pos->stats);
      sort(vSpuriousScore.begin(),vSpuriousScore.end());
      const int threshold=vSpuriousScore[vSpuriousScore.size()/2]*5;
      for(vector<PeakList::hkl>::iterator pos=mpPeakList->mvHKL.begin();pos!=mpPeakList->mvHKL.end();++pos)
         if(pos->stats > threshold) pos->isSpurious=true;
         else pos->isSpurious=false;
      mpPeakList->Print(cout);
   }
   */
   this->ReduceSolutions();
   bestscore=0;bestpos=mvSolution.end();
   for(list<pair<RecUnitCell,float> >::iterator pos=mvSolution.begin();pos!=mvSolution.end();++pos)
   {
      const float score=Score(*mpPeakList,pos->first,mNbSpurious);
      if(score>bestscore) {bestpos=pos;bestscore=score;}
      vector<float> uc=pos->first.DirectUnitCell();
      cout<<__FILE__<<":"<<__LINE__<<" Solution ? a="<<uc[0]<<", b="<<uc[1]<<", c="<<uc[2]
          <<", alpha="<<uc[3]*RAD2DEG<<", beta="<<uc[4]*RAD2DEG<<", gamma="<<uc[5]*RAD2DEG
          <<", V="<<uc[6]<<", score="<<score<<endl;
   }
   if(bestpos!=mvSolution.end())
   {
      vector<float> uc=bestpos->first.DirectUnitCell();
      cout<<__FILE__<<":"<<__LINE__<<" BEST ? a="<<uc[0]<<", b="<<uc[1]<<", c="<<uc[2]
            <<", alpha="<<uc[3]*RAD2DEG<<", beta="<<uc[4]*RAD2DEG<<", gamma="<<uc[5]*RAD2DEG
            <<", V="<<uc[6]<<", score="<<bestscore<<endl;
      cout<<nbCalc<<"unit cells tested, "<<nbCalc/((clock()-mTime0)/(float)CLOCKS_PER_SEC)<<" tests/s,   Elapsed time="
          <<(clock()-mTime0)/(float)CLOCKS_PER_SEC<<"s"<<endl;
   }
}

bool SimilarRUC(const RecUnitCell &c0,const RecUnitCell &c1, const float delta=0.005)
{
   vector<float> uc0=c0.DirectUnitCell();
   vector<float> uc1=c1.DirectUnitCell();
   float diff=0;
   for(unsigned int i=0;i<6;++i) diff += abs(uc0[i]-uc1[i]);
   return (diff/6)<delta;
}

bool compareRUCScore(std::pair<RecUnitCell,float> &p1, std::pair<RecUnitCell,float> &p2)
{
   return p1.second > p2.second;
}

void CellExplorer::ReduceSolutions()
{
   const bool verbose=false;
   std::list<std::pair<RecUnitCell,float> > vSolution2;
   while(mvSolution.size()>0)
   {
      vSolution2.push_back(mvSolution.front());
      mvSolution.pop_front();
      vector<float> uc=vSolution2.back().first.DirectUnitCell();
      if(verbose)
         cout<<__FILE__<<":"<<__LINE__<<" SOLUTION: a="<<uc[0]<<", b="<<uc[1]<<", c="<<uc[2]
               <<", alpha="<<uc[3]*RAD2DEG<<", beta="<<uc[4]*RAD2DEG<<", gamma="<<uc[5]*RAD2DEG
               <<", V="<<uc[6]<<", score="<<vSolution2.back().second<<",   SIMILAR TO:"<<endl;
      for(list<pair<RecUnitCell,float> >::iterator pos=mvSolution.begin();pos!=mvSolution.end();)
      {
         if(SimilarRUC(pos->first,vSolution2.back().first))
         {
            uc=pos->first.DirectUnitCell();
            if(verbose)
               cout<<__FILE__<<":"<<__LINE__<<"        1: a="<<uc[0]<<", b="<<uc[1]<<", c="<<uc[2]
                     <<", alpha="<<uc[3]*RAD2DEG<<", beta="<<uc[4]*RAD2DEG<<", gamma="<<uc[5]*RAD2DEG
                     <<", V="<<uc[6]<<", score="<<pos->second<<"       ("<<mvSolution.size()<<")"<<endl;
            if(vSolution2.back().first.mlattice==pos->first.mlattice)
            {
               if(pos->second>vSolution2.back().second) vSolution2.back()=*pos;
            }
            else if(vSolution2.back().first.mlattice<pos->first.mlattice) vSolution2.back()=*pos;
            pos=mvSolution.erase(pos);
         }
         else
         {
            uc=pos->first.DirectUnitCell();
            if(verbose)
               cout<<__FILE__<<":"<<__LINE__<<"        0: a="<<uc[0]<<", b="<<uc[1]<<", c="<<uc[2]
                     <<", alpha="<<uc[3]*RAD2DEG<<", beta="<<uc[4]*RAD2DEG<<", gamma="<<uc[5]*RAD2DEG
                     <<", V="<<uc[6]<<", score="<<pos->second<<"       ("<<mvSolution.size()<<")"<<endl;
            ++pos;
         }
      }
   }
   mvSolution=vSolution2;
   mvSolution.sort(compareRUCScore);
}


void CellExplorer::Init()
{
   // Prepare global optimisation
   //for(unsigned int i=0;i<mpPeakList->nb;++i)
   //   cout<<__FILE__<<":"<<__LINE__<<":d*="<<mpPeakList->mvdobs[i]<<", d*^2="<<mpPeakList->mvd2obs[i]<<endl;
   srand(time(NULL));
   vector<pair<RecUnitCell,float> >::iterator pos;
   const float min_latt=1./mLengthMax;
   const float max_latt=1./mLengthMin;
   const float amp_crossp=abs(cos(mAngleMax));
   //mMin[0]=-.002;mAmp[0]=.004;
   mMin[0]=.00;mAmp[0]=.00;
   switch(mlattice)
   {
      case TRICLINIC:
         mMin[1]=min_latt;mAmp[1]=max_latt-min_latt;
         mMin[2]=min_latt;mAmp[2]=max_latt-min_latt;
         mMin[3]=min_latt;mAmp[3]=max_latt-min_latt;
         mMin[4]=0;mAmp[4]=amp_crossp;
         mMin[5]=0;mAmp[5]=amp_crossp;
         mMin[6]=0;mAmp[6]=amp_crossp;
         mnpar=7;
	 break;
      case MONOCLINIC:
         mMin[1]=min_latt;mAmp[1]=max_latt-min_latt;
         mMin[2]=min_latt;mAmp[2]=max_latt-min_latt;
         mMin[3]=min_latt;mAmp[3]=max_latt-min_latt;
         mMin[4]=0;mAmp[4]=amp_crossp;
	 mnpar=5;
         break;
      case ORTHOROMBIC:
         mMin[1]=min_latt;mAmp[1]=max_latt-min_latt;
         mMin[2]=min_latt;mAmp[2]=max_latt-min_latt;
         mMin[3]=min_latt;mAmp[3]=max_latt-min_latt;
	 mnpar=4;
         break;
      case HEXAGONAL:
         mMin[1]=min_latt;mAmp[1]=max_latt-min_latt;
         mMin[2]=min_latt;mAmp[2]=max_latt-min_latt;
         mnpar=3;
         break;
      case RHOMBOEDRAL:
         mMin[1]=min_latt;mAmp[1]=max_latt-min_latt;
         mMin[2]=-amp_crossp;mAmp[2]=2*amp_crossp;
         mnpar=3;
         break;
      case TETRAGONAL:
         mMin[1]=min_latt;mAmp[1]=max_latt-min_latt;
         mMin[2]=min_latt;mAmp[2]=max_latt-min_latt;
         mnpar=3;
         break;
      case CUBIC:
         mMin[1]=min_latt;mAmp[1]=max_latt-min_latt;
         mnpar=2;
         break;
   }
   for(unsigned int k=0;k<mnpar;++k) cout<<"par["<<k<<"]: "<<mMin[k]<<"->"<<mMin[k]+mAmp[k]<<endl;
   
   unsigned int nb1=0,nb2=0;
   switch(mlattice)
   {
      case TRICLINIC:
      {
         nb1=3;
         nb2=3;
         break;
      }
      case MONOCLINIC:
      {
         nb1=3;
         nb2=1;
         break;
      }
      case ORTHOROMBIC:
      {
         nb1=3;
         break;
      }
      case HEXAGONAL:
      {
         nb1=2;
         break;
      }
      case RHOMBOEDRAL:
      {
         nb1=2;
         break;
      }
      case TETRAGONAL:
      {
         nb1=2;
         break;
      }
      case CUBIC:
      {
         nb1=1;
         break;
      }
   }
   this->ResetParList();
   {
      RefinablePar tmp("Zero",mRecUnitCell.par+0,-0.01,0.01,gpRefParTypeObjCryst,REFPAR_DERIV_STEP_ABSOLUTE,
                       true,false,true,false);
      tmp.SetDerivStep(1e-4);
      this->AddPar(tmp);
   }
   char buf [50];
   string str="Reciprocal unit cell par #";
   for(unsigned int i=0;i<nb1;++i)
   {
      sprintf(buf,"%i",i);
      RefinablePar tmp(str+(string)buf,mRecUnitCell.par+i+1,
                       0.01,1.,gpRefParTypeObjCryst,REFPAR_DERIV_STEP_ABSOLUTE,
                       false,false,true,false);
      tmp.SetDerivStep(1e-4);
      this->AddPar(tmp);
   }
   for(unsigned int i=nb1;i<(nb1+nb2);++i)
   {
      sprintf(buf,"%i",i);
      RefinablePar tmp(str+(string)buf,mRecUnitCell.par+i+1,
                       0.0,0.5,gpRefParTypeObjCryst,REFPAR_DERIV_STEP_ABSOLUTE,
                       false,false,true,false);
      tmp.SetDerivStep(1e-4);
      this->AddPar(tmp);
   }
}

}//namespace
