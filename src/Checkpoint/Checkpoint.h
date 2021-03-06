/*
 * Checkpoint.h
 *
 *  Created on: 3 juil. 2013
 */

#ifndef CHECKPOINT_H
#define CHECKPOINT_H

#include <string>
#include <vector>

#include <hdf5.h>
#include <Tools.h>

class Params;
class SmileiMPI;
class Patch;
class SimWindow;
class ElectroMagn;
class Field;
class Species;
class VectorPatch;

#include <csignal>

//  --------------------------------------------------------------------------------------------------------------------
//! Class Checkpoint
//  --------------------------------------------------------------------------------------------------------------------
class Checkpoint {
public:
    Checkpoint( Params& params, SmileiMPI* smpi );
    //! Destructor for Checkpoint
    virtual ~Checkpoint(){};
    
    //! Space dimension of a particle
    unsigned int nDim_particle;
    
    //! restart everything to file per processor
    void restartAll( VectorPatch &vecPatches,  SmileiMPI* smpi, SimWindow* simWin, Params &params);
    void restartPatch( ElectroMagn* EMfields,std::vector<Species*> &vecSpecies, Params& params, hid_t patch_gid );
    
    //! restart field per proc
    void restartFieldsPerProc(hid_t fid, Field* field);
    
    //! load moving window parameters
    void restartMovingWindow(hid_t fid, SimWindow* simWindow);
    
    //! test before writing everything to file per processor
    //bool dump(unsigned int itime, double time, Params &params);
    void dump( VectorPatch &vecPatches, unsigned int itime, SmileiMPI* smpi, SimWindow* simWindow, Params &params );
    // OK
    
    //! dump everything to file per processor
    void dumpAll( VectorPatch &vecPatches, unsigned int itime,  SmileiMPI* smpi, SimWindow* simWin, Params &params );
    void dumpPatch( ElectroMagn* EMfields, std::vector<Species*> vecSpecies, hid_t patch_gid );
    
    //! incremental number of times we've done a dump
    unsigned int dump_number;
    
    //! incremental number of times we've done a dump_minutes
    unsigned int dump_minutes_times;
    
    //! this static variable is defined (in the .cpp) as false but becomes true when
    //! the signal SIGUSR1 is captured by the signal_callback_handler fnction
    static int signal_received;
    
    //! this function catches the SIGUSR1 signal and sets the signal_received to true
    static void signal_callback_handler(int signum) {
        MESSAGE("----------------------------------------------");
        MESSAGE("Caught signal " << signum << " : dump + exit");
        MESSAGE("----------------------------------------------");
        if (signum!=SIGUSR2)
            signal_received = signum;
    }
    
    //! start step of this run: zero if a first run, otherwise the number of the restart step
    unsigned int this_run_start_step;
    
    //! checkpoint asks to politely quit from simulation
    bool exit_asap;
    
private:
    //! get dump name based on number and rank
    std::string dumpName(unsigned int num, SmileiMPI *smpi);
    
    //! initialize the time zero of the simulation 
    void initDumpCases();
    
    //! dump field per proc
    void dumpFieldsPerProc(hid_t fid, Field* field);
    
    //! dump moving window parameters
    void dumpMovingWindow(hid_t fid, SimWindow* simWindow);
    
    //! function that returns elapsed time from creator (uses private var time_reference)
    //double time_seconds();
    
    //! to dump and stop a simulation you might just check if a file named stop has been created this variable
    //! is true if since last time a file named stop appeared
    bool stop_file_seen_since_last_check;
    
    //! time of the constructor
    double time_reference;
    
    //! vector containing the step at which perform a dump in case time_dump returns true
    unsigned int time_dump_step;
    
    //! Timestep to dump everything
    unsigned int dump_step;
    
    //! Human minutes to dump everything
    double dump_minutes;
    
    //! exit once dump done
    bool exit_after_dump;
    
    //! keep the last dump_file_sequence dump files
    unsigned int dump_file_sequence;
    
    //! write dump drectory
    std::string dump_dir;
    
    //! int deflate dump value
    int dump_deflate;
    
    //! directory of the restart
    std::string restart_dir;
    
    std::vector<MPI_Request> dump_request;
    MPI_Status dump_status_prob;
    MPI_Status dump_status_recv;
    
    //! group checkpoint files in subdirs of file_grouping files
    unsigned int file_grouping;
    
    //! restart from checkpoint number
    int restart_number;
};

#endif /* CHECKPOINT_H_ */
