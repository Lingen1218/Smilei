#ifndef SOLVERFACTORY_H
#define SOLVERFACTORY_H

#include "MF_Solver1D_Yee.h"
#include "MF_Solver2D_Yee.h"
#include "MF_Solver3D_Yee.h"
#include "MF_Solver2D_Grassi.h"
#include "MF_Solver2D_GrassiSpL.h"
#include "MF_Solver2D_Cowan.h"
#include "MF_Solver2D_Lehe.h"

#include "Params.h"

#include "Tools.h"

class SolverFactory {
public:
    static Solver* create(Params& params) {
        Solver* solver = NULL;
        DEBUG(params.maxwell_sol);
        
        if ( params.geometry == "1d3v" ) {
            if (params.maxwell_sol == "Yee") {
                solver = new MF_Solver1D_Yee(params);
            }
        } else if ( params.geometry == "2d3v" ) {
            if (params.maxwell_sol == "Yee") {
                solver = new MF_Solver2D_Yee(params);
            } else if (params.maxwell_sol == "Grassi") {
                solver = new MF_Solver2D_Grassi(params);
            } else if (params.maxwell_sol == "GrassiSpL") {
                solver = new MF_Solver2D_GrassiSpL(params);
            } else if (params.maxwell_sol == "Cowan") {
                solver = new MF_Solver2D_Cowan(params);
            } else if(params.maxwell_sol == "Lehe" ){
                solver = new MF_Solver2D_Lehe(params);
            }
            
        } else if ( params.geometry == "3d3v" ) {
            if (params.maxwell_sol == "Yee") {
                solver = new MF_Solver3D_Yee(params);
            }
        }
        
        if (!solver) {
            ERROR( "Unknwon solver '" << params.maxwell_sol << "' for geometry '" << params.geometry <<"'" );
        }
        
        return solver;
    }
    
};

#endif
