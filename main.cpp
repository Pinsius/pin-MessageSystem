#include <iostream>

#include "Messages.hpp"

class Foo
{
	public:

		void test()
		{
			std::cout << "test function is being called" << std::endl;
		}

		void test_int(int number)
		{
			std::cout << "Value is: " << number << std::endl;
		}

};

int main(int argc, char** argv)
{
	Foo foo;

	pin::MessageHandler tut;


	pin::MessageManager::createMessage<>("Blank message");
	pin::MessageManager::createMessage<int>("Int message");

	tut.listenToMessage("Blank message", &Foo::test, &foo);
	tut.listenToMessage("Int message", &Foo::test_int, &foo);

	tut.broadcastMessage("Blank message");
	tut.broadcastMessage("Int message", 5);


	system("pause");

	return 0;
}