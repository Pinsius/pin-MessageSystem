#pragma once

/**
    PinGUI
    Copyright (c) 2017 Lubomir Barantal <l.pinsius@gmail.com>
    This software is provided 'as-is', without any express or implied
    warranty. In no event will the authors be held liable for any damages
    arising from the use of this software.
    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
**/

#include <vector>
#include <unordered_map>
#include <typeinfo>
#include <functional>
#include <utility>
#include <algorithm>
#include <memory>

namespace pin
{
	const std::string PIN_BLANK = "Blank";

	//Variadic functor wrapper
	template<class... Args>
	class variadicFunction
	{
		private:
			std::function<void(Args...)> _function;

		public:

			variadicFunction() {};

			template<class T>
			void bindFunction(void(T::*function)(Args... args), T* ownerPtr)
			{

				//Saved via lambda to avoid generic placeholders with std::bind
				_function = [function, ownerPtr](Args... args)
				{
					(ownerPtr->*function)(args...);
				};
			}

			void exec(Args&&... args)
			{
				if (_function)
					_function(std::forward<Args>(args)...);
			}
	};

	//Base class for parameter pack so its able to combine it with polymorphism
	class base_param_pack{};

	//Variadic parameter pack
	template<class... Args>
	class param_pack{};

	class BaseMessage
	{
	protected:
		//Text of the message
		std::string _messageText;

		//Hash generated to check the type of the passed parameter pack
		std::size_t _parameterHash;

		//Hashing function
		virtual void createHash() {};

	public:
		//Default constructor
		BaseMessage() :
			_messageText(PIN_BLANK),
			_parameterHash(0)
		{}

		//Constructing with the message name
		BaseMessage(const std::string& messageName) :
			_messageText(messageName),
			_parameterHash(0)
		{}

		virtual ~BaseMessage() {};

		virtual void clearListeners() = 0;

		//Checking the parameter hashes
		bool haveSameParameterHash(const std::size_t& paramHash)
		{
			return (_parameterHash == paramHash);
		}

		//Setting the message text
		void setMessageText(const std::string& text) { _messageText = text; }
		const std::string& getMessageText() { return _messageText; }
	};

	//Forward declarated handler class
	class MessageHandler;

	template <class... Args>
	class Message :public BaseMessage
	{
		friend class MessageHandler;
		typedef std::pair<bool*, variadicFunction<Args...>> listener;
	private:

		//Vector of listeners represented as variadic functions
		std::vector<listener> _listeners;

	protected:

		void createHash() override
		{
			param_pack<Args...> tmpPack;

			//Save the ID of the pack
			_parameterHash = typeid(tmpPack).hash_code();
		}

		//Function checking if the listener exists or not
		bool isValidListener(const listener& listener)
		{
			return (listener.first);
		}

		void clearListeners() override
		{
			//Erase-remove idiom
			_listeners.erase(std::remove_if
							(
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
		{
			createHash();
		}

		Message(const std::string& messageName) :
			BaseMessage(messageName)
		{
			createHash();
		}

		//Broadcasting the message - no check if the parameter pack sets the hash
		void broadcast(Args&&... args)
		{
			for (auto& listener : _listeners)
			{
				listener.second.exec(std::forward<Args>(args)...);
			}
		}

		//Binding the function listener - no check if the parameter pack sets the hash
		template<class T, class... Args2>
		void bindFunction(bool* handler, void(T::*function)(Args2... args), T* ownerPtr)
		{
			_listeners.emplace_back();

			_listeners.back().first = handler;

			_listeners.back().second.bindFunction(function, ownerPtr);
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
			static void incorrectHash()
			{
				std::cout << "pin::MessageSystem : The hash of given arguments do not fit" << std::endl;
			}

			/*
			*	Replace this with your own behaviour for giving bad message name
			*/
			static void incorrectMessage()
			{
				std::cout << "pin::MessageSystem : Message not found" << std::endl;
			}

		protected:

			template<class T, class... Args>
			static BaseMessage* listenToMessage(bool* handler, const std::string& messageName, void(T::*function)(Args... args), T* ownerPtr)
			{
				//Check if the message with messageName exists
				auto it = _messages.find(messageName);

				if (it != _messages.end())
				{
					//Create temporary hash
					std::size_t tmpHash = typeid(param_pack<Args...>).hash_code();

					//Check the created hash with the hash of the message
					if (it->second->haveSameParameterHash(typeid(param_pack<Args...>).hash_code()))
					{
						//Bind the function
						static_cast<Message<Args...>*>(it->second.get())->bindFunction(handler, function, ownerPtr);

						return it->second.get();
					}
					else
					{
						/*
						* User tried to pass a different parameter pack that was passed to message when you created it. Pass your own behavior here
						*/
						incorrectHash();
					}
				}
				else
				{
					/*
					* User tried to find a message that does not exists, put your own behaviour here
					*/
					incorrectMessage();

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
				//Generated hash for current parameter pack
				std::size_t tmpHash = typeid(param_pack<Args...>).hash_code();

				//Check if its the right hash
				if (it->second->haveSameParameterHash(typeid(param_pack<Args...>).hash_code()))
				{
					//If its right broadcast the message
					static_cast<Message<Args...>*>(it->second.get())->broadcast(std::forward<Args>(args)...);
				}
				else
				{
					incorrectHash();
				}
			}
			else
			{
				incorrectMessage();
			}
		}

		public:

			template<class... Args>
			static void createMessage(const std::string& messageName)
			{
				_messages[messageName] = std::make_unique<Message<Args...>>(messageName);
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
					//Vector of messages being listened to by this handler
					_listenedMessages.emplace_back(MessageManager::listenToMessage(&_active, messageName, function, ownerPtr));
				}

				template<class... Args>
				void broadcastMessage(const std::string& messageName, Args... args)
				{
					//Broadcast message
					MessageManager::broadcastMessage(messageName, args...);
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
