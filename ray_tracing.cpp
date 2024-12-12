#include <GL/freeglut.h>
#include <vector>
#include <cmath>
#include <string>
#include <iostream>
#include <random>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// 格式化的目标窗口大小
const int WIDTH = 800;
const int HEIGHT = 600;

// 数据结构以定义完整场景信息
struct Vec3
{
    float x, y, z;

    Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z)
    {
    }

    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    float dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }

    Vec3 normalize() const
    {
        float len = std::sqrt(x * x + y * y + z * z);
        return Vec3(x / len, y / len, z / len);
    }
};

static int sphereIdCnt = 0;

struct Sphere
{
    int id;
    Vec3 center;
    float radius;
    Vec3 color;
    float diffuseCoefficient; // 漫反射系数
    enum Material { DIFFUSE, REFLECTIVE } material;

    Sphere(const Vec3& center, float radius, const Vec3& color, float diffuseCoefficient, Material material)
        : id(sphereIdCnt++), center(center), radius(radius), color(color), diffuseCoefficient(diffuseCoefficient),
          material(material)

    {
    }

    bool intersect(const Vec3& rayOrigin, const Vec3& rayDir, float& t) const
    {
        Vec3 oc = rayOrigin - center;
        float a = rayDir.dot(rayDir);
        float b = 2.0f * oc.dot(rayDir);
        float c = oc.dot(oc) - radius * radius;
        float discriminant = b * b - 4 * a * c;
        if (discriminant < 0) return false;
        t = (-b - std::sqrt(discriminant)) / (2.0f * a);
        return t >= 0;
    }
};

static int cubeIdCnt = 0;

struct Cube
{
    int id;
    Vec3 min, max;
    Vec3 color;
    float diffuseCoefficient; // 漫反射系数
    enum Material { DIFFUSE, REFLECTIVE } material;

    unsigned char* textureData;
    int textureWidth, textureHeight, textureChannels;

    Cube(const Vec3& min, const Vec3& max, const Vec3& color, float diffuseCoefficient, Material material,
         const std::string& texturePath)
        : id(cubeIdCnt++), min(min), max(max), color(color), diffuseCoefficient(diffuseCoefficient), material(material)
    {
        textureData = stbi_load(texturePath.c_str(), &textureWidth, &textureHeight, &textureChannels, 0);
        if (!textureData)
        {
            std::cerr << "Failed to load texture: " << texturePath << std::endl;
        }
    }

    Cube(const Vec3& min, const Vec3& max, const Vec3& color, float diffuseCoefficient, Material material)
        : id(sphereIdCnt++), min(min), max(max), color(color), diffuseCoefficient(diffuseCoefficient),
          material(material),
          textureData(nullptr), textureWidth(0),
          textureHeight(0),
          textureChannels(0)
    {
    }

    ~Cube()
    {
        if (textureData != nullptr)
        {
            // stbi_image_free(textureData);
            textureData = nullptr;
        }
    }

    bool intersect(const Vec3& rayOrigin, const Vec3& rayDir, float& t) const
    {
        float tMin = (min.x - rayOrigin.x) / rayDir.x;
        float tMax = (max.x - rayOrigin.x) / rayDir.x;
        if (tMin > tMax) std::swap(tMin, tMax);

        float tyMin = (min.y - rayOrigin.y) / rayDir.y;
        float tyMax = (max.y - rayOrigin.y) / rayDir.y;
        if (tyMin > tyMax) std::swap(tyMin, tyMax);

        if ((tMin > tyMax) || (tyMin > tMax)) return false;
        if (tyMin > tMin) tMin = tyMin;
        if (tyMax < tMax) tMax = tyMax;

        float tzMin = (min.z - rayOrigin.z) / rayDir.z;
        float tzMax = (max.z - rayOrigin.z) / rayDir.z;
        if (tzMin > tzMax) std::swap(tzMin, tzMax);

        if ((tMin > tzMax) || (tzMin > tMax)) return false;
        if (tzMin > tMin) tMin = tzMin;
        if (tzMax < tMax) tMax = tzMax;

        t = tMin;
        return t >= 0;
    }

    Vec3 getTextureColor(const Vec3& hitPoint) const
    {
        if (textureData == nullptr) return color;

        float u, v;
        if (std::abs(hitPoint.x - min.x) < 1e-4) // 左边
        {
            u = (hitPoint.z - min.z) / (max.z - min.z);
            v = 1.0f - (hitPoint.y - min.y) / (max.y - min.y);
        }
        else if (std::abs(hitPoint.x - max.x) < 1e-4) // 右边
        {
            u = 1.0f - (hitPoint.z - min.z) / (max.z - min.z);
            v = 1.0f - (hitPoint.y - min.y) / (max.y - min.y);
        }
        else if (std::abs(hitPoint.y - min.y) < 1e-4) // 底部
        {
            u = (hitPoint.x - min.x) / (max.x - min.x);
            v = 1.0f - (hitPoint.z - min.z) / (max.z - min.z);
        }
        else if (std::abs(hitPoint.y - max.y) < 1e-4) // 顶部
        {
            u = (hitPoint.x - min.x) / (max.x - min.x);
            v = (hitPoint.z - min.z) / (max.z - min.z);
        }
        else if (std::abs(hitPoint.z - min.z) < 1e-4) // 正面
        {
            u = 1.0f - (hitPoint.x - min.x) / (max.x - min.x);
            v = (hitPoint.y - min.y) / (max.y - min.y);
        }
        else if (std::abs(hitPoint.z - max.z) < 1e-4) // 背面
        {
            u = (hitPoint.x - min.x) / (max.x - min.x);
            v = 1.0f - (hitPoint.y - min.y) / (max.y - min.y);
        }

        int x = static_cast<int>(u * textureWidth);
        int y = static_cast<int>(v * textureHeight);

        int index = (y * textureWidth + x) * textureChannels;
        if (index >= 0 && index + 2 < textureWidth * textureHeight * textureChannels)
        {
            float r = textureData[index] / 255.0f;
            float g = textureData[index + 1] / 255.0f;
            float b = textureData[index + 2] / 255.0f;
            return Vec3(r, g, b);
        }
        return color; // Fallback to the default color if the texture data is invalid
    }
};

std::vector<Sphere> spheres;
std::vector<Cube> walls;
Vec3 lightSource(0, 2.5, -4);
float lightColor = 1.1;
std::string texturePath1 = "texture1.png";
std::string texturePath2 = "texture2.png";

void initializeScene()
{
    // 增加球体
    spheres.emplace_back(Vec3(-1.0, 0.0, -5.0), 1.0, Vec3(1.0, 0.9, 0.7),
                         0.8f, Sphere::DIFFUSE); // 漫反射球体
    spheres.emplace_back(Vec3(2.0, 0.0, -6.0), 1.0, Vec3(1.0, 0.9, 0.7),
                         0.5f, Sphere::REFLECTIVE); // 反射球体

    // 增加墙体
    walls.emplace_back(Vec3(-3.5, -1, -7), Vec3(-3, 3, 1), Vec3(1.0, 0.5, 0.0),
                       0.7f, Cube::DIFFUSE, texturePath2); // 左墙
    walls.emplace_back(Vec3(3, -1, -7), Vec3(3.5, 3, 1), Vec3(1.0, 0.5, 0.0),
                       0.7f, Cube::DIFFUSE, texturePath1); // 右墙
    walls.emplace_back(Vec3(-3.5, 3, -7), Vec3(3, 3.5, 1), Vec3(1.0, 0.9, 0.7),
                       0.7f, Cube::REFLECTIVE); // 天花板
    walls.emplace_back(Vec3(-3.5, -1.5, -7), Vec3(3, -1, 1), Vec3(1.0, 0.9, 0.7),
                       0.7f, Cube::REFLECTIVE); // 地板
    walls.emplace_back(Vec3(-3, -1, -7.5), Vec3(3, 3, -7), Vec3(0.8, 0.7, 0.5),
                       0.7f, Cube::REFLECTIVE); // 后墙
}

Vec3 trace(const Vec3& rayOrigin, const Vec3& rayDir, int depth)
{
    if (depth > 3) return Vec3(0, 0, 0); // 超过最大递归深度，返回黑色

    float nearestT = 1e20; // 初始化最近的交点距离
    const Sphere* hitSphere = nullptr; // 初始化命中的球体
    const Cube* hitCube = nullptr; // 初始化命中的立方体
    Vec3 hitNormal; // 初始化命中的法线

    // 检测与球体的交点
    for (const auto& sphere : spheres)
    {
        float t;
        if (sphere.intersect(rayOrigin, rayDir, t) && t < nearestT)
        {
            nearestT = t;
            hitSphere = &sphere;
            hitCube = nullptr;
            hitNormal = (rayOrigin + rayDir * t - sphere.center).normalize();
        }
    }

    // 检测与立方体的交点
    for (const auto& cube : walls)
    {
        float t;
        if (cube.intersect(rayOrigin, rayDir, t) && t < nearestT)
        {
            nearestT = t;
            hitCube = &cube;
            hitSphere = nullptr;

            Vec3 hitPoint = rayOrigin + rayDir * t;
            if (std::abs(hitPoint.x - cube.min.x) < 1e-4) hitNormal = Vec3(-1, 0, 0);
            else if (std::abs(hitPoint.x - cube.max.x) < 1e-4) hitNormal = Vec3(1, 0, 0);
            else if (std::abs(hitPoint.y - cube.min.y) < 1e-4) hitNormal = Vec3(0, -1, 0);
            else if (std::abs(hitPoint.y - cube.max.y) < 1e-4) hitNormal = Vec3(0, 1, 0);
            else if (std::abs(hitPoint.z - cube.min.z) < 1e-4) hitNormal = Vec3(0, 0, -1);
            else if (std::abs(hitPoint.z - cube.max.z) < 1e-4) hitNormal = Vec3(0, 0, 1);
        }
    }

    if (!hitSphere && !hitCube) return Vec3(0.2, 0.2, 0.2); // 背景色

    Vec3 hitPoint = rayOrigin + rayDir * nearestT; // 计算命中的点
    Vec3 normal = hitNormal; // 获取命中的法线

    // 对法线向量添加小的随机扰动
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-0.005, 0.005);
    normal = (normal + Vec3(dis(gen), dis(gen), dis(gen))).normalize();

    Vec3 lightDir = (lightSource - hitPoint).normalize(); // 计算光线方向
    Vec3 viewDir = (rayOrigin - hitPoint).normalize(); // 计算视线方向
    Vec3 reflectDir = (lightDir - normal * 2.0f * normal.dot(lightDir)).normalize(); // 计算反射方向

    // 软阴影检测
    int numShadowSamples = 256; // 阴影样本数量
    int numShadowHits = 0; // 被遮挡的光源样本数量
    std::uniform_real_distribution<> lightDis(-0.35, 0.35);

    for (int i = 0; i < numShadowSamples; ++i)
    {
        Vec3 sampleLightPos = lightSource + Vec3(lightDis(gen), lightDis(gen), lightDis(gen));
        Vec3 sampleLightDir = (sampleLightPos - hitPoint).normalize();
        Vec3 shadowRayOrigin = hitPoint + normal * 0.001f; // 偏移以避免自相交
        bool inShadow = false;

        if (!hitSphere)
        {
            for (const auto& sphere : spheres)
            {
                float t;
                if (sphere.intersect(shadowRayOrigin, sampleLightDir, t) && t > 0.001f)
                {
                    inShadow = true;
                    break;
                }
            }
        }

        // if (!hitCube)
        // {
        //     for (const auto& cube : walls)
        //     {
        //         float t;
        //         if (cube.intersect(shadowRayOrigin, sampleLightDir, t) && t > 0.001f)
        //         {
        //             inShadow = true;
        //             break;
        //         }
        //     }
        // }

        if (inShadow) ++numShadowHits;
    }

    float shadowFactor = 1.0f - static_cast<float>(numShadowHits) / numShadowSamples; // 计算阴影因子

    // Phong光照模型
    float diffuseCoefficient = hitSphere ? hitSphere->diffuseCoefficient : hitCube->diffuseCoefficient;
    Vec3 ambient = (hitSphere ? hitSphere->color : hitCube->getTextureColor(hitPoint)) * 0.3f; // 环境光
    float diffuseStrength = std::max(0.0f, normal.dot(lightDir));
    Vec3 diffuse = (hitSphere ? hitSphere->color : hitCube->getTextureColor(hitPoint))
        * diffuseStrength * lightColor * diffuseCoefficient * shadowFactor; // 增强漫反射光
    float specularStrength = std::pow(std::max(0.0f, viewDir.dot(reflectDir)), 32);
    Vec3 specular = Vec3(0.3, 0.3, 0.3) * specularStrength * lightColor * shadowFactor; // 镜面反射光

    Vec3 color = ambient + diffuse + specular;

    // 处理反射材料
    if ((hitSphere && hitSphere->material == Sphere::REFLECTIVE || hitCube && hitCube->material == Cube::REFLECTIVE)
        && depth < 3)
    {
        Vec3 reflectionDir = (rayDir - normal * 2.0f * normal.dot(rayDir)).normalize();
        Vec3 reflectionColor = trace(hitPoint + reflectionDir * 0.001f, reflectionDir, depth + 1);
        color = reflectionColor * 0.8f + diffuse * 0.55f; // 混合反射颜色和少量漫反射颜色
    }

    return color;
}

void renderScene()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_POINTS);

    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            float u = (x - WIDTH / 2.0f) / HEIGHT;
            float v = -(y - HEIGHT / 2.0f) / HEIGHT;
            Vec3 rayDir = Vec3(u, v, -1).normalize();
            Vec3 color = trace(Vec3(0, 1, 1), rayDir, 0); // 调整摄像头位置
            glColor3f(color.x, color.y, color.z);
            glVertex2i(x, y);
        }
    }

    glEnd();
    glFlush();
}

void setupOpenGL()
{
    glClearColor(0, 0, 0, 1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIDTH, HEIGHT, 0); // 翻转 y 轴方向
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Ray Tracing in OpenGL");

    initializeScene();
    setupOpenGL();

    glutDisplayFunc(renderScene);
    glutMainLoop();
    return 0;
}
