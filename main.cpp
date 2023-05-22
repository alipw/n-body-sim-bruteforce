#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>
#include <iostream>
#include <math.h>
#include <random>
#define FPS_LIMIT 165

#define LOG(x) std::cout << x << std::endl;

#define SCREENX 1000
#define SCREENY 1000

const sf::Vector2f NO_FORCE = sf::Vector2f{0., 0.};
const float MIN_VISIBLE_RADIUS = 0.0113382;
const float BIG_G = 0.000000000066742;

inline sf::Vector2f toCartesian(sf::Vector2f from, int screenx, int screeny)
{
  return sf::Vector2f{from.x - screenx / 2, -from.y + screeny / 2};
}

inline sf::Vector2f toSFML(sf::Vector2f from, int screenx, int screeny)
{
  return sf::Vector2f{from.x + (screenx / 2), -from.y - (screeny / 2)};
}

// for linear movements
inline sf::Vector2f toSFML(sf::Vector2f from)
{
  return sf::Vector2f{-from.x, from.y};
}

inline float norm(sf::Vector2f a)
{
  return sqrt(pow(a.x, 2.f) + pow(a.y, 2.f));
}

class GravityBody : public sf::CircleShape
{
public:
  sf::Vector2f velocity;
  float mass = 100.;

  GravityBody(sf::Vector2f vel = NO_FORCE, float m = 100., float radius = 100.) : CircleShape(radius)
  {
    velocity = vel;
    mass = m;
  }

  inline void updateForce(std::vector<GravityBody> &nbody, int selfindex, sf::Vector2f accel = NO_FORCE, float intervalsec = 0.016)
  {
    auto dt = intervalsec;
    sf::Vector2f pos = getPosition();

    const auto &self = nbody[selfindex];
    const auto selfpos = self.getPosition();

    for (int i = 0; i < selfindex; i++)
    {
      const auto &curbody = nbody[i];
      const auto curbodypos = curbody.getPosition();

      const auto gravity_force = self.mass * curbody.mass / ((curbodypos.x - selfpos.x) * (curbodypos.x - selfpos.x) + (curbodypos.y - selfpos.y) * (curbodypos.y - selfpos.y)) * BIG_G;
      accel += gravity_force * (curbodypos - selfpos);
    }

    for (int i = selfindex + 1; i < nbody.size(); i++)
    {
      const auto &curbody = nbody[i];
      const auto curbodypos = curbody.getPosition();

      const auto gravity_force = self.mass * curbody.mass / ((curbodypos.x - selfpos.x) * (curbodypos.x - selfpos.x) + (curbodypos.y - selfpos.y) * (curbodypos.y - selfpos.y)) * BIG_G;
      accel += gravity_force * (curbodypos - selfpos);
    }

    velocity += (accel * dt) / mass;
    pos += (velocity * dt);
    setPosition(pos);
  }
};

int main()
{
  std::random_device rd;  // Obtain a random seed from the hardware
  std::mt19937 gen(rd()); // Seed the generator
  std::normal_distribution<float> rand_velocity(-200.f, 200.f);
  std::normal_distribution<float> rand_position(-20000.f, 20000.f);
  std::uniform_int_distribution<int> rand_color(0, 255);
  std::uniform_real_distribution<float> rand_mass(2000000000000000, 20000000000000000);

  // window initialization
  sf::RenderWindow window(sf::VideoMode(SCREENX, SCREENY), "crappy gravity simulator");
  window.setFramerateLimit(FPS_LIMIT);
  sf::Clock clock;
  sf::Clock clockdbg;
  sf::View view;

  // window view variables initialization
  float zoom_step = 1.1;
  float zoom_factor = 1;
  float move_coefficient = 0.5;
  bool isDragging = false;
  sf::Vector2f startDragPos;

  std::vector<GravityBody> bodylist = std::vector<GravityBody>{};

  // physics body declaration
  for (int i = 0; i < 4000; i++)
  {

    auto random_color = sf::Color(rand_color(gen), rand_color(gen), rand_color(gen));
    GravityBody circular(sf::Vector2f{0, rand_velocity(gen)}, 2000000000000000., 100.);
    const auto pos = sf::Vector2f(rand_position(gen), rand_position(gen));
    circular.setPosition(toSFML(pos, SCREENX, SCREENY));
    circular.velocity = sf::Vector2f{rand_velocity(gen), rand_velocity(gen)};
    circular.setFillColor(random_color);
    bodylist.push_back(circular);
  }
  // auto random_color = sf::Color(rand_color(gen), rand_color(gen), rand_color(gen));
  // GravityBody circular(sf::Vector2f{0, rand_velocity(gen)}, 200000000000000000000., 10000.);
  // const auto pos = sf::Vector2f(0, 0);
  // circular.setPosition(pos);
  // circular.velocity = sf::Vector2f{0, 0};
  // circular.setFillColor(random_color);
  // bodylist.push_back(circular);

  auto i = 0;
  while (window.isOpen())
  {
    sf::Event event;
    while (window.pollEvent(event))
    {
      switch (event.type)
      {
      case sf::Event::Closed:
      {
        window.close();
        break;
      }

      case sf::Event::MouseWheelScrolled:
      {
        if (event.mouseWheelScroll.delta < 0)
        {
          view.zoom(zoom_step);
        }
        else if (event.mouseWheelScroll.delta > 0)
        {
          view.zoom(1 / zoom_step);
        }
        zoom_factor = SCREENX / view.getSize().x;
        break;
      }

      case sf::Event::MouseButtonPressed:
      {
        auto mousePos = sf::Vector2f{event.mouseButton.x, event.mouseButton.y};
        auto pos = toCartesian(mousePos, SCREENX, SCREENY);

        isDragging = true;
        startDragPos = pos;
        break;
      }

      case sf::Event::MouseButtonReleased:
      {
        isDragging = false;
      }

      case sf::Event::MouseMoved:
      {
        auto mousePos = sf::Vector2f{event.mouseMove.x, event.mouseMove.y};
        auto pos = toCartesian(mousePos, SCREENX, SCREENY);

        if (isDragging)
        {
          view.move(toSFML(pos - startDragPos) * 1.f / zoom_factor);
          startDragPos = pos;
        }
        break;
      }

      default:
      {
        break;
      }
      }
    }

    window.clear();
    float dt = clock.restart().asSeconds();

#pragma emp parallel for
    for (int i = 0; i < bodylist.size(); i++)
    {
      auto &circular = bodylist[i];
      circular.updateForce(bodylist, i, NO_FORCE, dt);

      auto dbg = MIN_VISIBLE_RADIUS * 1.f / zoom_factor;
      auto dbg2 = circular.getRadius() / view.getSize().x;
      auto circularViewSize = 100 / view.getSize().x;

      circular.setRadius(std::max(100.f, MIN_VISIBLE_RADIUS / zoom_factor * 50.f));
      window.draw(circular);
    }

    printf("fps: %f\n", 1.f / clockdbg.restart().asSeconds());
    window.setView(view);
    window.display();
    i++;
  }

  return 0;
}
