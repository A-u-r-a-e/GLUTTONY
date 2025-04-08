import arcade

class Gluttony(arcade.Window):
    def __init__(self, w_width, w_height, w_title, fullscreen = False, FPS=60):
        super().__init__(width=w_width, height=w_height, title=w_title, fullscreen=fullscreen, resizable=false, update_rate=FPS)
    
    def setup(self):
        pass
