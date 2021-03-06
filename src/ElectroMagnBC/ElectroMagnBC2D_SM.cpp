#include "ElectroMagnBC2D_SM.h"

#include <cstdlib>

#include <iostream>
#include <string>

#include "Params.h"
#include "Patch.h"
#include "ElectroMagn.h"
#include "Field2D.h"
#include "Tools.h"
#include "Laser.h"

using namespace std;

ElectroMagnBC2D_SM::ElectroMagnBC2D_SM( Params &params, Patch* patch )
  : ElectroMagnBC( params, patch )
{
    // conversion factor from degree to radian
    conv_deg2rad = M_PI/180.0;
    
    // number of nodes of the primal and dual grid in the x-direction
    nx_p = params.n_space[0]+1+2*params.oversize[0];
    nx_d = nx_p+1;
    // number of nodes of the primal and dual grid in the y-direction
    ny_p = params.n_space[1]+1+2*params.oversize[1];
    ny_d = ny_p+1;
    
    // spatial-step and ratios time-step by spatial-step & spatial-step by time-step (in the x-direction)
    dx       = params.cell_length[0];
    dt_ov_dx = dt/dx;
    dx_ov_dt = 1.0/dt_ov_dx;
    
    // spatial-step and ratios time-step by spatial-step & spatial-step by time-step (in the y-direction)
    dy       = params.cell_length[1];
    dt_ov_dy = dt/dy;
    dy_ov_dt = 1.0/dt_ov_dy;
    
    // BCs at the x-border
    Bx_xvalmin_Long.resize(ny_d,0.); // dual in the y-direction
    Bx_xvalmax_Long.resize(ny_d,0.);
    By_xvalmin_Long.resize(ny_p,0.); // primal in the y-direction
    By_xvalmax_Long.resize(ny_p,0.);
    Bz_xvalmin_Long.resize(ny_d,0.); // dual in the y-direction
    Bz_xvalmax_Long.resize(ny_d,0.);
    
    // BCs in the y-border
    Bx_yvalmin_Trans.resize(nx_p,0.); // primal in the x-direction
    Bx_yvalmax_Trans.resize(nx_p,0.);
    By_yvalmin_Trans.resize(nx_d,0.); // dual in the x-direction
    By_yvalmax_Trans.resize(nx_d,0.);
    Bz_yvalmin_Trans.resize(nx_d,0.); // dual in the x-direction
    Bz_yvalmax_Trans.resize(nx_d,0.);
    
    
    // -----------------------------------------------------
    // Parameters for the Silver-Mueller boundary conditions
    // -----------------------------------------------------
    
    //! \todo (MG) Check optimal angle for Silver-Muller BCs
    
    // Xmin boundary
    double theta  = 0.0*conv_deg2rad; //0.0;
    double factor = 1.0 / (cos(theta) + dt_ov_dx);
    Alpha_SM_W    = 2.0                     * factor;
    Beta_SM_W     = - (cos(theta)-dt_ov_dx) * factor;
    Gamma_SM_W    = 4.0 * cos(theta)        * factor;
    Delta_SM_W    = - (sin(theta)+dt_ov_dy) * factor;
    Epsilon_SM_W  = - (sin(theta)-dt_ov_dy) * factor;
    
    // Xmax boundary
    theta         = M_PI;
    factor        = 1.0 / (cos(theta) - dt_ov_dx);
    Alpha_SM_E    = 2.0                      * factor;
    Beta_SM_E     = - (cos(theta)+dt_ov_dx)  * factor;
    Gamma_SM_E    = 4.0 * cos(theta)         * factor;
    Delta_SM_E    = - (sin(theta)+dt_ov_dy)  * factor;
    Epsilon_SM_E  = - (sin(theta)-dt_ov_dy)  * factor;
    
    // Ymin boundary
    theta  = 0.0;
    factor = 1.0 / (cos(theta) + dt_ov_dy );
    Alpha_SM_S    = 2.0                     * factor;
    Beta_SM_S     = - (cos(theta)-dt_ov_dy) * factor;
    Delta_SM_S    = - (sin(theta)+dt_ov_dx) * factor;
    Epsilon_SM_S  = - (sin(theta)-dt_ov_dx) * factor;
    
    // Ymax boundary
    theta  = M_PI;
    factor = 1.0 / (cos(theta) - dt_ov_dy);
    Alpha_SM_N    = 2.0                     * factor;
    Beta_SM_N     = - (cos(theta)+dt_ov_dy) * factor;
    Delta_SM_N    = - (sin(theta)+dt_ov_dx) * factor;
    Epsilon_SM_N  = - (sin(theta)-dt_ov_dx) * factor;

}

ElectroMagnBC2D_SM::~ElectroMagnBC2D_SM()
{
}

void ElectroMagnBC2D_SM::save_fields_BC2D_Long(Field* my_field) {
    Field2D* field2D=static_cast<Field2D*>(my_field);
    
    if (field2D->name=="Bx"){
        // (Bx)^(pd)
        for (unsigned int j=0; j<ny_d; j++) {
            Bx_xvalmin_Long[j]=(*field2D)(0,j);
            Bx_xvalmax_Long[j]=(*field2D)(nx_p-1,j);
        }
    }
    
    if (field2D->name=="By"){
        // (By)^(dp)
        for (unsigned int j=0; j<ny_p; j++) {
            By_xvalmin_Long[j]=(*field2D)(0,j);
            By_xvalmax_Long[j]=(*field2D)(nx_d-1,j);
        }
    }
    
    if (field2D->name=="Bz"){
        // (By)^(dd)
        for (unsigned int j=0; j<ny_d; j++) {
            Bz_xvalmin_Long[j]=(*field2D)(0,j);
            Bz_xvalmax_Long[j]=(*field2D)(nx_d-1,j);
        }
    }
    
}

void ElectroMagnBC2D_SM::save_fields_BC2D_Trans(Field* my_field) {
    Field2D* field2D=static_cast<Field2D*>(my_field);
  
    if (field2D->name=="Bx"){
        // (Bx)^(pd)
        for (unsigned int i=0; i<nx_p; i++) {
            Bx_yvalmin_Trans[i]=(*field2D)(i,0);
            Bx_yvalmax_Trans[i]=(*field2D)(i,ny_d-1);
        }
    }
    
    if (field2D->name=="By"){
        // (By)^(dp)
        for (unsigned int i=0; i<nx_d; i++) {
            By_yvalmin_Trans[i]=(*field2D)(i,0);
            By_yvalmax_Trans[i]=(*field2D)(i,ny_p-1);
        }
    }
    
    if (field2D->name=="Bz"){
        // (By)^(dd)
        for (unsigned int i=0; i<nx_d; i++) {
            Bz_yvalmin_Trans[i]=(*field2D)(i,0);
            Bz_yvalmax_Trans[i]=(*field2D)(i,ny_d-1);
        }
    }
    
}


// ---------------------------------------------------------------------------------------------------------------------
// Apply Boundary Conditions
// ---------------------------------------------------------------------------------------------------------------------
void ElectroMagnBC2D_SM::apply_xmin(ElectroMagn* EMfields, double time_dual, Patch* patch)
{
    if ( patch->isXmin() ) {
        
        // Static cast of the fields
        //Field2D* Ex2D = static_cast<Field2D*>(EMfields->Ex_);
        Field2D* Ey2D = static_cast<Field2D*>(EMfields->Ey_);
        Field2D* Ez2D = static_cast<Field2D*>(EMfields->Ez_);
        Field2D* Bx2D = static_cast<Field2D*>(EMfields->Bx_);
        Field2D* By2D = static_cast<Field2D*>(EMfields->By_);
        Field2D* Bz2D = static_cast<Field2D*>(EMfields->Bz_);
        
        // for By^(d,p)
        vector<double> yp(1);
        yp[0] = patch->getDomainLocalMin(1) - EMfields->oversize[1]*dy;
        for (unsigned int j=0 ; j<ny_p ; j++) {
            
            double byW = 0.;
            yp[0] += dy;
            
            // Lasers
            for (unsigned int ilaser=0; ilaser< vecLaser.size(); ilaser++) {
                byW += vecLaser[ilaser]->getAmplitude0(yp, time_dual, j, 0);
            }
            
            /*(*By2D)(0,j) = Alpha_SM_W   * (*Ez2D)(0,j)
            +              Beta_SM_W    * (*By2D)(1,j)
            +              Gamma_SM_W   * byW
            +              Delta_SM_W   * (*Bx2D)(0,j+1)
            +              Epsilon_SM_W * (*Bx2D)(0,j);*/
            (*By2D)(0,j) = Alpha_SM_W   * (*Ez2D)(0,j)
            +              Beta_SM_W    *( (*By2D)(1,j)-By_xvalmin_Long[j])
            +              Gamma_SM_W   * byW
            +              Delta_SM_W   *( (*Bx2D)(0,j+1)-Bx_xvalmin_Long[j+1] )
            +              Epsilon_SM_W *( (*Bx2D)(0,j)-Bx_xvalmin_Long[j] )
            +              By_xvalmin_Long[j];
            
        }//j  ---end compute By
        
        
        // for Bz^(d,d)
        vector<double> yd(1);
        yd[0] = patch->getDomainLocalMin(1) - (0.5+EMfields->oversize[1])*dy;
        for (unsigned int j=0 ; j<ny_d ; j++) {
            
            double bzW = 0.;
            yd[0] += dy;
            
            // Lasers
            for (unsigned int ilaser=0; ilaser< vecLaser.size(); ilaser++) {
                bzW += vecLaser[ilaser]->getAmplitude1(yd, time_dual, j, 0);
            }
            
            /*(*Bz2D)(0,j) = -Alpha_SM_W * (*Ey2D)(0,j)
            +               Beta_SM_W  * (*Bz2D)(1,j)
            +               Gamma_SM_W * bzW;*/
            (*Bz2D)(0,j) = -Alpha_SM_W * (*Ey2D)(0,j)
            +               Beta_SM_W  *( (*Bz2D)(1,j)- Bz_xvalmin_Long[j])
            +               Gamma_SM_W * bzW
            +               Bz_xvalmin_Long[j];
            
        }//j  ---end compute Bz
        
        
    }//if Xmin

}
// ---------------------------------------------------------------------------------------------------------------------
// Apply Boundary Conditions
// ---------------------------------------------------------------------------------------------------------------------
void ElectroMagnBC2D_SM::apply_xmax(ElectroMagn* EMfields, double time_dual, Patch* patch)
{
    if ( patch->isXmax() ) {
        
        // Static cast of the fields
        //Field2D* Ex2D = static_cast<Field2D*>(EMfields->Ex_);
        Field2D* Ey2D = static_cast<Field2D*>(EMfields->Ey_);
        Field2D* Ez2D = static_cast<Field2D*>(EMfields->Ez_);
        Field2D* Bx2D = static_cast<Field2D*>(EMfields->Bx_);
        Field2D* By2D = static_cast<Field2D*>(EMfields->By_);
        Field2D* Bz2D = static_cast<Field2D*>(EMfields->Bz_);
        
        // for By^(d,p)
        vector<double> yp(1);
        yp[0] = patch->getDomainLocalMin(1) - EMfields->oversize[1]*dy;
        for (unsigned int j=0 ; j<ny_p ; j++) {
            
            double byE = 0.;
            yp[0] += dy;
            
            // Lasers
            for (unsigned int ilaser=0; ilaser< vecLaser.size(); ilaser++) {
                byE += vecLaser[ilaser]->getAmplitude0(yp, time_dual, j, 0);
            }
            
            /*(*By2D)(nx_d-1,j) = Alpha_SM_E   * (*Ez2D)(nx_p-1,j)
            +                   Beta_SM_E    * (*By2D)(nx_d-2,j)
            +                   Gamma_SM_E   * byE
            +                   Delta_SM_E   * (*Bx2D)(nx_p-1,j+1) // Check x-index
            +                   Epsilon_SM_E * (*Bx2D)(nx_p-1,j);*/
            (*By2D)(nx_d-1,j) = Alpha_SM_E   * (*Ez2D)(nx_p-1,j)
            +                   Beta_SM_E    *( (*By2D)(nx_d-2,j) -By_xvalmax_Long[j])
            +                   Gamma_SM_E   * byE
            +                   Delta_SM_E   *( (*Bx2D)(nx_p-1,j+1) -Bx_xvalmax_Long[j+1])// Check x-index
            +                   Epsilon_SM_E *( (*Bx2D)(nx_p-1,j) -Bx_xvalmax_Long[j])
            +                   By_xvalmax_Long[j];
            
        }//j  ---end compute By
        
        
        // for Bz^(d,d)
        vector<double> yd(1);
        yd[0] = patch->getDomainLocalMin(1) - (0.5+EMfields->oversize[1])*dy;
        for (unsigned int j=0 ; j<ny_d ; j++) {
            
            double bzE = 0.;
            yd[0] += dy;
            
            // Lasers
            for (unsigned int ilaser=0; ilaser< vecLaser.size(); ilaser++) {
                bzE += vecLaser[ilaser]->getAmplitude1(yd, time_dual, j, 0);
            }
            
            /*(*Bz2D)(nx_d-1,j) = -Alpha_SM_E * (*Ey2D)(nx_p-1,j)
            +                    Beta_SM_E  * (*Bz2D)(nx_d-2,j)
            +                    Gamma_SM_E * bzE;*/
            (*Bz2D)(nx_d-1,j) = -Alpha_SM_E * (*Ey2D)(nx_p-1,j)
            +                    Beta_SM_E  *( (*Bz2D)(nx_d-2,j) -Bz_xvalmax_Long[j])
            +                    Gamma_SM_E * bzE
            +                    Bz_xvalmax_Long[j];
            
        }//j  ---end compute Bz
        
        
    }//if Xmax

}

// ---------------------------------------------------------------------------------------------------------------------
// Apply Boundary Conditions
// ---------------------------------------------------------------------------------------------------------------------
void ElectroMagnBC2D_SM::apply_ymin(ElectroMagn* EMfields, double time_dual, Patch* patch)
{
    if ( patch->isYmin() ) {
        
        // Static cast of the fields
        Field2D* Ex2D = static_cast<Field2D*>(EMfields->Ex_);
        //Field2D* Ey2D = static_cast<Field2D*>(EMfields->Ey_);
        Field2D* Ez2D = static_cast<Field2D*>(EMfields->Ez_);
        Field2D* Bx2D = static_cast<Field2D*>(EMfields->Bx_);
        Field2D* By2D = static_cast<Field2D*>(EMfields->By_);
        Field2D* Bz2D = static_cast<Field2D*>(EMfields->Bz_);
        
        // for Bx^(p,d)
        for (unsigned int j=0 ; j<nx_p ; j++) {
            /*(*Bx2D)(j,0) = -Alpha_SM_S   * (*Ez2D)(j,0)
            +               Beta_SM_S    * (*Bx2D)(j,1)
            +               Delta_SM_S   * (*By2D)(j+1,0)
            +               Epsilon_SM_S * (*By2D)(j,0);*/
            (*Bx2D)(j,0) = -Alpha_SM_S   * (*Ez2D)(j,0)
            +               Beta_SM_S    *( (*Bx2D)(j,1)-Bx_yvalmin_Trans[j])
            +               Delta_SM_S   *( (*By2D)(j+1,0)-By_yvalmin_Trans[j+1])
            +               Epsilon_SM_S *( (*By2D)(j,0)-By_yvalmin_Trans[j])
            +               Bx_yvalmin_Trans[j];
        }//j  ---end Bx
        
        
        // for Bz^(d,d)
        for (unsigned int j=0 ; j<nx_d ; j++) {
            /*(*Bz2D)(j,0) = Alpha_SM_S * (*Ex2D)(j,0)
            +               Beta_SM_S * (*Bz2D)(j,1);*/
            (*Bz2D)(j,0) = Alpha_SM_S * (*Ex2D)(j,0)
            +               Beta_SM_S  *( (*Bz2D)(j,1)-Bz_yvalmin_Trans[j])
            +               Bz_yvalmin_Trans[j];
        }//j  ---end Bz
        
    }//if Ymin
    
}
// ---------------------------------------------------------------------------------------------------------------------
// Apply Boundary Conditions
// ---------------------------------------------------------------------------------------------------------------------
void ElectroMagnBC2D_SM::apply_ymax(ElectroMagn* EMfields, double time_dual, Patch* patch)
{
    if ( patch->isYmax() ) {
        
        // Static cast of the fields
        Field2D* Ex2D = static_cast<Field2D*>(EMfields->Ex_);
        //Field2D* Ey2D = static_cast<Field2D*>(EMfields->Ey_);
        Field2D* Ez2D = static_cast<Field2D*>(EMfields->Ez_);
        Field2D* Bx2D = static_cast<Field2D*>(EMfields->Bx_);
        Field2D* By2D = static_cast<Field2D*>(EMfields->By_);
        Field2D* Bz2D = static_cast<Field2D*>(EMfields->Bz_);
        
        // for Bx^(p,d)
        for (unsigned int j=0 ; j<nx_p ; j++) {
            /*(*Bx2D)(j,ny_d-1) = -Alpha_SM_N   * (*Ez2D)(j,ny_p-1)
            +                    Beta_SM_N    * (*Bx2D)(j,ny_d-2)
            +                    Delta_SM_N   * (*By2D)(j+1,ny_p-1)
            +                    Epsilon_SM_N * (*By2D)(j,ny_p-1);*/
            (*Bx2D)(j,ny_d-1) = -Alpha_SM_N   * (*Ez2D)(j,ny_p-1)
            +                   Beta_SM_N    *( (*Bx2D)(j,ny_d-2) -Bx_yvalmax_Trans[j])
            +                   Delta_SM_N   *( (*By2D)(j+1,ny_p-1) -By_yvalmax_Trans[j+1])
            +                   Epsilon_SM_N *( (*By2D)(j,ny_p-1) -By_yvalmax_Trans[j])
            +                   Bx_yvalmax_Trans[j];
        }//j  ---end Bx
        
        
        // for Bz^(d,d)
        for (unsigned int j=0 ; j<nx_d ; j++) {
            /*(*Bz2D)(j,ny_d-1) = Alpha_SM_N * (*Ex2D)(j,ny_p-1)
            +                   Beta_SM_N  * (*Bz2D)(j,ny_d-2);*/
            (*Bz2D)(j,ny_d-1) = Alpha_SM_N * (*Ex2D)(j,ny_p-1)
            +                   Beta_SM_N  *( (*Bz2D)(j,ny_d-2)- Bz_yvalmax_Trans[j])
            +                   Bz_yvalmax_Trans[j];
        }//j  ---end Bx
        
    }//if Ymax
    
}


void ElectroMagnBC2D_SM::apply_zmin(ElectroMagn* EMfields, double time_dual, Patch* patch)
{
}
void ElectroMagnBC2D_SM::apply_zmax(ElectroMagn* EMfields, double time_dual, Patch* patch)
{
}
