from dataclasses import dataclass

'''
General Features:
 - Generate point type from a tuple of values
'''

'''
BasicPoint Features:
 - Distance from another
 - Angle from another
 - Vector Normalization (in respect to a point)
 - Vector Computations (sum, dot)
 - Constant Computations (add, minus, mul, div)
'''
@dataclass
class BasicPoint:
    def __init__(self, x = 0, y = 0):
        self.x: float = x
        self.y: float = y

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