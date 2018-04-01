#pragma once

#include <vector>
#include <unordered_map>
#include <typeinfo>
#include <functional>
#include <algorithm>
#include <memory>
#include <stdexcept>

namespace pin
{
	const std::string BLANK_MESSAGE_NAME = "Blank";

	//Base exception class for message system exceptions
	class message_exception : public std::runtime_error 
	{
		public:
			message_exception(const std::string& messageName):
				std::runtime_error("Message system exception at message: " + messageName)
			{}

			message_exception(const std::string& messageName, const std::string& errorDescription) :
				std::runtime_error("Message system exception at message: " + messageName + "-" + errorDescription)
			{}
	};

	//exception thrown at bad parameters
	class bad_message_parameters : public message_exception
	{
		public:
			bad_message_parameters(const std::string& messageName) :
				message_exception(messageName, "Bad message parameters") {};
	};

	//Exception thrown if no message with given name exists
	class bad_message_name : public message_exception
	{
	public:
		bad_message_name(const std::string& messageName) :
			message_exception(messageName, "Bad message name") {};
	};


	//Base class for parameter pack so its able to combine it with polymorphism
	class base_param_pack 
	{
		public:
			virtual ~base_param_pack() {}
	};

	//Variadic parameter pack
	template<class... Args>
	class param_pack :public base_param_pack {};

	//Variadic function wrapper
	template<class... Args>
	class VariadicFunction
	{
		private:
			//Functor
			std::function<void(Args...)> _function;

		public:
			VariadicFunction() {};

			void bindFunction(std::function<void(Args...)> function)
			{
				_function = function;
			}

			template<class T>
			void bindFunction(void(T::*function)(Args... args), T* ownerPtr)
			{
				//Saved via lambda to avoid generic placeholders with std::bind
				_function = [function, ownerPtr](Args... args)
				{
					(ownerPtr->*function)(args...);
				};
			}

			void operator()(Args&&... args)
			{
				if (_function)
					_function(std::forward<Args>(args)...);
			}
	};

	class BaseMessage
	{
	protected:
		//Text of the message
		std::string _messageText;

		//Ptr to identifying param_pack
		std::unique_ptr<base_param_pack> _paramPack;

	public:

		//Default constructor
		BaseMessage() :
			_messageText(BLANK_MESSAGE_NAME)
		{}

		//Constructing with the message name
		BaseMessage(const std::string& messageName) :
			_messageText(messageName)
		{}

		virtual ~BaseMessage() {};

		virtual void clearListeners() = 0;

		//Setting the message text
		void setMessageText(const std::string& text) { _messageText = text; }
		const std::string& getMessageText() { return _messageText; }

		//Method to compare if two messages contain same parameter packs
		bool isSame(base_param_pack* testPack)
		{
			return typeid(*_paramPack) == typeid(*testPack);
		}
	};

	template <class... Args>
	class Message :public BaseMessage
	{
		typedef std::pair<bool*, VariadicFunction<Args...>> listener;
	private:

		//Vector of listeners represented as variadic functions
		std::vector<listener> _listeners;

	protected:

		//Function checking if the listener exists or not
		bool isValidListener(const listener& listener)
		{
			return (listener.first);
		}

		void clearListeners() override
		{
			//Erase-remove idiom
			_listeners.erase(std::remove_if(
											_listeners.begin(),
											_listeners.end(),
											[](const listener& listener) 
											{
												return !*listener.first;
											}
											),
							_listeners.end());
		}

	public:

		Message():
			BaseMessage()
		{
			_paramPack = std::make_unique<param_pack<Args...>>();
		}

		Message(const std::string& messageName) :
			BaseMessage(messageName)
		{
			_paramPack = std::make_unique<param_pack<Args...>>();
		}

		//Broadcasting the message - no check if the parameter pack sets the hash
		void broadcast(Args&&... args)
		{
			for (auto& listener : _listeners)
			{
				listener.second(std::forward<Args>(args)...);
			}
		}

		//Binding the function listener - no check
		template<class T>
		void bindFunction(bool* handler, void(T::*function)(Args... args), T* ownerPtr)
		{
			_listeners.emplace_back();

			_listeners.back().first = handler;

			_listeners.back().second.bindFunction(function, ownerPtr);
		}

		//Binding the function listener - no check 
		template<class T>
		void bindFunction(bool* handler, std::function<void(Args...)> function)
		{
			_listeners.emplace_back();

			_listeners.back().first = handler;

			_listeners.back().second.bindFunction(function);
		}
	};

	typedef std::unordered_map<std::string, std::unique_ptr<BaseMessage>> MessageMap;

	//Class that creates messages
	class MessageManager
	{
		friend class MessageHandler;

		private:
			static MessageMap _messages;

			/*
			*	Replace this with your own behaviour for giving bad hash 
			*/
			static void incorrectParameters(const std::string& messageName)
			{
				throw bad_message_parameters(messageName);
			}

			/*
			*	Replace this with your own behaviour for giving bad message name
			*/
			static void incorrectMessage(const std::string& messageName)
			{
				throw bad_message_name(messageName);
			}

		protected:

			template<class T, class... Args>
			static BaseMessage* listenToMessage(bool* handler, const std::string& messageName, void(T::*function)(Args... args), T* ownerPtr)
			{
				//Check if the message with messageName exists
				auto it = _messages.find(messageName);

				if (it != _messages.end())
				{
					param_pack<Args...> tmpPack;

					//Check the parameter pack with the pack of the message
					if (it->second->isSame(&tmpPack))
					{
						//Bind the function
						static_cast<Message<Args...>*>(it->second.get())->bindFunction(handler, function, ownerPtr);

						return it->second.get();
					}
					else
					{
						// This function also throws bad_message_parameters exception
						incorrectParameters(messageName);
					}
				}
				else
				{
					// This function also throws bad_message_name exception
					incorrectMessage(messageName);
				}

				return nullptr;
			}

			//Overload with the std::function
			template<class... Args>
			static BaseMessage* listenToMessage(bool* handler, const std::string& messageName, std::function<void(Args...)> function)
			{
				//Check if the message with messageName exists
				auto it = _messages.find(messageName);

				if (it != _messages.end())
				{
					param_pack<Args...> tmpPack;

					//Check the parameter pack with the pack of the message
					if (it->second->isSame(&tmpPack))
					{
						//Bind the function
						static_cast<Message<Args...>*>(it->second.get())->bindFunction(handler, function);

						return it->second.get();
					}
					else
					{
						// This function also throws bad_message_parameters exception
						incorrectParameters(messageName);
					}
				}
				else
				{
					// This function also throws bad_message_name exception
					incorrectMessage(messageName);
				}

				return nullptr;
			}

			template<class... Args>
			static void broadcastMessage(const std::string& messageName, Args... args)
			{
				//Check if the message with messageName exists
				auto it = _messages.find(messageName);

				if (it != _messages.end())
				{
					param_pack<Args...> tmpPack;

					//Check the created hash with the hash of the message
					if (it->second->isSame(&tmpPack))
					{
						//If its right broadcast the message
						static_cast<Message<Args...>*>(it->second.get())->broadcast(std::forward<Args>(args)...);
					}
					else
					{
						incorrectParameters(messageName);
					}
				}
				else
				{
					incorrectMessage(messageName);
				}
			}

		public:

			template<class... Args>
		    static Message<Args...>* createMessage(const std::string& messageName)
			{
				_messages[messageName] = std::make_unique<Message<Args...>>(messageName);

				return static_cast<Message<Args...>*>(_messages[messageName].get());
			}
	};

	MessageMap MessageManager::_messages;

	//Class that can listen to messages or broadcast them
	class MessageHandler
	{
			private:
				bool _active;

				std::vector<BaseMessage*> _listenedMessages;
			public:

				MessageHandler():
					_active(true){}

				~MessageHandler()
				{
					if (_active)
						deleteHandler();
				}

				template<class T, class... Args>
				void listenToMessage(const std::string& messageName, void(T::*function)(Args... args), T* ownerPtr)
				{
					//Add message to the vector of listened messages
					_listenedMessages.emplace_back(MessageManager::listenToMessage(&_active, messageName, function, ownerPtr));
				}

				template<class... Args>
				void listenToMessage(const std::string& messageName, std::function<void(Args...)> function)
				{
					//Add message to the vector of listened messages
					_listenedMessages.emplace_back(MessageManager::listenToMessage(&_active, messageName, function));
				}

				template<class T, class... Args>
				void listenToMessage(Message<Args...>* message, void(T::*function)(Args... args), T* ownerPtr)
				{
					//Bind the function manually
					message->bindFunction(&_active, function, ownerPtr);

					//Add message to the vector of listened messages
					_listenedMessages.emplace_back(message);
				}

				template<class T, class... Args>
				void listenToMessage(Message<Args...>* message, std::function<void(Args...)> function)
				{
					//Bind the function manually
					message->bindFunction(&_active, function);

					//Add message to the vector of listened messages
					_listenedMessages.emplace_back(message);
				}

				template<class... Args>
				void broadcastMessage(const std::string& messageName, Args... args)
				{
					//Broadcast message
					MessageManager::broadcastMessage(messageName, args...);
				}

				template<class... Args>
				void broadcastMessage(Message<Args...>* message, Args... args)
				{
					//Broadcast the message manually
					message->broadcast(std::forward<Args>(args)...);
				}

				void deleteHandler()
				{
					_active = false;

					//All messages must clear their listeners - delete those who are inactive
					for (auto& message : _listenedMessages)
						message->clearListeners();
				}
	};
}