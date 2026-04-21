# Node.js / TypeScript Project Template

## Directory Structure

```
[project-name]/
├── src/
│   ├── index.ts          # Entry point
│   ├── config.ts         # Environment config and validation
│   └── [feature]/        # Feature modules
├── tests/
│   ├── unit/
│   └── integration/
├── docs/
├── .env.example
├── .gitignore
├── package.json
├── tsconfig.json
└── CLAUDE.md
```

## package.json

```json
{
  "name": "[project-slug]",
  "version": "0.1.0",
  "private": true,
  "scripts": {
    "dev": "tsx watch src/index.ts",
    "build": "tsc",
    "start": "node dist/index.js",
    "test": "jest",
    "test:watch": "jest --watch",
    "test:coverage": "jest --coverage",
    "lint": "eslint src --ext .ts",
    "typecheck": "tsc --noEmit"
  },
  "devDependencies": {
    "@types/node": "^20.0.0",
    "tsx": "^4.0.0",
    "typescript": "^5.0.0",
    "jest": "^29.0.0",
    "@types/jest": "^29.0.0",
    "ts-jest": "^29.0.0",
    "eslint": "^8.0.0",
    "@typescript-eslint/parser": "^6.0.0",
    "@typescript-eslint/eslint-plugin": "^6.0.0"
  }
}
```

## tsconfig.json

```json
{
  "compilerOptions": {
    "target": "ES2022",
    "module": "commonjs",
    "lib": ["ES2022"],
    "outDir": "./dist",
    "rootDir": "./src",
    "strict": true,
    "esModuleInterop": true,
    "skipLibCheck": true,
    "forceConsistentCasingInFileNames": true,
    "resolveJsonModule": true
  },
  "include": ["src/**/*"],
  "exclude": ["node_modules", "dist"]
}
```

## .env.example

```bash
# Application
NODE_ENV=development
PORT=3000
LOG_LEVEL=info

# Add project-specific variables below
```

## src/config.ts

```typescript
import { z } from 'zod';

const envSchema = z.object({
  NODE_ENV: z.enum(['development', 'test', 'production']).default('development'),
  PORT: z.coerce.number().default(3000),
  LOG_LEVEL: z.enum(['debug', 'info', 'warn', 'error']).default('info'),
});

export const config = envSchema.parse(process.env);
```

## Framework-specific additions

### Fastify (API)
Add: `fastify`, `@fastify/cors`, `@fastify/helmet`

### Next.js (Web app)
Use `npx create-next-app@latest --typescript --tailwind --app`

### CLI (commander)
Add: `commander`, `chalk`
Entry point structure: `src/cli.ts` with commander program
