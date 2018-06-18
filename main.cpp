#include "wayland-display.hxx"


int main(int argc, char *argv[]) {
	WaylandDisplay display(640, 480, WL_SHM_FORMAT_ABGR8888);

	display.createBuffer();
	display.createSurface();

	while (true) {
		const int buf = display.acquireBuffer(void);
		display.draw(buf);
		display.presentBuffer(buf);
	}


	return 0;
}
