#pragma once

#include "Type.h"

namespace spvgentwo
{
	class Constant
	{
	public:
		Constant(IAllocator* _pAllocator);
		Constant(const Constant& _other);
		Constant(Constant&& _other) noexcept;
		~Constant();

		Constant& operator=(const Constant& _other);
		Constant& operator=(Constant&& _other) noexcept;

		spv::Op getOperation() const { return m_Operation; }
		const Type& getType() const { return m_Type; }

		const List<unsigned int>& getData() const { return m_literalData; }
		const List<Constant>& getComponents() const { return m_Components; }

		Constant& make(const bool _value, const bool _spec = false);

		Constant& make(const short _value, const bool _spec = false);
		Constant& make(const unsigned short _value, const bool _spec = false);

		Constant& make(const int _value, const bool _spec = false);
		Constant& make(const unsigned int _value, const bool _spec = false);

		Constant& make(const long long _value, const bool _spec = false);
		Constant& make(const unsigned long long _value, const bool _spec = false);

		Constant& make(const float _value, const bool _spec = false);
		Constant& make(const double _value, const bool _spec = false);

	private:
		//Constant* m_pParent = nullptr;
		spv::Op m_Operation = spv::Op::OpConstantNull;
		Type m_Type;

		List<unsigned int> m_literalData;
		List<Constant> m_Components;
	};

	template <>
	struct Hasher<Constant>
	{
		Hash64 operator()(const Constant& _const, FNV1aHasher& _hasher) const
		{
			_hasher << _const.getOperation();
			Hasher<Type>()(_const.getType(), _hasher);
			for (const auto& l : _const.getData())
			{
				_hasher << l;
			}

			for (const Constant& component : _const.getComponents())
			{
				operator()(component, _hasher);
			}

			return _hasher;
		}

		Hash64 operator()(const Constant& _const) const
		{
			FNV1aHasher hasher;
			return operator()(_const, hasher);
		}
	};
} // !spvgentwo