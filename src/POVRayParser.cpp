#define _POSIX_SOURCE
#define _BSD_SOURCE

#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>
#include <boost/assert.hpp>
#include <boost/lexical_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "POVRayParser.h"

using namespace std;
using namespace boost;

namespace RDST {
    SceneDescription POVRayParser::ParseFile(const string &fname) {
        SceneDescription sd;
        struct stat s;
        int fd;
        int r = 0;
        char *f = (char *) MAP_FAILED;

        FILE *file = fopen(fname.c_str(), "r");
        if (!file) {
            perror(fname.c_str());
            r = -1;
            goto cleanup;
        }

        fd = fileno(file);
        if (fstat(fd, &s) == -1) {
            perror("fstat");
            r = -1;
            goto cleanup;
        }

        // Map the matrix file into memory
        f = (char *)mmap(NULL, s.st_size + 1, PROT_WRITE, MAP_PRIVATE, fd, 0);
        if (f == MAP_FAILED) {
            r = -1;
            goto cleanup;
        }
        f[s.st_size] = '\0';

        sd = POVRayParser(f, s.st_size + 1).parse().getScene();

        // Advise the kernel that the memory will be accessed sequentially
        if (madvise(f, s.st_size + 1, MADV_SEQUENTIAL) == -1) {
            perror("madvise");
            r = -1;
            goto cleanup;
        }

cleanup:
        if (file)
            fclose(file);
        if (f != MAP_FAILED && munmap(f, s.st_size) == -1)
            r = -1;
        if (r == -1) {
            exit(EXIT_FAILURE);
        }

        return sd;
    }

    POVRayParser &POVRayParser::parse() {
        _lights.reset(new std::vector<PointLightPtr>());
        _objs.reset(new std::vector<GeomObjectPtr>());
        _spheres.reset(new std::vector<SpherePtr>());
        _triangles.reset(new std::vector<TrianglePtr>());

        string cObject;
        int oBraces = 0;
        int cBraces = 0;
        for (string::iterator i = _str.begin(); i != _str.end(); ) {
            char c = *i++;
            if (c == '\n' || c == '\r')
                c = ' ';
            if (c == '/' && i != _str.end() && *i == '/') {
                for ( ;i != _str.end() && *i != '\n'; i++) {}
                c = ' ';
            }
            cObject.push_back(c);
            if (c == '{') {
                oBraces++;
            } else if (c == '}') {
                cBraces++;
                if (oBraces && oBraces == cBraces) {
                    parseObject(cObject);
                    oBraces = 0;
                    cBraces = 0;
                    cObject.erase(cObject.begin(), cObject.end());
                }
            }
        }
        return *this;
    }

    void POVRayParser::parseObject(const std::string &o) {
        tokenizer fields(o, separator("{}<>, \t"));
        tokenizer::iterator i = fields.begin();
        string type(*i++);
        _ptr = i;
        _end = fields.end();
        if (type == "camera") {
            parseCamera();
        } else if (type == "light_source") {
            parseLight();
        } else {
            _ptr = fields.begin();
            parseGeomObj();
        }
    }

    void POVRayParser::parseCamera() {
        glm::vec3 posVec, upVec, rightVec, lookAtVec;
        for (; _ptr != _end; ) {
            string attr = *_ptr++;
            if (attr == "location") {
                posVec = parseVec3();
            } else if (attr == "up") {
                upVec = parseVec3();
            } else if (attr == "right") {
                rightVec = parseVec3();
            } else if (attr == "look_at") {
                lookAtVec = parseVec3();
            } else {
                BOOST_ASSERT(false);
            }
        }
        BOOST_ASSERT(_ptr == _end);
        _camera = CameraPtr(new Camera(posVec, upVec, rightVec, lookAtVec-posVec));
    }

    float POVRayParser::parseFloat() {
        return lexical_cast<float>(*_ptr++);
    }

    glm::vec3 POVRayParser::parseVec3() {
        float x = parseFloat();
        float y = parseFloat();
        float z = parseFloat();
        return glm::vec3(x, y, z);
    }

    glm::vec4 POVRayParser::parseVec4() {
        glm::vec3 v = parseVec3();
        float f = parseFloat();
        return glm::vec4(v, f);
    }

    void POVRayParser::parseLight() {
        glm::vec3 posVec;
        glm::vec4 color;

        posVec = parseVec3();
        BOOST_ASSERT(*_ptr == "color");
        _ptr++;
        string type(*_ptr++);
        if (type == "rgb") {
            color = glm::vec4(parseVec3(), 1.f);
        } else if (type == "rgbf") {
            color = parseVec4();
        } else {
            BOOST_ASSERT(false);
        }
        BOOST_ASSERT(_ptr == _end);

        _lights->push_back(PointLightPtr(new PointLight(posVec, color)));
    }

    void POVRayParser::parseGeomObj() {
        string type(*_ptr++);
        if (type == "box") {
            parseBox();
        } else if (type == "sphere") {
            parseSphere();
        } else if (type == "cone") {
            parseCone();
        } else if (type == "plane") {
            parsePlane();
        } else if (type == "triangle") {
            parseTriangle();
        }
    }

    void POVRayParser::parseBox() {
        glm::vec3 corner1, corner2;
        Finish finish;
        glm::mat4 xforms(1.f);
        glm::vec4 color;

        corner1 = parseVec3();
        corner2 = parseVec3();
        parseModifiers(color, xforms, finish);
        BOOST_ASSERT(_ptr == _end);
        _objs->push_back(BoxPtr(new Box(corner1, corner2, color,
                        xforms, finish)));
    }
    void POVRayParser::parseSphere() {
        glm::vec3 center;
        float radius;
        Finish finish;
        glm::mat4 xforms(1.f);
        glm::vec4 color;

        center = parseVec3();
        radius = parseFloat();
        parseModifiers(color, xforms, finish);
        BOOST_ASSERT(_ptr == _end);
        _spheres->push_back(SpherePtr(new Sphere(center, radius, color, xforms,
                        finish)));
    }
    void POVRayParser::parseCone() {
        glm::vec3 end1, end2;
        float radius1, radius2;
        Finish finish;
        glm::mat4 xforms(1.f);
        glm::vec4 color;

        end1 = parseVec3();
        radius1 = parseFloat();
        end2 = parseVec3();
        radius2 = parseFloat();
        parseModifiers(color, xforms, finish);
        BOOST_ASSERT(_ptr == _end);
        _objs->push_back(ConePtr(new Cone(end1, radius1, end2, radius2, color,
                        xforms, finish)));
    }
    void POVRayParser::parsePlane() {
        glm::vec3 normal;
        float distance;
        Finish finish;
        glm::mat4 xforms(1.f);
        glm::vec4 color;

        normal = glm::normalize(parseVec3());
        distance = parseFloat();
        parseModifiers(color, xforms, finish);
        BOOST_ASSERT(_ptr == _end);
        _objs->push_back(PlanePtr(new Plane(normal, distance, color,
                        xforms, finish)));
    }
    void POVRayParser::parseTriangle() {
        glm::vec3 vert1, vert2, vert3;
        Finish finish;
        glm::mat4 xforms(1.f);
        glm::vec4 color;

        vert1 = parseVec3();
        vert2 = parseVec3();
        vert3 = parseVec3();
        parseModifiers(color, xforms, finish);
        BOOST_ASSERT(_ptr == _end);
        _triangles->push_back(TrianglePtr(new Triangle(vert1, vert2, vert3,
                        color, xforms, finish)));
    }

    void POVRayParser::parseModifiers(glm::vec4 &color, glm::mat4 &xforms,
            Finish &finish) {
        for (; _ptr != _end; ) {
            string op(*_ptr++);
            if (op == "translate") {
                xforms = glm::translate(glm::mat4(1.f),
                        parseVec3()) * xforms;
            } else if (op == "rotate") {
                glm::vec3 axisRotsVec(parseVec3());
                xforms = glm::rotate(glm::mat4(1.f), axisRotsVec.x,
                        glm::vec3(1.f, 0.f, 0.f)) * xforms;
                xforms = glm::rotate(glm::mat4(1.f), axisRotsVec.y,
                        glm::vec3(0.f, 1.f, 0.f)) * xforms;
                xforms = glm::rotate(glm::mat4(1.f), axisRotsVec.z,
                        glm::vec3(0.f, 0.f, 1.f)) * xforms;
            } else if (op == "scale") {
                boost::tokenizer<separator>::iterator tPtr(_ptr);
                tPtr++;
                glm::vec3 scale(1.f);
                if (tPtr->find_first_not_of("1234567890.-") == string::npos) {
                    scale = parseVec3();
                } else {
                    scale = glm::vec3(parseFloat());
                }
            } else if (op == "pigment") {
                BOOST_ASSERT(*_ptr == "color");
                _ptr++;
                string type(*_ptr++);
                if (type == "rgb") {
                    color = glm::vec4(parseVec3(), 1.f);
                } else if (type == "rgbf") {
                    color = parseVec4();
                }
            } else if (op == "finish") {
                finish = parseFinish();
            } else {
                BOOST_ASSERT(false);
            }
        }
    }

    Finish POVRayParser::parseFinish() {
        float ambient, diffuse, specular, roughness, reflection, refraction, ior;
        ambient = diffuse = specular = roughness = reflection = refraction = ior = 0.f;
        boost::tokenizer<separator>::iterator oPtr;
        for (; _ptr != _end; ) {
            oPtr = _ptr;
            string op(*_ptr++);
            if (op == "ambient") {
                ambient = parseFloat();
            } else if (op == "diffuse") {
                diffuse = parseFloat();
            } else if (op == "specular") {
                specular = parseFloat();
            } else if (op == "roughness") {
                roughness = parseFloat();
            } else if (op == "reflection") {
                reflection = parseFloat();
            } else if (op == "refraction") {
                refraction = parseFloat();
            } else if (op == "ior") {
                ior = parseFloat();
            } else {
                _ptr = oPtr;
                break;
            }
        }
        return Finish(ambient, diffuse, specular, roughness, reflection,
                refraction, ior);
    }
}
