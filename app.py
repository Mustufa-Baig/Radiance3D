import radiance3d as r3d
import time, math

r3d.init_window(800, 600, "Radiance3D - Full PBR Materials")
r3d.set_fps_camera(True)

box = r3d.load_model("sponza.glb") 
r3d.set_position(box, 0, 0, 0)
r3d.set_rotation(box, math.radians(0),0,0)

# Set the base material values (these act as fallbacks if a texture isn't loaded)
r3d.set_material(box, 1.0, 1.0, 1.0, 0.0, 0.5) 

# Map the PBR Textures
texture_name="tiles"

r3d.set_albedo_texture(box, texture_name+".jpg")
r3d.set_roughness_texture(box, texture_name+"_roughness.jpg")
#r3d.set_metallic_texture(box, texture_name+"_metallic.jpg")
r3d.set_normal_texture(box, texture_name+"_normal.jpg") 

r3d.set_camera_position(0, 2, 0)
r3d.camera_look_at(0, 2, 0)

r3d.set_sun_direction(-0.5, -1.0, 0.2)
r3d.set_sun_color(8.0, 7.5, 7.0)

start_time = time.time()

while r3d.is_running():
    # Optional: Animate the sun moving across the sky!
    elapsed = time.time() - start_time
    r3d.set_sun_direction(0.1*math.sin(elapsed * 0.15), -1.0, 0.1*math.cos(elapsed * 0.15))

    r3d.render_frame()