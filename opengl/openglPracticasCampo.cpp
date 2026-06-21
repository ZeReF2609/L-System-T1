#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <stack>
#include <random>
#include "src/Shader.h"

float posX = 0.0f, posY = 0.0f;
float deltaTime = 0.0f, lastFrame = 0.0f;
void responsive(GLFWwindow* w, int width, int height);
void userInput(GLFWwindow* w);

std::string generateLSystem(const std::string& axiom, int iters, int type, std::mt19937& rng) {
    std::uniform_real_distribution<float> uni(0.f, 1.f);
    std::string cur = axiom;
    for (int i = 0; i < iters; i++) {
        std::string next;
        for (char c : cur) {
            if (c == 'F') {
                float r = uni(rng);
                switch (type) {
                case 0: next += (r < .33f) ? "F[+F]F[-F][F]" : (r < .66f) ? "F[+F]F[-F]F" : "F[+F[-F]F][-F[+F]]"; break;
                case 1: next += (r < .5f) ? "F[++F][-F]F" : "F[+F][-F][+F]"; break;
                case 2: next += (r < .34f) ? "F[-F][+F][F]" : (r < .67f) ? "F[+F[-F]][-F[+F]]" : "F[+F][-F]F[+F][-F]"; break;
                case 3: next += (r < .5f) ? "F[+F]F[-F][F]" : "F[-F[+F]]F[+F[-F]]"; break;
                default:next += (r < .25f) ? "F[+F]F[-F]F" : (r < .5f) ? "F[+F][-F]F" : (r < .75f) ? "F[++F]F[--F]F" : "F[+F[+F]][-F[-F]]F"; break;
                }
            }
            else { next += c; }
        }
        cur = next;
    }
    return cur;
}

struct TurtleState { float x, y, angle; int depth; };

void pushLeaf(std::vector<float>& v, float cx, float cy, float size,
    float baseAngle, float lr, float lg, float lb, float windSway)
{
    int rays = 7;
    float spread = 55.f;
    for (int i = 0; i < rays; i++) {
        float t = (float)i / (rays - 1);
        float a = (baseAngle - spread / 2.f + t * spread + windSway) * (3.14159265f / 180.f);
        float len = size * (0.6f + 0.4f * std::sin(t * 3.14159f));
        float ex = cx + len * std::cos(a);
        float ey = cy + len * std::sin(a) * 1.6f;
        float fade = 0.65f + 0.35f * std::sin(t * 3.14159f);
        v.push_back(cx); v.push_back(cy); v.push_back(0.f);
        v.push_back(lr * fade * 0.7f); v.push_back(lg * fade * 0.85f); v.push_back(lb * fade * 0.7f);
        v.push_back(ex); v.push_back(ey); v.push_back(0.f);
        v.push_back(lr * fade); v.push_back(lg * fade); v.push_back(lb * fade);
    }
}

std::vector<float> generateTreeVertices(
    const std::string& sentence,
    float startX, float startY,
    float length, float angleDeg,
    float windTime, float windAmp,
    float leafR, float leafG, float leafB,
    int maxDepth)
{
    std::vector<float> verts;
    std::stack<TurtleState> stk;
    float x = startX, y = startY, angle = 90.f;
    int depth = 0;

    auto toRad = [](float a) { return a * (3.14159265f / 180.f); };

    for (char c : sentence) {
        if (c == 'F') {
            float df = 1.f - (float)depth / (float)(maxDepth + 1);
            float segLen = length * (0.55f + df * 0.45f);
            float wf = (float)depth / (float)(maxDepth + 1);
            float sway = std::sin(windTime + depth * 0.6f) * windAmp * wf;
            float effAng = angle + sway;
            float nx = x + segLen * std::cos(toRad(effAng));
            float ny = y + segLen * std::sin(toRad(effAng)) * 1.6f;

            float tr = 0.28f + df * 0.18f;
            float tg = 0.15f + df * 0.09f;
            float tb = 0.04f + df * 0.02f;

            verts.push_back(x);  verts.push_back(y);  verts.push_back(0.f);
            verts.push_back(tr); verts.push_back(tg); verts.push_back(tb);
            verts.push_back(nx); verts.push_back(ny); verts.push_back(0.f);
            verts.push_back(tr); verts.push_back(tg); verts.push_back(tb);

            if (depth >= 3) {
                float leafSway = std::sin(windTime * 1.4f + x * 30.f) * windAmp * 0.8f;
                float ls = length * 2.2f;
                pushLeaf(verts, nx, ny, ls, effAng + 15.f,
                    leafR, leafG, leafB, leafSway);
                float mx = (x + nx) * 0.5f, my = (y + ny) * 0.5f;
                pushLeaf(verts, mx, my, ls * 0.65f, effAng - 20.f,
                    leafR * 0.85f, leafG * 0.9f, leafB * 0.85f, leafSway * 0.7f);
            }

            x = nx; y = ny;
        }
        else if (c == '+') { angle -= angleDeg; }
        else if (c == '-') { angle += angleDeg; }
        else if (c == '[') { stk.push({ x, y, angle, depth }); depth++; }
        else if (c == ']') {
            auto st = stk.top(); stk.pop();
            x = st.x; y = st.y; angle = st.angle; depth = st.depth;
        }
    }
    return verts;
}

struct Bush { float x, y, phase, speed, r, g, b; };

std::vector<float> generateBushVertices(const std::vector<Bush>& bushes, float windTime)
{
    std::vector<float> v;
    for (const Bush& b : bushes) {
        float sway = std::sin(windTime * 1.4f);

        int stems = 6;
        float baseAngle = 80.f;
        float spread = 70.f;
        float stemLen = 0.055f + b.r * 0.01f;
        for (int s = 0; s < stems; s++) {
            float t = (float)s / (stems - 1);
            float ang = (baseAngle - spread / 2.f + t * spread + sway * 4.f) * (3.14159265f / 180.f);
            float px = b.x, py = b.y;
            for (int seg = 0; seg < 3; seg++) {
                float sl = stemLen * (1.f - seg * 0.25f);
                float nx = px + sl * std::cos(ang);
                float ny = py + sl * std::sin(ang) * 1.6f;
                float bright = 0.5f + seg * 0.15f;
                v.push_back(px); v.push_back(py); v.push_back(0.f);
                v.push_back(b.r * bright * 0.5f); v.push_back(b.g * bright); v.push_back(b.b * bright * 0.5f);
                v.push_back(nx); v.push_back(ny); v.push_back(0.f);
                v.push_back(b.r * bright * 0.6f); v.push_back(b.g * bright * 0.95f); v.push_back(b.b * bright * 0.6f);
                for (int h = -1; h <= 1; h++) {
                    float ha = (ang * 180.f / 3.14159265f + h * 30.f + sway * 5.f) * (3.14159265f / 180.f);
                    float hls = stemLen * 0.35f;
                    v.push_back(nx); v.push_back(ny); v.push_back(0.f);
                    v.push_back(b.r * 0.3f); v.push_back(b.g * 0.85f); v.push_back(b.b * 0.3f);
                    v.push_back(nx + hls * std::cos(ha)); v.push_back(ny + hls * std::sin(ha)); v.push_back(0.f);
                    v.push_back(b.r * 0.4f); v.push_back(b.g); v.push_back(b.b * 0.4f);
                }
                float sideAng = ang + (s % 2 == 0 ? 0.4f : -0.4f) + sway * 0.05f;
                float sx = px + sl * 0.6f * std::cos(ang), sy = py + sl * 0.6f * std::sin(ang);
                float ex = sx + sl * 0.55f * std::cos(sideAng), ey = sy + sl * 0.55f * std::sin(sideAng);
                v.push_back(sx); v.push_back(sy); v.push_back(0.f);
                v.push_back(b.r * 0.35f); v.push_back(b.g * 0.75f); v.push_back(b.b * 0.35f);
                v.push_back(ex); v.push_back(ey); v.push_back(0.f);
                v.push_back(b.r * 0.45f); v.push_back(b.g * 0.85f); v.push_back(b.b * 0.45f);

                px = nx; py = ny;
                ang += (t - 0.5f) * 0.15f + sway * 0.03f;
            }
        }
    }
    return v;
}

// ── Fondo con triángulos ─────────────────────────────────────
std::vector<float> generateSkyGradient() {
    std::vector<float> v;
    int segs = 50;
    float topY = 1.0f, bottomY = -1.0f;
    for (int i = 0; i <= segs; i++) {
        float t = (float)i / segs;
        float yy = topY - t * (topY - bottomY);
        // t^2 para énfasis en azul profundo arriba, más suave abajo
        float r = 0.03f + t * t * 0.65f;
        float g = 0.12f + t * 0.72f;
        float b = 0.50f + t * 0.45f;
        for (int side = 0; side < 2; side++) {
            v.push_back(side == 0 ? -1.f : 1.f);
            v.push_back(yy);
            v.push_back(0.f);
            v.push_back(r); v.push_back(g); v.push_back(b);
        }
    }
    return v;
}

std::vector<float> generateSunGlow(float cx, float cy, float radius) {
    std::vector<float> v;
    int segs = 40;
    v.push_back(cx); v.push_back(cy); v.push_back(0.f);
    v.push_back(0.55f); v.push_back(0.72f); v.push_back(0.60f);
    for (int i = 0; i <= segs; i++) {
        float a = (float)i / segs * 2.f * 3.14159265f;
        v.push_back(cx + radius * std::cos(a));
        v.push_back(cy + radius * std::sin(a));
        v.push_back(0.f);
        v.push_back(0.40f); v.push_back(0.60f); v.push_back(0.48f);
    }
    return v;
}

std::vector<float> generateSunDisk(float cx, float cy, float radius) {
    std::vector<float> v;
    int segs = 40;
    v.push_back(cx); v.push_back(cy); v.push_back(0.f);
    v.push_back(1.0f); v.push_back(0.95f); v.push_back(0.40f);
    for (int i = 0; i <= segs; i++) {
        float a = (float)i / segs * 2.f * 3.14159265f;
        float bright = 1.0f - 0.12f * std::abs(std::sin(a * 2.f));
        v.push_back(cx + radius * std::cos(a));
        v.push_back(cy + radius * std::sin(a));
        v.push_back(0.f);
        v.push_back(1.0f * bright); v.push_back(0.90f * bright); v.push_back(0.35f * bright);
    }
    return v;
}

// ── Sol (líneas dinámicas) ──────────────────────────────────
std::vector<float> generateSunRays(float cx, float cy, float radius) {
    std::vector<float> v;
    for (int i = 0; i < 12; i++) {
        float a = (float)i / 12.f * 2.f * 3.14159265f;
        float ix = cx + radius * 1.15f * std::cos(a);
        float iy = cy + radius * 1.15f * std::sin(a);
        float ox = cx + radius * 2.0f * std::cos(a);
        float oy = cy + radius * 2.0f * std::sin(a);
        v.push_back(ix); v.push_back(iy); v.push_back(0.f);
        v.push_back(0.95f); v.push_back(0.80f); v.push_back(0.25f);
        v.push_back(ox); v.push_back(oy); v.push_back(0.f);
        v.push_back(0.75f); v.push_back(0.60f); v.push_back(0.15f);
    }
    return v;
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(1200, 500, "L-System Forest", NULL, NULL);
    if (!window) { std::cerr << "Error GLFW\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, responsive);
    if (glewInit() != GLEW_OK) { std::cerr << "Error GLEW\n"; return -1; }

    Shader myShader("res/Shader/vertexShader.glsl", "res/Shader/fragmentShader.glsl");

    // ── Árboles ─────────────────────────────────────────────
    struct TMeta {
        float x, y, length, angle;
        int type, iters;
        float lr, lg, lb;
    };

    const int NUM_TREES = 9;
    TMeta metas[NUM_TREES] = {
        {-0.65f,-1.00f, 0.030f, 23.f, 0, 4, 0.12f,0.48f,0.10f},
        { 0.35f,-1.00f, 0.028f, 27.f, 3, 3, 0.11f,0.46f,0.09f},
        { 0.75f,-1.00f, 0.035f, 22.f, 4, 3, 0.14f,0.52f,0.12f},
        {-0.42f,-1.00f, 0.037f, 25.f, 2, 3, 0.16f,0.55f,0.14f},
        { 0.08f,-1.00f, 0.034f, 28.f, 2, 3, 0.15f,0.53f,0.13f},
        {-0.88f,-1.00f, 0.030f, 22.f, 0, 4, 0.20f,0.62f,0.18f},
        {-0.22f,-1.00f, 0.050f, 25.f, 1, 4, 0.18f,0.60f,0.16f},
        { 0.50f,-1.00f, 0.032f, 20.f, 3, 4, 0.22f,0.65f,0.20f},
        { 0.90f,-1.00f, 0.026f, 27.f, 4, 4, 0.17f,0.58f,0.15f},
    };

    struct TreeData {
        std::string sentence;
        TMeta meta;
        int maxDepth;
    };
    std::vector<TreeData> trees;

    for (int i = 0; i < NUM_TREES; i++) {
        std::mt19937 rng(1000u + i * 137u);
        std::string sentence = generateLSystem("F", metas[i].iters, metas[i].type, rng);
        int d = 0, md = 0;
        for (char c : sentence) {
            if (c == '[') { d++; md = std::max(md, d); }
            else if (c == ']') d--;
        }
        trees.push_back({ sentence, metas[i], md });
    }

    // arbustos
    std::vector<Bush> bushes;
    {
        std::mt19937 rng(55u);
        std::uniform_real_distribution<float> ux(-0.98f, 0.98f);
        std::uniform_real_distribution<float> uc(0.f, 1.f);
        std::uniform_real_distribution<float> up(0.f, 6.28f);
        std::uniform_real_distribution<float> us(0.7f, 1.3f);
        for (int i = 0; i < 80; i++) {
            Bush b;
            b.x = ux(rng);
            b.y = -1.0f;
            b.r = 0.05f + uc(rng) * 0.05f;
            b.g = 0.32f + uc(rng) * 0.20f;
            b.b = 0.04f + uc(rng) * 0.04f;
            b.phase = up(rng);
            b.speed = us(rng);
            bushes.push_back(b);
        }
    }

    // ── Fondo (triángulos, estático) ───────────────────────
    auto skyV = generateSkyGradient();          // GL_TRIANGLE_STRIP
    auto glowV = generateSunGlow(0.55f, 0.68f, 0.15f); // GL_TRIANGLE_FAN
    auto diskV = generateSunDisk(0.55f, 0.68f, 0.07f); // GL_TRIANGLE_FAN

    std::vector<float> bgVerts;
    bgVerts.insert(bgVerts.end(), skyV.begin(), skyV.end());
    int skyCount = (int)skyV.size() / 6;
    int glowStart = skyCount;
    bgVerts.insert(bgVerts.end(), glowV.begin(), glowV.end());
    int glowCount = (int)glowV.size() / 6;
    int diskStart = glowStart + glowCount;
    bgVerts.insert(bgVerts.end(), diskV.begin(), diskV.end());
    int diskCount = (int)diskV.size() / 6;

    unsigned int bgVAO, bgVBO;
    glGenVertexArrays(1, &bgVAO);
    glGenBuffers(1, &bgVBO);
    glBindVertexArray(bgVAO);
    glBindBuffer(GL_ARRAY_BUFFER, bgVBO);
    glBufferData(GL_ARRAY_BUFFER, bgVerts.size() * sizeof(float), bgVerts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // ── Líneas (árboles + arbustos + rayos, dinámico) ─────
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Posición del sol en coordenadas de mundo
    float sunX = 0.55f, sunY = 0.68f, sunR = 0.07f;

    while (!glfwWindowShouldClose(window))
    {
        float cur = (float)glfwGetTime();
        deltaTime = cur - lastFrame; lastFrame = cur;
        userInput(window);

        float windTime = cur * 2.5f;
        float windAmp = std::sin(cur * 0.35f) * 2.0f + 2.5f;

        // ── 1) Fondo ──────────────
        myShader.use();
        myShader.setFloat("xOffset", 0.0f);
        myShader.setFloat("yOffset", 0.0f);

        glClearColor(0.63f, 0.82f, 0.95f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(bgVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, skyCount);
        glDrawArrays(GL_TRIANGLE_FAN, glowStart, glowCount);
        glDrawArrays(GL_TRIANGLE_FAN, diskStart, diskCount);

        // ── 2) Líneas dinámicas ───
        std::vector<float> all;
        all.reserve(1200000);

        // Rayos del sol (se mueven con la cámara)
        auto raysV = generateSunRays(sunX, sunY, sunR);
        all.insert(all.end(), raysV.begin(), raysV.end());

        // árboles
        for (auto& td : trees) {
            auto tv = generateTreeVertices(
                td.sentence,
                td.meta.x, td.meta.y,
                td.meta.length, td.meta.angle,
                windTime, windAmp,
                td.meta.lr, td.meta.lg, td.meta.lb,
                td.maxDepth);
            all.insert(all.end(), tv.begin(), tv.end());
        }

        // arbustos
        auto bv = generateBushVertices(bushes, windTime);
        all.insert(all.end(), bv.begin(), bv.end());

        myShader.setFloat("xOffset", posX);
        myShader.setFloat("yOffset", posY);

        glBindVertexArray(VAO);
        glBufferData(GL_ARRAY_BUFFER, all.size() * sizeof(float), all.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_LINES, 0, (GLsizei)(all.size() / 6));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &bgVAO);
    glDeleteBuffers(1, &bgVBO);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void responsive(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }
void userInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    float speed = 1.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) posY -= speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) posY += speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) posX += speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) posX -= speed;
}
