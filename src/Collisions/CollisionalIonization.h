
#ifndef COLLISIONALIONIZATION_H
#define COLLISIONALIONIZATION_H

#include <vector>

#include "Tools.h"
#include "Species.h"

class CollisionalIonization
{

public:
    //! Constructor
    CollisionalIonization(int, double);
    virtual ~CollisionalIonization() {};
    
    //! Gets the k-th binding energy of any neutral or ionized atom with atomic number Z and charge Zstar
    double binding_energy(int Zstar, int k);
    
    //! Coefficients used for interpolating the energy over a given initial list
    static const double a1, a2;
    static const int npoints;
    
    //! Methods to prepare the ionization
    inline void prepare1(int Z_firstgroup) {
        electronFirst = Z_firstgroup==0 ? true : false;
        ne = 0.; ni = 0.; nei = 0.;
        };
    virtual void prepare2(Particles *p1, int i1, Particles *p2, int i2);
    virtual void prepare3(double timestep, int n_cell_per_cluster);
    //! Method to apply the ionization
    virtual void apply(double vrel, double gamma1_COM, double gamma2_COM,
        Particles *p1, int i1, Particles *p2, int i2);
    
    //! Table of integrated cross-section
    std::vector<std::vector<double> > crossSection;
    //! Table of average secondary electron energy
    std::vector<std::vector<double> > transferredEnergy;
    //! Table of average incident electron energy lost
    std::vector<std::vector<double> > lostEnergy;
    
    
private:
    
    //! Atomic number
    int atomic_number;
    
    //! Table of first ionization energies of ions
    static const std::vector<std::vector<double> > ionizationEnergy;
    //! Table of binding energies of all electrons in neutral atoms
    static const std::vector<std::vector<double> > bindingEnergy;
    
    //! True if first group of species is the electron
    bool electronFirst;
    //! Ionizing electron density
    double ne;
    //! Ionizing "hybrid" density
    double nei;
    //! Ion density
    double ni;
    //! Coefficient for the ionization frequency
    double coeff;
    
    //! Method to interpolate the tables at a given energy
    void interpolateTables();
    
    //! Quantities used during computation
    double cs, w, e; // cross section, transferred energy, lost energy
    double x;  // electron energy index in tables
    int Zstar; // ion charge
};

//! Class to make empty ionization objects
class CollisionalNoIonization : public CollisionalIonization
{
public:
    CollisionalNoIonization() : CollisionalIonization(0,1.) {};
    ~CollisionalNoIonization(){};
    
    void prepare2(Particles *p1, int i1, Particles *p2, int i2){};
    void prepare3(double timestep, int n_cell_per_cluster){};
    void apply(double vrel, double gamma1_COM, double gamma2_COM,
        Particles *p1, int i1, Particles *p2, int i2){};
};

#endif
