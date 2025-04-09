from dataclasses import dataclass
from math import *

'''
General Features:
 - Generate point type from a tuple of values
'''

'''
BasicPoint Features:
 - Distance from another [DONE]
 - Angle from another [DONE]
 - Vector Normalization (in respect to a point) [DONE]
 - Vector Computations (sum, dot) [DONE]
 - Constant Computations (add, minus, mul, div) [DONE]
'''
@dataclass
class BasicPoint:
    def __init__(self, x = 0, y = 0):
        self.x: float = x
        self.y: float = y

    def __repr__(self):
        return f"BasicPoint({self.x}, {self.y})"
    
    def __add__(self, other):
        return BasicPoint(self.x+other, self.y+other)
    
    def __sub__(self, other):
        return BasicPoint(self.x-other, self.y-other)
    
    def __mul__(self, other):
        return BasicPoint(self.x*other, self.y*other)
    
    def __truediv__(self, other):
        if other == 0:
            return ValueError
        return BasicPoint(round(self.x/other, 9), round(self.y/other, 9))
    
    def distanceFrom(self, point: BasicPoint):
        dx = self.x - point.x
        dy = self.y - point.y
        return sqrt(dx**2+dy**2)

    def angleFrom(self, point: BasicPoint):
        dx = point.x - self.x
        dy = point.y - self.y
        return atan(dx/dy)
    
    def vectorNorm(self, origin: BasicPoint = BasicPoint(0, 0)):
        scale = self.distanceFrom(origin)
        return BasicPoint(self.x/scale, self.y/scale)
    
    def pointSum(self, *args):
        newPoint = BasicPoint(self.x, self.y)
        if isinstance(args, list):
            newargs = [x for arg in args for x in arg]
        else:
            newargs = list(args)
        for arg in newargs:
            if not isinstance(arg, BasicPoint):
                return TypeError
            newPoint.x += arg.x
            newPoint.y += arg.y
        return newPoint
    
    def pointDot(self, *args):
        newPoint = BasicPoint(self.x, self.y)
        if isinstance(args, list):
            newargs = [x for arg in args for x in arg]
        else:
            newargs = list(args)
        for arg in newargs:
            if not isinstance(arg, BasicPoint):
                return TypeError
            newPoint.x *= arg.x
            newPoint.y *= arg.y
        return newPoint
    

        
        

'''
ThetaPoint Features:
 - Ray intersection point
 - Point's distance from ray
 - Projection (create point on the ray at a certain distance away)
 - EXT Summing angles
'''
@dataclass
class ThetaPoint(BasicPoint):
    def __init__(self, x = 0, y = 0, theta = 0.0):
        super().__init__(x = x, y = y)
        self.theta: float = theta

'''
ConePoint Features:
 - New Cone generation from two cone's intersecting
 - Point Collision (whether point is in range)
 - Merging Cones
 - EXT Projection (point will have theta on the line from this point)
 - EXT Point Distance
'''
@dataclass
class ConePoint(ThetaPoint):
    def __init__(self, x = 0, y = 0, theta = 0.0, psi = 45.0):
        super().__init__(x = x, y = y, theta = theta)
        self.psi: float = psi

'''
SlicePoint Features:
 - EXT New Slice generation
 - EXT Point Collision
 - EXT Projection to radius
 - EXT Point Distance
Features Disabled:
 - Merging Cones
'''
@dataclass
class SlicePoint(ConePoint):
    def __init__(self, x = 0, y = 0, theta = 0.0, psi = 45.0, radius = 1):
        super().__init__(x = x, y = y, theta = theta, psi = psi)
        self.radius: float = radius