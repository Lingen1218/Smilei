from .._Utils import *

class Diagnostic(object):
	"""Mother class for all Diagnostics.
	To create a diagnostic, refer to the doc of the Smilei class.
	Once a Smilei object is created, you may get help on its diagnostics.
	
	Example:
		S = Smilei("path/to/simulation") # Load a simulation
		help( S )                        # General help on the simulation's diagnostics
		help( S.Field )                  # Help on loading a Field diagnostic
	"""
	
	def __init__(self, simulation, *args, **kwargs):
		self.valid = False
		self._tmpdata = None
		self._animateOnAxes = None
		self._shape = []
		self._centers = []
		self._type = []
		self._label = []
		self._units = []
		self._log = []
		self._data_log = False
		self._error = ""
		
		# The 'simulation' is a Smilei object. It is passed as an instance attribute
		self.Smilei = simulation
		
		# Transfer the simulation's packages to the diagnostic
		self._h5py = self.Smilei._h5py
		self._np   = self.Smilei._np
		self._os   = self.Smilei._os
		self._glob = self.Smilei._glob
		self._re   = self.Smilei._re
		self._plt  = self.Smilei._plt
		
		# Reload the simulation, in case it has been updated
		self.Smilei.reload()
		if not self.Smilei.valid:
			print("Invalid Smilei simulation")
			return
		
		# Copy some parameters from the simulation
		self._results_path = self.Smilei._results_path
		self.namelist      = self.Smilei.namelist
		self._ndim         = self.Smilei._ndim
		self._cell_length  = self.Smilei._cell_length
		self._ncels        = self.Smilei._ncels
		self.timestep      = self.Smilei._timestep
		
		# Make the Options object
		self.options = Options(**kwargs)
		
		# Make or retrieve the Units object
		self.units = kwargs.pop("units", [""])
		if type(self.units) in [list, tuple]: self.units = Units(*self.units)
		if type(self.units) is dict         : self.units = Units(**self.units)
		if type(self.units) is not Units:
			print("Could not understand the 'units' argument")
			return
		
		# Call the '_init' function of the child class
		self._init(*args, **kwargs)
		
		# Prepare units
		self.dim = len(self._shape)
		if self.valid:
			xunits = None
			yunits = None
			if self.dim > 0: xunits = self._units[0]
			if self.dim > 1: yunits = self._units[1]
			self.units.prepare(self.Smilei._referenceAngularFrequency_SI, xunits, yunits, self._vunits)
	
	# When no action is performed on the object, this is what appears
	def __repr__(self):
		self.info()
		return ""
	
	# Method to verify everything was ok during initialization
	def _validate(self):
		try:
			self.Smilei.valid
		except:
			print("No valid Smilei simulation selected")
			return False
		if not self.Smilei.valid or not self.valid:
			print("Diagnostic is invalid")
			return False
		return True
	
	# Method to set optional plotting arguments
	def set(self, **kwargs):
		self.options.set(**kwargs)
	
	# Method to obtain the plot limits
	def limits(self):
		"""Gets the overall limits of the diagnostic along its axes
		
		Returns:
		--------
		A list of [min, max] for each axis.
		"""
		l = []
		for i in range(self.dim):
			l.append([min(self._centers[i]), max(self._centers[i])])
		return l
	
	# Method to print info on this diag
	def info(self):
		if not self._validate():
			print(self._error)
		else:
			print(self._info())
	
	# Method to get only the arrays of data
	def getData(self):
		"""Obtains the data from the diagnostic.
		
		Returns:
		--------
		An array copy of the diagnostic data.
		"""
		if not self._validate(): return
		self._prepare1() # prepare the vfactor
		data = []
		for t in self.times:
			data.append( self._vfactor*self._getDataAtTime(t) )
		return data
	
	# Method to obtain the data and the axes
	def get(self):
		"""Obtains the data from the diagnostic and some additional information.
		
		Returns:
		--------
		A dictionnary with several values, being various information on the diagnostic.
		One of the values is an array copy of the diagnostic data.
		"""
		if not self._validate(): return
		# obtain the data arrays
		data = self.getData()
		# format the results into a dictionary
		result = {"data":data, "times":self.times}
		for i in range(len(self._type)):
			result.update({ self._type[i]:self._centers[i] })
		return result
	
	# Method to plot the current diagnostic
	def plot(self, movie="", fps=15, dpi=200, saveAs=None, **kwargs):
		""" Plots the diagnostic.
		If the data is 1D, it is plotted as a curve, and is animated for all requested timesteps.
		If the data is 2D, it is plotted as a map, and is animated for all requested timesteps.
		If the data is 0D, it is plotted as a curve as function of time.
		
		
		Parameters:
		-----------
		figure: int (default: 1)
			The figure number that is passed to matplotlib.
		vmin, vmax: floats (default to the current limits)
			Data value limits.
		xmin, xmax, ymin, ymax: floats (default to the current limits)
			Axes limits.
		xfactor, yfactor: floats (default: 1)
			Factors to rescale axes.
		streakPlot: bool (default: False)
			When True, will not be an animation, but will have time
			on the vertical axis instead (only for 1D data).
		movie: path string (default: "")
			Name of a file to create a movie, such as "movie.avi"
			If movie="" no movie is created.
		fps: int (default: 15)
			Number of frames per second (only if movie requested).
		dpi: int (default: 200)
			Number of dots per inch (only if movie requested).
		saveAs: path string (default: "")
			Name of a directory where to save each frame as figures.
			You can even specify a filename such as mydir/prefix.png
			and it will automatically make successive files showing
			the timestep: mydir/prefix0.png, mydir/prefix1.png, etc.
		
		Example:
		--------
			S = Smilei("path/to/my/results")
			S.ParticleDiagnostic(1).plot(vmin=0, vmax=1e14)
			
			This takes the particle diagnostic #1 and plots the resulting array in figure 1 from 0 to 3e14.
		"""
		if not self._validate(): return
		if not self._prepare(): return
		self.set(**kwargs)
		self.info()
		
		# Make figure
		fig = self._plt.figure(**self.options.figure0)
		fig.set(**self.options.figure1)
		fig.clf()
		ax = fig.add_subplot(1,1,1)
		
		# Case of a streakPlot (no animation)
		if self.options.streakPlot:
			if len(self.times) < 2:
				print("ERROR: a streak plot requires at least 2 times")
				return
			if not hasattr(self,"_getDataAtTime"):
				print("ERROR: this diagnostic cannot do a streak plot")
				return
			if self.dim != 1:
				print("ERROR: Diagnostic must be 1-D for a streak plot")
				return
			if not (self._np.diff(self.times)==self.times[1]-self.times[0]).all():
				print("WARNING: times are not evenly spaced. Time-scale not plotted")
				ylabel = "Unevenly-spaced times"
			else:
				ylabel = "Timesteps"
			# Loop times and accumulate data
			A = []
			for time in self.times: A.append(self._getDataAtTime(time))
			A = self._np.double(A)
			# Plot
			ax.cla()
			xmin = self._xfactor*self._centers[0][0]
			xmax = self._xfactor*self._centers[0][-1]
			extent = [xmin, xmax, self.times[0], self.times[-1]]
			if self._log[0]: extent[0:2] = [self._np.log10(xmin), self._np.log10(xmax)]
			im = ax.imshow(self._np.flipud(A), vmin = self.options.vmin, vmax = self.options.vmax, extent=extent, **self.options.image)
			ax.set_xlabel(self._xlabel)
			ax.set_ylabel(ylabel)
			self._setLimits(ax, xmin=self.options.xmin, xmax=self.options.xmax, ymin=self.options.ymin, ymax=self.options.ymax)
			try: # if colorbar exists
				ax.cax.cla()
				self._plt.colorbar(mappable=im, cax=ax.cax, **self.options.colorbar)
			except AttributeError:
				ax.cax = self._plt.colorbar(mappable=im, ax=ax, **self.options.colorbar).ax
			self._setSomeOptions(ax)
			self._plt.draw()
			self._plt.pause(0.00001)
			return
		
		# Possible to skip animation
		if self.options.skipAnimation:
			self._animateOnAxes(ax, self.times[-1])
			self._plt.draw()
			self._plt.pause(0.00001)
			return
		
		# Otherwise, animation
		# Movie requested ?
		mov = Movie(fig, movie, fps, dpi)
		# Save to file requested ?
		save = SaveAs(saveAs, fig, self._plt)
		# Loop times for animation
		for time in self.times:
			print("timestep "+str(time))
			# plot
			ax.cla()
			if self._animateOnAxes(ax, time) is None: return
			self._plt.draw()
			self._plt.pause(0.00001)
			mov.grab_frame()
			save.frame(time)
		# Movie ?
		if mov.writer is not None: mov.finish()
	
	
	# Method to select specific timesteps among those available in times
	def _selectTimesteps(self, timesteps, times):
		ts = self._np.array(self._np.double(timesteps),ndmin=1)
		if ts.size==2:
			# get all times in between bounds
			times = times[ (times>=ts[0]) * (times<=ts[1]) ]
		elif ts.size==1:
			# get nearest time
			times = self._np.array([times[(self._np.abs(times-ts)).argmin()]])
		else:
			raise
		return times
	
	# Method to prepare some data before plotting
	def _prepare(self):
		self._prepare1()
		if not self._prepare2(): return False
		self._prepare3()
		self._prepare4()
		return True
	
	# Methods to prepare stuff
	def _prepare1(self):
		# prepare the factors
		self._xfactor = (self.options.xfactor or 1.) * self.units.xcoeff
		self._yfactor = (self.options.yfactor or 1.) * self.units.ycoeff
		self._vfactor = self.units.vcoeff
		self._tfactor = (self.options.xfactor or 1.) * self.units.tcoeff * self.timestep
	def _prepare2(self):
		# prepare the animating function
		if not self._animateOnAxes:
			if   self.dim == 0: self._animateOnAxes = self._animateOnAxes_0D
			elif self.dim == 1: self._animateOnAxes = self._animateOnAxes_1D
			elif self.dim == 2: self._animateOnAxes = self._animateOnAxes_2D
			else:
				print("Cannot plot in "+str(self.dim)+" dimensions !")
				return False
		# prepare t label
		self._tlabel = self.units.tname
		if self.options.xfactor: self._tlabel += "/"+str(self.options.xfactor)
		self._tlabel = 'Time ( '+self._tlabel+' )'
		# prepare x label
		if self.dim > 0:
			self._xlabel = self.units.xname
			if self.options.xfactor: self._xlabel += "/"+str(self.options.xfactor)
			self._xlabel = self._label[0] + " (" + self._xlabel + ")"
			if self._log[0]: self._xlabel = "Log[ "+self._xlabel+" ]"
		# prepare y label
		if self.dim > 1:
			self._ylabel = self.units.yname
			if self.options.yfactor: self._ylabel += "/"+str(self.options.yfactor)
			self._ylabel = self._label[1] + " (" + self._ylabel + ")"
			if self._log[1]: self._ylabel = "Log[ "+self._ylabel+" ]"
			# prepare extent for 2d plots
			self._extent = [self._xfactor*self._centers[0][0], self._xfactor*self._centers[0][-1], self._yfactor*self._centers[1][0], self._yfactor*self._centers[1][-1]]
			if self._log[0]:
				self._extent[0] = self._np.log10(self._extent[0])
				self._extent[1] = self._np.log10(self._extent[1])
			if self._log[1]:
				self._extent[2] = self._np.log10(self._extent[2])
				self._extent[3] = self._np.log10(self._extent[3])
		# prepare title
		self._vlabel = ""
		if self.units.vname: self._vlabel += " (" + self.units.vname + ")"
		if self._title     : self._vlabel = self._title + self._vlabel
		if self._data_log  : self._vlabel = "Log[ "+self._vlabel+" ]"
		return True
	def _prepare3(self):
		# prepare temporary data if zero-d plot
		if self.dim == 0 and self._tmpdata is None:
			self._tmpdata = self._np.zeros(self.times.size)
			for i, t in enumerate(self.times):
				self._tmpdata[i] = self._getDataAtTime(t)
	def _prepare4(self): pass
	
	# Method to set limits to a plot
	def _setLimits(self, ax, xmin=None, xmax=None, ymin=None, ymax=None):
		if xmin is not None: ax.set_xlim(xmin=xmin)
		if xmax is not None: ax.set_xlim(xmax=xmax)
		if ymin is not None: ax.set_ylim(ymin=ymin)
		if ymax is not None: ax.set_ylim(ymax=ymax)
	
	# Methods to plot the data when axes are made
	def _animateOnAxes_0D(self, ax, t):
		times = self.times[self.times<=t]
		A     = self._tmpdata[self.times<=t]
		im, = ax.plot(self._tfactor*times, self._vfactor*A, **self.options.plot)
		ax.set_xlabel(self._tlabel)
		self._setLimits(ax, xmax=self._tfactor*self.times[-1], ymin=self.options.vmin, ymax=self.options.vmax)
		self._setSomeOptions(ax, t)
		return im
	def _animateOnAxes_1D(self, ax, t):
		A = self._getDataAtTime(t)
		im, = ax.plot(self._xfactor*self._centers[0], self._vfactor*A, **self.options.plot)
		if self._log[0]: ax.set_xscale("log")
		ax.set_xlabel(self._xlabel)
		self._setLimits(ax, xmin=self.options.xmin, xmax=self.options.xmax, ymin=self.options.vmin, ymax=self.options.vmax)
		self._setSomeOptions(ax, t)
		return im
	def _animateOnAxes_2D(self, ax, t):
		A = self._getDataAtTime(t)
		im = self._animateOnAxes_2D_(ax, self._vfactor*A)
		ax.set_xlabel(self._xlabel)
		ax.set_ylabel(self._ylabel)
		self._setLimits(ax, xmin=self.options.xmin, xmax=self.options.xmax, ymin=self.options.ymin, ymax=self.options.ymax)
		try: # if colorbar exists
			ax.cax.cla()
			self._plt.colorbar(mappable=im, cax=ax.cax, **self.options.colorbar)
		except AttributeError:
			ax.cax = self._plt.colorbar(mappable=im, ax=ax, **self.options.colorbar).ax
		self._setSomeOptions(ax, t)
		return im
	
	# Special case: 2D plot
	# This is overloaded by class "Probe" because it requires to replace imshow
	def _animateOnAxes_2D_(self, ax, A):
		im = ax.imshow( self._np.flipud(A.transpose()),
			vmin = self.options.vmin, vmax = self.options.vmax, extent=self._extent, **self.options.image)
		return im
	
	# set options during animation
	def _setSomeOptions(self, ax, t=None):
		title = []
		if self._vlabel: title += [self._vlabel]
		if t is not None: title += ["t = "+str(t)]
		ax.set_title("  ".join(title))
		ax.set(**self.options.axes)
		try:
			if len(self.options.xtick)>0: ax.ticklabel_format(axis="x",**self.options.xtick)
		except:
			print("Cannot format x ticks (typically happens with log-scale)")
			self.xtickkwargs = []
		try:
			if len(self.options.ytick)>0: ax.ticklabel_format(axis="y",**self.options.ytick)
		except:
			print("Cannot format y ticks (typically happens with log-scale)")
			self.xtickkwargs = []
	
	# Convert to XDMF format for ParaView (do nothing in the mother class)
	def toXDMF(self):
		pass