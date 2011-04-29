all : src/raytrace

src/blueprint.lua : 
	init.pl /opt/NVIDIA_GPU_Computing_SDK

src/raytrace : cubuild/cubuild src/blueprint.lua
	build.pl release /opt/cuda/bin

cubuild/cubuild :
	cd cubuild; make

.PHONY: clean 
clean:
	build.pl clean
