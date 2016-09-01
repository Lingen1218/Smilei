
#include "SyncVectorPatch.h"

#include <vector>

#include "VectorPatch.h"
#include "Params.h"
#include "SmileiMPI.h"

using namespace std;

void SyncVectorPatch::exchangeParticles(VectorPatch& vecPatches, int ispec, Params &params, SmileiMPI* smpi)
{
    #pragma omp for schedule(runtime)
    for (unsigned int ipatch=0 ; ipatch<vecPatches.size() ; ipatch++) {
        vecPatches(ipatch)->initExchParticles(smpi, ispec, params);
    }

    //cout << "init exch done" << endl;

    // Per direction
    for (unsigned int iDim=0 ; iDim<params.nDim_particle ; iDim++) {
        //cout << "initExchParticles done for " << iDim << endl;
        #pragma omp for schedule(runtime)
        for (unsigned int ipatch=0 ; ipatch<vecPatches.size() ; ipatch++) {
            vecPatches(ipatch)->initCommParticles(smpi, ispec, params, iDim, &vecPatches);
        }
        #pragma omp for schedule(runtime)
        for (unsigned int ipatch=0 ; ipatch<vecPatches.size() ; ipatch++) {
            vecPatches(ipatch)->CommParticles(smpi, ispec, params, iDim, &vecPatches);
        }
        //cout << "init comm done for dim " << iDim << endl;
        //cout << "initCommParticles done for " << iDim << endl;
        #pragma omp for schedule(runtime)
        for (unsigned int ipatch=0 ; ipatch<vecPatches.size() ; ipatch++) {
            vecPatches(ipatch)->finalizeCommParticles(smpi, ispec, params, iDim, &vecPatches);
        }
        //cout << "final comm done for dim " << iDim << endl;
    }

    #pragma omp for schedule(runtime)
    for (unsigned int ipatch=0 ; ipatch<vecPatches.size() ; ipatch++)
        vecPatches(ipatch)->vecSpecies[ispec]->sort_part();

}

void SyncVectorPatch::sumRhoJ(VectorPatch& vecPatches, unsigned int diag_flag )
{

    SyncVectorPatch::sum( vecPatches.listJx_ , vecPatches );
    SyncVectorPatch::sum( vecPatches.listJy_ , vecPatches );
    SyncVectorPatch::sum( vecPatches.listJz_ , vecPatches );
    if(diag_flag) SyncVectorPatch::sum( vecPatches.listrho_, vecPatches );
}

void SyncVectorPatch::sumRhoJs(VectorPatch& vecPatches, int ispec )
{

    SyncVectorPatch::sum( vecPatches.listJxs_,  vecPatches );
    SyncVectorPatch::sum( vecPatches.listJys_,  vecPatches );
    SyncVectorPatch::sum( vecPatches.listJzs_,  vecPatches );
    SyncVectorPatch::sum( vecPatches.listrhos_, vecPatches );
}

void SyncVectorPatch::exchangeE( VectorPatch& vecPatches )
{

    SyncVectorPatch::exchange( vecPatches.listEx_, vecPatches );
    SyncVectorPatch::exchange( vecPatches.listEy_, vecPatches );
    SyncVectorPatch::exchange( vecPatches.listEz_, vecPatches );
}

void SyncVectorPatch::exchangeB( VectorPatch& vecPatches )
{

    if (vecPatches.listBx_[0]->dims_.size()==1) {
        SyncVectorPatch::exchange0( vecPatches.listBy_, vecPatches );
        SyncVectorPatch::exchange0( vecPatches.listBz_, vecPatches );
    }
    else if ( vecPatches.listBx_[0]->dims_.size()==2 ) {
        SyncVectorPatch::exchange1( vecPatches.listBx_, vecPatches );
        SyncVectorPatch::exchange0( vecPatches.listBy_, vecPatches );
        SyncVectorPatch::exchange ( vecPatches.listBz_, vecPatches );
    }
    else if ( vecPatches.listBx_[0]->dims_.size()==3 ) {
        SyncVectorPatch::exchange1( vecPatches.listBx_, vecPatches );
        SyncVectorPatch::exchange2( vecPatches.listBx_, vecPatches );
        SyncVectorPatch::exchange0( vecPatches.listBy_, vecPatches );
        SyncVectorPatch::exchange2( vecPatches.listBy_, vecPatches );
        SyncVectorPatch::exchange0( vecPatches.listBz_, vecPatches );
        SyncVectorPatch::exchange1( vecPatches.listBz_, vecPatches );
    }

}


void SyncVectorPatch::sum( std::vector<Field*> fields, VectorPatch& vecPatches )
{
    unsigned int nx_, ny_, nz_, h0, oversize[3], n_space[3], gsp[3];
    double *pt1,*pt2;
    h0 = vecPatches(0)->hindex;

    oversize[0] = vecPatches(0)->EMfields->oversize[0];
    oversize[1] = vecPatches(0)->EMfields->oversize[1];
    oversize[2] = vecPatches(0)->EMfields->oversize[2];
    
    n_space[0] = vecPatches(0)->EMfields->n_space[0];
    n_space[1] = vecPatches(0)->EMfields->n_space[1];
    n_space[2] = vecPatches(0)->EMfields->n_space[2];
    
    nx_ = fields[0]->dims_[0];
    ny_ = 1;
    nz_ = 1;
    if (fields[0]->dims_.size()>1) {
        ny_ = fields[0]->dims_[1];
        if (fields[0]->dims_.size()>2) 
            nz_ = fields[0]->dims_[2];
    }
    
    gsp[0] = 1+2*oversize[0]+fields[0]->isDual_[0]; //Ghost size primal

    // iDim = 0 sync
    #pragma omp for schedule(runtime) private(pt1,pt2)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++) {

        if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[0][0]){
            //The patch on my left belongs to the same MPI process than I.
            pt1 = &(*fields[vecPatches(ipatch)->neighbor_[0][0]-h0])(n_space[0]*ny_*nz_);
            pt2 = &(*fields[ipatch])(0);
            for (unsigned int i = 0; i < gsp[0]* ny_*nz_ ; i++) pt1[i] += pt2[i];
            memcpy( pt2, pt1, gsp[0]*ny_*nz_*sizeof(double)); 
                    
        }

    }

    // Sum per direction :
    //   - iDim = 0, complete non local sync through MPI
    for (int iDim=0;iDim<1;iDim++) {
        #pragma omp for schedule(runtime)
        for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++) {
            vecPatches(ipatch)->initSumField( fields[ipatch], iDim ); // initialize
        }
    
        #pragma omp for schedule(runtime)
        for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++) {
            vecPatches(ipatch)->finalizeSumField( fields[ipatch], iDim ); // finalize (waitall + sum)
        }
    }

    // iDim = 1 sync
    if (fields[0]->dims_.size()>1) {
        gsp[1] = 1+2*oversize[1]+fields[0]->isDual_[1]; //Ghost size primal
        #pragma omp for schedule(runtime) private(pt1,pt2)
        for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++) {

            if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[1][0]){
                //The patch below me belongs to the same MPI process than I.
                pt1 = &(*fields[vecPatches(ipatch)->neighbor_[1][0]-h0])(n_space[1]*nz_);
                pt2 = &(*fields[ipatch])(0);
                for (unsigned int j = 0; j < nx_ ; j++){
                    for (unsigned int i = 0; i < gsp[1]*nz_ ; i++) pt1[i] += pt2[i];
                    memcpy( pt2, pt1, gsp[1]*nz_*sizeof(double)); 
                    pt1 += ny_*nz_;
                    pt2 += ny_*nz_;
                }
            }
        }

        // Sum per direction :
        //   - iDim = 1, complete non local sync through MPI
        for (int iDim=1;iDim<2;iDim++) {
            #pragma omp for schedule(runtime)
            for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++) {
                vecPatches(ipatch)->initSumField( fields[ipatch], iDim ); // initialize
            }

            #pragma omp for schedule(runtime)
            for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++) {
                vecPatches(ipatch)->finalizeSumField( fields[ipatch], iDim ); // finalize (waitall + sum)
            }
        }

        // iDim = 2 sync
        if (fields[0]->dims_.size()>2) {
#ifdef _PATCH3D_TODO
#endif
            // Sum per direction :
            //   - iDim = 2, complete non local sync through MPI
            for (int iDim=2;iDim<3;iDim++) {
                #pragma omp for schedule(runtime)
                for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++) {
                    vecPatches(ipatch)->initSumField( fields[ipatch], iDim ); // initialize
                }

                #pragma omp for schedule(runtime)
                for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++) {
                    vecPatches(ipatch)->finalizeSumField( fields[ipatch], iDim ); // finalize (waitall + sum)
                }
            }

        } // End if dims_.size()>2

    } // End if dims_.size()>1

}


void SyncVectorPatch::exchange( std::vector<Field*> fields, VectorPatch& vecPatches )
{
    unsigned int nx_, ny_, nz_, h0, oversize[3], n_space[3], gsp[3];
    double *pt1,*pt2;
    h0 = vecPatches(0)->hindex;

    oversize[0] = vecPatches(0)->EMfields->oversize[0];
    oversize[1] = vecPatches(0)->EMfields->oversize[1];
    oversize[2] = vecPatches(0)->EMfields->oversize[2];

    n_space[0] = vecPatches(0)->EMfields->n_space[0];
    n_space[1] = vecPatches(0)->EMfields->n_space[1];
    n_space[2] = vecPatches(0)->EMfields->n_space[2];

    nx_ = fields[0]->dims_[0];
    ny_ = 1;
    if (fields[0]->dims_.size()>1) {
        ny_ = fields[0]->dims_[1];
        if (fields[0]->dims_.size()>2) 
            nz_ = fields[0]->dims_[2];
    }


    //For minimum comm
    //gsp[0] = 2*oversize[0]+fields[0]->isDual_[0]; //Ghost size primal
    //gsp[1] = 2*oversize[1]+fields[0]->isDual_[1]; //Ghost size primal
    //for filter
    gsp[0] = ( oversize[0] + 1 + fields[0]->isDual_[0] ); //Ghost size primal

    #pragma omp for schedule(runtime) 
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++) {

        if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[0][0]){
            pt1 = &(*fields[vecPatches(ipatch)->neighbor_[0][0]-h0])((n_space[0])*ny_*nz_);
            pt2 = &(*fields[ipatch])(0);
            //For minimum comm
            //memcpy( pt2, pt1, ny_*sizeof(double)); 
            //memcpy( pt1+gsp[0]*ny_, pt2+gsp[0]*ny_, ny_*sizeof(double)); 
            //for filter
            memcpy( pt2, pt1, oversize[0]*ny_*nz_*sizeof(double)); 
            memcpy( pt1+gsp[0]*ny_*nz_, pt2+gsp[0]*ny_*nz_, oversize[0]*ny_*nz_*sizeof(double)); 
        } // End if ( MPI_me_ == MPI_neighbor_[0][0] ) 

        if (fields[0]->dims_.size()>1) {
            gsp[1] = ( oversize[1] + 1 + fields[0]->isDual_[1] ); //Ghost size primal
            if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[1][0]){
                pt1 = &(*fields[vecPatches(ipatch)->neighbor_[1][0]-h0])(n_space[1]*nz_);
                pt2 = &(*fields[ipatch])(0);
                for (unsigned int i = 0 ; i < nx_*ny_*nz_ ; i += ny_*nz_){
                    //For minimum comm
                    //pt2[i] = pt1[i] ;
                    //pt1[i+gsp[1]] = pt2[i+gsp[1]] ;
                    // for filter
                    for (unsigned int j = 0 ; j < oversize[1]*nz_ ; j++ ){
                        pt2[i+j] = pt1[i+j] ;
                        pt1[i+j+gsp[1]] = pt2[i+j+gsp[1]] ;
                    } // mempy to do
                } 
            } // End if ( MPI_me_ == MPI_neighbor_[1][0] ) 

            if (fields[0]->dims_.size()>2) {
#ifdef _PATCH3D_TODO
#endif                
            }// End if dims_.size()>2

        } // End if dims_.size()>1

    } // End for( ipatch )

    for ( int iDim=0 ; iDim<fields[0]->dims_.size() ; iDim++ ) {
        #pragma omp for schedule(runtime)
        for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++)
            vecPatches(ipatch)->initExchange( fields[ipatch], iDim );

        #pragma omp for schedule(runtime)
        for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++)
            vecPatches(ipatch)->finalizeExchange( fields[ipatch], iDim );
    } // End for iDim

}

void SyncVectorPatch::exchange0( std::vector<Field*> fields, VectorPatch& vecPatches )
{
    unsigned int nx_, ny_, nz_, h0, oversize[3], n_space[2], gsp;
    double *pt1,*pt2;
    h0 = vecPatches(0)->hindex;

    oversize[0] = vecPatches(0)->EMfields->oversize[0];
    oversize[1] = vecPatches(0)->EMfields->oversize[1];
    oversize[2] = vecPatches(0)->EMfields->oversize[2];

    n_space[0] = vecPatches(0)->EMfields->n_space[0];
    n_space[1] = vecPatches(0)->EMfields->n_space[1];
    n_space[2] = vecPatches(0)->EMfields->n_space[2];

    ny_ = 1;
    nz_ = 1;
    if (fields[0]->dims_.size()>1) {
        ny_ = fields[0]->dims_[1];
        if (fields[0]->dims_.size()>2) 
            nz_ = fields[0]->dims_[2];
    }

    //gsp[0] = 2*oversize[0]+fields[0]->isDual_[0]; //Ghost size primal
    //for filter
    gsp = ( oversize[0] + 1 + fields[0]->isDual_[0] ); //Ghost size primal

    #pragma omp for schedule(runtime) private(pt1,pt2)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++) {

        if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[0][0]){
            pt1 = &(*fields[vecPatches(ipatch)->neighbor_[0][0]-h0])((n_space[0])*ny_*nz_);
            pt2 = &(*fields[ipatch])(0);
            //memcpy( pt2, pt1, ny_*sizeof(double)); 
            //memcpy( pt1+gsp[0]*ny_, pt2+gsp[0]*ny_, ny_*sizeof(double)); 
            //for filter
            memcpy( pt2, pt1, oversize[0]*ny_*nz_*sizeof(double)); 
            memcpy( pt1+gsp*ny_*nz_, pt2+gsp*ny_*nz_, oversize[0]*ny_*nz_*sizeof(double)); 
        } // End if ( MPI_me_ == MPI_neighbor_[0][0] ) 


    } // End for( ipatch )

    #pragma omp for schedule(runtime)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++)
        vecPatches(ipatch)->initExchange( fields[ipatch], 0 );

    #pragma omp for schedule(runtime)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++)
        vecPatches(ipatch)->finalizeExchange( fields[ipatch], 0 );


}

void SyncVectorPatch::exchange1( std::vector<Field*> fields, VectorPatch& vecPatches )
{
    unsigned int nx_, ny_, nz_, h0, oversize[3], n_space[3], gsp;
    double *pt1,*pt2;
    h0 = vecPatches(0)->hindex;

    oversize[0] = vecPatches(0)->EMfields->oversize[0];
    oversize[1] = vecPatches(0)->EMfields->oversize[1];
    oversize[2] = vecPatches(0)->EMfields->oversize[2];

    n_space[0] = vecPatches(0)->EMfields->n_space[0];
    n_space[1] = vecPatches(0)->EMfields->n_space[1];
    n_space[2] = vecPatches(0)->EMfields->n_space[2];

    nx_ = fields[0]->dims_[0];
    ny_ = fields[0]->dims_[1];
    nz_ = 1;
    if (fields[0]->dims_.size()>2) 
        nz_ = fields[0]->dims_[2];

    //gsp = 2*oversize[1]+fields[0]->isDual_[1]; //Ghost size primal
    //for filter
    gsp = ( oversize[1] + 1 + fields[0]->isDual_[1] ); //Ghost size primal

    #pragma omp for schedule(runtime) private(pt1,pt2)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++) {

        if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[1][0]){
            pt1 = &(*fields[vecPatches(ipatch)->neighbor_[1][0]-h0])(n_space[1]*nz_);
            pt2 = &(*fields[ipatch])(0);
            for (unsigned int i = 0 ; i < nx_*ny_*nz_ ; i += ny_*nz_){
                // for filter
                for (unsigned int j = 0 ; j < oversize[1]*nz_ ; j++ ){
                    pt2[i+j] = pt1[i+j] ;
                    pt1[i+j+gsp] = pt2[i+j+gsp] ;
                } // mempy to do
            } 
        } // End if ( MPI_me_ == MPI_neighbor_[1][0] ) 

    } // End for( ipatch )

    #pragma omp for schedule(runtime)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++)
        vecPatches(ipatch)->initExchange( fields[ipatch], 1 );

    #pragma omp for schedule(runtime)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++)
        vecPatches(ipatch)->finalizeExchange( fields[ipatch], 1 );


}

void SyncVectorPatch::exchange2( std::vector<Field*> fields, VectorPatch& vecPatches )
{
    unsigned int nx_, ny_, nz_, h0, oversize[3], n_space[3], gsp;
    double *pt1,*pt2;
    h0 = vecPatches(0)->hindex;

    oversize[0] = vecPatches(0)->EMfields->oversize[0];
    oversize[1] = vecPatches(0)->EMfields->oversize[1];
    oversize[2] = vecPatches(0)->EMfields->oversize[2];

    n_space[0] = vecPatches(0)->EMfields->n_space[0];
    n_space[1] = vecPatches(0)->EMfields->n_space[1];
    n_space[2] = vecPatches(0)->EMfields->n_space[2];

    nx_ = fields[0]->dims_[0];
    ny_ = fields[0]->dims_[1];
    nz_ = fields[0]->dims_[2];

    //gsp = 2*oversize[1]+fields[0]->isDual_[1]; //Ghost size primal
    //for filter
    gsp = ( oversize[2] + 1 + fields[0]->isDual_[2] ); //Ghost size primal

    #pragma omp for schedule(runtime) private(pt1,pt2)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++) {

        if (vecPatches(ipatch)->MPI_me_ == vecPatches(ipatch)->MPI_neighbor_[1][0]){
#ifdef _PATCH3D_TODO
#endif                
        } // End if ( MPI_me_ == MPI_neighbor_[1][0] ) 

    } // End for( ipatch )

    #pragma omp for schedule(runtime)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++)
        vecPatches(ipatch)->initExchange( fields[ipatch], 2 );

    #pragma omp for schedule(runtime)
    for (unsigned int ipatch=0 ; ipatch<fields.size() ; ipatch++)
        vecPatches(ipatch)->finalizeExchange( fields[ipatch], 2 );

}
