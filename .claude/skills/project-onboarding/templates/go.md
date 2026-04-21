# Go Project Template

## Directory Structure

```
[project-name]/
├── cmd/
│   └── [project-name]/
│       └── main.go         # Entry point
├── internal/               # Private packages
│   ├── config/
│   │   └── config.go
│   └── [feature]/
│       ├── [feature].go
│       └── [feature]_test.go
├── pkg/                    # Public packages (if library)
├── docs/
├── .env.example
├── .gitignore
├── go.mod
├── go.sum
└── CLAUDE.md
```

## go.mod

```
module github.com/[owner]/[project-name]

go 1.22
```

## internal/config/config.go

```go
package config

import (
    "os"
)

type Config struct {
    Environment string
    Port        string
    LogLevel    string
}

func Load() *Config {
    return &Config{
        Environment: getEnv("ENVIRONMENT", "development"),
        Port:        getEnv("PORT", "8080"),
        LogLevel:    getEnv("LOG_LEVEL", "info"),
    }
}

func getEnv(key, fallback string) string {
    if v := os.Getenv(key); v != "" {
        return v
    }
    return fallback
}
```

## cmd/[project-name]/main.go

```go
package main

import (
    "log"
    "[module]/internal/config"
)

func main() {
    cfg := config.Load()
    log.Printf("Starting in %s mode", cfg.Environment)
}
```

## .env.example

```bash
ENVIRONMENT=development
PORT=8080
LOG_LEVEL=info
```

## Development commands

```bash
go mod tidy               # Install dependencies
go run ./cmd/[project-name]
go test ./...             # Run all tests
go test -race ./...       # Run with race detector
go vet ./...              # Static analysis
golangci-lint run         # Full lint suite (install separately)
go build -o bin/[name] ./cmd/[project-name]
```

## Framework-specific additions

### HTTP API (Chi or Gin)
Add: `github.com/go-chi/chi/v5` or `github.com/gin-gonic/gin`

### CLI (Cobra)
Add: `github.com/spf13/cobra`
Add: `github.com/spf13/viper` for config

### gRPC
Add: `google.golang.org/grpc`, `google.golang.org/protobuf`
