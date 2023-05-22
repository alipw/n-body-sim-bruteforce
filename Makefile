build:
	@echo "Building gravitysim..."
	g++ -O3 -Wall main.cpp -lsfml-graphics -lsfml-window -lsfml-system

build-run:
	@echo "building..."
	g++ -O3 -Wall main.cpp -lsfml-graphics -lsfml-window -lsfml-system && ./a.out



