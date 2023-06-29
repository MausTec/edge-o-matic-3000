REM This is an example application for the Edge-o-Matic 3000

import "@application"
import "@toast"

print "start"

class Point
	var px = 0
	var py = 0

	def assign(x, y)
		px = x
		py = y
	enddef
endclass

print "Point defined."

class Snake
	dim points(10)

	def add_point(x, y)
		let point = new(Point)
		point.assign(x, y)
		push(points, point)
	enddef
endclass

print "Snake defined."

def setup()
	toast("Application Setup!")
enddef

def loop()
	print "loop"
enddef

start