#include <cstdio>
#include <cmath>

struct Vec3 { float x, y, z; };
struct Quat { float x, y, z, w; };
struct Pose { Vec3 position; Quat orientation; };

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

Quat multiply(const Quat& a, const Quat& b) {
    return {
        a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
        a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
        a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w,
        a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z
    };
}

Vec3 apply_pose(const Pose& pose, const Vec3& p) {
    return add(rotate(pose.orientation, p), pose.position);
}

// Composé
Pose compose(const Pose& parent, const Pose& child) {
    Pose result;
    result.orientation = multiply(parent.orientation, child.orientation);
    result.position = add(rotate(parent.orientation, child.position), parent.position);
    return result;
}

int main() {
    Pose parent = { {1.0f, 2.0f, 3.0f}, {0.0f, 0.70710678f, 0.0f, 0.70710678f} };

    Pose child = { {0.0f, 1.0f, 0.0f}, {0.70710678f, 0.0f, 0.0f, 0.70710678f} };

    Pose composed = compose(parent, child);

    Vec3 point = { 0.5f, -1.0f, 2.0f };

    // Methode 1 : composer, puis appliquer une seule fois
    Vec3 via_compose = apply_pose(composed, point);

    // Methode 2 : appliquer l'enfant, puis appliquer le parent au resultat
    Vec3 step1 = apply_pose(child, point);
    Vec3 via_chain = apply_pose(parent, step1);

    float dx = via_compose.x - via_chain.x;
    float dy = via_compose.y - via_chain.y;
    float dz = via_compose.z - via_chain.z;
    float ecart = std::sqrt(dx*dx + dy*dy + dz*dz);

    std::printf("Point (repere de l'enfant) : %.4f %.4f %.4f\n\n", point.x, point.y, point.z);
    std::printf("Composer puis appliquer : %.4f %.4f %.4f\n", via_compose.x, via_compose.y, via_compose.z);
    std::printf("Appliquer enfant puis parent : %.4f %.4f %.4f\n", via_chain.x, via_chain.y, via_chain.z);
    std::printf("Ecart (norme) : %.8f\n", ecart);

    return 0;
}