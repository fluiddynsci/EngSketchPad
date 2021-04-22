// Structures for general CFD analysis - Written by Dr. Ryan Durscher AFRL/RQVC

#ifndef CFDTYPES_H
#define CFDTYPES_H

typedef enum {UnknownBoundary, Inviscid, Viscous, Farfield, Extrapolate, Freestream,
              BackPressure, Symmetry, SubsonicInflow, SubsonicOutflow,
              MassflowIn, MassflowOut, FixedInflow, FixedOutflow, MachOutflow} cfdSurfaceTypeEnum;

// Structure to hold CFD surface information
typedef struct {
    char   *name;

    cfdSurfaceTypeEnum surfaceType; // "Global" boundary condition types

    int    bcID;                 // ID of boundary

    // Wall specific properties
    int    wallTemperatureFlag;  // Temperature flag
    double wallTemperature;      // Temperature value -1 = adiabatic ; >0 = isothermal
    double wallHeatFlux;	     // Wall heat flux. to use Temperature flag should be true and wallTemperature < 0

    // Symmetry plane
    int symmetryPlane;        // Symmetry flag / plane

    // Stagnation quantities
    double totalPressure;    // Total pressure
    double totalTemperature; // Total temperature
    double totalDensity;     // Total density

    // Static quantities
    double staticPressure;   // Static pressure
    double staticTemperature;// Static temperature
    double staticDensity;    // Static temperature

    // Velocity components
    double uVelocity;  // x-component of velocity
    double vVelocity;  // y-component of velocity
    double wVelocity;  // z-component of velocity
    double machNumber; // Mach number

    // Massflow
    double massflow; // Mass flow through a boundary

} cfdSurfaceStruct;


// Structure to hold CFD boundary condition information
typedef struct {
    char             *name;   // Name of BCsStruct
    cfdSurfaceStruct *surfaceProps;  // Surface properties for each bc - length of numBCID
    int              numBCID;       // Number of unique BC ids
} cfdBCsStruct;


// Structure to hold EigenValue information as used in CFD solvers
typedef struct {

	char *name;

	int modeNumber;

	double frequency;
	double damping;

	double generalMass;
	double generalDisplacement;
	double generalVelocity;
	double generalForce;

} eigenValueStruct;

// Structure to hold a collection of EigenValue information as used in CFD solvers
typedef struct {
	int surfaceID;

	int numEigenValue;
	eigenValueStruct *eigenValue;

	double freestreamVelocity;
	double freestreamDynamicPressure;
	double lengthScaling;

} modalAeroelasticStruct;

#endif
