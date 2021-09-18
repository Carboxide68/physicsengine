links = -lz -lGL -lGLU -lGLEW -lglfw3 -lrt -lm -ldl -lX11 -lXdmcp -lIrrXML -lassimp -lpthread -lxcb -lXau
files = main.cpp PhysicsHandler.cpp SoftBody.cpp Drawer.cpp globals.cpp

app: libdebug FORCE
	sleep 1s
	g++ -o app $(files) $(wildcard Oxide/Oxide/build/*.o) -g -DDEBUG -I vendor/ -I Oxide/Oxide/src/ -I Oxide/Oxide/vendor/ -LOxide/Oxide/vendor/lib -L/usr/local/lib $(links) -std=c++2a

profiling: libprofiling FORCE
	sleep 1s
	g++ -o app $(files) $(wildcard Oxide/Oxide/build/*.o) -g -DTRACY_ENABLE -I vendor/ -I Oxide/Oxide/src/ -I Oxide/Oxide/vendor/ -LOxide/Oxide/vendor/lib -L/usr/local/lib $(links) -std=c++2a -O2 -march=native

release: librelease FORCE
	sleep 1s
	g++ -o app $(files) $(wildcard Oxide/Oxide/build/*.o) -I vendor/ -I Oxide/Oxide/src/ -I Oxide/Oxide/vendor/ -LOxide/Oxide/vendor/lib -L/usr/local/lib $(links) -std=c++2a -O3 -march=native

libdebug:
	$(MAKE) -C Oxide/Oxide/ debug

libprofiling:
	$(MAKE) -C Oxide/Oxide/ profiling

librelease:
	$(MAKE) -C Oxide/Oxide/ release

precompile: FORCE
	g++ -o precompiled.cc Sandbox/main.cpp -I Oxide/Oxide/src/ -I Oxide/Oxide/vendor/ -E -std=c++2a

clean: FORCE
	rm -f Oxide/Oxide/build/* || true

FORCE:
