#include <cmath>
#include <iostream>
#include <random>
#include <chrono>
#include <gl/glut.h>

enum { MINE = 9 };
enum { TILE_SIZE = 20 };
enum { MARGIN = 40 };
enum { PADDING = 10 };
enum { BOARD_SIZE = 24 };
enum { MINE_COUNT = 100 };

// Create a list of colors
enum Color {
	RED,
	DARKRED,
	BLUE,
	DARKBLUE,
	DARKGREEN,
	CYAN,
	DARKCYAN,
	YELLOW,
	DARKYELLOW,
	WHITE,
	BLACK,
	DARKGRAY,
	LIGHTGRAY,
	VERYLIGHTGRAY
};

// Fill list above with RGB values
static const struct {float r, g, b;} colors[] = {
	{1, 0, 0},		// red
	{0.5f, 0, 0},		// dark red

	{0, 0, 1},		// blue
	{0, 0, 0.5f},		// dark blue

	{0, 0.5f, 0},		// dark green

	{0, 1, 1},		// cyan
	{0, 0.5f, 0.5f},	// dark  cyan

	{1, 1, 0},		// yellow
	{0.5f, 0.5f, 0},	// dark yellow

	{1, 1, 1},		// White
	{0, 0, 0},          	// black

	{0.25, 0.25, 0.25}, 	// dark gray
	{0.5, 0.5, 0.5},    	// light gray
	{0.75, 0.75, 0.75}  	// very-light gray
};

struct cell {
	int type;
	bool flag;
	bool open;
};

cell board[BOARD_SIZE * BOARD_SIZE];
int death;
int width;
int height;
int num_opened;

// Random int between min and max
int rand_int(int min, int max) {
	static std::default_random_engine re{ std::random_device{}() };
	using Dist = std::uniform_int_distribution<int>;
	static Dist uid{};
	return uid(re, Dist::param_type{ min, max });
}

// https://en.wikibooks.org/wiki/OpenGL_Programming/GLStart/Tut3
void drawRect(int x, int y, float width, float height, const Color& color = DARKGRAY, bool outline = true) {
	glColor3f(colors[color].r, colors[color].g, colors[color].b);
	glBegin(outline ? GL_LINE_STRIP : GL_TRIANGLE_FAN);
	{
		glVertex2i(x, y);
		glVertex2i(x + width, y);
		glVertex2i(x + width, y + height);
		glVertex2i(x, y + height);
	}
	glEnd();
}

void drawCircle(int cx, int cy, float radius, const Color& color = LIGHTGRAY, bool outline = true) {
	glColor3f(colors[color].r, colors[color].g, colors[color].b);
	glBegin(outline ? GL_LINE_LOOP : GL_TRIANGLE_FAN);
	for (int i = 0; i <= 32; i++) {
		float angle = 2 * 3.14159 * i / 32.0f;
		float x = radius * cosf(angle);
		float y = radius * sinf(angle);
		glVertex2f(x + cx, y + cy);
	}
	glEnd();
}

void drawFlag(int x, int y) {
	glColor3f(colors[BLACK].r, colors[BLACK].g, colors[BLACK].b);
	x = (x * TILE_SIZE) + PADDING + 3;
	y = (y * TILE_SIZE) + PADDING + 1;

	// Flag base
	glBegin(GL_QUADS);
	{
		glVertex2i(x, y + 2);
		glVertex2i(x, y + 5);
		glVertex2i(x + 13, y + 2);
		glVertex2i(x + 13, y + 5);
	}
	glEnd();
	glBegin(GL_QUADS);
	{
		glVertex2i(x + 3, y + 4);
		glVertex2i(x + 3, y + 6);
		glVertex2i(x + 10, y + 4);
		glVertex2i(x + 10, y + 6);
	}
	glEnd();

	// Flag line
	glBegin(GL_LINES);
	{
		glVertex2i(x + 7, y + 5);
		glVertex2i(x + 7, y + 15);
	}
	glEnd();

	// Flag
	glColor3f(colors[RED].r, colors[RED].g, colors[RED].b);
	glBegin(GL_TRIANGLES);
	{
		glVertex2i(x + 7, y + 7);
		glVertex2i(x + 7, y + 15);
		glVertex2i(x, y + 11);
	}
	glEnd();
}

void drawMine(int x, int y, bool dead) {
	// If you landed on a mine the background of the current cell is painted red
	if (dead) {
		drawRect(x * TILE_SIZE + PADDING, y * TILE_SIZE + PADDING, TILE_SIZE,
			TILE_SIZE, RED, false);
	}

	// Rough center of the cell
	x = (x * TILE_SIZE) + PADDING + 4;
	y = (y * TILE_SIZE) + PADDING + 4;

	// Main ball
	glBegin(GL_POLYGON);
	{
		glVertex2i(x + 3, y + 1);
		glVertex2i(x + 1, y + 4);
		glVertex2i(x + 1, y + 7);
		glVertex2i(x + 3, y + 10);
		glVertex2i(x + 8, y + 10);
		glVertex2i(x + 10, y + 7);
		glVertex2i(x + 10, y + 4);
		glVertex2i(x + 8, y + 1);
	}
	glEnd();

	// Spikes
	glColor3f(colors[BLACK].r, colors[BLACK].g, colors[BLACK].b);
	glBegin(GL_LINES);
	{
		glVertex2i(x + 5, y - 1);
		glVertex2i(x + 5, y + 12);

		glVertex2i(x - 1, y + 5);
		glVertex2i(x + 12, y + 5);

		glVertex2i(x + 1, y + 1);
		glVertex2i(x + 10, y + 10);

		glVertex2i(x + 1, y + 10);
		glVertex2i(x + 10, y + 1);
	}
	glEnd();

	// White shine on mine
	drawRect(x + 3, y + 5, 2, 2, WHITE, false);
}
// Draw numbers inside board cells, colors taken from
// https://minesweeper.online/
void drawNum(int x, int y, int value) {
	switch (value) {
	case 1:
		glColor3f(colors[BLUE].r, colors[BLUE].g, colors[BLUE].b);
		break;
	case 2:
		glColor3f(colors[DARKGREEN].r, colors[DARKGREEN].g, colors[DARKGREEN].b);
		break;
	case 3:
		glColor3f(colors[RED].r, colors[RED].g, colors[RED].b);
		break;
	case 4:
		glColor3f(colors[DARKBLUE].r, colors[DARKBLUE].g, colors[DARKBLUE].b);
		break;
	case 5:
		glColor3f(colors[DARKRED].r, colors[DARKRED].g, colors[DARKRED].b);
		break;
	case 6:
		glColor3f(colors[DARKYELLOW].r, colors[DARKYELLOW].g,
			colors[DARKYELLOW].b);
		break;
	case 7:
		glColor3f(colors[CYAN].r, colors[CYAN].g, colors[CYAN].b);
		break;
	case 8:
		glColor3f(colors[DARKCYAN].r, colors[DARKCYAN].g, colors[DARKCYAN].b);
		break;
	}
	// Roughly center the number inside the cell
	glRasterPos2i(x * TILE_SIZE + PADDING + 4, y * TILE_SIZE + PADDING + 3);
	// Using Helvetica font with size 18
	glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, '0' + value);
}

void drawFrame(float x, float y, float width, float height, bool border = true) {
	glColor3f(colors[WHITE].r, colors[WHITE].g, colors[WHITE].b);

	// Left and top side - white frame border
	glBegin(GL_LINE_STRIP);
	{
		glVertex2i(x, y);
		glVertex2i(x, y + height - 1);
		glVertex2i(x + width - 1, y + height - 1);
		glVertex2i(x + width - 2, y + height - 2);
		glVertex2i(x + 1, y + height - 2);
		glVertex2i(x + 1, y + 1);
	}
	glEnd();

	glColor3f(colors[LIGHTGRAY].r, colors[LIGHTGRAY].g, colors[LIGHTGRAY].b);

	// Right and bottom side - gray frame border
	glBegin(GL_LINE_STRIP);
	{
		glVertex2f(x + width - 2, y + height - 2);
		glVertex2f(x + width - 2, y + 1);
		glVertex2f(x + 1, y + 1);
		glVertex2f(x, y);
		glVertex2f(x + width - 1, y);
		glVertex2f(x + width - 1, y + height - 1);
	}
	glEnd();

	// Creates the thick "border" look of the outside frame by drawing a new smaller frame
	if (!border) return;
	width = width - 2 * PADDING;
	height = height - 2 * PADDING;

	// Gray highlight on left and top side
	glBegin(GL_LINE_STRIP);
	{
		glVertex2i(x + PADDING, y + PADDING);
		glVertex2i(x + PADDING, y + PADDING + height);
		glVertex2i(x + PADDING + width, y + PADDING + height);
	}
	glEnd();

	// White highlight on bottom and right side
	glColor3f(colors[WHITE].r, colors[WHITE].g, colors[WHITE].b);
	glBegin(GL_LINE_STRIP);
	{
		glVertex2i(x + width + PADDING, y + height + PADDING);
		glVertex2i(x + width + PADDING, y + PADDING);
		glVertex2i(x + PADDING, y + PADDING);
	}
	glEnd();
}

// Upper (Emoji) frame
void drawUpperFrame(int x = 0, int y = 0) {
	static const float frame_width = width;
	static const float frame_height = 2 * MARGIN;
	static const float offset = height - frame_height;

	drawFrame(0, offset, frame_width, frame_height);
}

// Main board frame
void drawLowerFrame(int x = 0, int y = 0) {
	static const float lower_frame_size = width;
	drawFrame(0, 0, lower_frame_size, lower_frame_size);
}

void drawClosedCell(int x, int y) {
	drawFrame(x * TILE_SIZE + PADDING, y * TILE_SIZE + PADDING, TILE_SIZE,
		TILE_SIZE, false);
}
void drawEmoji(int x = 0, int y = 0) {
	// Emoji is the size of 4 cells
	static const float icon_size = 2 * TILE_SIZE;

	// Square frame behind emoji
	drawFrame((width - icon_size) / 2, (height - MARGIN) - icon_size / 2, icon_size, icon_size, false);

	static const float cx = width / 2.0f;
	static const float cy = (height - MARGIN);

	// Face
	drawCircle(x + cx, y + cy, TILE_SIZE * 0.707f, YELLOW, false);
	drawCircle(x + cx, y + cy, TILE_SIZE * 0.707f, BLACK);

	// Eyes
	glBegin(GL_POINTS);
	glVertex2f(-4.707 + cx, 1.707 + cy);
	glVertex2f(4.707 + cx, 1.707 + cy);
	glEnd();

	// Mouth
	glBegin(GL_LINES);
	{
		glVertex2f(-3.707 + cx, -8.707 + cy);
		glVertex2f(3.707 + cx, -8.707 + cy);
	}

	glEnd();
}
void drawOpenCell(int x, int y) {
	drawRect(x * TILE_SIZE + PADDING, y * TILE_SIZE + PADDING, TILE_SIZE,
		TILE_SIZE);
}

int index(int x, int y) { return x + (y * BOARD_SIZE); }

bool isOpen(int x, int y) { return board[index(x, y)].open; }

int getType(int x, int y) { return board[index(x, y)].type; }

void setType(int x, int y, int value) { board[index(x, y)].type = value; }

bool isMine(int x, int y) {
	if (x < 0 || y < 0 || x > BOARD_SIZE - 1 || y > BOARD_SIZE - 1) return false;

	if (getType(x, y) == MINE) return true;
	return false;
}

// Calculate number of mines around given cell
int calcMine(int x, int y) {
	return isMine(x - 1, y - 1)
		+ isMine(x + 1, y - 1)
		+ isMine(x - 1, y)
		+ isMine(x + 1, y)
		+ isMine(x, y - 1)
		+ isMine(x, y + 1)
		+ isMine(x - 1, y + 1)
		+ isMine(x + 1, y + 1);
}

bool isFlag(int x, int y) { return board[index(x, y)].flag; }

bool gameOver() { return death != -1; }

bool isDead(int x, int y) { return death == index(x, y); }

bool hasWon() { return num_opened == MINE_COUNT; }

void openMines(bool open = true) {
	for (int y = 0; y < BOARD_SIZE; y++) {
		for (int x = 0; x < BOARD_SIZE; x++) {
			if (isMine(x, y)) board[index(x, y)].open = open;
		}
	}
}

void openCell(int x, int y) {
	// If outside board
	if (x < 0 || y < 0 || y > BOARD_SIZE - 1 || x > BOARD_SIZE - 1) return;
	// If already opened
	if (isOpen(x, y)) return;

	num_opened--;
	board[index(x, y)].open = true;

	if (isMine(x, y)) {
		death = index(x, y);
		openMines();
		return;
	}

	// Types are 0, number of bombs or MINE - if cell is completely empty, open every neighbour
	if (getType(x, y) == 0) {
		openCell(x - 1, y + 1);
		openCell(x, y + 1);
		openCell(x + 1, y + 1);
		openCell(x - 1, y);
		openCell(x + 1, y);
		openCell(x - 1, y - 1);
		openCell(x, y - 1);
		openCell(x + 1, y - 1);
	}
}

void toggleFlag(int x, int y) { board[index(x, y)].flag = !isFlag(x, y); }

void drawOpen(int x, int y, int type, bool dead) {
	switch (type) {
	case 0:
		drawOpenCell(x, y);
		break;
	case 9:
		if (!dead) {
			drawOpenCell(x, y);
		}
		drawMine(x, y, dead);
		break;
	default:
		drawOpenCell(x, y);
		drawNum(x, y, type);
	}
}

void drawClosed(int x, int y) {
	drawClosedCell(x, y);
	if (isFlag(x, y)) drawFlag(x, y);
}

void draw() {
	for (int y = 0; y < BOARD_SIZE; y++) {
		for (int x = 0; x < BOARD_SIZE; x++) {
			if (isOpen(x, y))
				drawOpen(x, y, getType(x, y), isDead(x, y));
			else
				drawClosed(x, y);
		}
	}
	if (hasWon()) {
		std::cout << "Game WON!" << std::endl;
	}
	// Open all mines on game over
	if (gameOver() || hasWon()) {
		openMines(1);
	}
}

void init() {
	// Initialize empty board
	for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; i++) {
		board[i].type = 0;
		board[i].flag = false;
		board[i].open = false;
	}

	// Add Mines in random locations
	for (int i = 0; i < MINE_COUNT; i++) {
		bool tmp = true;
		do {
			int x = rand_int(0, BOARD_SIZE - 1);
			int y = rand_int(0, BOARD_SIZE - 1);
			if (!isMine(x, y)) {
				tmp = false;
				setType(x, y, MINE);
			}
		} while (tmp);
	}

	// Add numbers to cells depending on nearby mines
	for (int y = 0; y < BOARD_SIZE; y++) {
		for (int x = 0; x < BOARD_SIZE; x++) {
			if (!isMine(x, y)) {
				setType(x, y, calcMine(x, y));
			}
		}
	}

	death = -1;
	num_opened = BOARD_SIZE * BOARD_SIZE;

	// GL Options found on StackOverflow to enable antialieasing etc.
	glClearColor(0.8f, 0.8f, 0.8f, 1.f);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, 0, height, -1.f, 1.f);
	glPointSize(5.0);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// glut callbacks
void display() {
	glClear(GL_COLOR_BUFFER_BIT);
	drawLowerFrame();
	drawUpperFrame();
	drawEmoji();
	draw();

	glutSwapBuffers();
}

// Handle ESC press - Found on StackOverflow
void key(unsigned char key, int x, int y) {
	switch (key) {
	case 27:
		exit(0);
		break;
	}
}

// Roughly the screen position of the emoji
bool requestRestart(int x, int y) {
	return (x >= BOARD_SIZE / 2 - 1 && x <= BOARD_SIZE / 2 + 1 &&
		y >= BOARD_SIZE + 1 && y <= BOARD_SIZE + 3);
}
void mouse(int button, int state, int x, int y) {
	x = (x + PADDING) / TILE_SIZE - 1;
	y = (height - y + PADDING) / TILE_SIZE - 1;

	switch (button) {
	case GLUT_LEFT_BUTTON:
		if (state == GLUT_DOWN) {
			if (requestRestart(x, y)) {
				std::cout << "Restarting..." << std::endl;
				init();
			}
			else if (!gameOver() && !hasWon()) {
				openCell(x, y);
			}
		}
		break;
	case GLUT_RIGHT_BUTTON:
		if (state == GLUT_DOWN) {
			if (gameOver() || hasWon()) break;
			toggleFlag(x, y);
		}
		break;
	}
	std::cout << "X: " << x << " Y: " << y << " FLAG: " << isFlag(x, y)
		<< std::endl;
}

int main(int argc, char** argv) {
	width = BOARD_SIZE * TILE_SIZE + 2 * PADDING;
	height = BOARD_SIZE * TILE_SIZE + 2 * PADDING + 2 * MARGIN;

	glutInit(&argc, argv);
	// Double buffered, RGB mode, multisampling/anti-aliasing enabled
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_MULTISAMPLE);
	glutInitWindowSize(width, height);
	// Center Window to screen
	glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH) - width) / 2,
		(glutGet(GLUT_SCREEN_HEIGHT) - height) / 2);
	glutCreateWindow("Minesweeper");
	glutIdleFunc(display);
	glutDisplayFunc(display);
	glutKeyboardFunc(key);
	glutMouseFunc(mouse);

	init();

	glutMainLoop();
	return 0;
}
