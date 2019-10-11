#include "BasicBlock.h"
#include "Function.h"
#include "Module.h"

spvgentwo::BasicBlock::BasicBlock(IAllocator* _pAllocator) :
	m_pFunction(nullptr), List(_pAllocator)
{
	addInstruction()->opLabel();
}

spvgentwo::BasicBlock::BasicBlock(Function* _pFunction) :
	m_pFunction(_pFunction), List(_pFunction->getAllocator())
{
	addInstruction()->opLabel();
}

spvgentwo::BasicBlock::~BasicBlock()
{
}

spvgentwo::Module* spvgentwo::BasicBlock::getModule()
{
	return m_pFunction->getModule();
}

const spvgentwo::Module* spvgentwo::BasicBlock::getModule() const
{
	return m_pFunction->getModule();
}

spvgentwo::IAllocator* spvgentwo::BasicBlock::getAllocator()
{
	return getModule()->getAllocator();;
}

spvgentwo::BasicBlock::Iterator spvgentwo::BasicBlock::getTerminator()
{
	if (m_pLast != nullptr && (*m_pLast)->isTerminator())
	{
		return Iterator(m_pLast);
	}

	return Iterator(nullptr);
}

spvgentwo::Instruction* spvgentwo::BasicBlock::returnValue(Instruction* _pValue)
{
	Instruction* pRet = addInstruction();

	if (_pValue == nullptr)
	{
		pRet->opReturn();
	}
	else
	{
		// TODO: check return type with m_FunctionType
		pRet->opReturnValue(_pValue);
	}

	return pRet;
}

void spvgentwo::BasicBlock::write(IWriter* _pWriter, spv::Id& _resultId)
{
	for (Instruction& instr : *this)
	{
		instr.write(_pWriter, _resultId);
	}
}

spvgentwo::Instruction* spvgentwo::BasicBlock::If(Instruction* _pCondition, BasicBlock& _trueBlock, BasicBlock& _falseBlock, BasicBlock& _mergeBlock, const spv::SelectionControlMask _mask)
{
	// this block has not been terminated yet
	//if (getTerminator() == nullptr)
	{
		addInstruction()->opSelectionMerge(&_mergeBlock, _mask);
		addInstruction()->opBranchConditional(_pCondition, &_trueBlock, &_falseBlock);
		_trueBlock->opBranch(&_mergeBlock);
		_falseBlock->opBranch(&_mergeBlock);
	}

	for (auto it = _mergeBlock.last(); it != _mergeBlock.begin(); --it)
	{
		if (it->isTerminator() == false && it->hasResult()) // hasResultAndType
		{
			return it.operator->();
		}
	}

	return nullptr;
}

spvgentwo::Instruction* spvgentwo::BasicBlock::Add(Instruction* _pLeft, Instruction* _pRight)
{
	const Type* lType = _pLeft->getType();
	const Type* rType = _pRight->getType();

	if (lType->hasSameBase(*rType) == false)
	{
		return nullptr;
	}

	if ((lType->isVectorOfInt() && rType->isVectorOfInt() && lType->getVectorComponentCount() == rType->getVectorComponentCount()) ||
		(lType->isInt() && rType->isInt()))
	{
		addInstruction()->makeOp(spv::Op::OpIAdd, _pLeft, _pRight);
	}
	else if ((lType->isVectorOfFloat() && rType->isVectorOfFloat() && lType->getVectorComponentCount() == rType->getVectorComponentCount()) ||
		(lType->isFloat() && rType->isFloat()))
	{
		addInstruction()->makeOp(spv::Op::OpFAdd, _pLeft, _pRight);
	}

	return nullptr;
}

spvgentwo::Instruction* spvgentwo::BasicBlock::Sub(Instruction* _pLeft, Instruction* _pRight)
{
	const Type* lType = _pLeft->getType();
	const Type* rType = _pRight->getType();

	if (lType->hasSameBase(*rType) == false)
	{
		return nullptr;
	}

	if ((lType->isVectorOfInt() && rType->isVectorOfInt() && lType->getVectorComponentCount() == rType->getVectorComponentCount()) ||
		(lType->isInt() && rType->isInt()))
	{
		addInstruction()->makeOp(spv::Op::OpISub, _pLeft, _pRight);
	}
	else if ((lType->isVectorOfFloat() && rType->isVectorOfFloat() && lType->getVectorComponentCount() == rType->getVectorComponentCount()) || 
		(lType->isFloat() && rType->isFloat()))
	{
		addInstruction()->makeOp(spv::Op::OpFSub, _pLeft, _pRight);
	}

	return nullptr;
}

spvgentwo::Instruction* spvgentwo::BasicBlock::Mul(Instruction* _pLeft, Instruction* _pRight)
{
	const Type* lType = _pLeft->getType();
	const Type* rType = _pRight->getType();

	if (lType->hasSameBase(*rType) == false)
	{
		return nullptr;
	}

	// both are scalar or vector of int
	if ((lType->isInt() && rType->isInt()) ||
		(lType->isVectorOfInt() && rType->isVectorOfInt()))
	{
		return addInstruction()->makeOp(spv::Op::OpIMul, _pLeft, _pRight);
	}// both are scalar or vector of float
	else if ((lType->isFloat() && rType->isFloat()) ||
		(lType->isVectorOfFloat() && rType->isVectorOfFloat()))
	{
		return addInstruction()->makeOp(spv::Op::OpFMul, _pLeft, _pRight);
	}// left scalar times right vector float
	else if ((lType->isFloat() && rType->isVectorOfFloat()) &&
		(lType->isInt() && rType->isVectorOfInt()))
	{// OpVectorTimesScalar expects vector as first operand
		return addInstruction()->makeOp(spv::Op::OpVectorTimesScalar, _pRight, _pLeft);
	}// left vector times right scalar
	else if ((lType->isVectorOfFloat() && rType->isFloat()) &&
		(lType->isVectorOfInt() && rType->isInt()))
	{
		return addInstruction()->makeOp(spv::Op::OpVectorTimesScalar, _pLeft, _pRight);
	}
	else if (lType->isScalar() && rType->isMatrix())
	{// OpMatrixTimesScalar expects matrix as first operand
		return addInstruction()->makeOp(spv::Op::OpMatrixTimesScalar, _pRight, _pLeft);
	}
	else if (lType->isMatrix() && rType->isScalar())
	{
		return addInstruction()->makeOp(spv::Op::OpMatrixTimesScalar, _pLeft, _pRight);
	}
	else if (lType->isVector() && rType->isMatrix())
	{// OpMatrixTimesVector expects matrix as first operand
		return addInstruction()->makeOp(spv::Op::OpMatrixTimesVector, _pRight, _pLeft);
	}
	else if (lType->isMatrix() && rType->isVector())
	{
		return addInstruction()->makeOp(spv::Op::OpMatrixTimesVector, _pLeft, _pRight);
	}
	else if (lType->isMatrix() && rType->isMatrix())
	{
		return addInstruction()->makeOp(spv::Op::OpMatrixTimesMatrix, _pLeft, _pRight);
	}

	return nullptr;
}

spvgentwo::Instruction* spvgentwo::BasicBlock::Div(Instruction* _pLeft, Instruction* _pRight)
{
	return nullptr;
}