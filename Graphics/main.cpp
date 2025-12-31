// ============================================================
// main.cpp
// Entry point for the AI Battle Simulation project
// ============================================================

#include <cstdlib>
#include <ctime>
#include <cmath>
#include "glut.h"
#include "Definitions.h"
#include "Game.h"

// ------------------------------------------------------------
// Global game pointer
// ------------------------------------------------------------
static Game* g = nullptr;

// ------------------------------------------------------------
// OpenGL initialization
// ------------------------------------------------------------
static void initGL() {
    glClearColor(0.3, 0.3, 0.3, 0.0);  // gray background
    glOrtho(0, MSZ, 0, MSZ, -1, 1);    // top-down orthographic view
}

// ------------------------------------------------------------
// Display callback
// ------------------------------------------------------------
static void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    if (g) g->render();
    glutSwapBuffers();
}

// ------------------------------------------------------------
// Idle callback (main game loop trigger)
// ------------------------------------------------------------
static void idle() {
    if (g) g->update();
    glutPostRedisplay();
}

// ------------------------------------------------------------
// Main entry point
// ------------------------------------------------------------
int main(int argc, char* argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(WIN_W, WIN_H);
    glutInitWindowPosition(200, 100);
    glutCreateWindow("Final Project AI Course");

    // Register callbacks
    glutDisplayFunc(display);
    glutIdleFunc(idle);

    // Initialize OpenGL and game
    initGL();
    g = new Game();
    g->init();

    // Enter main event loop
    glutMainLoop();
    return 0;
}
