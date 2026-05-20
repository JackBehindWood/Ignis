# Dev wrapper — takes precedence over premake-generated Makefile for GNU Make.
# Delegates everything to `python3 -m build` so you can just type `make`.
# Usage: make [target] [CONFIG=Debug|Release|Distribution] [ARGS="--verbose --clean"]
#        make help           — list all targets
#        make help-<cmd>     — show python -m build <cmd> --help  (e.g. make help-build)

PYTHON := python3
CLI    := -m build
CONFIG ?= Debug
ARGS   ?=

.PHONY: all build run br clean setup setup-hooks bootstrap doctor vscode release dist test help

all: build

help: ## Show available targets
	@awk 'BEGIN {FS = ":.*##"; printf "\nUsage:\n  make \033[36m<target>\033[0m [CONFIG=Debug|Release|Distribution] [ARGS=\"...\"]\n\nTargets:\n"} /^[a-zA-Z_-]+:.*?##/ { printf "  \033[36m%-16s\033[0m %s\n", $$1, $$2 } END {printf "\nPer-command help:\n  make \033[36mhelp-<cmd>\033[0m   e.g. make help-build, make help-setup\n\n"}' $(MAKEFILE_LIST)

help-%: ## Show python -m build <cmd> --help
	$(PYTHON) $(CLI) $* --help

build: ## Generate Makefiles + compile (default: Debug)
	$(PYTHON) $(CLI) build --config $(CONFIG) $(ARGS)

run: ## Run the compiled executable
	$(PYTHON) $(CLI) run --config $(CONFIG) $(ARGS)

br: ## Build then run
	$(PYTHON) $(CLI) build-run --config $(CONFIG) $(ARGS)

clean: ## Remove bin/, bin-int/, compile commands
	$(PYTHON) $(CLI) clean $(ARGS)

setup: ## Install/update premake and DXC
	$(PYTHON) $(CLI) setup $(ARGS)

setup-hooks: ## Install git pre-commit hooks
	$(PYTHON) $(CLI) setup-hooks $(ARGS)

bootstrap: ## Fresh checkout: init + setup + build in one shot
	$(PYTHON) $(CLI) bootstrap $(ARGS)

doctor: ## Check all prerequisites and print status
	$(PYTHON) $(CLI) doctor $(ARGS)

vscode: ## Regenerate .vscode/ files
	$(PYTHON) $(CLI) vscode $(ARGS)

release: ## Build with CONFIG=Release
	$(PYTHON) $(CLI) build --config Release $(ARGS)

dist: ## Build with CONFIG=Distribution
	$(PYTHON) $(CLI) build --config Distribution $(ARGS)

test: ## Run test suite
	$(PYTHON) $(CLI) test --config $(CONFIG) $(ARGS)
