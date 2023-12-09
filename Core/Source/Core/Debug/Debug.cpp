#include "Debug.h"

#include "../layers/ConsoleLayer.h"

namespace Core {

	void Debug::Log(const std::string& message)
	{
		auto console = Layers::Console::Get();
		console->buffer.emplace_back(message);
	}

}
