all : src/raytrace

src/raytrace : cubuild/cubuild src/blueprint.lua
	build.pl release /opt/cuda/bin

cubuild/cubuild :
	cd cubuild; make

.PHONY: clean 
clean:
	build.pl clean
