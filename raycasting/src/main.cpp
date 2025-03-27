#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <iostream>
#include <cmath>

#define WIDTH 900
#define HEIGHT 600

struct Circle
{
    double x;
    double y;
    double r;
};

struct Line
{
    double sx;
    double sy;
    double ex;
    double ey;
};

struct LineEquation
{
    double a;
    double b;
    double c;
};

LineEquation GetLineEquation(const Line &line)
{
    double a = line.sy - line.ey;
    double b = line.ex - line.sx;
    double c = line.sx * line.ey - line.ex * line.sy;
    return {a, b, c};
}

Uint32 MapRGB(SDL_Surface *surface, Uint8 r, Uint8 g, Uint8 b)
{
    return SDL_MapRGB(surface->format, r, g, b);
}

void SetPixel(SDL_Surface *surface, int x, int y, Uint32 color)
{
    if (x < 0 || y < 0 || x >= surface->w || y >= surface->h)
        return;

    Uint32 *pixels = (Uint32 *)surface->pixels;
    pixels[(y * surface->w) + x] = color;
}

void FillCircle(SDL_Surface *surface, const Circle &circle, Uint32 color, int outline)
{
    int cx = (int)circle.x;
    int cy = (int)circle.y;
    int r = (int)circle.r;
    int r2 = r * r;

    int useOutline = outline == 0 ? 0 : 1;

    for (int y = cy - r; y <= cy + r; y++)
    {
        for (int x = cx - r; x <= cx + r; x++)
        {
            int dx = x - cx;
            int dy = y - cy;
            int d2 = dx * dx + dy * dy;
            if (useOutline)
            {
                if (d2 <= r2 + outline && d2 >= r2 - outline)
                {
                    SetPixel(surface, x, y, color);
                }
            }
            else
            {
                if (d2 <= r2)
                {
                    SetPixel(surface, x, y, color);
                }
            }
        }
    }
}

// bresenham's line algorithm
void DrawLine(SDL_Surface *surface, const Line &line, Uint32 color)
{
    int x0 = (int)line.sx;
    int y0 = (int)line.sy;
    int x1 = (int)line.ex;
    int y1 = (int)line.ey;

    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);

    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;

    int steps = 0;
    const int maxSteps = 10000;

    while (true)
    {
        SetPixel(surface, x0, y0, color);
        if (x0 == x1 && y0 == y1)
            break;
        if (++steps > maxSteps)
            break; // prevent infinite loop

        int e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

bool PointOnRay(double px, double py, const Line &line)
{
    double dx = line.ex - line.sx;
    double dy = line.ey - line.sy;

    double lenSq = dx * dx + dy * dy;
    double dot = (px - line.sx) * dx + (py - line.sy) * dy;

    return dot >= 0 && dot <= lenSq;
}

// calculate intersections of line and circle
int CheckRayCastCollision(SDL_Surface *surface, Line &line, const Circle &circle)
{
    double dx = line.ex - line.sx;
    double dy = line.ey - line.sy;

    double fx = line.sx - circle.x;
    double fy = line.sy - circle.y;

    double A = dx * dx + dy * dy;
    double B = 2 * (fx * dx + fy * dy);
    double C = fx * fx + fy * fy - circle.r * circle.r;

    double D = B * B - 4 * A * C;

    if (D < 0 || std::isnan(D) || std::isinf(D))
        return 0;

    double sqrtD = sqrt(D);
    double t = (-B - sqrtD) / (2 * A); // nearest intersection

    if (t < 0 || t > 1)
        return 0; // ignore if intersection is behind or beyond

    line.ex = line.sx + t * dx;
    line.ey = line.sy + t * dy;

    if ((int)line.ex == (int)line.sx && (int)line.ey == (int)line.sy)
        return 0;

    return 1;
}

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        return 1;

    SDL_Window *window = SDL_CreateWindow("Raycast", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if (!window)
        return 1;

    SDL_Surface *surface = SDL_GetWindowSurface(window);
    SDL_Rect screenRect = {0, 0, WIDTH, HEIGHT};

    // objects
    Circle circle = {150, 300, 25};
    Circle object = {750, 300, 100};

    // colors
    Uint32 bgColor = MapRGB(surface, 51, 51, 51);
    Uint32 circleColor = MapRGB(surface, 255, 255, 255);
    Uint32 lineColor = MapRGB(surface, 255, 255, 0);

    bool running = true;
    SDL_Event event;

    // raycast circle values
    double angle = 1.25;

    while (running)
    {
        // standard sdl stuff
        SDL_FillRect(surface, &screenRect, bgColor);

        SDL_LockSurface(surface);

        // event handling
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                running = false;
            if (event.type == SDL_MOUSEMOTION && event.motion.state != 0)
            {
                if (event.motion.state == 1) {
                    circle.x = event.motion.x;
                    circle.y = event.motion.y;
                } else {
                    object.x = event.motion.x;
                    object.y = event.motion.y;
                }
            }
        }

        // rendering
        for (double i = 0; i <= 360; i += angle)
        {
            double radians = i * M_PI / 180.0;
            double sx = circle.x;
            double sy = circle.y;
            double ex = sx + circle.r * cos(radians) * 50;
            double ey = sy + circle.r * sin(radians) * 50;
            Line newLine = {sx, sy, ex, ey};
            bool isHit = CheckRayCastCollision(surface, newLine, object);
            DrawLine(surface, newLine, lineColor); // normal
        }

        FillCircle(surface, circle, circleColor, 0);
        FillCircle(surface, object, circleColor, 0);

        SDL_UnlockSurface(surface);
        SDL_UpdateWindowSurface(window);

        // control of framerate
        SDL_Delay(16);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}