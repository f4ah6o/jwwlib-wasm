# Contributing to jwwlib-wasm

First off, thank you for considering contributing to jwwlib-wasm! It's people like you that make jwwlib-wasm such a great tool.

## Code of Conduct

This project and everyone participating in it is governed by our Code of Conduct. By participating, you are expected to uphold this code.

## How Can I Contribute?

### Reporting Bugs

Before creating bug reports, please check existing issues as you might find out that you don't need to create one. When you are creating a bug report, please include as many details as possible.

**How to Submit A Good Bug Report:**

Bugs are tracked as GitHub issues. Create an issue and provide the following information:

* **Use a clear and descriptive title** for the issue to identify the problem
* **Describe the exact steps which reproduce the problem** in as many details as possible
* **Provide specific examples to demonstrate the steps**
* **Describe the behavior you observed after following the steps**
* **Explain which behavior you expected to see instead and why**
* **Include screenshots and animated GIFs** if possible
* **Include the JWW file** that causes the issue (if applicable and not confidential)

### Suggesting Enhancements

Enhancement suggestions are tracked as GitHub issues. Create an issue and provide the following information:

* **Use a clear and descriptive title** for the issue to identify the suggestion
* **Provide a step-by-step description of the suggested enhancement**
* **Provide specific examples to demonstrate the steps**
* **Describe the current behavior** and **explain which behavior you expected to see instead**
* **Explain why this enhancement would be useful**

### Pull Requests

* Fill in the required template
* Do not include issue numbers in the PR title
* Follow the JavaScript/TypeScript styleguide
* Include thoughtfully-worded, well-structured tests
* Document new code
* End all files with a newline

## Development Process

### Setup Development Environment

1. Fork the repo and create your branch from `main`
2. Clone your fork:
   ```bash
   git clone https://github.com/your-username/jwwlib-wasm.git
   cd jwwlib-wasm
   ```

3. Install dependencies:
   ```bash
   pnpm install
   ```

4. Install and activate Emscripten:
   ```bash
   # Follow instructions at https://emscripten.org/docs/getting_started/downloads.html
   git clone https://github.com/emscripten-core/emsdk.git
   cd emsdk
   ./emsdk install latest
   ./emsdk activate latest
   source ./emsdk_env.sh
   ```

5. Build the project:
   ```bash
   pnpm run build
   ```

### Coding Standards

#### JavaScript/TypeScript
* Use ES modules
* Use async/await over promises
* Use meaningful variable names
* Add JSDoc comments for public APIs

#### C++
* Follow the existing code style
* Use modern C++ features (C++14)
* Avoid raw pointers when possible
* Document complex algorithms

#### Commit Messages
We use [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <subject>

<body>

<footer>
```

Types:
* **feat**: A new feature
* **fix**: A bug fix
* **docs**: Documentation only changes
* **style**: Changes that don't affect code meaning
* **refactor**: Code change that neither fixes a bug nor adds a feature
* **perf**: Code change that improves performance
* **test**: Adding or correcting tests
* **build**: Changes to build system or dependencies
* **ci**: Changes to CI configuration
* **chore**: Other changes that don't modify src or test files

### Testing

#### Unit Tests
```bash
pnpm test
```

#### E2E Tests
```bash
pnpm run test:e2e
```

#### Property-Based Tests
```bash
cd build-debug
./tests/pbt/pbt_tests
```

### Building

#### Debug Build
```bash
pnpm run build:wasm:debug
```

#### Release Build
```bash
pnpm run build:wasm:release
```

### Documentation

* Update README.md if needed
* Add JSDoc comments for new public APIs
* Update TypeScript definitions in index.d.ts
* Add examples for new features

## Project Structure

```
jwwlib-wasm/
├── src/
│   ├── core/          # Core JWW parsing logic (from LibreCAD)
│   ├── wasm/          # WebAssembly bindings
│   └── js/            # JavaScript wrapper
├── include/           # C++ headers
├── tests/             # Test files
│   ├── unit/          # Unit tests
│   ├── e2e/           # End-to-end tests
│   └── pbt/           # Property-based tests
├── examples/          # Example usage
└── docs/              # Documentation
```

## Adding New Entity Types

1. Update the C++ parser in `src/core/dl_jww.cpp`
2. Add entity definition in `include/dl_entities.h`
3. Update WebAssembly bindings in `src/wasm/wasm_bindings.cpp`
4. Add TypeScript types in `index.d.ts`
5. Write tests for the new entity type
6. Update documentation

## Release Process

We use semantic-release for automated releases:

1. Merge PR to `main` branch
2. semantic-release analyzes commits
3. Automatically bumps version based on commit types
4. Publishes to npm
5. Creates GitHub release

## Questions?

Feel free to open an issue with your question or reach out to the maintainers.

## License

By contributing, you agree that your contributions will be licensed under the GPL-2.0 License.