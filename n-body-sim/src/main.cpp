#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <iostream>
#include <vector>
#include <cmath>

#define WIDTH 900
#define HEIGHT 600
#define G 6.67430e-11f
#define EARTH_MASS 5.972e24f
#define MOON_MASS 7.348e22f
#define EARTH_RADIUS 6371000.0f
#define MOON_RADIUS 1737000.0f
#define EARTH_MOON_DISTANCE 384400000.0f
#define SCALE 1e-5f
#define RADIUS_SCALE 300.0f

struct Vec3 {
    float x, y, z;

    Vec3 operator+(const Vec3& o) const { return { x + o.x, y + o.y, z + o.z }; }
    Vec3 operator-(const Vec3& o) const { return { x - o.x, y - o.y, z - o.z }; }
    Vec3 operator*(float s) const { return { x * s, y * s, z * s }; }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }

    float lengthSquared() const { return x * x + y * y + z * z; }
    float length() const { return std::sqrt(lengthSquared()); }
    Vec3 normalized() const {
        float len = length();
        return len > 0.0f ? *this * (1.0f / len) : Vec3{0, 0, 0};
    }
};

struct Object {
    Vec3 position;
    Vec3 velocity;
    float radius;
    float mass;
    Uint32 color;
};

Vec3 cameraPos = { 0.0f, 0.0f, -EARTH_MOON_DISTANCE * SCALE * 2.5f };
Vec3 cameraRot = { 0.3f, 0.0f, 0.0f };
Vec3 lightPos = { 0.0f, 0.0f, -EARTH_MOON_DISTANCE * SCALE * 3.0f };

Vec3 rotateY(const Vec3& v, float angle) {
    float c = std::cos(angle), s = std::sin(angle);
    return { c * v.x + s * v.z, v.y, -s * v.x + c * v.z };
}

Vec3 rotateX(const Vec3& v, float angle) {
    float c = std::cos(angle), s = std::sin(angle);
    return { v.x, c * v.y - s * v.z, s * v.y + c * v.z };
}

Vec3 worldToCamera(const Vec3& pos) {
    Vec3 relative = pos - cameraPos;
    relative = rotateX(relative, cameraRot.x);
    relative = rotateY(relative, cameraRot.y);
    return relative;
}

Vec3 projectToScreen(const Vec3& pos) {
    float fov = 500.0f;
    float factor = fov / (fov + pos.z);
    return {
        WIDTH / 2.0f + pos.x * factor,
        HEIGHT / 2.0f - pos.y * factor,
        pos.z
    };
}

Uint32 applyShading(Uint32 baseColor, Vec3 normal, Vec3 toLight) {
    float intensity = std::max(0.0f, normal.normalized().x * toLight.normalized().x + normal.normalized().y * toLight.normalized().y + normal.normalized().z * toLight.normalized().z);
    float factor = 0.1f + 0.9f * intensity;
    Uint8 r = (baseColor >> 24) & 0xFF;
    Uint8 g = (baseColor >> 16) & 0xFF;
    Uint8 b = (baseColor >> 8) & 0xFF;
    Uint8 a = baseColor & 0xFF;
    r = static_cast<Uint8>(r * factor);
    g = static_cast<Uint8>(g * factor);
    b = static_cast<Uint8>(b * factor);
    return (r << 24) | (g << 16) | (b << 8) | a;
}

void FillMarker(SDL_Surface* surface, const Vec3& screenPos, Uint32 color) {
    SDL_Rect rect = { (int)(screenPos.x - 2), (int)(screenPos.y - 2), 4, 4 };
    SDL_FillRect(surface, &rect, color);
}

void FillSphere(SDL_Surface* surface, const Object& obj) {
    Vec3 camSpace = worldToCamera(obj.position);
    if (camSpace.z < 1.0f) return;
    Vec3 screen = projectToScreen(camSpace);
    FillMarker(surface, screen, 0xFF00FFFF);

    float radius = obj.radius * SCALE * RADIUS_SCALE * (500.0f / (500.0f + camSpace.z));
    float r2 = radius * radius;

    for (int x = (int)(screen.x - radius); x < (int)(screen.x + radius); ++x) {
        for (int y = (int)(screen.y - radius); y < (int)(screen.y + radius); ++y) {
            float dx = x - screen.x;
            float dy = y - screen.y;
            float distSq = dx * dx + dy * dy;
            if (distSq <= r2) {
                float dz = std::sqrt(std::max(0.0f, r2 - distSq));
                Vec3 normal = { dx / radius, -dy / radius, dz / radius };
                Vec3 lightVec = lightPos - obj.position;
                Uint32 shaded = applyShading(obj.color, normal, lightVec);
                SDL_Rect pixel = { x, y, 1, 1 };
                SDL_FillRect(surface, &pixel, shaded);
            }
        }
    }
}

void applyGravity(Object& a, const Object& b, float dt) {
    Vec3 diff = b.position - a.position;
    float distSq = diff.lengthSquared();
    float dist = std::sqrt(distSq);
    if (dist < 1.0f) return;

    Vec3 dir = diff * (1.0f / dist);
    float force = G * (a.mass * b.mass) / distSq;
    float accel = force / a.mass;

    a.velocity += dir * (accel * dt);
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
    SDL_Window* window = SDL_CreateWindow("Realistic 3D Orbit Simulation", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) return 1;
    SDL_Surface* surface = SDL_GetWindowSurface(window);
    SDL_Rect screenRect = { 0, 0, WIDTH, HEIGHT };

    std::vector<Object> objects = {
        {{0, 0, 0}, {0, 0, 0}, EARTH_RADIUS, EARTH_MASS, 0xFFFFFFFF},
        {{EARTH_MOON_DISTANCE, 0, 0}, {0, 1022, 0}, MOON_RADIUS, MOON_MASS, 0xFFCCCCCC},
    };

    Uint64 now = SDL_GetPerformanceCounter();
    Uint64 last = now;
    float dt = 0.0f;
    bool running = true;
    SDL_Event event;

    while (running) {
        last = now;
        now = SDL_GetPerformanceCounter();
        dt = (float)(now - last) / SDL_GetPerformanceFrequency();

        SDL_FillRect(surface, &screenRect, 0x00000000);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_MOUSEMOTION && (event.motion.state & SDL_BUTTON(SDL_BUTTON_RIGHT))) {
                cameraRot.y += event.motion.xrel * 0.005f;
                cameraRot.x += event.motion.yrel * 0.005f;
            }
            if (event.type == SDL_MOUSEWHEEL) {
                cameraPos.z += event.wheel.y * 2000000.0f * SCALE;
            }
        }

        for (size_t i = 0; i < objects.size(); ++i) {
            for (size_t j = 0; j < objects.size(); ++j) {
                if (i != j) applyGravity(objects[i], objects[j], dt);
            }
        }

        for (auto& obj : objects) {
            obj.position += obj.velocity * dt;
            FillSphere(surface, obj);
        }

        SDL_UpdateWindowSurface(window);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
