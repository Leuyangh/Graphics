Leu-yang Huang 1811415408 CSCI 420 Homework 2 - Rollercoaster
Features - right click menu with options to:
		Increase speed
		Increase speed significantly
		Reset ride
		Decrease crossbar frequency
		Decrease crossbar frequency significantly
		Toggle Portals (At the end of a spline, renders a portal to 'justify' the jump between the end of one spline and the start of another)
		Reset Camera angle
		Reset speed
		Stop coaster
		Able to use mouse to adjust camera angle, as well as use shift/ctrl to adjust scale and translation
Implementation
		Sky - rendered a cube but used a texture that has a lot of black on the corners to kind of hide hard edges and a random pattern helps too	
		Camera normals - Personally found that crossing the tangent with the unit Y vector was the best resulting normals