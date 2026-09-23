#include <cstdio>

struct Vec3 {
    float x, y, z;
};

struct Quat {
    float x, y, z, w;
};

struct Pose {
    Vec3 position;
    Quat orientation;
};

Vec3 add(const Vec3& a, const Vec3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
Vec3 scale(const Vec3& a, float s) { return { a.x * s,   a.y * s,   a.z * s }; }
Vec3 cross(const Vec3& a, const Vec3& b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

Vec3 rotate(const Quat& q, const Vec3& v) {
    Vec3 u = { q.x, q.y, q.z };
    Vec3 uxv = cross(u, v);
    Vec3 uxuxv = cross(u, uxv);
    return add(v, add(scale(uxv, 2.0f * q.w), scale(uxuxv, 2.0f)));
}

Vec3 apply_pose(const Pose& pose, const Vec3& p) {
    Vec3 rotated = rotate(pose.orientation, p);
    return add(rotated, pose.position);
}

int main() {
    Pose pose;
    Vec3 point;

    int n = std::scanf("%f %f %f %f %f %f %f %f %f %f",
                        &pose.orientation.x, &pose.orientation.y, &pose.orientation.z, &pose.orientation.w,
                        &pose.position.x, &pose.position.y, &pose.position.z,
                        &point.x, &point.y, &point.z);
    if (n != 10) {
        std::fprintf(stderr, "Entree invalide : attendu qx qy qz qw px py pz x y z\n");
        return 1;
    }

    Vec3 result = apply_pose(pose, point);
    std::printf("%.4f %.4f %.4f\n", result.x, result.y, result.z);

    return 0;
}