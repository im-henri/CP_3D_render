#include "RenderFP3D.hpp"

/*
def rotateOnPlane(self,a,b,radians):
    sin = math.sin(radians)
    cos = math.cos(radians)
    rotatedA = a*cos - b*sin
    rotatedB = b*cos + a*sin
    return rotatedA, rotatedB
*/
void rotateOnPlane(
    Fix16& a, Fix16& b,
    Fix16 radians
) {
    auto sin = radians.sin();
    auto cos = radians.cos();
    // Temp values
    auto rot_a = a*cos - b*sin;
    auto rot_b = b*cos + a*sin;
    // Overwrite a, b
    a = rot_a;
    b = rot_b;
}


fix16_vec2 getScreenCoordinate(Fix16 FOV, fix16_vec3 point, fix16_vec3 camera_pos, fix16_vec2 camera_rot)
{
    Fix16 sx, sy;
    fix16_vec3 temp({
        point.x - camera_pos.x,
        point.y - camera_pos.y,
        point.z - camera_pos.z,
    });

    /*
    x, z = self.rotateOnPlane(x, z, self.camera.xRotation)
    y, z = self.rotateOnPlane(y, z, self.camera.yRotation)
    */
    rotateOnPlane(temp.x, temp.z, camera_rot.x);
    rotateOnPlane(temp.y, temp.z, camera_rot.y);

    /*
    if z == 0:
        z = 0.0001
    */
    if (temp.z == 0.0f){
        temp.z = 0.001f;
    }
    // fov/z
    auto focal = FOV/temp.z;
    auto realx = ((temp.x)*focal);
    auto realy = ((temp.y)*focal);
    // Shift to screen center (from coordinate center)
    sx = Fix16((int16_t) (SCREEN_X/2)) + (realx);
    sy = Fix16((int16_t) (SCREEN_Y/2)) + (realy);

    /*
    drawit = False
    extra = 2000
    if z>0 and x < self.width+ extra and x > -extra and y >-extra and y < self.height+extra:
        drawit = True
    */
    return fix16_vec2({sx, sy});
}

/*
def getScreenCoordinates(self, vertex, skip = False):
    '''
    LOGIC OF GETTING DRAWING COORDINATES
    '''
    vx = vertex[0]; vy = vertex[1]; vz = vertex[2]
    camx = self.camera.pos[0]; camy = self.camera.pos[1]; camz = self.camera.pos[2]

    x = vx - camx
    y = vy - camy
    z = vz - camz

    x, z = self.rotateOnPlane(x, z, self.camera.xRotation)
    y, z = self.rotateOnPlane(y, z, self.camera.yRotation)


    if z == 0:
        z = 0.0001

    focal = self.fov/z

    realx = ((x)*focal)
    realy = ((y)*focal)

    x = self.centerX + int(realx)
    y = self.centerY + int(realy)
    drawit = False
    extra = 2000
    if z>0 and x < self.width+ extra and x > -extra and y >-extra and y < self.height+extra:
        drawit = True

    return x,y, drawit
*/
