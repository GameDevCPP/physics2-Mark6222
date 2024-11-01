#include <cstdint>

#include "Box2D/Collision/Shapes/b2PolygonShape.h"
#include "Box2D/Dynamics/b2Body.h"
#include "Box2D/Dynamics/b2Fixture.h"
#include "Box2D/Dynamics/b2World.h"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/RenderWindow.hpp"
#include "SFML/System/Clock.hpp"
#include "SFML/System/Vector2.hpp"
#include "SFML/Window/Event.hpp"
//main.cpp
b2World *world;
// 1 sfml unit = 30 physics units
const float physics_scale = 30.0f;
// inverse of physics_scale, useful for calculations
const float physics_scale_inv = 1.0f / physics_scale;
// Magic numbers for accuracy of physics simulation
const int32 velocityIterations = 6;
const int32 positionIterations = 2;
const uint16_t gameWidth = 700;
const uint16_t gameHeight = 720;
using namespace sf;
std::vector<b2Body *> bodies;
std::vector<RectangleShape *> sprites;

//Convert from b2Vec2 to a Vector2f
inline const Vector2f bv2_to_sv2(const b2Vec2 &in) {
    return Vector2f(in.x * physics_scale, (in.y * physics_scale));
}

//Convert from Vector2f to a b2Vec2
inline const b2Vec2 sv2_to_bv2(const Vector2f &in) {
    return b2Vec2(in.x * physics_scale_inv, (in.y * physics_scale_inv));
}

//Convert from screenspace.y to physics.y (as they are the other way around)
inline const Vector2f invert_height(const Vector2f &in) {
    return Vector2f(in.x, gameHeight - in.y);
}

//Create a Box2D body with a box fixture
b2Body *CreatePhysicsBox(b2World &World, const bool dynamic, const Vector2f &position, const Vector2f &size) {
    b2BodyDef BodyDef;
    //Is Dynamic(moving), or static(Stationary)
    BodyDef.type = dynamic ? b2_dynamicBody : b2_staticBody;
    BodyDef.position = sv2_to_bv2(position);
    //Create the body
    b2Body *body = World.CreateBody(&BodyDef);

    //Create the fixture shape
    b2PolygonShape Shape;
    Shape.SetAsBox(sv2_to_bv2(size).x * 0.5f, sv2_to_bv2(size).y * 0.5f);
    b2FixtureDef FixtureDef;
    //Fixture properties
    FixtureDef.density = dynamic ? 10.f : 0.f;
    FixtureDef.friction = dynamic ? 0.8f : 1.f;
    FixtureDef.restitution = 1.0;
    FixtureDef.shape = &Shape;
    //Add to body
    body->CreateFixture(&FixtureDef);
    return body;
}

// Create a Box2d body with a box fixture, from a sfml::RectangleShape
b2Body *CreatePhysicsBox(b2World &world, const bool dynamic, const RectangleShape &rs) {
    return CreatePhysicsBox(world, dynamic, rs.getPosition(), rs.getSize());
}

void init() {
    b2Vec2 gravity(0.0f, -9.8f);
    world = new b2World(gravity);

    for (int i = 1; i < 11; ++i) {
        // Create SFML shapes for each box
        auto s = new RectangleShape();
        s->setPosition(Vector2f(i * (gameWidth / 12.f), gameHeight * .7f));
        s->setSize(Vector2f(50.0f, 50.0f));
        s->setOrigin(Vector2f(25.0f, 25.0f));
        s->setFillColor(Color::White);
        sprites.push_back(s);

        // Create a dynamic physics body for the box
        auto b = CreatePhysicsBox(*world, true, *s);
        // Give the box a spin
        b->ApplyAngularImpulse(5.0f, true);
        bodies.push_back(b);
    }
    // Wall Dimensions
    Vector2f walls[] = {
        Vector2f(gameWidth * .5f, 10.f),
        {gameWidth, 20.0f},
        Vector2f(gameWidth * .5f, gameHeight - 10.f),
        {gameWidth, 20.0f},
        Vector2f(10.f, gameHeight * .5f),
        {20.0f, gameHeight},
        Vector2f(gameWidth - 10.f, gameHeight * .5),
        {20.0f, gameHeight}
    };
    // Build Walls
    int wallSize = -1;
    int wallIndex = 0;
    for (int i = 0; i < 4; ++i) {
        wallSize = wallSize + 2;
        auto s = new RectangleShape(walls[wallSize]);
        s->setPosition(walls[wallIndex]);
        s->setFillColor(Color::Green);
        s->setOrigin(walls[wallSize] * .5f);
        sprites.push_back(s);

        auto b = CreatePhysicsBox(*world, false, walls[wallIndex], walls[wallSize]);
        bodies.push_back(b);
        wallIndex = wallIndex + 2;
    }
}

void Update(RenderWindow &window) {
    static sf::Clock clock;
    float dt = clock.restart().asSeconds();
    Event event;
    while (window.pollEvent(event)) {
        if (event.type == Event::Closed) {
            window.close();
            return;
        }
    }
    // Step Physics world by dt (non-fixed timestep) - THIS DOES ALL THE ACTUAL SIMULATION, DON'T FORGET THIS!
    world->Step(dt, velocityIterations, positionIterations);

    for (int i = 0; i < bodies.size(); ++i) {
        // Sync Sprites to physics position
        sprites[i]->setPosition(invert_height(bv2_to_sv2(bodies[i]->GetPosition())));
        // Sync Sprites to physics Rotation
        sprites[i]->setRotation((180 / b2_pi) * bodies[i]->GetAngle());
    }
}

void Render(RenderWindow &window) {
    for (const auto &sprite: sprites) {
        window.draw(*sprite);
    }
}

int main() {
    init();
    RenderWindow window(VideoMode(gameWidth, gameHeight), "Physics Simulation");

    while (window.isOpen()) {
        window.clear();
        Update(window);
        Render(window);
        window.display();
    }
    return 0;
}
