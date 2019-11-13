#include <iostream>

#include "Messages.hpp"

class Foo
{
private:
	
	int id;

public:
	pin::MessageHandler handler;

	void init(pin::MessageManager* manager)
	{
		handler.initHandler(manager);

		//Listen to two types of messages
		handler.listenToMessage("NoParameterMessage", &Foo::info, this);
		handler.listenToMessage("IntFloatMessage", &Foo::intFloatFunction, this);
	}

	void info()
	{
		std::cout << "Foo info recieved" << std::endl;
	}

	void intFloatFunction(int i, float f)
	{
		std::cout << "Integer is " << i << " and float " << f << std::endl;
	}

	int getId()
	{
		return id;
	}

	void setId(int Id)
	{
		id = Id;
	}
};

class Foo2
{
private:
	

public:
	pin::MessageHandler handler;

	void init(pin::MessageManager* manager)
	{
		handler.initHandler(manager);

		//Listen to two types of messages as well
		handler.listenToMessage("NoParameterMessage", &Foo2::info, this);
		handler.listenToMessage("CustomClassMessage", &Foo2::fooFunction, this);
	}

	void info()
	{
		std::cout << "Foo2 info recieved" << std::endl;
	}

	void fooFunction(Foo* f)
	{
		std::cout << "id is " << f->getId() << std::endl;
	}
};


int main()
{
	pin::MessageManager manager;

	manager.createMessage<>("NoParameterMessage");
	manager.createMessage<int, float>("IntFloatMessage");
	manager.createMessage<Foo*>("CustomClassMessage");

	Foo foo;
	foo.init(&manager);
	foo.setId(100);

	Foo2 foo2;
	foo2.init(&manager);

	foo.handler.broadcastMessage("NoParameterMessage");
	foo2.handler.broadcastMessage("CustomClassMessage", &foo);
	foo.handler.broadcastMessage("IntFloatMessage", 5, 42.5f);

	system("pause");
	return 0;

}
