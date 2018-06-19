#include "wayland-display.hxx"


int main(int argc, char *argv[]) {
	WaylandDisplay display(640, 480, WL_SHM_FORMAT_ABGR8888);

	display.createBuffer();
	display.createSurface();
	return 0;

	while (true) {
		const int buf = display.acquireBuffer();
		display.draw(buf);
		display.presentBuffer(buf);
	}


	return 0;
}
