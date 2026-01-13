#include <print>
#include "sugar_string.h"
#define LIBSUGARX_UUID_IMPLEMENTATION
#include "sugar_uuid.h"

using namespace libsugarx;

int main(int argc, char **argv)
{
	constexpr const char aName[] = "libsugarx@bamcane.teemidnight.online";
	uuid Uuidv7 = uuid::generate_v7_nullable();
	std::println("What I want to say is: Hello UUID!");
	std::println("Also, it's a good idea to compute a UUIDv7!");
	std::println("I think, the uuid generated just now, was {}", Uuidv7.to_string());
	std::println("And UUIDv4!");
	std::println("{}", uuid::generate_v4_nullable());
	std::println("Besides, UUIDv3 with UUIDv5!");
	std::println("UUID name is '{}'", aName);
	std::println("Namespace is '{}'. That was what we have generated.", Uuidv7.to_string());
	std::println("{} {}", uuid::generate_v3(aName, Uuidv7).to_string(), uuid::generate_v5(aName, Uuidv7).to_string());
}