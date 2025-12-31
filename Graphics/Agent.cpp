#include "Agent.h"
#include "State.h"
#include "Map.h"
#include "glut.h"
#include "Order.h"
#include "MoveToTarget.h"
#include "Idle.h"
#include "Globals.h"
#include "Warrior.h"
#include <algorithm> // for clampValue

// ============================================================
// Manual clamp (for older C++ versions)
// ============================================================
template <typename T>
inline T clampValue(T v, T lo, T hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// ============================================================
// Constructor / Destructor
// ============================================================
Agent::Agent(TeamColor t, int r, int c) : team(t) { pos = { r, c }; }
Agent::~Agent() {}

// ============================================================
// Update
// ============================================================
void Agent::update(Map& world)
{
    // Skip update if agent is dead
    if (!alive) return;

    computeVisibility(world);

    if (current)
        current->Transition(this);
}

// ============================================================
// Render
// ============================================================
void Agent::render() const
{
    // --- Team color ---
    if (!alive)
        glColor3d(0.3, 0.3, 0.3);
    else if (team == TEAM_ORANGE)
        glColor3d(0.95, 0.45, 0.05);
    else
        glColor3d(0.10, 0.55, 0.95);

    // --- Body ---
    const double inset = 0.06;
    const double x0 = pos.c + inset;
    const double y0 = pos.r + inset;
    const double x1 = pos.c + 1 - inset;
    const double y1 = pos.r + 1 - inset;

    glBegin(GL_POLYGON);
    glVertex2d(x0, y0);
    glVertex2d(x0, y1);
    glVertex2d(x1, y1);
    glVertex2d(x1, y0);
    glEnd();

    // --- Outline ---
    glColor3d(0, 0, 0);
    glBegin(GL_LINE_LOOP);
    glVertex2d(x0, y0);
    glVertex2d(x0, y1);
    glVertex2d(x1, y1);
    glVertex2d(x1, y0);
    glEnd();

    // --- Role Letter ---
    {
        char ch = roleLetter() && roleLetter()[0] ? roleLetter()[0] : 'X';
        const double nominalH = 119.0;
        const double desiredH = 0.70;
        const double s = desiredH / nominalH;
        int wStroke = glutStrokeWidth(GLUT_STROKE_ROMAN, ch);
        double w = wStroke * s;
        double cx = pos.c + 0.5;
        double cy = pos.r + 0.5;

        glColor3d(0, 0, 0);
        glLineWidth(2.0);

        glPushMatrix();
        glTranslated(cx - w / 2.0, cy - desiredH / 2.0, 0.0);
        glScaled(s, s, 1.0);
        glutStrokeCharacter(GLUT_STROKE_ROMAN, ch);
        glPopMatrix();

        glLineWidth(1.0);
    }

    // ============================================================
    // Status Bars: HP / Ammo / Grenades
    // ============================================================
    double barWidth = 0.9;
    double barHeight = 0.08;
    double x = pos.c + 0.05;
    double y = pos.r - 0.25;

    // --- HP bar (green) ---
    double hpRatio = clampValue(hp / 100.0, 0.0, 1.0);
    glColor3d(0.0, 1.0, 0.0);
    glBegin(GL_POLYGON);
    glVertex2d(x, y);
    glVertex2d(x, y + barHeight);
    glVertex2d(x + barWidth * hpRatio, y + barHeight);
    glVertex2d(x + barWidth * hpRatio, y);
    glEnd();

    // HP outline
    glColor3d(0, 0, 0);
    glBegin(GL_LINE_LOOP);
    glVertex2d(x, y);
    glVertex2d(x + barWidth, y);
    glVertex2d(x + barWidth, y + barHeight);
    glVertex2d(x, y + barHeight);
    glEnd();

    // --- Ammo + Grenade bars (Warrior only) ---
    if (dynamic_cast<const Warrior*>(this))
    {
        // Ammo bar (red)
        double ammoRatio = clampValue(double(bullets) / double(maxBullets), 0.0, 1.0);
        double ammoY = y - barHeight - 0.1;

        glColor3d(1.0, 0.0, 0.0);
        glBegin(GL_POLYGON);
        glVertex2d(x, ammoY);
        glVertex2d(x, ammoY + barHeight);
        glVertex2d(x + barWidth * ammoRatio, ammoY + barHeight);
        glVertex2d(x + barWidth * ammoRatio, ammoY);
        glEnd();

        // Ammo outline
        glColor3d(0, 0, 0);
        glBegin(GL_LINE_LOOP);
        glVertex2d(x, ammoY);
        glVertex2d(x + barWidth, ammoY);
        glVertex2d(x + barWidth, ammoY + barHeight);
        glVertex2d(x, ammoY + barHeight);
        glEnd();

        // Grenade indicators (3 segments)
        int grenades = dynamic_cast<const Warrior*>(this)->getGrenades();
        double grenadeY = ammoY - barHeight - 0.1;
        double segment = barWidth / 3.0;

        for (int i = 0; i < 3; ++i)
        {
            if (i < grenades)
                glColor3d(0.0, 0.3, 1.0); // active grenade (blue)
            else
                glColor3d(0.7, 0.7, 0.7); // used grenade (gray)

            glBegin(GL_POLYGON);
            glVertex2d(x + i * segment, grenadeY);
            glVertex2d(x + (i + 1) * segment - 0.02, grenadeY);
            glVertex2d(x + (i + 1) * segment - 0.02, grenadeY + barHeight / 2);
            glVertex2d(x + i * segment, grenadeY + barHeight / 2);
            glEnd();
        }
    }
}

// ============================================================
// Movement Logic
// ============================================================
void Agent::stepTowardTarget(const Map& world)
{
    if (pos.r == target.r && pos.c == target.c) {
        moving = false;
        return;
    }

    int dr = (target.r > pos.r) ? 1 : (target.r < pos.r) ? -1 : 0;
    int dc = (target.c > pos.c) ? 1 : (target.c < pos.c) ? -1 : 0;

    // Try row step
    int nr = pos.r + dr, nc = pos.c;
    if (dr != 0 && world.inBounds(nr, nc)) {
        auto ct = world.at(nr, nc);
        if (ct != ROCK && ct != WATER) {
            pos.r = nr;
            moving = true;
            return;
        }
    }

    // Try col step
    nr = pos.r;
    nc = pos.c + dc;
    if (dc != 0 && world.inBounds(nr, nc)) {
        auto ct = world.at(nr, nc);
        if (ct != ROCK && ct != WATER) {
            pos.c = nc;
            moving = true;
            return;
        }
    }

    moving = false;
}

// ============================================================
// FSM State Management
// ============================================================
void Agent::setState(State* s)
{
    if (current)
        current->OnExit(this);

    current = s;

    if (current)
        current->OnEnter(this);
}

// ============================================================
// Path Following
// ============================================================
bool Agent::advanceAlongPath(const Map& world)
{
    if (pathIndex < 0 || pathIndex >= (int)path.size()) {
        moving = false;
        return false;
    }

    Vec2i next = path[pathIndex];
    CellType ct = world.at(next.r, next.c);

    if (ct == ROCK || ct == WATER) {
        moving = false;
        return false;
    }

    // Delay for animation
    if (moveDelayCounter > 0) {
        moveDelayCounter--;
        return true;
    }

    moveDelayCounter = MOVE_DELAY;
    pos = next;
    pathIndex++;

    if (pathIndex >= (int)path.size()) {
        moving = false;
        return false;
    }

    return true;
}

// ============================================================
// Visibility
// ============================================================
void Agent::computeVisibility(const Map& world)
{
    if (vis.size() != size_t(MSZ * MSZ))
        vis.assign(MSZ * MSZ, 0);

    std::fill(vis.begin(), vis.end(), 0);

    const int R = getSightRange();
    const int r0 = pos.r;
    const int c0 = pos.c;

    vis[r0 * MSZ + c0] = 1;

    int rMin = std::max(0, r0 - R);
    int rMax = std::min(MSZ - 1, r0 + R);
    int cMin = std::max(0, c0 - R);
    int cMax = std::min(MSZ - 1, c0 + R);

    for (int r = rMin; r <= rMax; ++r)
    {
        for (int c = cMin; c <= cMax; ++c)
        {
            int dr = r - r0;
            int dc = c - c0;
            if (dr * dr + dc * dc > R * R) continue;

            if (world.hasLineOfSight({ r0, c0 }, { r, c }))
                vis[r * MSZ + c] = 1;
        }
    }
}

// ============================================================
// Order Handling
// ============================================================
void Agent::receiveOrder(const Order& o)
{
    if (o.type == OrderType::MOVE ||
        o.type == OrderType::ATTACK ||
        o.type == OrderType::DEFEND)
    {
        setTarget(o.targetRow, o.targetCol);
        setState(new MoveToTarget());
    }
    else {
        setState(new Idle());
    }
}
