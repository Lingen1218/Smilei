#include "ElectroMagn.h"

#include <limits>
#include <iostream>

#include "Params.h"
#include "Species.h"
#include "Projector.h"
#include "Field.h"
#include "ElectroMagnBC.h"
#include "ElectroMagnBC_Factory.h"
#include "SimWindow.h"
#include "Patch.h"
#include "Profile.h"
#include "SolverFactory.h"

using namespace std;


// ---------------------------------------------------------------------------------------------------------------------
// Constructor for the virtual class ElectroMagn
// ---------------------------------------------------------------------------------------------------------------------
ElectroMagn::ElectroMagn(Params &params, vector<Species*>& vecSpecies, Patch* patch) :
timestep       ( params.timestep   ),
cell_length    ( params.cell_length),
n_species      ( vecSpecies.size() ),
nDim_field     ( params.nDim_field ),
cell_volume    ( params.cell_volume),
n_space        ( params.n_space    ),
oversize       ( params.oversize   ),
nrj_mw_lost    (  0.               ),
nrj_new_fields (  0.               )
{
    
    // initialize poynting vector
    poynting[0].resize(nDim_field,0.0);
    poynting[1].resize(nDim_field,0.0);
    poynting_inst[0].resize(nDim_field,0.0);
    poynting_inst[1].resize(nDim_field,0.0);
    
    // take useful things from params
    for (unsigned int i=0; i<3; i++) {
        DEBUG("____________________ OVERSIZE: " <<i << " " << oversize[i]);
    }
    
    if (n_space.size() != 3) ERROR("this should not happen");
    
    Ex_=NULL;
    Ey_=NULL;
    Ez_=NULL;
    Bx_=NULL;
    By_=NULL;
    Bz_=NULL;
    Bx_m=NULL;
    By_m=NULL;
    Bz_m=NULL;
    Jx_=NULL;
    Jy_=NULL;
    Jz_=NULL;
    rho_=NULL;
    
    // Species charge currents and density
    Jx_s.resize(n_species);
    Jy_s.resize(n_species);
    Jz_s.resize(n_species);
    rho_s.resize(n_species);
    for (unsigned int ispec=0; ispec<n_species; ispec++) {
        Jx_s[ispec]  = NULL;
        Jy_s[ispec]  = NULL;
        Jz_s[ispec]  = NULL;
        rho_s[ispec] = NULL;
    }
    
    for (unsigned int i=0; i<3; i++) {
        for (unsigned int j=0; j<2; j++) {
            istart[i][j]=0;
            bufsize[i][j]=0;
        }
    }
    
    
    emBoundCond = ElectroMagnBC_Factory::create(params, patch);
    
    MaxwellFaradaySolver_ = SolverFactory::create(params);
    
}


ElectroMagn::ElectroMagn( ElectroMagn* emFields, Params &params, Patch* patch ) :
timestep       ( emFields->timestep    ),
cell_length    ( emFields->cell_length ),
n_species      ( emFields->n_species   ),
nDim_field     ( emFields->nDim_field  ),
cell_volume    ( emFields->cell_volume ),
n_space        ( emFields->n_space     ),
oversize       ( emFields->oversize    ),
nrj_mw_lost    ( 0. ),
nrj_new_fields ( 0. )
{
    // initialize poynting vector
    poynting[0].resize(nDim_field,0.0);
    poynting[1].resize(nDim_field,0.0);
    poynting_inst[0].resize(nDim_field,0.0);
    poynting_inst[1].resize(nDim_field,0.0);
    
    if (n_space.size() != 3) ERROR("this should not happen");
    
    Ex_=NULL;
    Ey_=NULL;
    Ez_=NULL;
    Bx_=NULL;
    By_=NULL;
    Bz_=NULL;
    Bx_m=NULL;
    By_m=NULL;
    Bz_m=NULL;
    Jx_=NULL;
    Jy_=NULL;
    Jz_=NULL;
    rho_=NULL;
    
    // Species charge currents and density
    Jx_s.resize(n_species);
    Jy_s.resize(n_species);
    Jz_s.resize(n_species);
    rho_s.resize(n_species);
    for (unsigned int ispec=0; ispec<n_species; ispec++) {
        Jx_s[ispec]  = NULL;
        Jy_s[ispec]  = NULL;
        Jz_s[ispec]  = NULL;
        rho_s[ispec] = NULL;
    }
    
    for (unsigned int i=0; i<3; i++) {
        for (unsigned int j=0; j<2; j++) {
            istart[i][j]=0;
            bufsize[i][j]=0;
        }
    }
    
    
    emBoundCond = ElectroMagnBC_Factory::create(params, patch);
    
    MaxwellFaradaySolver_ = SolverFactory::create(params);
}


void ElectroMagn::finishInitialization(int nspecies, Patch* patch)
{

    // Fill allfields
    allFields.push_back(Ex_ );
    allFields.push_back(Ey_ );
    allFields.push_back(Ez_ );
    allFields.push_back(Bx_ );
    allFields.push_back(By_ );
    allFields.push_back(Bz_ );
    allFields.push_back(Bx_m);
    allFields.push_back(By_m);
    allFields.push_back(Bz_m);
    allFields.push_back(Jx_ );
    allFields.push_back(Jy_ );
    allFields.push_back(Jz_ );
    allFields.push_back(rho_);

    for (int ispec=0; ispec<nspecies; ispec++) {
        allFields.push_back(Jx_s[ispec] );
        allFields.push_back(Jy_s[ispec] );
        allFields.push_back(Jz_s[ispec] );
        allFields.push_back(rho_s[ispec]);
    }
    
}

// ---------------------------------------------------------------------------------------------------------------------
// Destructor for the virtual class ElectroMagn
// ---------------------------------------------------------------------------------------------------------------------
ElectroMagn::~ElectroMagn()
{
    delete Ex_;
    delete Ey_;
    delete Ez_;
    delete Bx_;
    delete By_;
    delete Bz_;
    delete Bx_m;
    delete By_m;
    delete Bz_m;
    delete Jx_;
    delete Jy_;
    delete Jz_;
    delete rho_;
    
    for( unsigned int idiag=0; idiag<allFields_avg.size(); idiag++ )
        for( unsigned int ifield=0; ifield<allFields_avg[idiag].size(); ifield++ )
            delete allFields_avg[idiag][ifield];
    
    for (unsigned int ispec=0; ispec<n_species; ispec++) {
        if( Jx_s [ispec] ) delete Jx_s [ispec];
        if( Jy_s [ispec] ) delete Jy_s [ispec];
        if( Jz_s [ispec] ) delete Jz_s [ispec];
        if( rho_s[ispec] ) delete rho_s[ispec];
    }
    
    int nBC = emBoundCond.size();
    for ( int i=0 ; i<nBC ;i++ )
        if (emBoundCond[i]!=NULL) delete emBoundCond[i];
    
    delete MaxwellFaradaySolver_;
    
    //antenna cleanup
    for (vector<Antenna>::iterator antenna=antennas.begin(); antenna!=antennas.end(); antenna++ ) {
        delete antenna->field;
        antenna->field=NULL;
    }
    
    /*for ( unsigned int iExt = 0 ; iExt < extFields.size() ; iExt++ ) {
        if (extFields[iExt].profile!=NULL) {
            delete extFields[iExt].profile;
            extFields[iExt].profile = NULL;
        } // Pb wih clones
    }*/

}//END Destructer


// ---------------------------------------------------------------------------------------------------------------------
// Maxwell solver using the FDTD scheme
// ---------------------------------------------------------------------------------------------------------------------
// In the main program 
//     - saveMagneticFields
//     - solveMaxwellAmpere
//     - solveMaxwellFaraday
//     - boundaryConditions
//     - vecPatches::exchangeB (patch & MPI sync)
//     - centerMagneticFields



void ElectroMagn::boundaryConditions(int itime, double time_dual, Patch* patch, Params &params, SimWindow* simWindow)
{
    // Compute EM Bcs
    if ( (!simWindow) || (!simWindow->isMoving(time_dual)) ) {
        if (emBoundCond[0]!=NULL) { // <=> if !periodic
            emBoundCond[0]->apply_xmin(this, time_dual, patch);
            emBoundCond[1]->apply_xmax(this, time_dual, patch);
        }
    }
    if (emBoundCond.size()>2) {
        if (emBoundCond[2]!=NULL) {// <=> if !periodic
            emBoundCond[2]->apply_ymin(this, time_dual, patch);
            emBoundCond[3]->apply_ymax(this, time_dual, patch);
        }
    }
    if (emBoundCond.size()>4) {
        if (emBoundCond[4]!=NULL) {// <=> if !periodic
            emBoundCond[4]->apply_zmin(this, time_dual, patch);
            emBoundCond[5]->apply_zmax(this, time_dual, patch);
        }
    }

}

// ---------------------------------------------------------------------------------------------------------------------
// Method used to create a dump of the data contained in ElectroMagn
// ---------------------------------------------------------------------------------------------------------------------
void ElectroMagn::dump()
{
    //!\todo Check for none-cartesian grid & for generic grid (neither all dual or all primal) (MG & JD)
    
    vector<unsigned int> dimPrim;
    dimPrim.resize(1);
    dimPrim[0] = n_space[0]+2*oversize[0]+1;
    vector<unsigned int> dimDual;
    dimDual.resize(1);
    dimDual[0] = n_space[0]+2*oversize[0]+2;
    
    // dump of the electromagnetic fields
    Ex_->dump(dimDual);
    Ey_->dump(dimPrim);
    Ez_->dump(dimPrim);
    Bx_->dump(dimPrim);
    By_->dump(dimDual);
    Bz_->dump(dimDual);
    // dump of the total charge density & currents
    rho_->dump(dimPrim);
    Jx_->dump(dimDual);
    Jy_->dump(dimPrim);
    Jz_->dump(dimPrim);
}


// ---------------------------------------------------------------------------------------------------------------------
// Reinitialize the total charge densities and currents
// - save current density as old density (charge conserving scheme)
// - put the new density and currents to 0
// ---------------------------------------------------------------------------------------------------------------------
void ElectroMagn::restartRhoJ()
{
    Jx_ ->put_to(0.);
    Jy_ ->put_to(0.);
    Jz_ ->put_to(0.);
    rho_->put_to(0.);
}

void ElectroMagn::restartRhoJs()
{
    for (unsigned int ispec=0 ; ispec < n_species ; ispec++) {
        if( Jx_s [ispec] ) Jx_s [ispec]->put_to(0.);
        if( Jy_s [ispec] ) Jy_s [ispec]->put_to(0.);
        if( Jz_s [ispec] ) Jz_s [ispec]->put_to(0.);
        if( rho_s[ispec] ) rho_s[ispec]->put_to(0.);
    }
    
    Jx_ ->put_to(0.);
    Jy_ ->put_to(0.);
    Jz_ ->put_to(0.);
    rho_->put_to(0.);
}

// ---------------------------------------------------------------------------------------------------------------------
// Increment an averaged field
// ---------------------------------------------------------------------------------------------------------------------
void ElectroMagn::incrementAvgField(Field * field, Field * field_avg)
{
    for( unsigned int i=0; i<field->globalDims_; i++ )
        (*field_avg)(i) += (*field)(i);
}//END incrementAvgField



void ElectroMagn::laserDisabled()
{
    if ( emBoundCond.size() )
        emBoundCond[0]->laserDisabled();
}

double ElectroMagn::computeNRJ() {
    double nrj(0.);

    nrj += Ex_->norm2(istart, bufsize);
    nrj += Ey_->norm2(istart, bufsize);
    nrj += Ez_->norm2(istart, bufsize);

    nrj += Bx_m->norm2(istart, bufsize);
    nrj += By_m->norm2(istart, bufsize);
    nrj += Bz_m->norm2(istart, bufsize);

    return nrj;
}

string LowerCase(string in){
    string out=in;
    std::transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}


void ElectroMagn::applyExternalFields(Patch* patch) {    
    Field * field;
    for (vector<ExtField>::iterator extfield=extFields.begin(); extfield!=extFields.end(); extfield++ ) {
        string name = LowerCase(extfield->field);
        if      ( Ex_ && name==LowerCase(Ex_->name) ) field = Ex_;
        else if ( Ey_ && name==LowerCase(Ey_->name) ) field = Ey_;
        else if ( Ez_ && name==LowerCase(Ez_->name) ) field = Ez_;
        else if ( Bx_ && name==LowerCase(Bx_->name) ) field = Bx_;
        else if ( By_ && name==LowerCase(By_->name) ) field = By_;
        else if ( Bz_ && name==LowerCase(Bz_->name) ) field = Bz_;
        else field = NULL;
        
        if( field ) {
            applyExternalField( field, extfield->profile, patch );
        }
    }
    Bx_m->copyFrom(Bx_);
    By_m->copyFrom(By_);
    Bz_m->copyFrom(Bz_);
}


void ElectroMagn::applyAntenna(unsigned int iAntenna, double intensity) {
    Field *field=nullptr;
    Field *antennaField = antennas[iAntenna].field;
    if (antennaField) {
        
        if     ( antennaField->name == "Jx" ) field = Jx_;
        else if( antennaField->name == "Jy" ) field = Jy_;
        else if( antennaField->name == "Jz" ) field = Jz_;
        else ERROR("Antenna applied to field " << antennaField << " unknonw. This should not happend, please contact developers");
        
        for (unsigned int i=0; i< field->globalDims_ ; i++)
            (*field)(i) += intensity * (*antennaField)(i);
        
    }
}

