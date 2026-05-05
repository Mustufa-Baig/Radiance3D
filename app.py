import radiance3d as r3d
import math

r3d.init_window(800, 600, "Radiance3D - Full PBR Materials")
r3d.set_fps_camera(True)

box = r3d.load_model("sponza.glb") 
r3d.set_position(box, 0, 0, 0)
r3d.set_rotation(box, math.radians(0),0,0)

# Set the base material values (these act as fallbacks if a texture isn't loaded)
r3d.set_material(box, 1.0, 1.0, 1.0, 0.0, 0.5) 

# Map the PBR Textures
r3d.set_albedo_texture(box, "plaster.jpg")
r3d.set_roughness_texture(box, "plaster_roughness.jpg")
#r3d.set_metallic_texture(box, "plaster_metallic.jpg") # If you have a metal object!
r3d.set_normal_texture(box, "plaster_normal.jpg") # The magic bluish image!

r3d.set_camera_position(0, 2, 0)
r3d.camera_look_at(0, 2, 0)

while r3d.is_running():
    r3d.render_frame()