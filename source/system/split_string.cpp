#include "split_string.hpp"
#include <string>
#include <algorithm>
#include <cstddef>

namespace nana
{
namespace system {
std::vector<split_string_type> split_string (const split_string_type& text, char sep)
{
	std::vector<split_string_type> retval;
	const auto estimated_size = std::count(text.begin(), text.end(), sep) + 1;
	retval.reserve(estimated_size);

	std::size_t sep_pos = 0;
	while (sep_pos != text.size()) {
		const std::size_t start = sep_pos;
		sep_pos = text.find(sep, sep_pos);
		sep_pos = (text.npos == sep_pos ? text.size() : sep_pos);
		const std::size_t end = sep_pos;
		while (sep_pos < text.size() and sep == text[sep_pos]) {
			++sep_pos;
		}

		retval.push_back(text.substr(start, end - start));
	}

	return retval;
}

}
}
