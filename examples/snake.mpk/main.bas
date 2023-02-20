REM This is an example application for the Edge-o-Matic 3000

import "@application"
import "@toast"

class Point
	var px = 0
	var py = 0

	def assign(x, y)
		px = x
		py = y
	enddef
endclass

class Snake
	dim points(10)

	def add_point(x, y)
		let point = new(Point)
		point.assign(x, y)
		push(points, point)
	enddef
endclass

' TODO: Implement the superclass Application for this to provide useful stubs.
' class SnakeGame(Application)
class SnakeGame
	def setup()
		toast("Application Setup!")
	enddef

	def loop()
		print "loop"
	enddef
endclass

let app = new(SnakeGame)
start(app)