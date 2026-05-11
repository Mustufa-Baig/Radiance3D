import radiance3d as r3d
import time, math

r3d.init_window(800, 600, "Radiance3D - Full PBR Materials")
r3d.set_fps_camera(True)

r3d.load_skybox("canary.hdr")

sponza = r3d.load_model("sponza.glb") 
cube = r3d.load_model("monkey.glb")


sponza_collider = r3d.physics_mesh("sponza rough blockout.glb")
cube_collider = r3d.physics_mesh("monkey.glb")


sponza_collider.set_physics_type("Compound Hull")
cube_collider.set_physics_type("Convex Hull")


r3d.set_physics_mass(sponza_collider, 0.0)

r3d.bind_physics_mesh(cube_collider, cube)


r3d.set_physics_rotation(cube_collider, math.radians(45), 0, math.radians(45))
r3d.set_physics_position(cube_collider, 0, 15, 0)

r3d.set_camera_position(7, 2, 0)
r3d.camera_look_at(0, 2, 0)

r3d.set_sun_direction(-0.5, -1.0, 0.2)
r3d.set_sun_color(18.0, 17.5, 17.0)

start_time = time.time()
panelty=0

while r3d.is_running():
    elapsed = time.time() - start_time
    r3d.set_sun_direction(0.2*math.sin(elapsed * 0.5), -1.0, 0.2*math.cos(elapsed * 0.5))
    
    if panelty>0:
        panelty-=1
    
    if r3d.get_key(ord('X')) and panelty<=0:
        panelty=10
        c = r3d.load_model("monkey.glb")
        cc = r3d.physics_mesh("monkey.glb")
        cc.set_physics_type("Convex Hull")
        r3d.bind_physics_mesh(cc, c)

        r3d.set_physics_rotation(cc, math.radians(45), 0, math.radians(45))
        r3d.set_physics_position(cc, 0, 15, 0)


    r3d.step_physics()
    r3d.render_frame()