
uvdClient:
	g++ -std=c++14 -o uvdClient.out  uvdClient.cpp DistortionPlayer.cpp uvdClient_demo.cpp originWindow.cpp distortionWindow.cpp -g -lSDL2 -lpthread `pkg-config --cflags --libs opencv`