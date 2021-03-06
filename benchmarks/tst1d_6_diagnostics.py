import math

L0 = 2.*math.pi # Wavelength in PIC units

Main(
	geometry = "1d3v",
	
	interpolation_order = 2,
	
	timestep = 0.005 * L0,
	sim_time  = 0.5 * L0,
	
	cell_length = [0.01 * L0],
	sim_length  = [1. * L0],
	
	number_of_patches = [ 4 ],
	
	bc_em_type_x  = ["periodic"],
	
	referenceAngularFrequency_SI = L0 * 3e8 /1.e-6,
	
	print_every = 10
)

# Ion species
Species(
	species_type = "ion1",
	initPosition_type = "random",
	initMomentum_type = "maxwell-juettner",
	n_part_per_cell = 2000,
	mass = 1836.0,
	charge = 1.0,
	nb_density = 10.,
	temperature = [0.00002],
	time_frozen = 0.0,
	bc_part_type_xmin = "none",
	bc_part_type_xmax = "none"
)

# Electron species
Species(
	species_type = "electron1",
	initPosition_type = "random",
	initMomentum_type = "maxwell-juettner",
	n_part_per_cell= 2000,
	mass = 1.0,
	charge = -1.0,
	nb_density = 10.,
	mean_velocity = [0.05, 0., 0.],
	temperature = [0.00002],
	time_frozen = 0.0,
	bc_part_type_xmin = "none",
	bc_part_type_xmax = "none"
)




DiagScalar(
	every = 1,
	vars = ['Utot','Ubal','Ukin']
)

DiagFields(
	every = 5,
	fields = ['Ex','Ey','Ez','Rho_electron1','Rho_ion1']
)


DiagParticles(
	output = "density",
	every = 4,
	time_average = 2,
	species = ["electron1"],
	axes = [
		["x", 0.*L0, 1.*L0, 100],
		["vx", -0.1, 0.1, 100]
	]
)

DiagParticles(
	output = "density",
	every = 4,
	time_average = 1,
	species = ["ion1"],
	axes = [
		("x", 0.*L0, 1.*L0, 100),
		("vx", -0.001, 0.001, 100)
	]
)

DiagParticles(
	output = "px_density",
	every = 4,
	time_average = 2,
	species = ["electron1"],
	axes = [
		["x", 0.*L0, 1.*L0, 100],
		["vx", -0.1, 0.1, 100]
	]
)

DiagParticles(
	output = "density",
	every = 1,
	time_average = 1,
	species = ["electron1"],
	axes = [
		["ekin", 0.0001, 0.1, 100, "logscale", "edge_inclusive"]
	]
)
