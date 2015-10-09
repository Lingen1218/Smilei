# Some predefined profiles (see doc)

def constant(value, xvacuum=0., yvacuum=0.):
    global dim, sim_length
    if dim == "1d3v": return lambda x: value #if x>=xvacuum else 0.
    if dim == "2d3v": return lambda x,y: value #if (x>=xvacuum and y>=yvacuum) else 0.

def trapezoidal(max,
                xvacuum=0., xplateau=None, xslope1=0., xslope2=0.,
                yvacuum=0., yplateau=None, yslope1=0., yslope2=0. ):
    global dim, sim_length
    if not dim or not sim_length:
        raise Exception("trapezoidal profile has been defined before `dim` or `sim_length`")
    if len(sim_length)>0 and xplateau is None: xplateau = sim_length[0]-xvacuum
    if len(sim_length)>1 and yplateau is None: yplateau = sim_length[1]-yvacuum
    def fx(x):
        # vacuum region
        if x < xvacuum: return 0.
        # linearly increasing density
        elif x < xvacuum+xslope1: return max*(x-xvacuum) / xslope1
        # density plateau
        elif x < xvacuum+xslope1+xplateau: return max
        # linearly decreasing density
        elif x < xvacuum+xslope1+xplateau+xslope2:
            return max*(1. - ( x - (xvacuum+xslope1+xslope2) ) / xslope2)
        # beyond the plasma
        else: return 0.0
    if dim == "1d3v": return fx
    def fy(y):
        # vacuum region
        if y < yvacuum: return 0.
        # linearly increasing density
        elif y < yvacuum+yslope1: return (y-yvacuum) / yslope1
        # density plateau
        elif y < yvacuum+yslope1+yplateau: return 1.
        # linearly decreasing density
        elif y < yvacuum+yslope1+yplateau+yslope2:
            return 1. - ( y - (yvacuum+yslope1+yslope2) ) / yslope2
        # beyond
        else: return 0.0
    if dim == "2d3v": return lambda x,y: fx(x)*fy(y)

def gaussian(max,
             xvacuum=0., xlength=None, xfwhm=None, xcenter=None, xorder=2,
             yvacuum=0., ylength=None, yfwhm=None, ycenter=None, yorder=2 ):
    import math
    global dim, sim_length
    if not dim or not sim_length:
        raise Exception("gaussian profile has been defined before `dim` or `sim_length`")
    if len(sim_length)>0:
        if xlength is None: xlength = sim_length[0]-xvacuum
        if xfwhm   is None: xfwhm   = xlength/3.
        if xcenter is None: xcenter = xvacuum + xlength/2.
    if len(sim_length)>1: 
        if ylength is None:ylength = sim_length[1]-yvacuum
        if yfwhm   is None:yfwhm   = ylength/3.
        if ycenter is None:ycenter = yvacuum + ylength/2.
    sigmax = (0.5*xfwhm)**xorder/math.log(2.0)
    def fx(x):
        # vacuum region
        if x < xvacuum: return 0.
        # gaussian
        elif x < xvacuum+xlength: return max*math.exp( -(x-xcenter)**xorder / sigmax )
        # beyond
        else: return 0.0
    if dim == "1d3v": return fx
    sigmay = (0.5*yfwhm)**yorder/math.log(2.0)
    def fy(y):
        if yorder == 0: return 1.
        # vacuum region
        if y < yvacuum: return 0.
        # gaussian
        elif y < yvacuum+ylength: return math.exp( -(y-ycenter)**yorder / sigmay )
        # beyond
        else: return 0.0
    if dim == "2d3v": return lambda x,y: fx(x)*fy(y)

def polygonal(xpoints=[], xvalues=[]):
    global dim, sim_length
    if not dim or not sim_length:
        raise Exception("polygonal profile has been defined before `dim` or `sim_length`")
    if len(xpoints)!=len(xvalues):
        raise Exception("polygonal profile requires as many points as values")
    if len(sim_length)>0 and len(xpoints)==0:
        xpoints = [0., sim_length[0]]
        xvalues = [1., 1.]
    N = len(xpoints)
    xpoints = [float(x) for x in xpoints]
    xvalues = [float(x) for x in xvalues]
    xslopes = [0. for i in range(1,N)]
    for i in range(1,N):
        if xpoints[i] == xpoints[i-1]: continue
        xslopes[i-1] = (xvalues[i]-xvalues[i-1])/(xpoints[i]-xpoints[i-1])
    def f(x,y=0.):
        if x < xpoints[0]: return 0.0;
        for i in range(1,N):
            if x<xpoints[i]: return xvalues[i-1] + xslopes[i-1] * ( x-xpoints[i-1] )
        return 0.
    return f

def cosine(base, amplitude=1.,
           xvacuum=0., xlength=None, phi=0., xnumber=1):
    import math
    global sim_length
    if not dim or not sim_length:
        raise Exception("cosine profile has been defined before `sim_length`")
    if len(sim_length)>0 and xlength is None: xlength = sim_length[0]-xvacuum
    def f(x,y=0.):
        #vacuum region
        if x < xvacuum: return 0.
        # profile region
        elif x < xvacuum+xlength:
            return base + amplitude * math.cos(phi + 2.*math.pi * xnumber * (x-xvacuum)/xlength)
        # beyond
        else: return 0.
    return f




def tconstant(start=0.):
    return lambda t: 1. if t>=start else 0.

def ttrapezoidal(start=0., plateau=None, slope1=0., slope2=0.):
    global sim_time
    if not sim_time:
        raise Exception("ttrapezoidal profile has been defined before `sim_time`")
    if plateau is None: plateau = sim_time - start
    def ft(t):
        if t < start: return 0.
        elif t < start+slope1: return (t-start) / slope1
        elif t < start+slope1+plateau: return 1.
        elif t < start+slope1+plateau+slope2:
            return 1. - ( t - (start+slope1+slope2) ) / slope2
        else: return 0.0
    return ft

def tgaussian(start=0., duration=None, fwhm=None, center=None, order=2):
    import math
    global sim_time
    if not sim_time:
        raise Exception("tgaussian profile has been defined before `sim_time`")
    if duration is None: duration = sim_time-start
    if fwhm     is None: fwhm     = duration/3.
    if center   is None: center   = start + duration/2.
    sigma = (0.5*fwhm)**order/math.log(2.0)
    def ft(t):
        if t < start: return 0.
        elif t < start+duration: return math.exp( -(t-center)**order / sigma )
        else: return 0.0

def tpolygonal(points=[], values=[]):
    global sim_time
    if not sim_time:
        raise Exception("tpolygonal profile has been defined before `sim_time`")
    if len(points)==0:
        points = [0., sim_time]
        values = [1., 1.]
    N = len(points)
    points = [float(x) for x in points]
    values = [float(x) for x in values]
    slopes = [0. for i in range(1,N)]
    for i in range(1,N):
        if points[i] == points[i-1]: continue
        slopes[i-1] = (values[i]-values[i-1])/(points[i]-points[i-1])
    def f(t):
        if t < points[0]: return 0.0;
        for i in range(1,N):
            if t<points[i]: return values[i-1] + slopes[i-1] * ( t-points[i-1] )
        return 0.
    return f

def tcosine(base=0., amplitude=1., start=0., duration=None, phi=0., freq=1.):
    import math
    global sim_time
    if not sim_time:
        raise Exception("tcosine profile has been defined before `sim_time`")
    if duration is None: duration = sim_time-start
    def f(t):
        if t < start: return 0.
        elif t < start+duration:
            return base + amplitude * math.cos(phi + freq*(t-start))
        else: return 0.
    return f