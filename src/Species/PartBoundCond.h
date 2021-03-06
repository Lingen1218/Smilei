#ifndef PARTBOUNDCOND_H
#define PARTBOUNDCOND_H

#include "Params.h"
#include "Species.h"
#include "Particles.h"
#include "tabulatedFunctions.h"

class Patch;

//  --------------------------------------------------------------------------------------------------------------------
//! Class PartBoundCond
//  --------------------------------------------------------------------------------------------------------------------
class PartBoundCond {
public:
    //! partBoundCond creator, (default no MPI set)
    PartBoundCond( Params& params, Species *species, Patch* patch );
    //! partBoundCond destructor
    ~PartBoundCond();

    //! Xmin particles boundary conditions pointers (same prototypes for all conditions)
    //! @see BoundaryConditionType.h for functions that this pointers will target
    int (*bc_xmin)  ( Particles &particles, int ipart, int direction, double limit_pos, Species *species, double &nrj_iPart);
    //! Xmax particles boundary conditions pointers
    int (*bc_xmax)  ( Particles &particles, int ipart, int direction, double limit_pos, Species *species, double &nrj_iPart);
    //! Ymin particles boundary conditions pointers
    int (*bc_ymin) ( Particles &particles, int ipart, int direction, double limit_pos, Species *species, double &nrj_iPart);
    //! Ymax particles boundary conditions pointers
    int (*bc_ymax) ( Particles &particles, int ipart, int direction, double limit_pos, Species *species, double &nrj_iPart);
    //! Zmin particles boundary conditions pointers
    int (*bc_zmin)( Particles &particles, int ipart, int direction, double limit_pos, Species *species, double &nrj_iPart);
    //! Zmax particles boundary conditions pointers
    int (*bc_zmax)    ( Particles &particles, int ipart, int direction, double limit_pos, Species *species, double &nrj_iPart);

    //! Method which applies particles boundary conditions.
    //! If the MPI process is not a border process, particles will be flagged as an exchange particle returning 0
    //! Conditions along X are applied first, then Y, then Z.
    //! The decision whether the particle is added or not on the Exchange Particle List is defined by the final
    //! value of keep_part. 
    //! Be careful, once an a BC along a given dimension set keep_part to 0, it will remain to 0. 
    inline int apply( Particles &particles, int ipart, Species *species, double &nrj_iPart ) {//, bool &contribute ) {
        
        int keep_part = 1;
        if ( particles.position(0, ipart) <  x_min ) {
            if (bc_xmin==NULL) keep_part = 0;
            else {
                keep_part = (*bc_xmin)( particles, ipart, 0, 2.*x_min, species,nrj_iPart );
            }
        }
        else if ( particles.position(0, ipart) >= x_max ) {
            if (bc_xmax==NULL) keep_part = 0;
            else {
                keep_part = (*bc_xmax)( particles, ipart, 0, 2.*x_max, species,nrj_iPart );
            }
        }
        if (nDim_particle >= 2) {
            
            if ( particles.position(1, ipart) <  y_min ) {
                if (bc_ymin==NULL) keep_part = 0;
                else {
                    keep_part *= (*bc_ymin)( particles, ipart, 1, 2.*y_min, species,nrj_iPart );
                }
            }
            else if ( particles.position(1, ipart) >= y_max ) {
                if (bc_ymax==NULL) keep_part = 0;
                else {
                    keep_part *= (*bc_ymax)( particles, ipart, 1, 2.*y_max, species,nrj_iPart );
                }
            }
            
            if (nDim_particle == 3) {
                
                if ( particles.position(2, ipart) <  z_min ) {
                    if (bc_zmin==NULL) keep_part = 0;
                    else {
                        keep_part *= (*bc_zmin)( particles, ipart, 2, 2.*z_min, species,nrj_iPart );
                    }
                }
                else if ( particles.position(2, ipart) >= z_max ) {
                    if (bc_zmax==NULL) keep_part = 0;
                    else {
                        keep_part *= (*bc_zmax)( particles, ipart, 2, 2.*z_max, species,nrj_iPart );
                    }
                }
            } // end if (nDim_particle == 3)
        } // end if (nDim_particle >= 2)


        return keep_part;
    };

    //! Set the condition window if restart (patch position not read)
    inline void updateMvWinLimits( double x_moved ) {
    }

private:
    //! Min value of the x coordinate of particles on the current processor
    //! Real value, oversize is not considered (same for all)
    double x_min;
    //! Max value of the x coordinate of particles on the current processor
    double x_max;
    //! Min value of the y coordinate of particles on the current processor
    double y_min;
    //! Max value of the y coordinate of particles on the current processor
    double y_max;
    //! Min value of the z coordinate of particles on the current processor
    double z_min;
    //! Max value of the z coordinate of particles on the current processor
    double z_max;

    //! Space dimension of a particle
    int nDim_particle;
    
};

#endif

