#include "chig/Context.hpp"
#include "chig/GraphFunction.hpp"
#include "chig/JsonModule.hpp"
#include "chig/LangModule.hpp"
#include "chig/NodeType.hpp"

#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

#include <boost/filesystem.hpp>

#include <chig/CModule.hpp>
#include <gsl/gsl>

using namespace chig;
using namespace llvm;

namespace fs = boost::filesystem;

Context::Context(const fs::path& workPath) : mWorkspacePath(workPath) {}
ChigModule* Context::moduleByName(gsl::cstring_span<> moduleName) noexcept
{
	Result res;

	for (auto& module : mModules) {
		if (module->name == moduleName) {
			return module.get();
		}
	}
	return nullptr;
}

chig::Result chig::Context::addModule(const gsl::cstring_span<> name)
{
	Result res;

	// check for built-in modules
	if (name == "lang") {
		return addModule(std::make_unique<LangModule>(*this));
	} else if (name == "c") {
		return addModule(std::make_unique<CModule>(*this));
	}

	// find it in the workspace
	fs::path fullPath = workspacePath() / "src" / (gsl::to_string(name) + ".chigmod");

	if (!fs::is_regular_file(fullPath)) {
		res.add_entry(
			"EUKN", "Failed to find module", {{"Module Name", gsl::to_string(name)},
												 {"Workspace Path", workspacePath().string()}});
		return res;
	}

	// load the JSON
	nlohmann::json readJson = {};
	{
		fs::ifstream inFile{fullPath};

		inFile >> readJson;
	}

	res += addModuleFromJson(readJson);
	return res;
}

Result Context::addModuleFromJson(const nlohmann::json& json, std::string* name)
{
	Result res;

	// parse module
	auto jmod = std::make_unique<JsonModule>(json, *this, &res);
	if (!res) {
		return res;
	}
	if (name != nullptr) {
		*name = jmod->name;
	}

	auto cPtr = jmod.get();
	res += addModule(std::move(jmod));
	if (!res) {
		return res;
	}

	// load graphs
	res += cPtr->loadGraphs();

	return res;
}

Result Context::addModule(std::unique_ptr<ChigModule> modToAdd) noexcept
{
	Expects(modToAdd != nullptr);

	Result res;

	// make sure it's unique
	auto ptr = moduleByName(modToAdd->name);
	if (ptr != nullptr) {
		res.add_entry(
			"E24", "Cannot add already existing module again", {{"moduleName", modToAdd->name}});
		return res;
	}

	mModules.emplace_back(std::move(modToAdd));

	return res;
}

Result Context::typeFromModule(
	gsl::cstring_span<> module, gsl::cstring_span<> name, DataType* toFill) noexcept
{
	Result res;

	ChigModule* mod = moduleByName(module);
	if (mod == nullptr) {
		res.add_entry("E36", "Could not find module", {{"module", gsl::to_string(module)}});
		return res;
	}

	*toFill = mod->getType(name);
	if (!toFill->isValid()) {
		res.add_entry("E37", "Could not find type in module",
			{{"type", gsl::to_string(name)}, {"module", gsl::to_string(module)}});
	}

	return res;
}

Result Context::nodeTypeFromModule(gsl::cstring_span<> moduleName, gsl::cstring_span<> typeName,
	const nlohmann::json& data, std::unique_ptr<NodeType>* toFill) noexcept
{
	Result res;

	auto module = moduleByName(moduleName);
	if (module == nullptr) {
		res.add_entry("E36", "Could not find module", {{"module", gsl::to_string(moduleName)}});
		return res;
	}

	res += module->createNodeType(typeName, data, toFill);

	return res;
}

std::string Context::stringifyType(llvm::Type* ty)
{
	Expects(ty != nullptr);

	std::string data;
	llvm::raw_string_ostream stream{data};
	ty->print(stream);
	return stream.str();
}

Result Context::compileModule(gsl::cstring_span<> name, std::unique_ptr<llvm::Module>* toFill)
{
	Expects(toFill != nullptr);

	Result res;

	auto chigmod = moduleByName(name);

	if (chigmod == nullptr) {
		res.add_entry("E36", "Could not find module", {{"module", gsl::to_string(name)}});
		return res;
	}

	std::unique_ptr<llvm::Module> llmod;
	res += chigmod->generateModule(&llmod);

	// verify the created module
	if (res) {
		std::string err;
		llvm::raw_string_ostream os(err);
		if (llvm::verifyModule(*llmod, &os)) {
			res.add_entry(
				"EUKN", "Internal compiler error: Invalid module created", {{"Error", err}});
		}
	}

	*toFill = std::move(llmod);

	return res;
}
