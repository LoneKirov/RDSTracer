#ifndef POV_PARSER_H
#define POV_PARSER_H

#include <string>
#include <vector>
#include <boost/tokenizer.hpp>
#include "RDSScene.h"

namespace RDST {
    class POVRayParser {
        private:
            typedef boost::char_separator<char> separator;
            typedef boost::tokenizer<separator > tokenizer;

            std::string _str;
            tokenizer::iterator _ptr;
            tokenizer::iterator _end;

            CameraPtr _camera;
            boost::shared_ptr<std::vector<PointLightPtr> > _lights;
            boost::shared_ptr<std::vector<GeomObjectPtr> > _objs;
            boost::shared_ptr<std::vector<SpherePtr> > _spheres;
            boost::shared_ptr<std::vector<TrianglePtr> > _triangles;


            void parseObject(const std::string &);
            float parseFloat();
            glm::vec3 parseVec3();
            glm::vec4 parseVec4();
            void parseCamera();
            void parseLight();
            void parseGeomObj();
            void parseBox();
            void parseSphere();
            void parseCone();
            void parsePlane();
            void parseTriangle();
            void parseModifiers(glm::vec4 &, glm::mat4 &, Finish &);
            Finish parseFinish();

        public:
            POVRayParser(char *str, size_t len) :
                _str(str) {}

            POVRayParser &parse();
            SceneDescription getScene() {
                return SceneDescription(_camera, _lights, _objs, _spheres,
                        _triangles);
            }
            static SceneDescription ParseFile(const std::string &fname);
    };

};

#endif
