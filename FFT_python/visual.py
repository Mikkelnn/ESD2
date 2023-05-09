import turtle
import math

def drawObj(AoA, dist_cm, mic_dist_cm = 7.5, n_channels = 2):
  t = turtle.Turtle()
  t.hideturtle() #this hides the arrow
  t.speed(0) #turn off animation

  def goto(x,y):
    t.pu()
    t.goto(x,y)
    t.pd()
    
  def drawZoomedCircle(x,y,r,n):
      goto(x*n,(y-r)*n) #-r because we want xy as center and Turtles starts from border    
      t.circle(r*n, steps=360)

  r_mic = 1.5
  scale = 10

  t.goto(0,0)
  t.color('red')
  t.circle(1)
  t.color('black')

  # draw microphones
  odd_n_channels = (n_channels % 2) != 0
  offset = 0 if odd_n_channels else mic_dist_cm / 2
  for i in range(-int(n_channels / 2), int(math.ceil(n_channels / 2))):  
    drawZoomedCircle(mic_dist_cm*i+offset,0, r_mic, scale)

  # draw direction
  goto(0,0)
  t.right(AoA - 90)
  t.color('red')
  t.pensize(scale/2)
  t.forward(dist_cm)
  t.color('black')

  t.up()
  t.back(dist_cm/2)
  t.right(-AoA + 90)
  t.forward(1*scale)
  t.write(f'distance: {int(dist_cm)} cm\nAoA: {AoA} deg', False)
  t.pd()

  turtle.done()
