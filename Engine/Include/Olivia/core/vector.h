#pragma once
#include "defines.h"

namespace olivia
{
	constexpr uint32_t VECTOR_SCALE{ 2 };

	template<typename _Ty>
	struct vector_t
	{
		_Ty*   data;
		size_t size;
		size_t capacity;
	};

	template<typename _Ty>
	auto create_vector(size_t capacity)
	{
		vector_t<_Ty> vec{};

		vec.capacity = capacity;
		vec.data     = (_Ty*)calloc(vec.capacity, sizeof(_Ty));

		assert(vec.data && "calloc failed");

		return vec;
	}

	template<typename _Ty>
	void destroy_vector(vector_t<_Ty>& vec)
	{
		free(vec.data);
	}

	template<typename _Ty>
	void vector_reserve(vector_t<_Ty>& vec, size_t capacity)
	{
		_Ty* data = (_Ty*)calloc(capacity, sizeof(_Ty));
		assert(vec.data && data && "resize failed");

		memcpy(data, vec.data, sizeof(_Ty) * vec.size);

		vec.capacity = capacity;

		free(vec.data);
		vec.data = data;
	}

	template<typename _Ty>
	void vector_push_back(vector_t<_Ty>& vec, const _Ty& elem)
	{
		if (vec.size == vec.capacity)
		{
			vector_reserve(vec, vec.capacity * VECTOR_SCALE);
		}

		vec.data[vec.size++] = elem;
	}
}