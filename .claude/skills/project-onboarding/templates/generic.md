# Generic Project Template

Use this template when the project type or language doesn't match Node.js/TypeScript, Python, or Go.

## Minimal Directory Structure

```
[project-name]/
├── src/                    # Source code
├── tests/                  # Test files
├── docs/                   # Documentation
├── .env.example            # Environment variable template
├── .gitignore
└── CLAUDE.md
```

## .env.example

```bash
# Environment
ENVIRONMENT=development
LOG_LEVEL=info

# Add project-specific variables here with descriptions
# VARIABLE_NAME=example_value  # Description of what this controls
```

## .gitignore additions by language

### Ruby
```
/.bundle
/vendor/bundle
*.gem
.ruby-version
```

### Java/Kotlin
```
target/
*.class
*.jar
.gradle/
build/
```

### Rust
```
target/
Cargo.lock  # (remove this line for applications; keep for libraries)
```

### PHP
```
vendor/
composer.lock  # (keep for applications)
```

## README.md template

```markdown
# [Project Name]

[Description from PRD]

## Requirements

- [Runtime/language version]
- [Any system dependencies]

## Setup

\`\`\`bash
[install command]
cp .env.example .env
# Edit .env with your values
\`\`\`

## Usage

\`\`\`bash
[run command]
\`\`\`

## Testing

\`\`\`bash
[test command]
\`\`\`
```

## Guidance

When using this template:
1. Research the idiomatic project structure for the specific language/framework
2. Follow the language's standard tooling (package manager, test runner, linter)
3. Create a language-appropriate manifest file (Gemfile, pom.xml, Cargo.toml, etc.)
4. Document setup and run commands clearly in CLAUDE.md
