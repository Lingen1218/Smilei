#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include "Params.h"
#include "Patch.h"
#include "SmileiMPI.h"


class Diagnostic {

public :

    Diagnostic() {};
    virtual ~Diagnostic() {};
    
    //! Opens the file. Only by MPI master for global diags. Only by patch master for local diags.
    virtual void openFile( Params& params, SmileiMPI* smpi, bool newfile ) = 0;
    //! Closes the file. Only by MPI master for global diags. Only by patch master for local diags.
    virtual void closeFile() = 0;
    
    //! Misc init.
    virtual void init(Params& params, SmileiMPI* smpi, VectorPatch& vecPatches) {};
    
    //! Prepares the diag and check whether it is time to run. Only by MPI master for global diags. Only by patch master for local diags.
    virtual bool prepare( int timestep ) = 0;
    
    //! Runs the diag for a given patch for global diags.
    virtual void run( Patch* patch, int timestep ) {};
    
    //! Runs the diag for all patches for local diags.
    virtual void run( SmileiMPI* smpi, VectorPatch& vecPatches, int timestep ) {};
    
    //! Writes out a global diag diag.
    virtual void write(int timestep, SmileiMPI* smpi) {};
    
    //! Tells whether this diagnostic requires the pre-calculation of the particle J & Rho
    virtual bool needsRhoJs(int timestep) { return false; };

    //! Time selection for writing the diagnostic
    TimeSelection * timeSelection;
    
    //! Time selection for flushing the file
    TimeSelection * flush_timeSelection;
    
    //! Get memory footprint of current diagnostic
    virtual int getMemFootPrint() = 0;

    //! this is the file name
    std::string filename;
    
    bool theTimeIsNow;

protected :
    
    //! Id of the file for one diagnostic
    hid_t fileId_;
};

#endif

