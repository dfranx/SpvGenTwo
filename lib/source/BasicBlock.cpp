#include "spvgentwo/BasicBlock.h"
#include "spvgentwo/Function.h"
#include "spvgentwo/Module.h"
#include "spvgentwo/Reader.h"

spvgentwo::BasicBlock::BasicBlock(Function* _pFunction, const char* _pName) : List(_pFunction->getAllocator()),
	m_pFunction(_pFunction),
	m_Label(this)
{
	m_Label.opLabel();

	if (_pName != nullptr)
	{
		getModule()->addName(&m_Label, _pName);
	}
}

spvgentwo::BasicBlock::BasicBlock(Function* _pFunction, BasicBlock&& _other) noexcept :
	List(stdrep::move(_other)),
	m_pFunction(_pFunction),
	m_Label(this)
{
	m_Label.opLabel();

	for (Instruction& instr : *this)
	{
		instr.m_parent.pBasicBlock = this;
	}
}

spvgentwo::BasicBlock::~BasicBlock()
{
}

spvgentwo::BasicBlock& spvgentwo::BasicBlock::operator=(BasicBlock&& _other) noexcept
{
	if (this == &_other) return *this;

	List::operator=(stdrep::move(_other));

	for (Instruction& instr : *this)
	{
		instr.m_parent.pBasicBlock = this;
	}

	return *this;
}

spvgentwo::Module* spvgentwo::BasicBlock::getModule() const
{
	return m_pFunction->getModule();
}

const char* spvgentwo::BasicBlock::getName() const
{
	return m_Label.getName();
}

spvgentwo::IAllocator* spvgentwo::BasicBlock::getAllocator()
{
	return getModule()->getAllocator();;
}

spvgentwo::Instruction* spvgentwo::BasicBlock::getTerminator()
{
	if (empty() == false && back().isTerminator())
	{
		return &back();
	}

	return nullptr;
}

const spvgentwo::Instruction* spvgentwo::BasicBlock::getTerminator() const
{
	if (empty() == false && back().isTerminator())
	{
		return &back();
	}

	return nullptr;
}

bool spvgentwo::BasicBlock::getBranchTargets(List<BasicBlock*>& _outTargetBlocks) const
{
	if (empty() == false) // there is more then just initial opLabel
	{
		return back().getBranchTargets(_outTargetBlocks);
	}

	return false;
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

void spvgentwo::BasicBlock::write(IWriter* _pWriter)
{
	m_Label.write(_pWriter);

	for (Instruction& instr : *this)
	{
		instr.write(_pWriter);
	}
}

bool spvgentwo::BasicBlock::read(IReader* _pReader, const Grammar& _grammar)
{
	// function alread consumed the first word of OpLabel, OpLabel as one result Id operand
	if (m_Label.readOperands(_pReader, _grammar, spv::Op::OpLabel, 1u) == false || m_Label.getOperation() != spv::Op::OpLabel) return false;

	unsigned int word{ 0 };

	while (_pReader->get(word))
	{
		const spv::Op op = getOperation(word);
		const unsigned int operands = getOperandCount(word) - 1u;

		if (emplace_back(this).readOperands(_pReader, _grammar, op, operands) == false)
		{
			break;
		}

		if (isTerminatorOp(op))
		{
			return true;
		}
	}

	return false;
}

bool spvgentwo::BasicBlock::remove(const Instruction* _pInstr)
{
	if(_pInstr != nullptr && this == _pInstr->getBasicBlock())
	{
		for(auto it = begin(), e = end(); it != e; ++it)
		{
			if(it.operator->() == _pInstr)
			{
				erase(it);
				return true;
			}
		}
	}

	return false;
}

spvgentwo::BasicBlock& spvgentwo::BasicBlock::If(Instruction* _pCondition, BasicBlock& _trueBlock, BasicBlock& _falseBlock, BasicBlock* _pMergeBlock, const Flag<spv::SelectionControlMask> _mask)
{
	// this block has not been terminated yet
	BasicBlock& mergeBB = _pMergeBlock != nullptr ? *_pMergeBlock : m_pFunction->addBasicBlock("IfMerge");

	addInstruction()->opSelectionMerge(&mergeBB, _mask);
	addInstruction()->opBranchConditional(_pCondition, &_trueBlock, &_falseBlock);

	if (_trueBlock.empty() == false && _trueBlock.back().isTerminator() == false)
	{
		_trueBlock->opBranch(&mergeBB);
	}
	if (_falseBlock.empty() == false && _falseBlock.back().isTerminator() == false)
	{
		_falseBlock->opBranch(&mergeBB);
	}

	return mergeBB;
}

spvgentwo::BasicBlock& spvgentwo::BasicBlock::If(Instruction* _pCondition, BasicBlock& _trueBlock, BasicBlock* _pMergeBlock, const Flag<spv::SelectionControlMask> _mask)
{
	// this block has not been terminated yet
	BasicBlock& mergeBB = _pMergeBlock != nullptr ? *_pMergeBlock : m_pFunction->addBasicBlock("IfMerge");

	addInstruction()->opSelectionMerge(&mergeBB, _mask);
	addInstruction()->opBranchConditional(_pCondition, &_trueBlock, &mergeBB);

	if (_trueBlock.empty() == false && _trueBlock.back().isTerminator() == false)
	{
		_trueBlock->opBranch(&mergeBB);
	}

	return mergeBB;
}

spvgentwo::BasicBlock& spvgentwo::BasicBlock::Loop(Instruction* _pCondition, BasicBlock& _continue, BasicBlock& _body, BasicBlock* _pMergeBlock, const Flag<spv::LoopControlMask> _mask)
{
	BasicBlock& mergeBB = _pMergeBlock != nullptr ? *_pMergeBlock : m_pFunction->addBasicBlock("LoopMerge");
	BasicBlock& condBB = *_pCondition->getBasicBlock();

	addInstruction()->opLoopMerge(&mergeBB, &_continue, _mask);
	addInstruction()->opBranch(&condBB);

	condBB->opBranchConditional(_pCondition, &_body, &mergeBB);
	_body->opBranch(&_continue); // branch to "increment" / continue block
	_continue->opBranch(this); // back edge to loop merge

	return mergeBB;
}