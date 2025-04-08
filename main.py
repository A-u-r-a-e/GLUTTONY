import arcade
from window import Gluttony

INIT_WINDOW_SIZE = (1280, 720)
INIT_WINDOW_TITLE = "Gluttony"
TICK_SPEED = 60 #operates under 60 fps

def main():
    game = Gluttony(w_width=INIT_WINDOW_SIZE[0],w_height=INIT_WINDOW_SIZE[1],w_title=INIT_WINDOW_TITLE,FPS=TICK_SPEED)
    game.setup()
    arcade.run()

if __name__ == "__main__":
    main()

