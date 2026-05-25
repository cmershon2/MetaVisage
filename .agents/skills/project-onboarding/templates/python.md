# Python Project Template

## Directory Structure

```
[project-name]/
├── src/
│   └── [package_name]/
│       ├── __init__.py
│       ├── main.py         # Entry point
│       └── config.py       # Settings via pydantic-settings
├── tests/
│   ├── __init__.py
│   ├── conftest.py
│   └── test_main.py
├── docs/
├── .env.example
├── .gitignore
├── pyproject.toml
└── AGENTS.md
```

## pyproject.toml

```toml
[build-system]
requires = ["hatchling"]
build-backend = "hatchling.build"

[project]
name = "[project-slug]"
version = "0.1.0"
description = "[description]"
requires-python = ">=3.11"
dependencies = [
  "pydantic-settings>=2.0",
]

[project.optional-dependencies]
dev = [
  "pytest>=7.0",
  "pytest-cov>=4.0",
  "ruff>=0.1.0",
  "mypy>=1.0",
]

[tool.hatch.build.targets.wheel]
packages = ["src/[package_name]"]

[tool.pytest.ini_options]
testpaths = ["tests"]
pythonpath = ["src"]

[tool.ruff]
line-length = 88
target-version = "py311"

[tool.ruff.lint]
select = ["E", "F", "I", "UP", "B"]

[tool.mypy]
python_version = "3.11"
strict = true
```

## .env.example

```bash
# Application
ENVIRONMENT=development
LOG_LEVEL=INFO

# Add project-specific variables below
```

## src/[package_name]/config.py

```python
from pydantic_settings import BaseSettings, SettingsConfigDict
from typing import Literal

class Settings(BaseSettings):
    model_config = SettingsConfigDict(env_file=".env", env_file_encoding="utf-8")

    environment: Literal["development", "test", "production"] = "development"
    log_level: str = "INFO"

settings = Settings()
```

## tests/conftest.py

```python
import pytest
from [package_name].config import Settings

@pytest.fixture
def test_settings():
    return Settings(environment="test")
```

## Development commands

```bash
python -m venv .venv
source .venv/bin/activate  # Windows: .venv\Scripts\activate
pip install -e ".[dev]"

pytest                      # Run tests
pytest --cov=src            # With coverage
ruff check src tests        # Lint
mypy src                    # Type check
```

## Framework-specific additions

### FastAPI (REST API)
Add: `fastapi[standard]`, `uvicorn`
Entry point: `src/[package_name]/main.py` with `FastAPI()` app

### Click (CLI)
Add: `click`, `rich`
Entry point: decorated `@click.command()` functions

### Data pipeline
Add: `pandas`, `pydantic`, appropriate connectors
