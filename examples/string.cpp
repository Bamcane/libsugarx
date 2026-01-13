#include <print>
#include "sugar_string.h"

using namespace libsugarx;

int main(int argc, char **argv)
{
	fixed_string<64> String("Hello world!");
	std::println("What I want to say is: {}", String);

	String.format("Also, it's a good idea to compute '48+39={:03d}'", 48 + 39);
	std::println("{}", String);

	fixed_string<32> Copy;
	Copy = String;
	std::println("Look! I copied this: {}", Copy);

	std::println("And I can do this:");
	fixed_string<32> Concat;
	for(int i = 0; i < 24; i++)
	{
		Concat.concat('A' + i);
		std::println("Line {:02d}: {}", i, Concat);
	}
	std::println("That's all :D, have a nice day!");
}