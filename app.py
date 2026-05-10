import radiance3d as r3d
import time, math

r3d.init_window(800, 600, "Radiance3D - Full PBR Materials")
r3d.set_fps_camera(True)

r3d.load_skybox("canary.hdr")

sponza = r3d.load_model("sponza.glb") 
cube = r3d.load_model("cube.glb")
cube_collider = r3d.physics_mesh("cube.glb")

r3d.set_position(sponza, 0, 0, 0)
r3d.set_rotation(sponza, math.radians(0),0,0)

r3d.load_model("floor.glb")
floor_collider = r3d.physics_mesh("floor.glb")

floor_collider.set_physics_type("Convex Hull")
cube_collider.set_physics_type("Convex Hull")

r3d.set_physics_mass(floor_collider, 0.0)

# Bind them together!
r3d.bind_physics_mesh(cube_collider, cube)
r3d.set_physics_rotation(cube_collider, math.radians(45), 0, math.radians(35))

r3d.set_physics_position(cube_collider, 0, 15, 0)

r3d.set_camera_position(7, 2, 0)
r3d.camera_look_at(0, 2, 0)

r3d.set_sun_direction(-0.5, -1.0, 0.2)
r3d.set_sun_color(8.0, 7.5, 7.0)

start_time = time.time()

while r3d.is_running():
    r3d.step_physics()
    r3d.render_frame()