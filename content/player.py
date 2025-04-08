import arcade
import os, json
from datatypes.points import *

class Player(arcade.Sprite):
    def __init__(self, coord = ThetaPoint(), texture_folder="resources/player", texture_data = "player_textures.json"):
        super().__init__(x=coord.x, y=coord.y, angle=coord.theta)

        self.texture_folder = texture_folder
        self.texture_datafile = texture_data
        self.load_textures(texture_metadatafile=os.path.join(self.texture_folder, self.texture_datafile), texture_folder = self.texture_folder)
    
    def load_textures(self, texture_metadatafile, texture_folder):
        self.texture_metadata = json.load(open(texture_metadatafile, "r"))
        pass

