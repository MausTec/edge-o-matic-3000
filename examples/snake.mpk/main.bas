REM This is an example application for the Edge-o-Matic 3000

import "@application"
import "@toast"
import "@graphics"

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

	def render()
	enddef
endclass

class SnakeGame
	var snake = new(Snake)
	var score = 0
	var food = new(Point)

	def start()
		food.assign(64, 32)
	enddef

	def render()
		snake.render()
		draw_pixel(food.x, food.y, 1);
		display_buffer()
	enddef
	
	def tick()
	enddef
endclass

let Game = new(SnakeGame)
Game.start()

def loop()
	Game.tick()
	Game.render()
	return nil
enddef

start