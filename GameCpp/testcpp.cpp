#include "pch.h"

class Archer
{
public:
	Archer() = default;
	Archer(const Archer& other) = default;
	Archer(Archer&& other) = default;
	Archer& operator=(const Archer& other) = default;
	Archer& operator=(Archer&& other) = default;

	inline void die() {}
};

void shot(Archer&& target)
{
	Archer&& temp = Archer();
	temp.die();
}

void sharedPtr()
{
	std::shared_ptr<Archer> sp(new Archer, [](Archer* a) { delete a; });
}
