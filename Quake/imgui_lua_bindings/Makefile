all:
	./generate_imgui_bindings.pl < ../imgui/imgui.h > imgui_iterator.h
	g++ -c -std=c++11 -I../imgui -I../lua imgui_lua_bindings.cpp -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS -o imgui_lua_bindings.o
