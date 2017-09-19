#pragma once

#include <string>


namespace bluff {

std::string slurp(const std::string& path);

std::string uuid();

std::string getExtension(const std::string& path);

std::string getContentType(const std::string& path);
}