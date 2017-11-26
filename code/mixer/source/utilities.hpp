#pragma once



namespace tabletopify
{

	// countof(X)
	template<typename T, size_t N>
	constexpr size_t countof(const T (&)[N]) { return N; }

	inline
	std::string string_format(const char* format, ...)
	{
	    va_list args1, args2;

	    va_start (args1, format);
	    // vsnprintf destructively modifes the va_list, so make an initial copy
	    va_copy(args2, args1);
	    auto len = vsnprintf(nullptr, 0, format, args1);
	    va_end(args1); 

	    std::string result(len, 0);
	    vsnprintf(const_cast<char*>(result.data()), len+1, format, args2);
	    va_end(args2);

	    return result;
	}

}