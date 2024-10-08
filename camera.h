extern Mat projection;
extern Vec3 camera;

bool in_view(Vec3 point, Vec3 *projected);
void camera_set(Vec3 pos, int angle_x, int angle_y);