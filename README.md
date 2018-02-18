# pin-MessageSystem

This is generic message system written in C++ that I made for my game engine. It supports also variadic messages.

## How to use it

Using it is very easy as it is a single header implementation. By including the header file, **Messages.hpp** you have access to the whole interface that is nested in the namespace **pin::** . It means that it is a part of a bigger set of tools with the same name as I plan to add more in the future.

You are going to control it using 2 classes : **MessageSystem** and **MessageHandlers**. **MessageSystem** is a static class, so you can access it anywhere using the right scope operator. However, via this class you can only create the messages. Images are created using the right function, then the parameters as its template arguments (for messages without parameters leave it blank, no void ) and then its name. 

![Picture of creating messages](http://oi66.tinypic.com/dc5192.jpg)


Then there are **MessageHandlers**. They are the main objects that work and react with the messages. They broadcast messages and also receive them. After not using the handler,  you can delete it using the appropriate function called **deleteHandler()**. If you do not call it, it will be called in its destructor. When listening the messages, you must pass a function with exactly the same parameters as the parameters passed when the message was created (be careful with consts, references, pointers).

![Picture of listening to messages and broadcasting them](http://oi64.tinypic.com/2q2itz9.jpg)

## Performance
It has a run-time checking for the validation of the passed message names and its parameters, so there is a slight overhead and consistent broadcasting is not recommended. 

## Error logging and assertions
When you open the header file you can see two functions for your own behavior when it does not accept the parameters or the message name.
