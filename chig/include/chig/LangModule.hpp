#ifndef CHIG_LANG_MODULE_HPP
#define CHIG_LANG_MODULE_HPP

#pragma once

#include "chig/ChigModule.hpp"
#include "chig/Context.hpp"
#include "chig/NodeType.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace chig
{
struct IfNodeType : NodeType {
	IfNodeType(Context& con);

	virtual Result codegen(size_t /*execInputID*/, llvm::Function*,
		const std::vector<llvm::Value*>& io, llvm::BasicBlock* codegenInto,
		const std::vector<llvm::BasicBlock*>& outputBlocks) const override;

	virtual std::unique_ptr<NodeType> clone() const override;
};

struct EntryNodeType : NodeType {
	EntryNodeType(Context& con, const std::vector<std::pair<llvm::Type*, std::string>>& funInputs);

	// the function doesn't have to do anything...this class just holds metadata
	virtual Result codegen(size_t /*inputExecID*/, llvm::Function* f,
		const std::vector<llvm::Value*>& io, llvm::BasicBlock* codegenInto,
		const std::vector<llvm::BasicBlock*>& outputBlocks) const override;

	virtual std::unique_ptr<NodeType> clone() const override;

	Result toJSON(nlohmann::json* ret_json) const override;
};

struct ConstIntNodeType : NodeType {
	ConstIntNodeType(Context& con, int num) ;

	// the function doesn't have to do anything...this class just holds metadata
	virtual Result codegen(size_t /*inputExecID*/, llvm::Function* f,
		const std::vector<llvm::Value*>& io, llvm::BasicBlock* codegenInto,
		const std::vector<llvm::BasicBlock*>& outputBlocks) const override;

	virtual std::unique_ptr<NodeType> clone() const override;

	Result toJSON(nlohmann::json* fill_json) const override;

	int number;
};

struct ExitNodeType : NodeType {
	ExitNodeType(Context& con, const std::vector<std::pair<llvm::Type*, std::string>>& funOutputs)
		;

	virtual Result codegen(size_t execInputID, llvm::Function* f,
		const std::vector<llvm::Value*>& io, llvm::BasicBlock* codegenInto,
		const std::vector<llvm::BasicBlock*>&) const override;

	virtual std::unique_ptr<NodeType> clone() const override;

	Result toJSON(nlohmann::json* ret_json) const override;
};

struct LangModule : ChigModule {
	LangModule(Context& context);
	~LangModule() = default;

	virtual std::unique_ptr<NodeType> createNodeType(
		const char* name, const nlohmann::json& json_data) const override;
	virtual llvm::Type* getType(const char* name) const override;

	std::unordered_map<std::string, std::function<std::unique_ptr<NodeType>(const nlohmann::json&)>>
		nodes;
};
}

#endif  // CHIG_LANG_MODULE_HPP

